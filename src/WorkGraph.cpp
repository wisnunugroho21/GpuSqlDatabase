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

#include "WorkGraph.h"
#include "Application.h"

WorkGraph::WorkGraph(const Device*        device,
                     ShaderCompiler&      shaderCompiler,
                     ID3D12RootSignature* rootSignature)
{
    // Name for work graph program inside the state object
    static const wchar_t* WorkGraphProgramName = L"WorkGraph";

    // Create work graph
    CD3DX12_STATE_OBJECT_DESC stateObjectDesc(D3D12_STATE_OBJECT_TYPE_EXECUTABLE);

    // set root signature for work graph
    auto rootSignatureSubobject = stateObjectDesc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
    rootSignatureSubobject->SetRootSignature(rootSignature);

    auto workgraphSubobject = stateObjectDesc.CreateSubobject<CD3DX12_WORK_GRAPH_SUBOBJECT>();
    workgraphSubobject->IncludeAllAvailableNodes();
    workgraphSubobject->SetProgramName(WorkGraphProgramName);

    // list of compiled shaders to be released once the work graph is created
    std::vector<ComPtr<IDxcBlob>> compiledShaders;

    // Adding a shader library to the work graph state object
    auto blob           = shaderCompiler.CompileShader("main.hlsl", L"lib_6_8", nullptr);
    auto shaderBytecode = CD3DX12_SHADER_BYTECODE(blob->GetBufferPointer(), blob->GetBufferSize());

    // add blob to state object
    auto librarySubobject = stateObjectDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
    librarySubobject->SetDXILLibrary(&shaderBytecode);

    // add shader blob to be released later
    compiledShaders.emplace_back(std::move(blob));

    // Create work graph state object
    ThrowIfFailed(device->GetDevice()->CreateStateObject(stateObjectDesc, IID_PPV_ARGS(&stateObject_)));

    // release all compiled shaders
    compiledShaders.clear();

    // Get work graph properties
    ComPtr<ID3D12StateObjectProperties1> stateObjectProperties;
    ComPtr<ID3D12WorkGraphProperties>    workGraphProperties;

    ThrowIfFailed(stateObject_->QueryInterface(IID_PPV_ARGS(&stateObjectProperties)));
    ThrowIfFailed(stateObject_->QueryInterface(IID_PPV_ARGS(&workGraphProperties)));

    // Get the index of our work graph inside the state object (state object can contain multiple work graphs)
    const auto workGraphIndex = workGraphProperties->GetWorkGraphIndex(WorkGraphProgramName);

    // Create backing memory buffer
    // See https://microsoft.github.io/DirectX-Specs/d3d/WorkGraphs.html#getworkgraphmemoryrequirements
    D3D12_WORK_GRAPH_MEMORY_REQUIREMENTS memoryRequirements = {};
    workGraphProperties->GetWorkGraphMemoryRequirements(workGraphIndex, &memoryRequirements);

    // Work graphs can also request no backing memory (i.e., MaxSizeInBytes = 0)
    if (memoryRequirements.MaxSizeInBytes > 0) {
        CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
        CD3DX12_RESOURCE_DESC   resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(memoryRequirements.MaxSizeInBytes,
                                                                           D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        ThrowIfFailed(device->GetDevice()->CreateCommittedResource(&heapProperties,
                                                                   D3D12_HEAP_FLAG_NONE,
                                                                   &resourceDesc,
                                                                   D3D12_RESOURCE_STATE_COMMON,
                                                                   NULL,
                                                                   IID_PPV_ARGS(&backingMemory_)));
    }

    // Prepare work graph desc
    // See https://microsoft.github.io/DirectX-Specs/d3d/WorkGraphs.html#d3d12_set_program_desc
    programDesc_.Type                        = D3D12_PROGRAM_TYPE_WORK_GRAPH;
    programDesc_.WorkGraph.ProgramIdentifier = stateObjectProperties->GetProgramIdentifier(WorkGraphProgramName);
    // Set flag to initialize backing memory.
    // We'll clear this flag once we've run the work graph for the first time.
    programDesc_.WorkGraph.Flags             = D3D12_SET_WORK_GRAPH_FLAG_INITIALIZE;
    // Set backing memory
    if (backingMemory_) {
        programDesc_.WorkGraph.BackingMemory.StartAddress = backingMemory_->GetGPUVirtualAddress();
        programDesc_.WorkGraph.BackingMemory.SizeInBytes  = backingMemory_->GetDesc().Width;
    }

    // All tutorial work graphs must declare a node named "Entry" with an empty record (i.e., no input record).
    // The D3D12_DISPATCH_GRAPH_DESC uses entrypoint indices instead of string-based node IDs to reference the enty node.
    // GetEntrypointIndex allows us to translate from a node ID (i.e., node name and node array index)
    // to an entrypoint index.
    // See https://microsoft.github.io/DirectX-Specs/d3d/WorkGraphs.html#getentrypointindex
    entryPointIndex_ = workGraphProperties->GetEntrypointIndex(workGraphIndex, {L"Entry", 0});

    // Check if entrypoint was found.
    if (entryPointIndex_ == 0xFFFFFFFFU) {
        throw std::runtime_error("work graph does not contain an entry node with [NodeId(\"Entry\", 0)].");
    }
}

void WorkGraph::Dispatch(ID3D12GraphicsCommandList10* commandList)
{
    D3D12_DISPATCH_GRAPH_DESC dispatchDesc        = {};
    dispatchDesc.Mode                             = D3D12_DISPATCH_MODE_NODE_CPU_INPUT;
    dispatchDesc.NodeCPUInput                     = {};
    dispatchDesc.NodeCPUInput.EntrypointIndex     = entryPointIndex_;
    // Launch graph with one record
    dispatchDesc.NodeCPUInput.NumRecords          = 1;
    // Record does not contain any data
    dispatchDesc.NodeCPUInput.RecordStrideInBytes = 0;
    dispatchDesc.NodeCPUInput.pRecords            = nullptr;

    // Set program and dispatch the work graphs.
    // See
    // https://microsoft.github.io/DirectX-Specs/d3d/WorkGraphs.html#setprogram
    // https://microsoft.github.io/DirectX-Specs/d3d/WorkGraphs.html#dispatchgraph

    commandList->SetProgram(&programDesc_);
    commandList->DispatchGraph(&dispatchDesc);

    // Clear backing memory initialization flag, as the graph has run at least once now
    // See https://microsoft.github.io/DirectX-Specs/d3d/WorkGraphs.html#d3d12_set_work_graph_flags
    programDesc_.WorkGraph.Flags &= ~D3D12_SET_WORK_GRAPH_FLAG_INITIALIZE;
}
