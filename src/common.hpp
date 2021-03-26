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
