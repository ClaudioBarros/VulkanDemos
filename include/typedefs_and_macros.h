#ifndef TYPEDEFS_AND_MACROS_H
#define TYPEDEFS_AND_MACROS_H

#include <stdint.h>

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;

#define VK_CHECK(x) do {\
        	VkResult err = x;\
			if (err) {std::runtime_error("VULKAN ERROR: ");}\
            } while (0)

#endif