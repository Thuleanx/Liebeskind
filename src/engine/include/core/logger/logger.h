#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#include "plog/Log.h"
#pragma GCC diagnostic pop

#define LLOG_VERBOSE PLOG_VERBOSE
#define LLOG_INFO PLOG_INFO
#define LLOG_WARNING PLOG_WARNING
#define LLOG_ERROR PLOG_ERROR
#define LLOG_FATAL PLOG_FATAL

namespace Logging {
    bool initializeLogger();
}

