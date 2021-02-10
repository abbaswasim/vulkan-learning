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

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <ios>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <typeindex>

// From Vulkan Samples project
#if defined(__clang__)
// CLANG ENABLE/DISABLE WARNING DEFINITION
#	define VKBP_DISABLE_WARNINGS()                             \
		_Pragma("clang diagnostic push")                        \
			_Pragma("clang diagnostic ignored \"-Wall\"")       \
				_Pragma("clang diagnostic ignored \"-Wextra\"") \
					_Pragma("clang diagnostic ignored \"-Wtautological-compare\"")
#	define VKBP_ENABLE_WARNINGS() \
		_Pragma("clang diagnostic pop")
#elif defined(__GNUC__) || defined(__GNUG__)
// GCC ENABLE/DISABLE WARNING DEFINITION
#	define VKBP_DISABLE_WARNINGS()                             \
		_Pragma("GCC diagnostic push")                          \
			_Pragma("GCC diagnostic ignored \"-Wall\"")         \
				_Pragma("clang diagnostic ignored \"-Wextra\"") \
					_Pragma("clang diagnostic ignored \"-Wtautological-compare\"")
#	define VKBP_ENABLE_WARNINGS() \
		_Pragma("GCC diagnostic pop")
#elif defined(_MSC_VER)
// MSVC ENABLE/DISABLE WARNING DEFINITION
#	define VKBP_DISABLE_WARNINGS() \
		__pragma(warning(push, 0))

#	define VKBP_ENABLE_WARNINGS() \
		__pragma(warning(pop))
#endif

VKBP_DISABLE_WARNINGS();
#include "profiling/rorlog.hpp"
#include "roar.hpp"
VKBP_ENABLE_WARNINGS();

// TODO:: This bit should be in CMakeLists.txt somewhere
#if defined __APPLE__
#	define VK_USE_PLATFORM_METAL_EXT
#elif defined __linux__
#	define VK_USE_PLATFORM_XCB_KHR
#endif

// #define USE_VOLK_INSTEAD

#if defined(USE_VOLK_INSTEAD)
#	define VK_ENABLE_BETA_EXTENSIONS
#	include "volk.h"
#else
#	include "vusym.hpp"
#endif

// #define GLFW_INCLUDE_VULKAN // Not required if vulkan.h included before this
#include <GLFW/glfw3.h>

#define VULKANED_USE_GLFW 1

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class VulkanApplication;

namespace cfg        // Should be moved out later
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

FORCE_INLINE uint32_t get_number_of_buffers()
{
	return 3;        // Tripple buffering
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

VkAllocationCallbacks *VkAllocator = nullptr;

}        // namespace cfg

namespace utl
{
// Not safe for unsigned to signed conversion but the compiler will complain about that
template <class _type_to,
		  class _type_from,
		  class _type_big = typename std::conditional<std::is_integral<_type_from>::value,
													  unsigned long long,
													  long double>::type>
FORCE_INLINE _type_to static_cast_safe(_type_from a_value)
{
	if (static_cast<_type_big>(a_value) > static_cast<_type_big>(std::numeric_limits<_type_to>::max()))
	{
		throw std::runtime_error("Loss of data doing casting.\n");        // TODO: Add some line number indication etc
	}

	return static_cast<_type_to>(a_value);
}

}        // namespace utl

