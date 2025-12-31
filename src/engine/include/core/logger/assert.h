#pragma once

#include "core/logger/logger.h"

#ifdef NDEBUG
#define ASSERT(condition, message) \
	do {                           \
	} while (false)
#else
#define ASSERT(condition, message)                                            \
	do {                                                                      \
		if (!(condition)) {                                                   \
			LLOG_ERROR << "Assertion `" #condition "` failed in " << __FILE__ \
					   << " line " << __LINE__ << ": " << message             \
					   << std::endl;                                          \
			std::terminate();                                                 \
        }                                                                     \
    } while (false)
#endif
