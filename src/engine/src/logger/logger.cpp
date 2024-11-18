#include "logger/logger.h"

#include "plog/Appenders/ColorConsoleAppender.h"
#include "plog/Formatters/TxtFormatter.h"
#include "plog/Severity.h"
#include "plog/Initializers/RollingFileInitializer.h"

namespace Logging {
bool initializeLogger() {
    // TODO: create log file
    static plog::ColorConsoleAppender<plog::TxtFormatter> appender;
    plog::init(plog::Severity::debug, "liebeskind_runtime.log").addAppender(&appender);
    PLOG_INFO << "Logger initialized.";
    return true;
}
}
