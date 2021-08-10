#ifndef TYPEDEFS_AND_MACROS_H
#define TYPEDEFS_AND_MACROS_H

#include <stdint.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include "to_string.h"

//------- TYPEDEFS ---------

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef int8_t   int8;
typedef int16_t  int16;
typedef int32_t  int32;


//----- MACROS ------

//NOTE: Following macros from the official Vulkan Samples repo:

#define LOGGER_FORMAT "[%^%l%$] %v"
#define PROJECT_NAME "VulkanDemos"

// #define __FILENAME__ (static_cast<const char *>(__FILE__) + ROOT_PATH_SIZE)

#define LOGI(...) spdlog::get("_logger")->info(__VA_ARGS__);
#define LOGW(...) spdlog::get("_logger")->warn(__VA_ARGS__);
#define LOGE(...) spdlog::get("_logger")->error("[{}:{}] {}", __FILE__, __LINE__, fmt::format(__VA_ARGS__));
#define LOGE_EXIT(...) spdlog::get("_logger")->error("[{}:{}] {}", __FILE__, __LINE__, fmt::format(__VA_ARGS__)); exit(1);
#define LOGD(...) spdlog::get("_logger")->debug(__VA_ARGS__);

#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		VkResult err = x;                                           \
		if (err)                                                    \
		{                                                           \
			LOGE("Detected Vulkan error: {}", vulkanToString(err)); \
			exit(1);                                                \
		}                                                           \
	} while (0)

#define ASSERT_VK_HANDLE(handle)        \
	do                                  \
	{                                   \
		if ((handle) == VK_NULL_HANDLE) \
		{                               \
			LOGE("Handle is NULL");     \
			abort();                    \
		}                               \
	} while (0)

#endif