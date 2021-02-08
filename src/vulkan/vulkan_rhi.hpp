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

// #define USE_VOLK_INSTEAD

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
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

#if defined(USE_VOLK_INSTEAD)
#	define VK_ENABLE_BETA_EXTENSIONS
#	include "volk.h"
#else
#	include "vusym.hpp"
#endif

// #define GLFW_INCLUDE_VULKAN // Not required if vulkan.h included before this
#include <GLFW/glfw3.h>

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
void glfw_create_surface(VkInstance &a_instance, VkSurfaceKHR &a_surface, void *a_window)
{
	assert(a_instance);
	assert(a_window);

	VkResult status = glfwCreateWindowSurface(a_instance, reinterpret_cast<GLFWwindow *>(a_window), nullptr, &a_surface);

	if (status != VK_SUCCESS)
		ror::log_critical("WARNING! Window surface creation failed");
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

template <class _property_type, bool _returns, class _context_type, typename _function>
std::vector<_property_type> enumerate_general_property(_function a_fptr, _context_type a_context = nullptr)
{
	VkResult result = VK_SUCCESS;

	unsigned int count;
	if constexpr (_returns)
		result = a_fptr(a_context, &count, nullptr);
	else
		a_fptr(a_context, &count, nullptr);

	assert(result == VK_SUCCESS && "enumerate general failed!");
	assert(count > 0 && "None of the properties required are available");

	std::vector<_property_type> items(count);
	if constexpr (_returns)
		result = a_fptr(a_context, &count, items.data());
	else
		a_fptr(a_context, &count, items.data());

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
		vkDestroySwapchainKHR(this->m_device, this->m_swapchain, cfg::VkAllocator);
		this->m_swapchain = nullptr;

		vkDestroySurfaceKHR(this->m_instance, this->m_surface, nullptr);
		this->m_surface = nullptr;

		vkDestroyDevice(this->m_device, cfg::VkAllocator);
		this->m_device = nullptr;
	}

	PhysicalDevice(VkInstance a_instance, void *a_window) :
		m_instance(a_instance)
	{
		// Order of these calls is important, don't reorder
		this->create_surface(a_window);
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
		glfw_create_surface(this->m_instance, this->m_surface, a_window);
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

		std::vector<VkSurfaceFormatKHR> surface_formats;
		std::vector<VkPresentModeKHR>   present_modes;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this->get_handle(), this->m_surface, &capabilities);
		assert(capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max());

		// TODO: There is better way of doing this
		this->m_swapchain_extent = capabilities.currentExtent;
		uint32_t image_count     = capabilities.minImageCount + 1;
		if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount)
		{
			image_count = capabilities.maxImageCount;
		}

		uint32_t format_count;
		VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(this->get_handle(), this->m_surface, &format_count, nullptr);
		assert(result == VK_SUCCESS);
		surface_formats.resize(format_count);
		result = vkGetPhysicalDeviceSurfaceFormatsKHR(this->get_handle(), this->m_surface, &format_count, surface_formats.data());
		assert(result == VK_SUCCESS);

		// Choose format
		VkSurfaceFormatKHR surface_format;
		bool               surface_found = false;
		for (const auto &available_format : surface_formats)
		{
			if (available_format.format == VK_FORMAT_B8G8R8A8_UNORM && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				surface_format = available_format;
				surface_found  = true;
				break;
			}
		}

		assert(surface_found);

		this->m_swapchain_format = surface_format.format;

		uint32_t present_mode_count;
		result = vkGetPhysicalDeviceSurfacePresentModesKHR(this->get_handle(), this->m_surface, &present_mode_count, nullptr);
		assert(result == VK_SUCCESS);
		present_modes.resize(present_mode_count);
		result = vkGetPhysicalDeviceSurfacePresentModesKHR(this->get_handle(), this->m_surface, &present_mode_count, present_modes.data());
		assert(result == VK_SUCCESS);

		// Choose present mode
		VkPresentModeKHR present_mode = VK_PRESENT_MODE_MAX_ENUM_KHR;

		for (const auto &available_present_mode : present_modes)
		{
			if (available_present_mode == VK_PRESENT_MODE_FIFO_KHR)
			{
				present_mode = available_present_mode;
				break;
			}
		}

		assert(present_mode != VK_PRESENT_MODE_MAX_ENUM_KHR);

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
		swapchain_create_info.imageUsage               = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;        // TODO: Need to rethink this one
		swapchain_create_info.imageSharingMode         = VK_SHARING_MODE_EXCLUSIVE;                  // TODO: Using same family index for same queues, configure/fix this too
		swapchain_create_info.queueFamilyIndexCount    = 0;
		swapchain_create_info.pQueueFamilyIndices      = nullptr;
		swapchain_create_info.preTransform             = capabilities.currentTransform;
		swapchain_create_info.compositeAlpha           = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchain_create_info.presentMode              = present_mode;
		swapchain_create_info.clipped                  = VK_TRUE;
		swapchain_create_info.oldSwapchain             = VK_NULL_HANDLE;

		result = vkCreateSwapchainKHR(this->m_device, &swapchain_create_info, cfg::VkAllocator, &this->m_swapchain);
		assert(result == VK_SUCCESS);

		vkGetSwapchainImagesKHR(this->m_device, this->m_swapchain, &image_count, nullptr);
		this->m_swapchain_images.resize(image_count);
		vkGetSwapchainImagesKHR(this->m_device, this->m_swapchain, &image_count, this->m_swapchain_images.data());
	}

	void create_imageviews()
	{
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
	VkSwapchainKHR             m_swapchain{nullptr};
	VkFormat                   m_swapchain_format{VK_FORMAT_B8G8R8A8_SRGB};
	VkExtent2D                 m_swapchain_extent{1024, 800};
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