namespace vkd
{
FORCE_INLINE auto get_surface_format()
{
	return VK_FORMAT_B8G8R8A8_SRGB;
}

FORCE_INLINE auto get_surface_colorspace()
{
	return VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
}

FORCE_INLINE auto get_surface_transform()
{
	// TODO: Fix the hardcode 90 degree rotation
	return cfg::get_window_prerotated() ? VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR : VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
}

FORCE_INLINE auto get_surface_composition_mode()
{
	return (cfg::get_window_transparent() ?
				(cfg::get_window_premultiplied() ?
					 VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR :
					 VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR) :
				VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR);
}

FORCE_INLINE auto get_swapchain_usage()
{
	return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
}

FORCE_INLINE auto get_swapchain_sharing_mode(uint32_t a_queue_family_indices[2])
{
	VkSwapchainCreateInfoKHR create_info{};

	if (a_queue_family_indices[0] != a_queue_family_indices[1])
	{
		create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = 2;
		create_info.pQueueFamilyIndices   = a_queue_family_indices;
	}
	else
	{
		create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
		create_info.queueFamilyIndexCount = 0;              // Optional
		create_info.pQueueFamilyIndices   = nullptr;        // Optional
	}

	return create_info;
}

void glfw_create_surface(VkInstance &a_instance, VkSurfaceKHR &a_surface, GLFWwindow *a_window)
{
	assert(a_instance);
	assert(a_window);

	VkResult status = glfwCreateWindowSurface(a_instance, a_window, nullptr, &a_surface);

	if (status != VK_SUCCESS)
		ror::log_critical("WARNING! Window surface creation failed");
}

auto glfw_get_buffer_size(GLFWwindow *a_window)
{
	assert(a_window);

	int w, h;
	glfwGetFramebufferSize(a_window, &w, &h);

	return std::make_pair(static_cast<uint32_t>(w), static_cast<uint32_t>(h));
}

static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_generic_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT      a_message_severity,
	VkDebugUtilsMessageTypeFlagsEXT             a_message_type,
	const VkDebugUtilsMessengerCallbackDataEXT *a_callback_data,
	void *                                      a_user_data)
{
	(void) a_message_type;
	(void) a_user_data;

	std::string prefix{};

	switch (a_message_type)
	{
		case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
			prefix = "performance";
		case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
			prefix = "validation";
		default:        // VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
			prefix = "general";
	}

	if (a_message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		ror::log_error("Validation layer {} error: {}", prefix, a_callback_data->pMessage);
	else if (a_message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		ror::log_warn("Validation layer {} warning: {}", prefix, a_callback_data->pMessage);
	else if (a_message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)        // includes VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
		ror::log_info("Validation layer {} info: {}", prefix, a_callback_data->pMessage);
	else
		ror::log_critical("Validation layer {} critical error: {}", prefix, a_callback_data->pMessage);

	return VK_FALSE;
}

template <class _type, typename std::enable_if<std::is_same<_type, VkExtensionProperties>::value>::type * = nullptr>
FORCE_INLINE std::string get_properties_type_name(_type a_type)
{
	return a_type.extensionName;
}

template <class _type, typename std::enable_if<std::is_same<_type, VkLayerProperties>::value>::type * = nullptr>
FORCE_INLINE std::string get_properties_type_name(_type a_type)
{
	return a_type.layerName;
}

template <class _type>
using properties_type = typename std::conditional<std::is_same<_type, VkExtensionProperties>::value, VkExtensionProperties, VkLayerProperties>::type;

template <class _type, class _property, typename std::enable_if<std::is_same<_type, VkInstance>::value>::type * = nullptr, typename std::enable_if<std::is_same<_property, VkExtensionProperties>::value>::type * = nullptr>
FORCE_INLINE VkResult get_properties_function(const char *a_name, uint32_t &a_count, properties_type<_property> *a_properties, _type a_context)
{
	(void) a_context;

	return vkEnumerateInstanceExtensionProperties(a_name, &a_count, a_properties);
}

template <class _type, class _property, typename std::enable_if<std::is_same<_type, VkInstance>::value>::type * = nullptr, typename std::enable_if<std::is_same<_property, VkLayerProperties>::value>::type * = nullptr>
FORCE_INLINE VkResult get_properties_function(const char *a_name, uint32_t &a_count, properties_type<_property> *a_properties, _type a_context)
{
	(void) a_name;
	(void) a_context;

	return vkEnumerateInstanceLayerProperties(&a_count, a_properties);
}

template <class _type, class _property, typename std::enable_if<std::is_same<_type, VkPhysicalDevice>::value>::type * = nullptr, typename std::enable_if<std::is_same<_property, VkExtensionProperties>::value>::type * = nullptr>
FORCE_INLINE VkResult get_properties_function(const char *a_name, uint32_t &a_count, properties_type<_property> *a_properties, _type a_context)
{
	return vkEnumerateDeviceExtensionProperties(a_context, a_name, &a_count, a_properties);
}

template <class _type, class _property, typename std::enable_if<std::is_same<_type, VkPhysicalDevice>::value>::type * = nullptr, typename std::enable_if<std::is_same<_property, VkLayerProperties>::value>::type * = nullptr>
FORCE_INLINE VkResult get_properties_function(const char *a_name, uint32_t &a_count, properties_type<_property> *a_properties, _type a_context)
{
	(void) a_name;

	return vkEnumerateDeviceLayerProperties(a_context, &a_count, a_properties);
}

template <class _type, class _property>
FORCE_INLINE auto get_properties_requested_list()
{
	if constexpr (std::is_same<_type, VkInstance>::value)
	{
		if constexpr (std::is_same<_property, VkExtensionProperties>::value)
			return cfg::get_instance_extensions_requested();
		else
			return cfg::get_instance_layers_requested();
	}
	else if constexpr (std::is_same<_type, VkPhysicalDevice>::value)
	{
		if constexpr (std::is_same<_property, VkExtensionProperties>::value)
			return cfg::get_device_extensions_requested();
		else
			return cfg::get_device_layers_requested();
	}

	return std::vector<const char *>{};
}

template <class _type>
FORCE_INLINE std::string get_name()
{
	static std::unordered_map<std::type_index, std::string> type_names;

	// This should all be compile time constants
	type_names[std::type_index(typeid(VkInstance))]            = "instance";
	type_names[std::type_index(typeid(VkPhysicalDevice))]      = "physical device";
	type_names[std::type_index(typeid(VkExtensionProperties))] = "extension";
	type_names[std::type_index(typeid(VkLayerProperties))]     = "layer";

	return type_names[std::type_index(typeid(_type))];
}

template <class _type, class _property>
FORCE_INLINE std::string get_properties_requested_erro_message(std::string a_prefix = "!.")
{
	static std::string output{"Failed to enumerate "};

	output.append(get_name<_type>());
	output.append(" ");
	output.append(get_name<_property>());
	output.append(a_prefix);

	return output;
}

template <class _type, class _property>
std::vector<const char *> enumerate_properties(_type a_context = nullptr)
{
	uint32_t properties_count{0};
	if (get_properties_function<_type, _property>(nullptr, properties_count, nullptr, a_context) != VK_SUCCESS)
		throw std::runtime_error(get_properties_requested_erro_message<_type, _property>());

	std::vector<properties_type<_property>> properties{properties_count};
	if (get_properties_function<_type, _property>(nullptr, properties_count, properties.data(), a_context) != VK_SUCCESS)
		throw std::runtime_error(get_properties_requested_erro_message<_type, _property>(" calling it again!."));

	ror::log_info("All available {} {}s:", get_name<_type>(), get_name<_property>());
	for (const auto &property : properties)
	{
		ror::log_info("\t{}", get_properties_type_name(property));
	}

	std::vector<const char *> properties_available;

	auto properties_requested = get_properties_requested_list<_type, _property>();

	for (const auto &property_requested : properties_requested)
	{
		if (std::find_if(properties.begin(),
						 properties.end(),
						 [&property_requested](properties_type<_property> &arg) {
							 return std::strcmp(get_properties_type_name(arg).c_str(), property_requested) == 0;
						 }) != properties.end())
		{
			properties_available.emplace_back(property_requested);
		}
		else
		{
			ror::log_critical("Requested {} {} not available.", get_name<_property>(), property_requested);
		}
	}

	ror::log_info("Enabling the following {}s:", get_name<_property>());
	for (const auto &property : properties_available)
	{
		ror::log_info("\t{}", property);
	}

	return properties_available;
}

// Inspired by vulkaninfo GetVectorInit
template <class _property_type, bool _returns, typename _function, typename... _rest>
std::vector<_property_type> enumerate_general_property(_function &&a_fptr, _rest &&...a_rest_of_args)
{
	// TODO: Add some indication of function name or where the error comes from

	VkResult                    result{VK_SUCCESS};
	unsigned int                count{0};
	std::vector<_property_type> items;

	do
	{
		if constexpr (_returns)
			result = a_fptr(a_rest_of_args..., &count, nullptr);
		else
			a_fptr(a_rest_of_args..., &count, nullptr);

		assert(result == VK_SUCCESS && "enumerate general failed!");
		assert(count > 0 && "None of the properties required are available");

		items.resize(count, _property_type{});

		if constexpr (_returns)
			result = a_fptr(a_rest_of_args..., &count, items.data());
		else
			a_fptr(a_rest_of_args..., &count, items.data());

	} while (result == VK_INCOMPLETE);

	assert(result == VK_SUCCESS && "enumerate general failed!");
	assert(count > 0 && "None of the properties required are available");

	return items;
}

const uint32_t graphics_index{0};
const uint32_t compute_index{1};
const uint32_t transfer_index{2};
const uint32_t sparse_index{3};
const uint32_t protected_index{4};

const std::vector<VkQueueFlags> all_family_flags{VK_QUEUE_GRAPHICS_BIT,
												 VK_QUEUE_COMPUTE_BIT,
												 VK_QUEUE_TRANSFER_BIT,
												 VK_QUEUE_SPARSE_BINDING_BIT,
												 VK_QUEUE_PROTECTED_BIT};

struct QueueData
{
	QueueData()
	{
		this->m_indicies.resize(all_family_flags.size());
	}

	std::vector<std::pair<uint32_t, uint32_t>> m_indicies{};
};

// Assumes the queue already has a_queue_flag available

// a_others can only be one flag for this function
auto get_dedicated_queue_family(std::vector<VkQueueFamilyProperties> &a_queue_families, VkQueueFlags a_queue_flag, VkQueueFlags a_others, uint32_t &a_index)
{
	uint32_t index = 0;
	for (auto &queue_family : a_queue_families)
	{
		if (((queue_family.queueFlags & a_queue_flag) == a_queue_flag) &&
			(queue_family.queueCount > 0) &&
			!((queue_family.queueFlags & a_others) == a_others))
		{
			a_index = index;
			queue_family.queueCount--;
			return true;
		}
		index++;
	}
	return false;
}

// TODO: Extract out
auto get_priority(VkQueueFlags a_flag)
{
	if (a_flag & VK_QUEUE_GRAPHICS_BIT)
		return 0.75f;
	if (a_flag & VK_QUEUE_COMPUTE_BIT)
		return 1.00f;
	if (a_flag & VK_QUEUE_TRANSFER_BIT)
		return 0.50f;
	if (a_flag & VK_QUEUE_SPARSE_BINDING_BIT)
		return 0.20f;
	if (a_flag & VK_QUEUE_PROTECTED_BIT)
		return 0.10f;

	return 0.0f;
}

auto get_queue_indices(VkPhysicalDevice a_physical_device, VkSurfaceKHR a_surface, std::vector<float32_t *> &a_priorities_pointers, QueueData &a_queue_data)
{
	auto queue_families = enumerate_general_property<VkQueueFamilyProperties, false>(vkGetPhysicalDeviceQueueFamilyProperties, a_physical_device);

	// Other tests
	// std::vector<VkQueueFamilyProperties> queue_families{
	//	{VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_SPARSE_BINDING_BIT,
	//	 16,
	//	 64,
	//	 {1, 1, 1}},
	//	{VK_QUEUE_TRANSFER_BIT | VK_QUEUE_SPARSE_BINDING_BIT,
	//	 2,
	//	 64,
	//	 {1, 1, 1}},
	//	{VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_SPARSE_BINDING_BIT,
	//	 8,
	//	 64,
	//	 {1, 1, 1}}};

	// std::vector<VkQueueFamilyProperties> queue_families{
	//	{VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT,
	//	 2,
	//	 0,
	//	 {1, 1, 1}}};

	std::vector<std::pair<bool, uint32_t>> found_indices{};
	found_indices.resize(all_family_flags.size());

	found_indices[graphics_index].first = get_dedicated_queue_family(queue_families, VK_QUEUE_GRAPHICS_BIT, static_cast<uint32_t>(~VK_QUEUE_GRAPHICS_BIT), found_indices[graphics_index].second);
	assert(found_indices[graphics_index].first && "No graphics queue found can't continue!");

	found_indices[compute_index].first = get_dedicated_queue_family(queue_families, VK_QUEUE_COMPUTE_BIT, VK_QUEUE_GRAPHICS_BIT, found_indices[compute_index].second);

	if (!found_indices[compute_index].first)
	{
		found_indices[compute_index].first = get_dedicated_queue_family(queue_families, VK_QUEUE_COMPUTE_BIT, static_cast<uint32_t>(~VK_QUEUE_GRAPHICS_BIT), found_indices[compute_index].second);
		assert(found_indices[compute_index].first && "No compute queue found can't continue!");
	}

	found_indices[transfer_index].first = get_dedicated_queue_family(queue_families, VK_QUEUE_TRANSFER_BIT, VK_QUEUE_COMPUTE_BIT, found_indices[transfer_index].second);

	if (!found_indices[transfer_index].first)
	{
		// Get the first one that supports transfer, quite possible the one with Graphics
		found_indices[transfer_index].first = get_dedicated_queue_family(queue_families, VK_QUEUE_TRANSFER_BIT, static_cast<uint32_t>(~VK_QUEUE_COMPUTE_BIT), found_indices[transfer_index].second);
		if (!found_indices[transfer_index].first)
			found_indices[transfer_index].second = found_indices[graphics_index].second;
	}

	found_indices[sparse_index].first    = get_dedicated_queue_family(queue_families, VK_QUEUE_SPARSE_BINDING_BIT, static_cast<uint32_t>(~VK_QUEUE_SPARSE_BINDING_BIT), found_indices[sparse_index].second);
	found_indices[protected_index].first = get_dedicated_queue_family(queue_families, VK_QUEUE_PROTECTED_BIT, static_cast<uint32_t>(~VK_QUEUE_PROTECTED_BIT), found_indices[protected_index].second);

	std::vector<VkDeviceQueueCreateInfo> device_queue_create_infos{};
	device_queue_create_infos.reserve(all_family_flags.size());

	VkDeviceQueueCreateInfo device_queue_create_info{};
	device_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	device_queue_create_info.pNext = nullptr;
	device_queue_create_info.flags = 0;        // Remember if had to change, then need to use vkGetDeviceQueue2

	// device_queue_create_info.queueFamilyIndex = // Assigned in the loop
	// device_queue_create_info.queueCount = 1;        // Assigned in the loop too
	// device_queue_create_info.pQueuePriorities =  // Assigned in the loop

	std::vector<std::pair<std::optional<uint32_t>, std::vector<float32_t>>> consolidated_families;
	consolidated_families.resize(queue_families.size());

	uint32_t priority_index = 0;
	for (const auto &index : found_indices)
	{
		if (index.first)
		{
			if (!consolidated_families[index.second].first.has_value())
				consolidated_families[index.second].first = index.second;

			assert(consolidated_families[index.second].first == index.second && "Index mismatch for queue family!");
			consolidated_families[index.second].second.push_back(get_priority(all_family_flags[priority_index]));
			a_queue_data.m_indicies[priority_index] = std::make_pair(index.second, consolidated_families[index.second].second.size() - 1);
		}
		priority_index++;
	}

	{
		VkBool32 present_support = false;
		auto     result          = vkGetPhysicalDeviceSurfaceSupportKHR(a_physical_device, a_queue_data.m_indicies[graphics_index].first, a_surface, &present_support);
		assert(result == VK_SUCCESS);
		assert(present_support && "Graphics queue chosen doesn't support presentation!");
	}
	{
		VkBool32 present_support = false;
		auto     result          = vkGetPhysicalDeviceSurfaceSupportKHR(a_physical_device, a_queue_data.m_indicies[compute_index].first, a_surface, &present_support);
		assert(result == VK_SUCCESS);
		assert(present_support && "Compute queue chosen doesn't support presentation!");
	}

	for (const auto &queue_family : consolidated_families)
	{
		if (queue_family.first.has_value())
		{
			auto pptr = new float32_t[queue_family.second.size()];
			a_priorities_pointers.push_back(pptr);

			for (size_t i = 0; i < queue_family.second.size(); ++i)
			{
				pptr[i] = queue_family.second[i];
			}

			device_queue_create_info.pQueuePriorities = pptr;
			device_queue_create_info.queueFamilyIndex = queue_family.first.value();
			device_queue_create_info.queueCount       = utl::static_cast_safe<uint32_t>(queue_family.second.size());

			device_queue_create_infos.push_back(device_queue_create_info);
		}
	}

	assert(device_queue_create_infos.size() >= 1);

	return device_queue_create_infos;
}

template <typename _type>
class VulkanObject
{
  public:
	FORCE_INLINE VulkanObject()                                = default;                   //! Default constructor
	FORCE_INLINE VulkanObject(const VulkanObject &a_other)     = default;                   //! Copy constructor
	FORCE_INLINE VulkanObject(VulkanObject &&a_other) noexcept = default;                   //! Move constructor
	FORCE_INLINE VulkanObject &operator=(const VulkanObject &a_other) = default;            //! Copy assignment operator
	FORCE_INLINE VulkanObject &operator=(VulkanObject &&a_other) noexcept = default;        //! Move assignment operator
	FORCE_INLINE virtual ~VulkanObject() noexcept                         = default;        //! Destructor

	// Will/Should be called by all derived classes to initialize m_handle, it can't be default initialized
	FORCE_INLINE VulkanObject(_type handle) :
		m_handle(handle)
	{}

	FORCE_INLINE _type get_handle()
	{
		return m_handle;
	}

	FORCE_INLINE void set_handle(_type a_handle)
	{
		this->m_handle = a_handle;
	}

	FORCE_INLINE void reset()
	{
		this->m_handle = nullptr;
	}

  protected:        // Not using protected instead providing accessors
  private:
	_type m_handle{VK_NULL_HANDLE};        // Handle to object initilised with null
};

class Instance : public VulkanObject<VkInstance>
{
  public:
	FORCE_INLINE Instance(const Instance &a_other)     = default;                   //! Copy constructor
	FORCE_INLINE Instance(Instance &&a_other) noexcept = default;                   //! Move constructor
	FORCE_INLINE Instance &operator=(const Instance &a_other) = default;            //! Copy assignment operator
	FORCE_INLINE Instance &operator=(Instance &&a_other) noexcept = default;        //! Move assignment operator
	FORCE_INLINE virtual ~Instance()
	{
		vkDestroyDebugUtilsMessengerEXT(this->get_handle(), this->m_messenger, cfg::VkAllocator);
		this->m_messenger = nullptr;

		vkDestroyInstance(this->get_handle(), cfg::VkAllocator);
		this->reset();
	}

	Instance()
	{
#if defined(USE_VOLK_INSTEAD)
		volkInitialize();
#else
													 // init_vk_global_symbols();
#endif

		// Set debug messenger callback setup required later after instance creation
		VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info{};
		debug_messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debug_messenger_create_info.pNext = nullptr;

		debug_messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
													  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
													  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

		debug_messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
												  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
												  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

		debug_messenger_create_info.pfnUserCallback = vk_debug_generic_callback;
		debug_messenger_create_info.pUserData       = nullptr;        // Optional

		VkInstance        instance_handle{VK_NULL_HANDLE};
		VkApplicationInfo app_info = {};

		app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pNext              = nullptr;
		app_info.pApplicationName   = cfg::get_application_name().c_str();
		app_info.applicationVersion = cfg::get_application_version();
		app_info.pEngineName        = cfg::get_engine_name().c_str();
		app_info.engineVersion      = cfg::get_engine_version();
		app_info.apiVersion         = cfg::get_api_version();
		// Should this be result of vkEnumerateInstanceVersion

		auto extensions = vkd::enumerate_properties<VkInstance, VkExtensionProperties>();
		auto layers     = vkd::enumerate_properties<VkInstance, VkLayerProperties>();

		VkInstanceCreateInfo instance_create_info    = {};
		instance_create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instance_create_info.pNext                   = &debug_messenger_create_info;        // nullptr;
		instance_create_info.flags                   = 0;
		instance_create_info.pApplicationInfo        = &app_info;
		instance_create_info.enabledLayerCount       = utl::static_cast_safe<uint32_t>(layers.size());
		instance_create_info.ppEnabledLayerNames     = layers.data();
		instance_create_info.enabledExtensionCount   = utl::static_cast_safe<uint32_t>(extensions.size());
		instance_create_info.ppEnabledExtensionNames = extensions.data();

		VkResult result{};
		result = vkCreateInstance(&instance_create_info, cfg::VkAllocator, &instance_handle);
		assert(result == VK_SUCCESS && "Failed to create vulkan instance!");

		this->set_handle(instance_handle);

		// Now lets init all the Instance related functions
#if defined(USE_VOLK_INSTEAD)
		volkLoadInstance(instance_handle);
#else
													 // init_vk_instance_symbols(this->get_handle());
#endif

		result = vkCreateDebugUtilsMessengerEXT(this->get_handle(), &debug_messenger_create_info, cfg::VkAllocator, &m_messenger);
		assert(result == VK_SUCCESS && "Failed to create Debug Utils Messenger!");
	}

  protected:
  private:
	VkDebugUtilsMessengerEXT m_messenger{nullptr};
};

class PhysicalDevice : public VulkanObject<VkPhysicalDevice>
{
  public:
	FORCE_INLINE PhysicalDevice()                                  = delete;                    //! Copy constructor
	FORCE_INLINE PhysicalDevice(const PhysicalDevice &a_other)     = default;                   //! Copy constructor
	FORCE_INLINE PhysicalDevice(PhysicalDevice &&a_other) noexcept = default;                   //! Move constructor
	FORCE_INLINE PhysicalDevice &operator=(const PhysicalDevice &a_other) = default;            //! Copy assignment operator
	FORCE_INLINE PhysicalDevice &operator=(PhysicalDevice &&a_other) noexcept = default;        //! Move assignment operator
	FORCE_INLINE virtual ~PhysicalDevice() noexcept
	{
		for (auto &image_view : this->m_swapchain_image_views)
		{
			vkDestroyImageView(this->m_device, image_view, cfg::VkAllocator);
			image_view = nullptr;
		}

		vkDestroySwapchainKHR(this->m_device, this->m_swapchain, cfg::VkAllocator);
		this->m_swapchain = nullptr;

		vkDestroySurfaceKHR(this->m_instance, this->m_surface, nullptr);
		this->m_surface = nullptr;

		vkDestroyDevice(this->m_device, cfg::VkAllocator);
		this->m_device = nullptr;
	}

	PhysicalDevice(VkInstance a_instance, void *a_window) :
		m_instance(a_instance), m_window(a_window)
	{
		// Order of these calls is important, don't reorder
		this->create_surface(this->m_window);
		this->create_physical_device();
		this->create_device();
		this->set_handle(this->m_physical_device);
		this->create_swapchain();
		this->create_imageviews();
	}

  protected:
  private:
	void create_surface(void *a_window)
	{
		// TODO: Remove the if from here
#if defined(VULKANED_USE_GLFW)
		glfw_create_surface(this->m_instance, this->m_surface, reinterpret_cast<GLFWwindow *>(a_window));
#endif
	}

	auto get_framebuffer_size(void *a_window)
	{
#if defined(VULKANED_USE_GLFW)
		return glfw_get_buffer_size(reinterpret_cast<GLFWwindow *>(a_window));
#else
		return nullptr;
#endif
	}

	void create_physical_device()
	{
		auto gpus = enumerate_general_property<VkPhysicalDevice, true>(vkEnumeratePhysicalDevices, this->m_instance);

		for (auto gpu : gpus)
		{
			vkGetPhysicalDeviceProperties(gpu, &this->m_physical_device_properties);

			if (this->m_physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				this->m_physical_device = gpu;
				break;
			}
		}

		if (this->m_physical_device == nullptr)
		{
			ror::log_critical("Couldn't find suitable discrete physical device, falling back to integrated gpu.");
			assert(gpus.size() > 1);
			this->m_physical_device = gpus[0];
		}
	}

	void create_device()
	{
		// TODO: Select properties/features you need here
		vkGetPhysicalDeviceFeatures(this->m_physical_device, &this->m_physical_device_features);

		VkDeviceCreateInfo       device_create_info{};
		std::vector<float32_t *> priorities_pointers;
		QueueData                queue_data{};

		auto extensions = vkd::enumerate_properties<VkPhysicalDevice, VkExtensionProperties>(this->m_physical_device);
		auto layers     = vkd::enumerate_properties<VkPhysicalDevice, VkLayerProperties>(this->m_physical_device);
		auto queues     = vkd::get_queue_indices(this->m_physical_device, this->m_surface, priorities_pointers, queue_data);

		device_create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_create_info.pNext                   = nullptr;
		device_create_info.flags                   = 0;
		device_create_info.queueCreateInfoCount    = utl::static_cast_safe<uint32_t>(queues.size());
		device_create_info.pQueueCreateInfos       = queues.data();
		device_create_info.enabledLayerCount       = utl::static_cast_safe<uint32_t>(layers.size());
		device_create_info.ppEnabledLayerNames     = layers.data();
		device_create_info.enabledExtensionCount   = utl::static_cast_safe<uint32_t>(extensions.size());
		device_create_info.ppEnabledExtensionNames = extensions.data();
		device_create_info.pEnabledFeatures        = &this->m_physical_device_features;

		auto result = vkCreateDevice(this->m_physical_device, &device_create_info, cfg::VkAllocator, &this->m_device);
		assert(result == VK_SUCCESS);

		// delete priorities_pointers;
		for (auto priority : priorities_pointers)
			delete priority;
		priorities_pointers.clear();

#if defined(USE_VOLK_INSTEAD)
		// Lets init this->m_device specific symbols
		volkLoadDevice(m_device);
#else
#endif

		vkGetDeviceQueue(this->m_device, queues[queue_data.m_indicies[graphics_index].first].queueFamilyIndex, queue_data.m_indicies[graphics_index].second, &this->m_graphics_queue);
		vkGetDeviceQueue(this->m_device, queues[queue_data.m_indicies[compute_index].first].queueFamilyIndex, queue_data.m_indicies[compute_index].second, &this->m_compute_queue);
		vkGetDeviceQueue(this->m_device, queues[queue_data.m_indicies[transfer_index].first].queueFamilyIndex, queue_data.m_indicies[transfer_index].second, &this->m_transfer_queue);
		vkGetDeviceQueue(this->m_device, queues[queue_data.m_indicies[sparse_index].first].queueFamilyIndex, queue_data.m_indicies[sparse_index].second, &this->m_sparse_queue);
		vkGetDeviceQueue(this->m_device, queues[queue_data.m_indicies[protected_index].first].queueFamilyIndex, queue_data.m_indicies[protected_index].second, &this->m_protected_queue);

		// Graphics and Present queues are the same
		this->m_present_queue = this->m_graphics_queue;
	}

	void create_swapchain()
	{
		VkSurfaceCapabilitiesKHR capabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this->get_handle(), this->m_surface, &capabilities);
		assert(capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max());

		if (capabilities.currentExtent.width == 0xFFFFFFFF && capabilities.currentExtent.height == 0xFFFFFFFF)
			this->m_swapchain_extent = capabilities.currentExtent;
		else
		{
			auto extent = this->get_framebuffer_size(this->m_window);

			VkExtent2D actualExtent         = {extent.first, extent.second};
			this->m_swapchain_extent.width  = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			this->m_swapchain_extent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		}

		uint32_t image_count = cfg::get_number_of_buffers();
		if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount)
		{
			image_count = capabilities.maxImageCount;
		}
		assert(image_count >= capabilities.minImageCount && image_count <= capabilities.maxImageCount && "Min image count for swapchain is bigger than requested!\n");

		auto surface_formats = enumerate_general_property<VkSurfaceFormatKHR, true>(vkGetPhysicalDeviceSurfaceFormatsKHR, this->get_handle(), this->m_surface);

		// Choose format
		VkSurfaceFormatKHR surface_format;
		bool               surface_found = false;
		for (const auto &available_format : surface_formats)
		{
			if (available_format.format == vkd::get_surface_format() &&
				available_format.colorSpace == vkd::get_surface_colorspace())
			{
				surface_format = available_format;
				surface_found  = true;
				break;
			}
		}

		if (!surface_found)
		{
			surface_format = surface_formats[0];        // Get the first one otherwise
			surface_found  = true;
			ror::log_error("Requested surface format and color space not available, chosing the first one!\n");
		}

		assert(surface_found);

		this->m_swapchain_format = surface_format.format;

		auto present_modes = enumerate_general_property<VkPresentModeKHR, true>(vkGetPhysicalDeviceSurfacePresentModesKHR, this->get_handle(), this->m_surface);

		// Start with the only available present mode and change if requested
		VkPresentModeKHR present_mode{VK_PRESENT_MODE_FIFO_KHR};

		if (cfg::get_vsync())
		{
			present_mode = VK_PRESENT_MODE_MAX_ENUM_KHR;
			VkPresentModeKHR present_mode_required{VK_PRESENT_MODE_IMMEDIATE_KHR};

			for (const auto &available_present_mode : present_modes)
			{
				if (available_present_mode == present_mode_required)
				{
					present_mode = available_present_mode;
					break;
				}
			}

			assert(present_mode != VK_PRESENT_MODE_MAX_ENUM_KHR);
		}

		uint32_t queue_family_indices[]{0, 0};        // TODO: Get graphics and present queue indices
		auto     sci = vkd::get_swapchain_sharing_mode(queue_family_indices);

		VkSwapchainCreateInfoKHR swapchain_create_info = {};
		swapchain_create_info.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchain_create_info.pNext                    = nullptr;
		swapchain_create_info.flags                    = 0;
		swapchain_create_info.surface                  = this->m_surface;
		swapchain_create_info.minImageCount            = image_count;
		swapchain_create_info.imageFormat              = surface_format.format;
		swapchain_create_info.imageColorSpace          = surface_format.colorSpace;
		swapchain_create_info.imageExtent              = this->m_swapchain_extent;
		swapchain_create_info.imageArrayLayers         = 1;
		swapchain_create_info.imageUsage               = vkd::get_swapchain_usage();
		swapchain_create_info.imageSharingMode         = sci.imageSharingMode;
		swapchain_create_info.queueFamilyIndexCount    = sci.queueFamilyIndexCount;
		swapchain_create_info.pQueueFamilyIndices      = sci.pQueueFamilyIndices;
		swapchain_create_info.preTransform             = vkd::get_surface_transform();
		swapchain_create_info.compositeAlpha           = vkd::get_surface_composition_mode();
		swapchain_create_info.presentMode              = present_mode;
		swapchain_create_info.clipped                  = VK_TRUE;
		swapchain_create_info.oldSwapchain             = VK_NULL_HANDLE;

		auto result = vkCreateSwapchainKHR(this->m_device, &swapchain_create_info, cfg::VkAllocator, &this->m_swapchain);
		assert(result == VK_SUCCESS);

		this->m_swapchain_images = enumerate_general_property<VkImage, true>(vkGetSwapchainImagesKHR, this->m_device, this->m_swapchain);
	}

	void create_imageviews()
	{
		// Creating an image view for all swapchain images
		this->m_swapchain_image_views.resize(this->m_swapchain_images.size());

		for (size_t i = 0; i < this->m_swapchain_images.size(); ++i)
		{
			VkImageViewCreateInfo image_view_create_info = {};
			image_view_create_info.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			image_view_create_info.pNext                 = nullptr;
			image_view_create_info.flags                 = 0;
			image_view_create_info.image                 = this->m_swapchain_images[i];
			image_view_create_info.viewType              = VK_IMAGE_VIEW_TYPE_2D;
			image_view_create_info.format                = this->m_swapchain_format;

			image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			image_view_create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
			image_view_create_info.subresourceRange.baseMipLevel   = 0;
			image_view_create_info.subresourceRange.levelCount     = 1;
			image_view_create_info.subresourceRange.baseArrayLayer = 0;
			image_view_create_info.subresourceRange.layerCount     = 1;

			VkResult result = vkCreateImageView(this->m_device, &image_view_create_info, nullptr, &this->m_swapchain_image_views[i]);
			assert(result == VK_SUCCESS);
		}
	}

	VkInstance                 m_instance{nullptr};               // Alias
	VkPhysicalDevice           m_physical_device{nullptr};        // TODO: This is not required, its already in handle, change will finalize the name of the class
	VkDevice                   m_device{nullptr};
	VkSurfaceKHR               m_surface{nullptr};
	VkPhysicalDeviceFeatures   m_physical_device_features{};
	VkPhysicalDeviceProperties m_physical_device_properties;
	VkQueue                    m_graphics_queue{nullptr};
	VkQueue                    m_compute_queue{nullptr};
	VkQueue                    m_transfer_queue{nullptr};
	VkQueue                    m_present_queue{nullptr};
	VkQueue                    m_sparse_queue{nullptr};
	VkQueue                    m_protected_queue{nullptr};
	std::vector<VkImage>       m_swapchain_images;
	std::vector<VkImageView>   m_swapchain_image_views;
	VkSwapchainKHR             m_swapchain{nullptr};
	VkFormat                   m_swapchain_format{VK_FORMAT_B8G8R8A8_SRGB};
	VkExtent2D                 m_swapchain_extent{1024, 800};
	void *                     m_window{nullptr};        // Window type that can be glfw or nullptr
};

