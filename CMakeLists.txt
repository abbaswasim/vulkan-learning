# VulkanEd Source Code
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

cmake_minimum_required (VERSION 3.12)

project(VulkanEd VERSION 1.1.0.0 LANGUAGES CXX C)
list(APPEND CMAKE_MESSAGE_CONTEXT Vulkaned_Engine)

set(VULKANED_NAME VulkanEd)

set(VULKANED_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}) # Always prefer the _LIST_ version FILES/DIRS instead of the _SOURCE_
set(VULKANED_SOURCE_DIR ${VULKANED_ROOT_DIR}/src)

include(${VULKANED_ROOT_DIR}/build.cmake)

# Always generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

add_subdirectory(${VULKANED_ROOT_DIR}/thirdparty)
add_subdirectory(${VULKANED_SOURCE_DIR})


set(VULKANED_HEADERS
  ${VULKANED_SOURCE_DIR}/dummy.hpp)

set(VULKANED_SOURCES
  ${VULKANED_SOURCE_DIR}/main.cpp)

add_executable(${VULKANED_NAME} ${VULKANED_SOURCES} ${VULKANED_HEADERS})
set_target_properties(${VULKANED_NAME} PROPERTIES VERSION 1.0.0.0 SOVERSION 1)          # TODO: Fix the hardcoded version
set_target_properties(${VULKANED_NAME} PROPERTIES CMAKE_VISIBILITY_INLINES_HIDDEN true) # Don't export all of those C++ template generated symbols
set_target_properties(${VULKANED_NAME} PROPERTIES CMAKE_CXX_VISIBILITY_PRESET hidden)   # Set default to hide symbols visibility for Roar
set_target_properties(${VULKANED_NAME} PROPERTIES CMAKE_C_VISIBILITY_PRESET hidden)     # Set default to hide symbols visibility for Roar

set(CMAKE_VISIBILITY_INLINES_HIDDEN YES)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)    # Set default to hide symbols visibility for all targets
set(CMAKE_C_VISIBILITY_PRESET hidden)      # Set default to hide symbols visibility for all targets

target_include_directories(${VULKANED_NAME} PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_include_directories(${VULKANED_NAME} PRIVATE ${VULKANED_SOURCE_DIR})
target_include_directories(${VULKANED_NAME} PRIVATE ${VULKANED_ROOT_DIR})

find_package(vulkan)

set(VULKANED_REQUIRED_LIBRARIES
  vulkan_headers
  Vulkan::Vulkan
  glfw
  roar)

target_link_libraries(${VULKANED_NAME} PRIVATE ${VULKANED_REQUIRED_LIBRARIES})

build_options(${VULKANED_NAME}) # Set common build options

# add_custom_command(
  # TARGET ${VULKANED_NAME} POST_BUILD
  # COMMENT "Copying compile_commands.json to root of the target so that ycmd can see it"
  # COMMAND ${CMAKE_COMMAND} -E copy_if_different compile_commands.json ${VULKANED_ROOT_DIR}
  # VERBATIM)

# Copy compile_commands.json to root so that ycmd can see it, maybe better fix how ycmd picks this?
# configure_file(${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json /tmp/vulkaned_build COPYONLY)
