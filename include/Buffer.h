#pragma once

#include "Device.h"
#include <D3D12MemAlloc.h>

class Buffer {
public:
    Buffer(Device* device_, UINT64 elementCount);
    ~Buffer();

    ComPtr<ID3D12Resource> GetResource();
        
    void BundleDescriptor(ComPtr<ID3D12DescriptorHeap> descriptorHeap, INT descriptorIndex);

private:
    ComPtr<ID3D12Resource> buffer_;
    D3D12MA::Allocation* allocation_;

    Device* device_;    
    UINT64 elementCount_;

    void CreateBuffer();
};