class Context
{
  public:
	FORCE_INLINE Context(const Context &a_other)     = default;                   //! Copy constructor
	FORCE_INLINE Context(Context &&a_other) noexcept = default;                   //! Move constructor
	FORCE_INLINE Context &operator=(const Context &a_other) = default;            //! Copy assignment operator
	FORCE_INLINE Context &operator=(Context &&a_other) noexcept = default;        //! Move assignment operator
	FORCE_INLINE virtual ~Context() noexcept                    = default;        //! Destructor

	FORCE_INLINE Context(GLFWwindow *a_window)
	{
		this->m_instances.emplace_back(std::make_shared<Instance>());
		this->m_gpus[0] = std::make_shared<PhysicalDevice>(this->m_instances[0]->get_handle(), a_window);
	}

  protected:
  private:
	std::vector<std::shared_ptr<Instance>>                        m_instances;
	std::unordered_map<uint32_t, std::shared_ptr<PhysicalDevice>> m_gpus;
};
}        // namespace vkd

// Loads data at sizeof(uint32_t) aligned address
bool align_load_file(const std::string &a_file_path, char **a_buffer, size_t &a_bytes_read)
{
	std::ifstream file(a_file_path, std::ios::in | std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		std::cout << "Error! opening file " << a_file_path.c_str() << std::endl;
		return false;
	}

	auto file_size = file.tellg();
	file.seekg(0, std::ios_base::beg);

	if (file_size <= 0)
	{
		std::cout << "Error! reading file size " << a_file_path.c_str() << std::endl;
		return false;
	}

	uint32_t *aligned_pointer = new uint32_t[static_cast<size_t>(file_size) / sizeof(uint32_t)];

	*a_buffer = reinterpret_cast<char *>(aligned_pointer);

	if (*a_buffer == NULL)
	{
		std::cout << "Error! Out of memory allocating *a_buffer" << std::endl;
		return false;
	}

	file.read(*a_buffer, file_size);
	file.close();

	a_bytes_read = static_cast<size_t>(file_size);

	return true;
}

