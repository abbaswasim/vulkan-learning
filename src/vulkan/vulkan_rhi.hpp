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

#include "profiling/rorlog.hpp"
#include "roar.hpp"

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
#	include "vk_symbols_generated.hpp"
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
		VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME        // "VK_KHR_portability_subset"
	};
}

FORCE_INLINE std::vector<const char *> get_device_layers_requested()
{
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
// struct PFN_Table
// {
//	PFN_Table()
//	{}
//	PFN_Table(VkDevice this->m_device) :
//		real_device(this->m_device)
//	{
//		// Do the thing here, populate the table
//		// load_device_function();
//	}

//	VkDevice             real_device;
//	PFN_vkAllocateMemory vkAllocateMemory    = nullptr;
//	PFN_vkCreateDevice   vkCreateDevice_real = nullptr;
// };

// static std::unordered_map<VkDevice *, PFN_Table> table;

// VKAPI_ATTR VkResult VKAPI_CALL vkAllocateMemory(VkDevice                     this->m_device,
//												const VkMemoryAllocateInfo * pAllocateInfo,
//												const VkAllocationCallbacks *pAllocator,
//												VkDeviceMemory *             pMemory)
// {
//	auto d = reinterpret_cast<PFN_Table *>(this->m_device);
//	d->vkAllocateMemory(d->real_device, pAllocateInfo, pAllocator, pMemory);

//	return VK_SUCCESS;
// }

// VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice(VkPhysicalDevice             physicalDevice,
//											  const VkDeviceCreateInfo *   pCreateInfo,
//											  const VkAllocationCallbacks *pAllocator,
//											  VkDevice *                   pDevice)
// {
//	PFN_vkCreateDevice vkCreateDevice_real = nullptr;
//	vkCreateDevice_real(physicalDevice, pCreateInfo, pAllocator, pDevice);
//	auto d = new PFN_Table(*pDevice);

//	pDevice = reinterpret_cast<VkDevice *>(d);

//	return VK_SUCCESS;
// }

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

auto get_queue_indices(VkPhysicalDevice a_physical_device, VkSurfaceKHR a_surface,
					   float &graphics_queue_priority, float &compute_queue_priority, float &transfer_queue_priority,
					   const uint32_t &graphics_queue_index, const uint32_t &compute_queue_index, const uint32_t &transfer_queue_index)
{
	auto queue_families = enumerate_general_property<VkQueueFamilyProperties, false>(vkGetPhysicalDeviceQueueFamilyProperties, a_physical_device);

	std::vector<VkDeviceQueueCreateInfo> device_queue_create_infos{};
	// device_queue_create_infos.reserve(3);
	device_queue_create_infos.resize(3);

	VkDeviceQueueCreateInfo device_queue_create_info{};

	device_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	device_queue_create_info.pNext = nullptr;
	device_queue_create_info.flags = 0;        // Remember if had to change, then need to use vkGetDeviceQueue2
	// device_queue_create_info.queueFamilyIndex = // assigned in the loop
	device_queue_create_info.queueCount = 1;
	// device_queue_create_info.pQueuePriorities =  // Assigned in the loop

	uint32_t index_count = 0;
	bool     found_graphics_queue{false};
	bool     found_compute_queue{false};
	bool     found_transfer_queue{false};

	for (const auto &queue_family : queue_families)
	{
		for (size_t i = 0; i < queue_family.queueCount; ++i)
		{
			VkBool32 present_support = false;

			auto result = vkGetPhysicalDeviceSurfaceSupportKHR(a_physical_device, index_count, a_surface, &present_support);
			assert(result == VK_SUCCESS);

			if (!found_graphics_queue && queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT && present_support)
			{
				device_queue_create_info.pQueuePriorities       = &graphics_queue_priority;
				device_queue_create_info.queueFamilyIndex       = index_count;
				device_queue_create_infos[graphics_queue_index] = device_queue_create_info;
				found_graphics_queue                            = true;
				break;
			}

			if (!found_compute_queue && queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT)
			{
				device_queue_create_info.pQueuePriorities      = &compute_queue_priority;
				device_queue_create_info.queueFamilyIndex      = index_count;
				device_queue_create_infos[compute_queue_index] = device_queue_create_info;
				found_compute_queue                            = true;
				break;
			}

			if (!found_transfer_queue && queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT)
			{
				device_queue_create_info.pQueuePriorities       = &transfer_queue_priority;
				device_queue_create_info.queueFamilyIndex       = index_count;
				device_queue_create_infos[transfer_queue_index] = device_queue_create_info;
				found_transfer_queue                            = true;
				break;
			}
		}

		index_count++;
	}

	assert(device_queue_create_infos.size() == 3);        // Probably very rigid

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
		init_vk_global_symbols();
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
		init_vk_instance_symbols(this->get_handle());
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
		vkDestroySurfaceKHR(this->m_instance, this->m_surface, nullptr);
		this->m_surface = nullptr;

		vkDestroyDevice(this->m_device, cfg::VkAllocator);
		this->m_device = nullptr;
	}

	void create_surface(void *a_window)
	{
		glfw_create_surface(this->m_instance, this->m_surface, a_window);
	}

	PhysicalDevice(VkInstance a_instance, void *a_window) :
		m_instance(a_instance)
	{
		this->create_surface(a_window);

		VkResult result = VK_SUCCESS;

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

		// TODO: Select properties you need here
		vkGetPhysicalDeviceFeatures(this->m_physical_device, &this->m_physical_device_features);

		float graphics_queue_priority = 1.0f;
		float compute_queue_priority  = 0.75f;
		float transfer_queue_priority = 0.50f;

		uint32_t graphics_queue_index = 0;
		uint32_t compute_queue_index  = 1;
		uint32_t transfer_queue_index = 2;

		VkDeviceCreateInfo device_create_info{};

		auto extensions = vkd::enumerate_properties<VkPhysicalDevice, VkExtensionProperties>(this->m_physical_device);
		auto layers     = vkd::enumerate_properties<VkPhysicalDevice, VkLayerProperties>(this->m_physical_device);
		auto queues     = vkd::get_queue_indices(this->m_physical_device, this->m_surface, graphics_queue_priority, compute_queue_priority, transfer_queue_priority, graphics_queue_index, compute_queue_index, transfer_queue_index);

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

		result = vkCreateDevice(this->m_physical_device, &device_create_info, cfg::VkAllocator, &this->m_device);
		assert(result == VK_SUCCESS);

// Lets init this->m_device specific symbols
#if defined(USE_VOLK_INSTEAD)
		volkLoadDevice(m_device);
#else
		init_vk_device_symbols(this->m_device);
#endif

		vkGetDeviceQueue(this->m_device, queues[graphics_queue_index].queueFamilyIndex, 0, &this->m_graphics_queue);
		vkGetDeviceQueue(this->m_device, queues[compute_queue_index].queueFamilyIndex, 0, &this->m_compute_queue);
		vkGetDeviceQueue(this->m_device, queues[transfer_queue_index].queueFamilyIndex, 0, &this->m_transfer_queue);

		// Tranfer and Present queues are the same
		this->m_present_queue = this->m_graphics_queue;

		this->set_handle(this->m_physical_device);
	}

  protected:
  private:
	VkInstance                 m_instance{nullptr};        // Alias
	VkPhysicalDevice           m_physical_device{nullptr};
	VkDevice                   m_device{nullptr};
	VkSurfaceKHR               m_surface{nullptr};
	VkPhysicalDeviceFeatures   m_physical_device_features{};
	VkPhysicalDeviceProperties m_physical_device_properties;
	VkQueue                    m_graphics_queue{nullptr};
	VkQueue                    m_compute_queue{nullptr};
	VkQueue                    m_transfer_queue{nullptr};
	VkQueue                    m_present_queue{nullptr};
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
