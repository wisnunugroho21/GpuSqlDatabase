#include "Buffer.h"

Buffer::Buffer(Device* device, UINT64 elementCount) : device_{device}, elementCount_{elementCount}
{
    CreateBuffer();
}

Buffer::~Buffer()
{
    allocation_->Release();
}

ComPtr<ID3D12Resource> Buffer::GetResource()
{
    return buffer_;
}

void Buffer::CreateBuffer()
{
    buffer_.Reset();
    const auto elementSize  = sizeof(std::uint32_t);

    CD3DX12_RESOURCE_DESC   resourceDesc =
        CD3DX12_RESOURCE_DESC::Buffer(elementCount_ * elementSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    D3D12MA::ALLOCATION_DESC allocationDesc{
        .HeapType = D3D12_HEAP_TYPE_DEFAULT
    };

    /* ThrowIfFailed(device_->GetMemoryAllocator()->CreateResource(
        &allocationDesc,
        &resourceDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr,
        &allocation_,
        IID_PPV_ARGS(&buffer_)
    )); */
}

void Buffer::BundleDescriptor(ComPtr<ID3D12DescriptorHeap> descriptorHeap, INT descriptorIndex)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension                    = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Format                           = DXGI_FORMAT_R32_TYPELESS;
    uavDesc.Buffer.CounterOffsetInBytes      = 0;
    uavDesc.Buffer.FirstElement              = 0;
    uavDesc.Buffer.NumElements               = elementCount_;
    uavDesc.Buffer.StructureByteStride       = 0;
    uavDesc.Buffer.Flags                     = D3D12_BUFFER_UAV_FLAG_RAW;

    const auto descriptorSize =
        device_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    device_->GetDevice()->CreateUnorderedAccessView(
        buffer_.Get(),
        nullptr,
        &uavDesc,
        CD3DX12_CPU_DESCRIPTOR_HANDLE(
            descriptorHeap->GetCPUDescriptorHandleForHeapStart(), descriptorIndex, descriptorSize));
}