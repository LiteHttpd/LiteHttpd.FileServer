#pragma once

#include <string>
#include <ctime>

class ModuleConfig final {
public:
	ModuleConfig() = delete;
	ModuleConfig(const std::string& path);

	time_t getSurvival() const;
	const std::string getRoot() const;
	const std::string get404Page() const;
	const std::string get403Page() const;
	const std::string getDefaultPage() const;

private:
	time_t survivalTime = 60;
	std::string root = "./$hostname$";
	std::string page404 = "404.html";
	std::string page403 = "403.html";
	std::string defaultPage = "index.html";

	static const std::string readFile(const std::string& path);
	static const std::string removeBOM(const std::string& src);
};