void create_shader_module(const VkDevice &a_device, const char *a_shader_path, VkShaderModule &a_shader_module)
{
	char * shader_code;
	size_t shader_size;

	align_load_file(a_shader_path, &shader_code, shader_size);

	VkShaderModuleCreateInfo shader_module_info = {};
	shader_module_info.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shader_module_info.pNext                    = nullptr;
	shader_module_info.flags                    = 0;
	shader_module_info.codeSize                 = shader_size;
	shader_module_info.pCode                    = reinterpret_cast<uint32_t *>(shader_code);

	VkResult result = vkCreateShaderModule(a_device, &shader_module_info, cfg::VkAllocator, &a_shader_module);
	assert(result == VK_SUCCESS);
}

void create_render_pass(const VkDevice &a_device, VkRenderPass &a_render_pass, const VkFormat &a_swapchain_image_format)
{
	VkAttachmentDescription attachment_description = {};
	attachment_description.flags                   = 0;
	attachment_description.format                  = a_swapchain_image_format;
	attachment_description.samples                 = VK_SAMPLE_COUNT_1_BIT;
	attachment_description.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment_description.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
	attachment_description.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment_description.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment_description.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
	attachment_description.finalLayout             = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference attachment_reference = {};
	attachment_reference.attachment            = 0;
	attachment_reference.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass_description    = {};
	subpass_description.flags                   = 0;
	subpass_description.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass_description.inputAttachmentCount    = 0;
	subpass_description.pInputAttachments       = nullptr;
	subpass_description.colorAttachmentCount    = 1;
	subpass_description.pColorAttachments       = &attachment_reference;
	subpass_description.pResolveAttachments     = nullptr;
	subpass_description.pDepthStencilAttachment = nullptr;
	subpass_description.preserveAttachmentCount = 0;
	subpass_description.pPreserveAttachments    = nullptr;

	VkSubpassDependency subpass_dependency = {};
	subpass_dependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
	subpass_dependency.dstSubpass          = 0;
	subpass_dependency.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpass_dependency.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpass_dependency.srcAccessMask       = 0;
	subpass_dependency.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpass_dependency.dependencyFlags     = 0;

	VkRenderPassCreateInfo render_pass_info = {};
	render_pass_info.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.pNext                  = nullptr;
	render_pass_info.flags                  = 0;
	render_pass_info.attachmentCount        = 1;
	render_pass_info.pAttachments           = &attachment_description;
	render_pass_info.subpassCount           = 1;
	render_pass_info.pSubpasses             = &subpass_description;
	render_pass_info.dependencyCount        = 1;
	render_pass_info.pDependencies          = &subpass_dependency;

	VkResult result = vkCreateRenderPass(a_device, &render_pass_info, cfg::VkAllocator, &a_render_pass);
	assert(result == VK_SUCCESS);
}

