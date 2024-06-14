#pragma once

#include <LiteHttpdDev.h>

class FileServerModule final : public ModuleBase {
public:
	FileServerModule() = default;

public:
	void processRequest(const RequestParams& rp) override;
};
