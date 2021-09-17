// VulkanEd Source Code
// Wasim Abbas
// http://www.waZim.com
// Copyright (c) 2021
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the 'Software'),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software
// is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
// OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// Version: 1.0.0

#pragma once

#include <foundation/rormacros.hpp>
#include "common.hpp"

#include <string>
#include <vector>

namespace cfg        // All vulkan depedencies should be removed
{
#define CFG_VK_MAKE_VERSION(major, minor, patch) \
	(((static_cast<uint32_t>(major)) << 22) | ((static_cast<uint32_t>(minor)) << 12) | (static_cast<uint32_t>(patch)))

// clang-format off
FORCE_INLINE std::string get_application_name()    { return "VulkanEd Application";}
FORCE_INLINE uint32_t    get_application_version() { return CFG_VK_MAKE_VERSION(1, 0, 0);}
FORCE_INLINE std::string get_engine_name()         { return "VulkanEd Engine";}
FORCE_INLINE uint32_t    get_engine_version()      { return CFG_VK_MAKE_VERSION(1, 0, 0);}
FORCE_INLINE uint32_t    get_api_version()         { return CFG_VK_MAKE_VERSION(1, 1, 0);} // TODO: Try 1.2 on Linux
// clang-format on

FORCE_INLINE std::vector<const char *> get_instance_extensions_requested()
{
	/* Usually available extensions
	  VK_KHR_device_group_creation
	  VK_KHR_external_fence_capabilities
	  VK_KHR_external_memory_capabilities
	  VK_KHR_external_semaphore_capabilities
	  VK_KHR_get_physical_device_properties2
	  VK_KHR_get_surface_capabilities2
	  VK_KHR_surface
	  VK_EXT_debug_report
	  VK_EXT_debug_utils
	  VK_EXT_metal_surface
	  VK_EXT_swapchain_colorspace
	  VK_MVK_macos_surface // MacOS specific
	*/
	return std::vector<const char *>
	{
		VK_KHR_SURFACE_EXTENSION_NAME,                                     // VK_KHR_surface
			VK_KHR_DISPLAY_EXTENSION_NAME,                                 // VK_KHR_display
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME,                             // VK_EXT_debug_utils
			VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,        // VK_KHR_get_physical_device_properties2
#if defined __APPLE__
			VK_EXT_METAL_SURFACE_EXTENSION_NAME,        // "VK_EXT_metal_surface"
#elif defined __linux__
			VK_KHR_XCB_SURFACE_EXTENSION_NAME        // "VK_EXT_xcb_surface??"
#endif
	};
}

FORCE_INLINE std::vector<const char *> get_instance_layers_requested()
{
	/* Usually available layers
	  VK_LAYER_LUNARG_api_dump
	  VK_LAYER_KHRONOS_validation
	*/
	return std::vector<const char *>{"VK_LAYER_KHRONOS_validation"};
}

FORCE_INLINE std::vector<const char *> get_device_extensions_requested()
{
	return std::vector<const char *>{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,                // VK_KHR_swapchain
		VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME        // "VK_KHR_portability_subset"
	};
}

FORCE_INLINE std::vector<const char *> get_device_layers_requested()
{
	// Device layers are now deprecated, this always return empty. Not backward compatible
	return std::vector<const char *>{};
}

FORCE_INLINE constexpr uint32_t get_number_of_buffers()
{
	return 3;        // Tripple buffering
}

FORCE_INLINE constexpr bool get_visualise_mipmaps()
{
	return false;
}

FORCE_INLINE constexpr uint32_t get_multisample_count()
{
	return 8;
}

FORCE_INLINE constexpr bool get_sample_rate_shading_enabled()
{
	return false;
}

FORCE_INLINE constexpr float get_sample_rate_shading()
{
	return 0.5f;
}


FORCE_INLINE auto get_vsync()
{
	// TODO: No vsync available on Android so when enabling make sure android is guarded
	return false;
}

FORCE_INLINE auto get_window_transparent()
{
	return false;
}

FORCE_INLINE auto get_window_premultiplied()
{
	return false;
}

FORCE_INLINE auto get_window_prerotated()
{
	return false;
}

}        // namespace cfg
