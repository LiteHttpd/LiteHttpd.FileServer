#pragma once

#include <ctime>
#include <string>
#include <map>
#include <memory>
#include <tuple>
#include <mutex>

class FileTemp final {
public:
	FileTemp(time_t survivalTime = 60);

	using MemoryBlock = std::tuple<std::shared_ptr<char>, size_t>;
	MemoryBlock get(const std::string& path);
	void checkTempTime();

private:
	const time_t survivalTime;
	
	using DataHolder = std::tuple<time_t, MemoryBlock>;
	std::map<std::string, DataHolder> tempList;
	std::mutex listLock;

	static MemoryBlock loadFile(const std::string& path);
	void checkTempTimeInternal(time_t survivalTime);
};