void create_graphics_pipeline(const VkDevice &a_device, const VkExtent2D &a_swapchain_extent, const VkRenderPass &a_render_pass, VkPipeline &a_graphics_pipeline, VkPipelineLayout &a_pipeline_layout, VkPipelineCache &a_pipeline_cache)
{
	VkShaderModule vert_shader_module;
	VkShaderModule frag_shader_module;

	create_shader_module(a_device, "assets/shaders/tri.vert.spv", vert_shader_module);
	create_shader_module(a_device, "assets/shaders/tri.frag.spv", frag_shader_module);

	VkPipelineShaderStageCreateInfo vert_shader_stage_info = {};
	vert_shader_stage_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_shader_stage_info.pNext                           = nullptr;
	vert_shader_stage_info.flags                           = 0;
	vert_shader_stage_info.stage                           = VK_SHADER_STAGE_VERTEX_BIT;
	vert_shader_stage_info.module                          = vert_shader_module;
	vert_shader_stage_info.pName                           = "main";
	vert_shader_stage_info.pSpecializationInfo             = nullptr;

	VkPipelineShaderStageCreateInfo frag_shader_stage_info = {};
	frag_shader_stage_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_shader_stage_info.pNext                           = nullptr;
	frag_shader_stage_info.flags                           = 0;
	frag_shader_stage_info.stage                           = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_shader_stage_info.module                          = frag_shader_module;
	frag_shader_stage_info.pName                           = "main";
	frag_shader_stage_info.pSpecializationInfo             = nullptr;

	VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, frag_shader_stage_info};

	// This is where you add where the vertex data is coming from
	// TODO: To be abstracted later so it can be configured properly
	VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_info = {};
	pipeline_vertex_input_state_info.sType                                = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	pipeline_vertex_input_state_info.pNext                                = nullptr;
	pipeline_vertex_input_state_info.flags                                = 0;
	pipeline_vertex_input_state_info.vertexBindingDescriptionCount        = 0;
	pipeline_vertex_input_state_info.pVertexBindingDescriptions           = nullptr;        // Optional
	pipeline_vertex_input_state_info.vertexAttributeDescriptionCount      = 0;
	pipeline_vertex_input_state_info.pVertexAttributeDescriptions         = nullptr;        // Optional

	VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_info = {};
	pipeline_input_assembly_info.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	pipeline_input_assembly_info.pNext                                  = nullptr;
	pipeline_input_assembly_info.topology                               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pipeline_input_assembly_info.primitiveRestartEnable                 = VK_FALSE;

	VkViewport viewport = {};
	viewport.x          = 0.0f;
	viewport.y          = 0.0f;
	viewport.width      = (float) a_swapchain_extent.width;
	viewport.height     = (float) a_swapchain_extent.height;
	viewport.minDepth   = 0.0f;
	viewport.maxDepth   = 1.0f;

	VkRect2D scissor = {};
	scissor.offset   = {0, 0};
	scissor.extent   = a_swapchain_extent;

	VkPipelineViewportStateCreateInfo pipeline_viewport_state_info = {};
	pipeline_viewport_state_info.sType                             = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	pipeline_viewport_state_info.pNext                             = nullptr;
	pipeline_viewport_state_info.flags                             = 0;
	pipeline_viewport_state_info.viewportCount                     = 1;
	pipeline_viewport_state_info.pViewports                        = &viewport;
	pipeline_viewport_state_info.scissorCount                      = 1;
	pipeline_viewport_state_info.pScissors                         = &scissor;

	VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state_info = {};
	pipeline_rasterization_state_info.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	pipeline_rasterization_state_info.pNext                                  = nullptr;
	pipeline_rasterization_state_info.flags                                  = 0;
	pipeline_rasterization_state_info.depthClampEnable                       = VK_FALSE;
	pipeline_rasterization_state_info.rasterizerDiscardEnable                = VK_FALSE;
	pipeline_rasterization_state_info.polygonMode                            = VK_POLYGON_MODE_FILL;
	pipeline_rasterization_state_info.lineWidth                              = 1.0f;
	pipeline_rasterization_state_info.cullMode                               = VK_CULL_MODE_BACK_BIT;
	pipeline_rasterization_state_info.frontFace                              = VK_FRONT_FACE_CLOCKWISE;        // TODO: Model3d is counter clock wise fix this

	pipeline_rasterization_state_info.depthBiasEnable         = VK_FALSE;
	pipeline_rasterization_state_info.depthBiasConstantFactor = 0.0f;        // Optional
	pipeline_rasterization_state_info.depthBiasClamp          = 0.0f;        // Optional
	pipeline_rasterization_state_info.depthBiasSlopeFactor    = 0.0f;        // Optional

	VkPipelineMultisampleStateCreateInfo pipeline_multisampling_state_info = {};
	pipeline_multisampling_state_info.sType                                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	pipeline_multisampling_state_info.pNext                                = nullptr;
	pipeline_multisampling_state_info.flags                                = 0;
	pipeline_multisampling_state_info.rasterizationSamples                 = VK_SAMPLE_COUNT_1_BIT;
	pipeline_multisampling_state_info.sampleShadingEnable                  = VK_FALSE;
	pipeline_multisampling_state_info.minSampleShading                     = 1.0f;            // Optional
	pipeline_multisampling_state_info.pSampleMask                          = nullptr;         // Optional
	pipeline_multisampling_state_info.alphaToCoverageEnable                = VK_FALSE;        // Optional
	pipeline_multisampling_state_info.alphaToOneEnable                     = VK_FALSE;        // Optional

	VkPipelineDepthStencilStateCreateInfo pipeline_depth_stencil_info = {};
	pipeline_depth_stencil_info.sType                                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	pipeline_depth_stencil_info.pNext                                 = nullptr;
	pipeline_depth_stencil_info.flags                                 = 0;
	pipeline_depth_stencil_info.depthTestEnable                       = VK_FALSE;        // TODO: Depth testing should be enabled later
	pipeline_depth_stencil_info.depthWriteEnable                      = VK_FALSE;
	pipeline_depth_stencil_info.depthCompareOp                        = VK_COMPARE_OP_EQUAL;
	pipeline_depth_stencil_info.depthBoundsTestEnable                 = VK_FALSE;
	pipeline_depth_stencil_info.stencilTestEnable                     = VK_FALSE;
	// pipeline_depth_stencil_info.front = VK_STENCIL_OP_KEEP;
	// pipeline_depth_stencil_info.back = VK_STENCIL_OP_KEEP;
	pipeline_depth_stencil_info.minDepthBounds = 0.0f;
	pipeline_depth_stencil_info.maxDepthBounds = 0.0f;

	VkPipelineColorBlendAttachmentState pipeline_color_blend_attachment_info = {};
	pipeline_color_blend_attachment_info.blendEnable                         = VK_FALSE;
	pipeline_color_blend_attachment_info.srcColorBlendFactor                 = VK_BLEND_FACTOR_ONE;         // Optional
	pipeline_color_blend_attachment_info.dstColorBlendFactor                 = VK_BLEND_FACTOR_ZERO;        // Optional
	pipeline_color_blend_attachment_info.colorBlendOp                        = VK_BLEND_OP_ADD;             // Optional
	pipeline_color_blend_attachment_info.srcAlphaBlendFactor                 = VK_BLEND_FACTOR_ONE;         // Optional
	pipeline_color_blend_attachment_info.dstAlphaBlendFactor                 = VK_BLEND_FACTOR_ZERO;        // Optional
	pipeline_color_blend_attachment_info.alphaBlendOp                        = VK_BLEND_OP_ADD;             // Optional
	pipeline_color_blend_attachment_info.colorWriteMask                      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	/*
		  // Could do simple alpha blending with this
		  colorBlendAttachment.blendEnable = VK_TRUE;
		  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		*/

	VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state_info = {};
	pipeline_color_blend_state_info.sType                               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	pipeline_color_blend_state_info.pNext                               = nullptr;
	pipeline_color_blend_state_info.flags                               = 0;
	pipeline_color_blend_state_info.logicOpEnable                       = VK_FALSE;
	pipeline_color_blend_state_info.logicOp                             = VK_LOGIC_OP_COPY;
	pipeline_color_blend_state_info.attachmentCount                     = 1;
	pipeline_color_blend_state_info.pAttachments                        = &pipeline_color_blend_attachment_info;
	pipeline_color_blend_state_info.blendConstants[0]                   = 0.0f;
	pipeline_color_blend_state_info.blendConstants[1]                   = 0.0f;
	pipeline_color_blend_state_info.blendConstants[2]                   = 0.0f;
	pipeline_color_blend_state_info.blendConstants[3]                   = 0.0f;

	VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_LINE_WIDTH};

	VkPipelineDynamicStateCreateInfo pipeline_dynamic_state_info = {};
	pipeline_dynamic_state_info.sType                            = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	pipeline_dynamic_state_info.pNext                            = nullptr;
	pipeline_dynamic_state_info.flags                            = 0;
	pipeline_dynamic_state_info.dynamicStateCount                = 2;
	pipeline_dynamic_state_info.pDynamicStates                   = dynamic_states;

	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.pNext                      = nullptr;
	pipeline_layout_info.flags                      = 0;
	pipeline_layout_info.setLayoutCount             = 0;              // Optional
	pipeline_layout_info.pSetLayouts                = nullptr;        // Optional
	pipeline_layout_info.pushConstantRangeCount     = 0;              // Optional
	pipeline_layout_info.pPushConstantRanges        = 0;              // Optional

	VkResult result = vkCreatePipelineLayout(a_device, &pipeline_layout_info, cfg::VkAllocator, &a_pipeline_layout);
	assert(result == VK_SUCCESS);

	VkGraphicsPipelineCreateInfo graphics_pipeline_info = {};
	graphics_pipeline_info.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphics_pipeline_info.pNext                        = nullptr;
	graphics_pipeline_info.flags                        = 0;
	graphics_pipeline_info.stageCount                   = 2;
	graphics_pipeline_info.pStages                      = shader_stages;
	graphics_pipeline_info.pVertexInputState            = &pipeline_vertex_input_state_info;
	graphics_pipeline_info.pInputAssemblyState          = &pipeline_input_assembly_info;
	graphics_pipeline_info.pTessellationState           = nullptr;
	graphics_pipeline_info.pViewportState               = &pipeline_viewport_state_info;
	graphics_pipeline_info.pRasterizationState          = &pipeline_rasterization_state_info;
	graphics_pipeline_info.pMultisampleState            = &pipeline_multisampling_state_info;
	graphics_pipeline_info.pDepthStencilState           = &pipeline_depth_stencil_info;
	graphics_pipeline_info.pColorBlendState             = &pipeline_color_blend_state_info;
	graphics_pipeline_info.pDynamicState                = &pipeline_dynamic_state_info;
	graphics_pipeline_info.layout                       = a_pipeline_layout;
	graphics_pipeline_info.renderPass                   = a_render_pass;
	graphics_pipeline_info.subpass                      = 0;
	graphics_pipeline_info.basePipelineHandle           = VK_NULL_HANDLE;
	graphics_pipeline_info.basePipelineIndex            = -1;

	result = vkCreateGraphicsPipelines(a_device, a_pipeline_cache, 1, &graphics_pipeline_info, cfg::VkAllocator, &a_graphics_pipeline);
	assert(result == VK_SUCCESS);

	// cleanup
	vkDestroyShaderModule(a_device, vert_shader_module, cfg::VkAllocator);
	vkDestroyShaderModule(a_device, frag_shader_module, cfg::VkAllocator);
	vkDestroyPipelineLayout(a_device, a_pipeline_layout, cfg::VkAllocator);
}

