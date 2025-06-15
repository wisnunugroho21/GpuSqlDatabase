// This file is part of the AMD & HSC Work Graph Playground.
//
// Copyright (C) 2024 Advanced Micro Devices, Inc. and Coburg University of Applied Sciences and Arts.
// All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "Application.h"

#include <iostream>
#include <sstream>
#include <rapidcsv.h>

#define ROW_COUNT 1000
#define COL_COUNT 2

Application::Application(const Options& options)
{
    device_ =
        std::make_unique<Device>(options.forceWarpAdapter, options.enableDebugLayer, options.enableGpuValidationLayer);

    CreateResourceDescriptorHeaps();
    CreateDataBuffer();
    CreateUploadBuffer();
    CreateReadbackBuffer();

    CreateWorkGraphRootSignature();
    CreateWorkGraph();
}

Application::~Application()
{

}

void Application::Run()
{
    // Check if re-creation of work graph is required
    if (shaderCompiler_.CheckShaderSourceFiles()) {
        std::cout << "Changes to shader source files detected. Recompiling work graph..." << std::endl;
        // Recompile shaders & re-create work graph
        const bool success = CreateWorkGraph();

        if (success) {
            // Reset error message time
            errorMessageEndTime_ = std::chrono::high_resolution_clock::now();
            std::cout << "Successfully compiling work graph..." << std::endl;
        } else {
            using namespace std::chrono_literals;
            // Show error message pop-up for 5s
            errorMessageEndTime_ = std::chrono::high_resolution_clock::now() + 5s;
        }
    }

    UploadBuffer();

    // Advance to next command buffer
    ID3D12GraphicsCommandList10* commandList = device_->GetNextFrameCommandList();

    OnExecute(device_->GetNextFrameCommandList());

    // Execute command list
    device_->ExecuteCurrentFrameCommandList();
    device_->WaitForDevice();

    std::cout << "Success!" << std::endl;

    ReadBackBuffer();
}

void Application::OnExecute(ID3D12GraphicsCommandList10* commandList)
{   
    // Copy upload buffer to default buffer
    {
        std::array<D3D12_RESOURCE_BARRIER, 1> preBarriers = {
            CD3DX12_RESOURCE_BARRIER::Transition(
                dataBuffer_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST)
        };

        std::array<D3D12_RESOURCE_BARRIER, 1> postBarriers = {
            CD3DX12_RESOURCE_BARRIER::Transition(
                dataBuffer_.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        };

        commandList->ResourceBarrier(preBarriers.size(), preBarriers.data());

        commandList->CopyBufferRegion(dataBuffer_.Get(), 0, uploadBuffer_.Get(), 0, COL_COUNT * ROW_COUNT * sizeof(int));        

        commandList->ResourceBarrier(postBarriers.size(), postBarriers.data());
    }

    // Set root signature for parameters
    commandList->SetComputeRootSignature(workGraphRootSignature_.Get());

    // Set descriptor heap & table
    commandList->SetDescriptorHeaps(1, resourceDescriptorHeap_.GetAddressOf());
    commandList->SetComputeRootDescriptorTable(0, resourceDescriptorHeap_->GetGPUDescriptorHandleForHeapStart());

    workGraph_->Dispatch(commandList);

    // Copy default buffer to readback buffer
    {
        std::array<D3D12_RESOURCE_BARRIER, 1> preBarriers = {
            CD3DX12_RESOURCE_BARRIER::Transition(
                dataBuffer_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE)
        };

        std::array<D3D12_RESOURCE_BARRIER, 1> postBarriers = {
            CD3DX12_RESOURCE_BARRIER::Transition(
                dataBuffer_.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        };

        commandList->ResourceBarrier(preBarriers.size(), preBarriers.data());

        commandList->CopyBufferRegion(readbackBuffer_.Get(), 0, dataBuffer_.Get(), COL_COUNT * ROW_COUNT * sizeof(int), ROW_COUNT * sizeof(int));

        commandList->ResourceBarrier(postBarriers.size(), postBarriers.data());
    }
}

bool Application::CreateWorkGraph()
{
    // Wait for all frames in fight before deleting old resources
    device_->WaitForDevice();

    try {
        workGraph_ = std::make_unique<WorkGraph>(device_.get(),
                                                 shaderCompiler_,
                                                 workGraphRootSignature_.Get());
    } catch (const std::exception& e) {
        // Re-throw exception if no fallback work graph exists
        if (!workGraph_) {
            throw e;
        }

        std::cerr << "Failed to re-create work graph:\n" << e.what() << std::endl;

        return false;
    }

    return true;
}

void Application::CreateWorkGraphRootSignature()
{
    const auto descriptorRange = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, COL_COUNT + 1, 0);

    std::array<CD3DX12_ROOT_PARAMETER, 1> rootParameters;
    rootParameters[0].InitAsDescriptorTable(1, &descriptorRange);

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(rootParameters.size(), rootParameters.data(), 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    ThrowIfFailed(device_->GetDevice()->CreateRootSignature(
        0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&workGraphRootSignature_)));
}

void Application::CreateResourceDescriptorHeaps()
{
    // Create descriptor heap to clear shader resources
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc {
            .Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            .NumDescriptors             = COL_COUNT + 1,
            .Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
            .NodeMask                   = 1
        };
        
        ThrowIfFailed(device_->GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&clearDescriptorHeap_)));
    }

    // Create resource descriptor heap for shader resources
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc {
            .Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            .NumDescriptors             = COL_COUNT + 1,
            .Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
            .NodeMask                   = 1
        };
        
        ThrowIfFailed(device_->GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&resourceDescriptorHeap_)));
    }
}

