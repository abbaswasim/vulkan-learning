# Roar Source Code
# Wasim Abbas
# http://www.waZim.com
# Copyright (c) 2008-2019
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the 'Software'),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the Software
# is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
# OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
# OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
# Version: 1.0.0

# Add roar library
add_subdirectory(roar)

# Add glfw
# add_subdirectory(glfw)

# Add vulkan
# add_library(vulkan_headers INTERFACE)
# set(VULKAN_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/include)
# target_sources(vulkan_headers INTERFACE ${VULKAN_INCLUDE_DIR}/vulkan/vulkan.h)
# target_include_directories(vulkan_headers INTERFACE ${VULKAN_INCLUDE_DIR})

# # Add basis
# set(BASIS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/basis_universal)
# set(BASIS_SOURCE_FILES
# 	${BASIS_DIR}/transcoder/basisu_transcoder.cpp
# 	${BASIS_DIR}/transcoder/basisu_transcoder.h
# 	${BASIS_DIR}/zstd/zstddeclib.c)

# add_library(basisu STATIC ${BASIS_SOURCE_FILES})
# target_include_directories(basisu PUBLIC ${BASIS_DIR})
# target_include_directories(basisu PUBLIC ${BASIS_DIR}/transcoder)
# target_compile_options(basisu PUBLIC "-fno-strict-aliasing")

# # Add cgltf
# add_library(cgltf INTERFACE)
# set(CGLTF_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cgltf)
# target_sources(cgltf INTERFACE ${CGLTF_INCLUDE_DIR}/cgltf.h)
# target_include_directories(cgltf INTERFACE ${CGLTF_INCLUDE_DIR})

# # Add CTPL
# add_library(ctpl INTERFACE)
# set(CTPL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/CTPL)
# target_sources(ctpl INTERFACE ${CTPL_INCLUDE_DIR}/ctpl_stl.h)
# target_include_directories(ctpl INTERFACE ${CTPL_INCLUDE_DIR})
