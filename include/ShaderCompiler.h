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

#pragma once

#include "Device.h"

//
#include <dxcapi.h>

#include <filesystem>
#include <unordered_map>

class ShaderCompiler {
public:
    ShaderCompiler();

    ComPtr<IDxcBlob> CompileShader(const std::string& shaderFile, const wchar_t* target, const wchar_t* entryPoint);

    // Checks shader source files for updates/changes
    bool CheckShaderSourceFiles();

private:
    friend class FileTrackingIncludeHandler;

    std::filesystem::path GetShaderSourceFilePath(const std::string& shaderFile);
    std::filesystem::path GetShaderSourceFilePath(const std::wstring& shaderFile);

    ComPtr<IDxcUtils>          utils_;
    ComPtr<IDxcCompiler>       compiler_;
    ComPtr<IDxcIncludeHandler> includeHandler_;

    std::filesystem::path shaderFolderPath_;

    std::unordered_map<std::filesystem::path, std::filesystem::file_time_type> trackedFiles_;
};