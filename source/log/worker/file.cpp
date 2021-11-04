#include <filesystem>

#include "clib/util.hpp"
#include "clib/log/worker/file.h"

using namespace clib;
using namespace log;

File::File(const std::string& directory) 
	: now{},
	  file{ nullptr }, 
	  directory{ directory } {
	if (!directory.empty()) {
		makeFile(this->now);
	}
}

File::File(File&& other) noexcept 
	: now{ std::move(other.now) },
	  file{ other.file },
	  directory{ std::move(other.directory) } {
	other.file = nullptr;
}

File::~File() noexcept {
	if (this->file) {
		std::fclose(this->file);
	}
}

File& File::operator=(File&& other) noexcept {
	this->now = std::move(other.now);
	this->directory = std::move(other.directory);
	this->file = other.file;
	other.file = nullptr;
	return *this;
}

void File::log(const std::string& level, const std::string& content, const Datetime& datetime) noexcept {
	if (this->now.compareDate(datetime) < 0) {
		std::fclose(this->file);
		makeFile(datetime);
	}
	fprintf(this->file, "%s   [%s]   \"%s\"\n", level.c_str(), datetime.toString().c_str(), content.c_str());
}

void File::makeFile(const Datetime& datetime) {
	std::string path{ format("%s/%d/%02d", this->directory.c_str(), this->now.getYear(), this->now.getMonth()) };
	std::filesystem::create_directories(path);
	this->now = datetime;
	// If the file does not exist, create new file. After the file exists, write append
	this->file = fopen(format("%s/log_of_day_%d.txt", path.c_str(), this->now.getDay()).c_str(), "a+");
}