#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-includes"
#include "logger/assert.h"
#pragma GCC diagnostic pop

#define VULKAN_ENSURE_SUCCESS(name, msg) ASSERT(name == vk::Result::eSuccess, msg << " " << to_string(name))
#define VULKAN_ENSURE_SUCCESS_EXPR(expr, msg) { auto cached = expr; ASSERT(cached == vk::Result::eSuccess, msg << " " << to_string(cached)); }
