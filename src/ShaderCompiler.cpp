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

#include "ShaderCompiler.h"

#include <sstream>

// Include handler library to collect all included files for tracking
class FileTrackingIncludeHandler : public IDxcIncludeHandler {
public:
    FileTrackingIncludeHandler(ShaderCompiler& parent) : parent_(parent) {}

    HRESULT STDMETHODCALLTYPE LoadSource(_In_ LPCWSTR                             pFilename,
                                         _COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource) override
    {
        if (pFilename == nullptr) {
            return E_FAIL;
        }
        if (ppIncludeSource == nullptr) {
            return E_FAIL;
        }

        const auto shaderSourceFilePath = parent_.GetShaderSourceFilePath(pFilename);

        IDxcBlobEncoding* includeSource;
        const auto result = parent_.utils_->LoadFile(shaderSourceFilePath.wstring().c_str(), nullptr, &includeSource);

        *ppIncludeSource = includeSource;

        if (SUCCEEDED(result)) {
            // Update/insert last file write time for hot-reloading
            parent_.trackedFiles_[shaderSourceFilePath] = std::filesystem::last_write_time(shaderSourceFilePath);
        }

        return result;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override
    {
        return E_FAIL;
    }

    ULONG STDMETHODCALLTYPE AddRef(void) override
    {
        return 0;
    }

    ULONG STDMETHODCALLTYPE Release(void) override
    {
        return 0;
    }

private:
    ShaderCompiler& parent_;
};

ShaderCompiler::ShaderCompiler()
{
    HMODULE dxcompilerModule = LoadLibraryW(L"dxcompiler.dll");

    if (!dxcompilerModule) {
        throw std::runtime_error("Failed to load dxcompiler.dll");
    }

    DxcCreateInstanceProc pfnDxcCreateInstance =
        DxcCreateInstanceProc(GetProcAddress(dxcompilerModule, "DxcCreateInstance"));

    if (pfnDxcCreateInstance == nullptr) {
        throw std::runtime_error("Failed to load DxcCreateInstance from dxcompiler.dll");
    }

    ThrowIfFailed(pfnDxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils_)));
    ThrowIfFailed(pfnDxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler_)));
    ThrowIfFailed(utils_->CreateDefaultIncludeHandler(&includeHandler_));

    shaderFolderPath_ = std::filesystem::current_path() / L"shaders";
}

ComPtr<IDxcBlob> ShaderCompiler::CompileShader(const std::string& shaderFile,
                                               const wchar_t*     target,
                                               const wchar_t*     entryPoint)
{
    const auto shaderSourceFilePath = GetShaderSourceFilePath(shaderFile);

    HRESULT                  loadSourceResult;
    ComPtr<IDxcBlobEncoding> source;

    loadSourceResult = utils_->LoadFile(shaderSourceFilePath.wstring().c_str(), nullptr, &source);

    if (FAILED(loadSourceResult) || (source == nullptr)) {
        // try load source again. Sometimes loading the file for hot-reloading will fail if the
        // file is still being written to.
        loadSourceResult = utils_->LoadFile(shaderSourceFilePath.wstring().c_str(), nullptr, &source);
    }

    if (FAILED(loadSourceResult) || (source == nullptr)) {
        // Second attempt failed as well. Throw error
        throw std::runtime_error("Failed to load shader file \"" + shaderFile + "\"");
    }

    const auto shaderIncludeArgument = std::wstring(L"-I") + shaderFolderPath_.wstring();

    std::vector<const wchar_t*> arguments = {
        L"-enable-16bit-types",
        // use HLSL 2021
        L"-HV",
        L"2021",
        // column major matrices
        DXC_ARG_PACK_MATRIX_COLUMN_MAJOR,
        // include path for "tutorials" folder
        shaderIncludeArgument.c_str(),
    };

    FileTrackingIncludeHandler includeHandler(*this);

    ComPtr<IDxcOperationResult> result = nullptr;
    ThrowIfFailed(compiler_->Compile(source.Get(),
                                     shaderSourceFilePath.wstring().c_str(),
                                     entryPoint,
                                     target,
                                     arguments.data(),
                                     static_cast<UINT32>(arguments.size()),
                                     nullptr,
                                     0,
                                     &includeHandler,
                                     &result));

    HRESULT compileStatus;
    ThrowIfFailed(result->GetStatus(&compileStatus));

    std::string errorString = "";

    // try get error string from DXC result
    {
        ComPtr<IDxcBlobEncoding> errorStringBlob = nullptr;
        if (SUCCEEDED(result->GetErrorBuffer(&errorStringBlob)) && (errorStringBlob != nullptr)) {
            ComPtr<IDxcBlobUtf8> errorStringBlob8 = nullptr;
            utils_->GetBlobAsUtf8(errorStringBlob.Get(), &errorStringBlob8);

            errorString = std::string(errorStringBlob8->GetStringPointer(), errorStringBlob8->GetStringLength());
        }
    }

    if (FAILED(compileStatus)) {
        std::stringstream stream;
        stream << "Failed to compile shader \"" << shaderFile << "\":\n" << errorString;

        throw std::runtime_error(stream.str());
    }

    ComPtr<IDxcBlob> outputBlob;
    ThrowIfFailed(result->GetResult(&outputBlob));

    // Update/insert last file write time for hot-reloading
    trackedFiles_[shaderSourceFilePath] = std::filesystem::last_write_time(shaderSourceFilePath);

    return outputBlob;
}

bool ShaderCompiler::CheckShaderSourceFiles()
{
    bool result = false;

    for (auto& [file, writeTime] : trackedFiles_) {
        try {
            const auto newFileWriteTime = std::filesystem::last_write_time(file);

            // Return true if any file was modified
            result |= (writeTime != newFileWriteTime);

            // Update file timestamp to only trigger update once
            writeTime = newFileWriteTime;
        } catch (const std::filesystem::filesystem_error& e) {
            // last_write_time can throw an error if the file is currently being written to
            continue;
        }
    }

    return result;
}

std::filesystem::path ShaderCompiler::GetShaderSourceFilePath(const std::string& shaderFile)
{
    return std::filesystem::absolute(shaderFolderPath_ / shaderFile).generic_string();
}

std::filesystem::path ShaderCompiler::GetShaderSourceFilePath(const std::wstring& shaderFile)
{
    return std::filesystem::absolute(shaderFolderPath_ / shaderFile).generic_string();
}
