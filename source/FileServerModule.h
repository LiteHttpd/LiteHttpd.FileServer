#pragma once

#include <LiteHttpdDev.h>

#include "FileTemp.h"
#include "ModuleConfig.h"

#include <memory>

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
	static const std::string getMIMEType(const std::string& path);
};