void Application::CreateDataBuffer()
{
    dataBuffer_.Reset();

    CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC   resourceDescription =
        CD3DX12_RESOURCE_DESC::Buffer((COL_COUNT + 1) * ROW_COUNT * sizeof(int), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    ThrowIfFailed(device_->GetDevice()->CreateCommittedResource(&heapProperties,
                                                                D3D12_HEAP_FLAG_NONE,
                                                                &resourceDescription,
                                                                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                                                nullptr,
                                                                IID_PPV_ARGS(&dataBuffer_)));

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension                    = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Format                           = DXGI_FORMAT_UNKNOWN;
    uavDesc.Buffer.CounterOffsetInBytes      = 0;
    uavDesc.Buffer.NumElements               = ROW_COUNT;
    uavDesc.Buffer.StructureByteStride       = sizeof(int);
    uavDesc.Buffer.Flags                     = D3D12_BUFFER_UAV_FLAG_NONE;

    const auto descriptorSize =
        device_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    for (uint32_t descriptorIndex = 0; descriptorIndex < (COL_COUNT + 1); descriptorIndex++)
    {
        uavDesc.Buffer.FirstElement = descriptorIndex * ROW_COUNT;

        device_->GetDevice()->CreateUnorderedAccessView(
            dataBuffer_.Get(),
            nullptr,
            &uavDesc,
            CD3DX12_CPU_DESCRIPTOR_HANDLE(
                clearDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(), descriptorIndex, descriptorSize));
                
        device_->GetDevice()->CreateUnorderedAccessView(
            dataBuffer_.Get(),
            nullptr,
            &uavDesc,
            CD3DX12_CPU_DESCRIPTOR_HANDLE(
                resourceDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(), descriptorIndex, descriptorSize));
    }
}

void Application::CreateUploadBuffer()
{
    uploadBuffer_.Reset();

    CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC   resourceDescription =
        CD3DX12_RESOURCE_DESC::Buffer((COL_COUNT + 1) * ROW_COUNT * sizeof(int), D3D12_RESOURCE_FLAG_NONE);
    ThrowIfFailed(device_->GetDevice()->CreateCommittedResource(&heapProperties,
                                                                D3D12_HEAP_FLAG_NONE,
                                                                &resourceDescription,
                                                                D3D12_RESOURCE_STATE_COPY_SOURCE,
                                                                nullptr,
                                                                IID_PPV_ARGS(&uploadBuffer_)));
}

void Application::CreateReadbackBuffer()
{
    readbackBuffer_.Reset();

    CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_READBACK);
    CD3DX12_RESOURCE_DESC   resourceDescription =
        CD3DX12_RESOURCE_DESC::Buffer(ROW_COUNT * sizeof(int), D3D12_RESOURCE_FLAG_NONE);
    ThrowIfFailed(device_->GetDevice()->CreateCommittedResource(&heapProperties,
                                                                D3D12_HEAP_FLAG_NONE,
                                                                &resourceDescription,
                                                                D3D12_RESOURCE_STATE_COPY_DEST,
                                                                nullptr,
                                                                IID_PPV_ARGS(&readbackBuffer_)));
}

void Application::UploadBuffer() {
    rapidcsv::Document doc("../../../data/random_integer.csv", rapidcsv::LabelParams(-1, -1));
    
    std::array<int, 10> datas1;
    for(size_t i = 0; i < 10; i++) {
        datas1[i] = doc.GetCell<int>(0, i);
    }
   
    void* mappedData;
    ThrowIfFailed(uploadBuffer_->Map(0, nullptr, &mappedData));
    
    memcpy(mappedData, datas1.data(), datas1.size() * sizeof(int));

    std::array<int, 10> datas2 = { 20, 3, 2, 3, 5, 4, 6, 7, 8, 77 };

    memcpy((int*) mappedData + ROW_COUNT, datas2.data(), datas2.size() * sizeof(int));

    uploadBuffer_->Unmap(0, nullptr);
}

void Application::ReadBackBuffer() {
    std::array<int, 10> datas = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

    void* mappedData;
    ThrowIfFailed(readbackBuffer_->Map(0, nullptr, &mappedData));

    memcpy(datas.data(), mappedData, datas.size() * sizeof(int));

    readbackBuffer_->Unmap(0, nullptr);

    for (size_t i = 0; i < datas.size(); i++) {
        std::cout << datas[i] << std::endl;
    }
}

void Application::ClearShaderResources(ID3D12GraphicsCommandList10* commandList)
{
    // Set descriptor heap for clear
    commandList->SetDescriptorHeaps(1, resourceDescriptorHeap_.GetAddressOf());

    const auto descriptorSize =
        device_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Clear data buffer
    {
        const auto descriptorIndex     = 0;
        const auto gpuDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
            resourceDescriptorHeap_->GetGPUDescriptorHandleForHeapStart(), descriptorIndex, descriptorSize);
        const auto cpuDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
            clearDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(), descriptorIndex, descriptorSize);

        std::uint32_t clearValue[4] = {0, 0, 0, 0};
        commandList->ClearUnorderedAccessViewUint(
            gpuDescriptorHandle, cpuDescriptorHandle, dataBuffer_.Get(), clearValue, 0, nullptr);
    }

    std::array<D3D12_RESOURCE_BARRIER, 1> uavBarriers {
        CD3DX12_RESOURCE_BARRIER::UAV(dataBuffer_.Get())
    };

    // Barrier for clear operation
    commandList->ResourceBarrier(uavBarriers.size(), uavBarriers.data());
}
