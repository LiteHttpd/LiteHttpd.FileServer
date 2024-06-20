#pragma once

#include <LiteHttpdDev.h>

#include "FileTemp.h"
#include "ModuleConfig.h"

#include <memory>
#include <vector>

class FileServerModule final : public ModuleBase {
public:
	FileServerModule();

public:
	void processRequest(const RequestParams& rp) override;

private:
	std::unique_ptr<FileTemp> temp = nullptr;
	std::unique_ptr<ModuleConfig> config = nullptr;

	static const std::string replaceString(const std::string& input,
		const std::string& what, const std::string& replaceTo);
	static bool isSubpath(const std::string& base, const std::string& path);
	static bool isPathType(const std::string& path, const std::string& type);
	static const std::string getMIMEType(const std::string& path);
	static void createFPMParam(RequestParams::ParamList& fpmParam,
		const RequestParams& rp, const ModuleConfig::FPMConfig& fpmConf,
		const std::string& path, const std::string& root,
		const std::tuple<int, int, int>& version);
	static const RequestParams::ParamList parseFPMHeader(const std::vector<char>& data);
	static const std::vector<char> parseFPMContent(const std::vector<char>& data);
	static const std::string trim(const std::string& str);
};