void create_framebuffers(const VkDevice &a_device, std::vector<VkFramebuffer> &a_swapchain_framebuffers, const std::vector<VkImageView> &a_swapchain_image_views, const VkRenderPass &a_render_pass, const VkExtent2D &a_swapchain_extent)
{
	// Create a framebuffer for each swapchain imageview
	a_swapchain_framebuffers.resize(a_swapchain_image_views.size());

	for (size_t i = 0; i < a_swapchain_image_views.size(); i++)
	{
		VkImageView attachments[] = {a_swapchain_image_views[i]};

		VkFramebufferCreateInfo framebuffer_info = {};
		framebuffer_info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.pNext                   = nullptr;
		framebuffer_info.flags                   = 0;
		framebuffer_info.renderPass              = a_render_pass;
		framebuffer_info.attachmentCount         = 1;
		framebuffer_info.pAttachments            = attachments;
		framebuffer_info.width                   = a_swapchain_extent.width;
		framebuffer_info.height                  = a_swapchain_extent.height;
		framebuffer_info.layers                  = 1;

		VkResult result = vkCreateFramebuffer(a_device, &framebuffer_info, cfg::VkAllocator, &a_swapchain_framebuffers[i]);
		assert(result == VK_SUCCESS);
	}
}

void create_command_pool(const VkDevice &a_device, VkCommandPool &a_command_pool, uint32_t a_queue_family_index)
{
	VkCommandPoolCreateInfo command_pool_info = {};
	command_pool_info.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_info.pNext                   = nullptr;
	command_pool_info.flags                   = 0;        // VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
	command_pool_info.queueFamilyIndex        = a_queue_family_index;

	VkResult result = vkCreateCommandPool(a_device, &command_pool_info, cfg::VkAllocator, &a_command_pool);
	assert(result == VK_SUCCESS);
}

