#include "clib/log/worker/console.h"

using namespace clib;
using namespace log;

void Console::log(const std::string& level, const std::string& content, const Datetime& datetime) noexcept {
	printf("%s   [%s]   \"%s\"\n", level.c_str(), datetime.toString().c_str(), content.c_str());
}