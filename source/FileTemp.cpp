#include "FileTemp.h"

#include <cstdio>

FileTemp::FileTemp(time_t survivalTime)
	: survivalTime(survivalTime) {}

FileTemp::MemoryBlock FileTemp::get(const std::string& path) {
	/** Result */
	MemoryBlock result;

	do {
		/** Lock */
		std::lock_guard locker(this->listLock);

		/** Find In Temp */
		auto it = this->tempList.find(path);
		if (it != this->tempList.end()) {
			/** Get Result */
			result = std::get<1>(it->second);

			/** Update Time */
			std::get<0>(it->second) = std::time(nullptr);

			/** Done */
			break;
		}

		/** Load File */
		auto data = FileTemp::loadFile(path);
		if (std::get<0>(data)) {
			/** Set Result */
			result = data;

			/** Add Temp */
			this->tempList.insert(std::make_pair(
				path, std::make_tuple( std::time(nullptr), data )));
		}

	} while (false);

	/** Check Temp Time */
	this->checkTempTime();

	/** Return */
	return result;
}

void FileTemp::checkTempTime() {
	this->checkTempTimeInternal(this->survivalTime);
}

FileTemp::MemoryBlock FileTemp::loadFile(const std::string& path) {
	FILE* file = fopen(path.c_str(), "rb");
	if (file) {
		fseek(file, 0, SEEK_END);
		size_t fileSize = ftell(file);
		rewind(file);

		auto buffer = std::shared_ptr<char>(new char[fileSize], [](char* p) { delete[] p; });
		fread(buffer.get(), 1, fileSize, file);
		fclose(file);

		return { buffer, fileSize };
	}

	return { nullptr, 0 };
}

void FileTemp::checkTempTimeInternal(time_t survivalTime) {
	/** Lock */
	std::lock_guard locker(this->listLock);

	/** Get Temp Valid Time */
	time_t validTime = 0;
	time_t currentTime = std::time(nullptr);
	if (currentTime > survivalTime) {
		validTime = currentTime - survivalTime;
	}

	/** Check Each Temp */
	for (auto it = this->tempList.begin(); it != tempList.end();) {
		if (std::get<0>(it->second) < survivalTime) {
			it = this->tempList.erase(it);
			continue;
		}

		it++;
	}
}