void create_command_buffers(const VkDevice &a_device, std::vector<VkCommandBuffer> &a_command_buffers, const VkCommandPool &a_command_pool, const std::vector<VkFramebuffer> &a_swapchain_framebuffers)
{
	a_command_buffers.resize(a_swapchain_framebuffers.size());

	VkCommandBufferAllocateInfo command_buffer_allocation_info = {};
	command_buffer_allocation_info.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_allocation_info.pNext                       = nullptr;
	command_buffer_allocation_info.commandPool                 = a_command_pool;
	command_buffer_allocation_info.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_buffer_allocation_info.commandBufferCount          = (uint32_t) a_command_buffers.size();

	VkResult result = vkAllocateCommandBuffers(a_device, &command_buffer_allocation_info, a_command_buffers.data());
	assert(result == VK_SUCCESS);
}

void record_command_buffers(const std::vector<VkCommandBuffer> &a_command_buffers, const VkRenderPass &a_render_pass, const std::vector<VkFramebuffer> &a_swapchain_framebuffers, const VkExtent2D &a_swapchain_extent, VkPipeline &a_graphics_pipeline)
{
	for (size_t i = 0; i < a_command_buffers.size(); i++)
	{
		const VkCommandBuffer &current_command_buffer = a_command_buffers[i];

		VkCommandBufferBeginInfo command_buffer_begin_info = {};
		command_buffer_begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		command_buffer_begin_info.pNext                    = nullptr;
		command_buffer_begin_info.flags                    = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		command_buffer_begin_info.pInheritanceInfo         = nullptr;        // Optional

		vkBeginCommandBuffer(current_command_buffer, &command_buffer_begin_info);

		VkClearValue clear_color = {{{1.0f, 0.5f, 1.0f, 1.0f}}};        // What the hell is compiler complaining about here

		VkRenderPassBeginInfo render_pass_begin_info = {};
		render_pass_begin_info.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_begin_info.pNext                 = nullptr;
		render_pass_begin_info.renderPass            = a_render_pass;
		render_pass_begin_info.framebuffer           = a_swapchain_framebuffers[i];
		render_pass_begin_info.renderArea.offset     = {0, 0};
		render_pass_begin_info.renderArea.extent     = a_swapchain_extent;
		render_pass_begin_info.clearValueCount       = 1;
		render_pass_begin_info.pClearValues          = &clear_color;

		vkCmdBeginRenderPass(current_command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);        // VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS for secondary

		VkViewport viewport = {};
		viewport.x          = 0.0f;
		viewport.y          = 0.0f;
		viewport.width      = (float) a_swapchain_extent.width;
		viewport.height     = (float) a_swapchain_extent.height;
		viewport.minDepth   = 0.0f;
		viewport.maxDepth   = 1.0f;

		vkCmdBindPipeline(current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, a_graphics_pipeline);
		vkCmdSetViewport(current_command_buffer, 0, 1, &viewport);

		vkCmdDraw(current_command_buffer, 3, 1, 0, 0);
		vkCmdEndRenderPass(current_command_buffer);

		VkResult result = vkEndCommandBuffer(current_command_buffer);
		assert(result == VK_SUCCESS);
	}
}

void create_semaphore(const VkDevice &a_device, VkSemaphore &a_semaphore)
{
	VkSemaphoreCreateInfo semaphore_info = {};
	semaphore_info.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphore_info.pNext                 = nullptr;
	semaphore_info.flags                 = 0;

	VkResult result = vkCreateSemaphore(a_device, &semaphore_info, cfg::VkAllocator, &a_semaphore);
	assert(result == VK_SUCCESS);
}
