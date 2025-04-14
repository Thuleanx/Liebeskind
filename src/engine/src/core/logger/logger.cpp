#include "core/logger/logger.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#include "plog/Appenders/ColorConsoleAppender.h"
// Txt formatter prints out the timestamp which doesn't really make sense to me
/* #include "plog/Formatters/TxtFormatter.h" */
#include "plog/Initializers/RollingFileInitializer.h"
#include "plog/Severity.h"
#pragma GCC diagnostic pop

namespace plog {
class MyFormatter {
   public:
    static util::nstring header() {
        return util::nstring();
    }

    static util::nstring format(const Record& record) {
        util::nostringstream ss;
        ss << std::setw(5) << std::left << severityToString(record.getSeverity()) << PLOG_NSTR(" ");
        ss << PLOG_NSTR("[") << record.getTid() << PLOG_NSTR("] ");
        ss << PLOG_NSTR("[") << record.getFunc() << PLOG_NSTR("@") << record.getLine() << PLOG_NSTR("] ");
        ss << record.getMessage() << PLOG_NSTR("\n");

        return ss.str();
    }
};
}  // namespace plog

namespace Logging {
bool initializeLogger() {
    // TODO: create log file
    static plog::ColorConsoleAppender<plog::MyFormatter> appender;
    plog::init(plog::Severity::debug, "liebeskind_runtime.log")
        .addAppender(&appender);
    PLOG_INFO << "Logger initialized.";
    return true;
}
}  // namespace Logging
