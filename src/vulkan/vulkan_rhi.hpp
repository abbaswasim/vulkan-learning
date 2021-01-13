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

#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <type_traits>

#undef VK_NO_PROTOTYPES
#include "profiling/rorlog.hpp"
#include "roar.hpp"
#include "vulkan/vulkan.h"        // This shouldn't be included anywhere else except here

#include <GLFW/glfw3.h>

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace cfg        // Should be moved out later
{
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

#define CFG_VK_MAKE_VERSION(major, minor, patch) \
	(((static_cast<uint32_t>(major)) << 22) | ((static_cast<uint32_t>(minor)) << 12) | (static_cast<uint32_t>(patch)))

// clang-format off
FORCE_INLINE std::string get_application_name()    { return "VulkanEd Application";}
FORCE_INLINE uint32_t    get_application_version() { return CFG_VK_MAKE_VERSION(1, 0, 0);}
FORCE_INLINE std::string get_engine_name()         { return "VulkanEd Engine";}
FORCE_INLINE uint32_t    get_engine_version()      { return CFG_VK_MAKE_VERSION(1, 0, 0);}
FORCE_INLINE uint32_t    get_api_version()         { return CFG_VK_MAKE_VERSION(1, 2, 0);}
// clang-format on

FORCE_INLINE std::vector<const char *> get_instance_extensions_requested()
{
	/* All available extensions
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
	  VK_MVK_macos_surface
	*/
	return std::vector<const char *>
	{
		"VK_KHR_surface",
#if defined __APPLE__
			"VK_EXT_metal_surface"
#elif defined __linux__
			"VK_EXT_xcb_surface??"
#endif
	};
}

FORCE_INLINE std::vector<const char *> get_instance_layers_requested()
{
	/* All available layers
	  VK_LAYER_LUNARG_api_dump
	  VK_LAYER_KHRONOS_validation
	*/
	return std::vector<const char *>{"VK_LAYER_KHRONOS_validation"};
}

FORCE_INLINE std::vector<const char *> get_device_extensions_requested()
{
	return std::vector<const char *>{};
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
std::vector<const char *> enumerate_instance_extensions()
{
	uint32_t     glfwExtensionCount = 0;
	const char **glfwExtensions;

	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	uint32_t extensions_properties_count{0};
	vkEnumerateInstanceExtensionProperties(nullptr, &extensions_properties_count, nullptr);

	std::vector<VkExtensionProperties> extensions_properties{extensions_properties_count};
	vkEnumerateInstanceExtensionProperties(nullptr, &extensions_properties_count, extensions_properties.data());

	ror::log_info("All available extensions:");
	for (const auto &extension : extensions_properties)
	{
		ror::log_info("\t{}", extension.extensionName);
	}

	std::vector<const char *> extensions_available;
	auto                      extensions_requested = cfg::get_instance_extensions_requested();

	for (const auto &extension_requested : extensions_requested)
	{
		if (std::find_if(extensions_properties.begin(),
						 extensions_properties.end(),
						 [&extension_requested](VkExtensionProperties &arg) {
							 return std::strcmp(arg.extensionName, extension_requested) == 0;
						 }) != extensions_properties.end())
		{
			extensions_available.emplace_back(extension_requested);
		}
		else
		{
			ror::log_critical("Requested extension {} not available.", extension_requested);
		}
	}

	return extensions_available;
}

std::vector<const char *> enumerate_instance_layers()
{
	uint32_t layer_properties_count;
	vkEnumerateInstanceLayerProperties(&layer_properties_count, nullptr);

	std::vector<VkLayerProperties> layers_properties(layer_properties_count);
	vkEnumerateInstanceLayerProperties(&layer_properties_count, layers_properties.data());

	ror::log_info("All available layers:");
	for (const auto &layers : layers_properties)
	{
		ror::log_info("\t{}", layers.layerName);
	}

	std::vector<const char *> layers_available;
	auto                      layers_requested = cfg::get_instance_layers_requested();

	for (const auto &layer_requested : layers_requested)
	{
		if (std::find_if(layers_properties.begin(),
						 layers_properties.end(),
						 [&layer_requested](VkLayerProperties &arg) {
							 return std::strcmp(arg.layerName, layer_requested) == 0;
						 }) != layers_properties.end())
		{
			layers_available.emplace_back(layer_requested);
		}
		else
		{
			ror::log_error("Requested layer {} not available.", layer_requested);
		}
	}

	return layers_available;
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
		vkDestroyInstance(this->get_handle(), cfg::VkAllocator);
	}

	Instance()
	{
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

		auto extensions = vkd::enumerate_instance_extensions();
		auto layers     = vkd::enumerate_instance_layers();

		VkInstanceCreateInfo instance_create_info    = {};
		instance_create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instance_create_info.pNext                   = nullptr;
		instance_create_info.flags                   = 0;
		instance_create_info.pApplicationInfo        = &app_info;
		instance_create_info.enabledLayerCount       = utl::static_cast_safe<uint32_t>(layers.size());
		instance_create_info.ppEnabledLayerNames     = layers.data();
		instance_create_info.enabledExtensionCount   = utl::static_cast_safe<uint32_t>(extensions.size());
		instance_create_info.ppEnabledExtensionNames = extensions.data();

		if (vkCreateInstance(&instance_create_info, cfg::VkAllocator, &instance_handle) != VK_SUCCESS)
			throw std::runtime_error("Failed to create vulkan instance!");

		this->set_handle(instance_handle);
	}

  protected:
  private:
};

class PhysicalDevice : public VulkanObject<VkPhysicalDevice>
{
  public:
	FORCE_INLINE PhysicalDevice(const PhysicalDevice &a_other)     = default;                   //! Copy constructor
	FORCE_INLINE PhysicalDevice(PhysicalDevice &&a_other) noexcept = default;                   //! Move constructor
	FORCE_INLINE PhysicalDevice &operator=(const PhysicalDevice &a_other) = default;            //! Copy assignment operator
	FORCE_INLINE PhysicalDevice &operator=(PhysicalDevice &&a_other) noexcept = default;        //! Move assignment operator
	FORCE_INLINE virtual ~PhysicalDevice() noexcept                           = default;        //! Destructor

	PhysicalDevice()
	{
		this->set_handle(VK_NULL_HANDLE);
	}

  protected:
  private:
};

class Context
{
  public:
	FORCE_INLINE Context(const Context &a_other)     = default;                   //! Copy constructor
	FORCE_INLINE Context(Context &&a_other) noexcept = default;                   //! Move constructor
	FORCE_INLINE Context &operator=(const Context &a_other) = default;            //! Copy assignment operator
	FORCE_INLINE Context &operator=(Context &&a_other) noexcept = default;        //! Move assignment operator
	FORCE_INLINE virtual ~Context() noexcept                    = default;        //! Destructor

	FORCE_INLINE Context()
	{
		this->m_instances.emplace_back(std::make_shared<Instance>());
		this->m_gpus[0] = std::make_shared<PhysicalDevice>(PhysicalDevice{});
	}

  protected:
  private:
	std::vector<std::shared_ptr<Instance>>                        m_instances;
	std::unordered_map<uint32_t, std::shared_ptr<PhysicalDevice>> m_gpus;
};
}        // namespace vkd
