# This file is part of the AMD & HSC Work Graph Playground.
#
# Copyright (C) 2024 Advanced Micro Devices, Inc. and Coburg University of Applied Sciences and Arts.
# All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files(the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions :
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

cmake_minimum_required(VERSION 3.17)
project(GpuSqlDatabase VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_EXECUTABLE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_compile_definitions(UNICODE _UNICODE)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory(imported)

file(GLOB PROJECT_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/**/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.cpp)
file(GLOB_RECURSE PROJECT_SHADER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.md
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.png
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.hlsl)
set_source_files_properties(${PROJECT_SHADER_FILES} PROPERTIES VS_TOOL_OVERRIDE "Text")

add_executable(${PROJECT_NAME} ${PROJECT_SOURCE_FILES} ${PROJECT_SHADER_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Microsoft.Direct3D.D3D12    
    Microsoft.Direct3D.DXC
    Microsoft.Direct3D.WARP
    d3d12
    dxcompiler
    dxgi
    dxguid
    D3D12MemoryAllocator)

set_target_properties(${PROJECT_NAME} PROPERTIES 
    VS_DPI_AWARE "PerMonitor"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>")
set_property(DIRECTORY "." PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

# set source group for shader files & link to bin folder
set(SHADER_BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
foreach(SHADER ${PROJECT_SHADER_FILES})
    get_filename_component(SHADER_FILE_DIRECTORY ${SHADER} DIRECTORY)
    get_filename_component(SHADER_FILE_NAME ${SHADER} NAME)
    file(RELATIVE_PATH SHADER_FILE_DIRECTORY_RELATIVE_PATH ${SHADER_BASE_DIRECTORY} ${SHADER_FILE_DIRECTORY})
            
    if ("${SHADER_FILE_DIRECTORY_RELATIVE_PATH}" STREQUAL "")
        source_group("Shader Source Files" FILES ${SHADER})
    else()
        source_group("Shader Source Files/${SHADER_FILE_DIRECTORY_RELATIVE_PATH}" FILES ${SHADER})
    endif()

    # to enable shader hot-reloading, instead of copying the shaders to the bin output folder at the end of the build,
    # we create hardlinks between a file in the bin folder and the shader source file.
    # This way, updates to the shader source file are automatically propagated to the bin folder,
    # and - unlike symlinks - the hardlinks allow copying/moving/compressing the bin folder without having broken links.

    # create parent folder
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory  
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/shaders/${SHADER_FILE_DIRECTORY_RELATIVE_PATH})
    # create hardlink
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_hardlink  
        ${SHADER}
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/shaders/${SHADER_FILE_DIRECTORY_RELATIVE_PATH}/${SHADER_FILE_NAME})
endforeach()
