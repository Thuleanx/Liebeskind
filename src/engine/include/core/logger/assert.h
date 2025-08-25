#pragma once

#include "core/logger/logger.h"
#include <cpptrace/cpptrace.hpp>

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
            LLOG_ERROR << "Stack trace: " <<                                  \
                cpptrace::generate_trace().to_string(true);                   \
			std::terminate();                                                 \
        }                                                                     \
    } while (false)
#endif
