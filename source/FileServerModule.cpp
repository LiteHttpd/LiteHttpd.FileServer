#include "FileServerModule.h"

#include <regex>
#include <filesystem>

FileServerModule::FileServerModule() {
	/** Load Config */
	this->config = std::make_unique<ModuleConfig>("LiteHttpd.FileServer.json");

	/** Init Temp */
	this->temp = std::make_unique<FileTemp>(this->config->getSurvival());
}

void FileServerModule::processRequest(const RequestParams& rp) {
	/** Get Path */
	std::string root = this->config->getRoot();
	root = FileServerModule::replaceString(root, "\\$hostname\\$", rp.addr);
	root = FileServerModule::replaceString(root, "\\$port\\$", std::to_string(rp.port));
	std::string path = root + rp.path;

	/** Default Page */
	if (path.empty()) {
		path += "/";
	}
	if (path.ends_with('/')) {
		path += this->config->getDefaultPage();
	}

	/** Check Path In Root */
	if (!FileServerModule::isSubpath(root + "/", path)) {
		/** Get 403 Path */
		std::string errPath = this->config->get403Page();
		errPath = FileServerModule::replaceString(errPath, "\\$hostname\\$", rp.addr);
		errPath = FileServerModule::replaceString(errPath, "\\$port\\$", std::to_string(rp.port));
		errPath = FileServerModule::replaceString(errPath, "\\$root\\$", root);

		/** Get 403 Page */
		auto [errPtr, errSize] = this->temp->get(errPath);
		if (!errPtr) {
			rp.reply(500, std::vector<char>{});
		}

		/** Reply 403 */
		std::vector<char> data;
		data.resize(errSize);
		std::memcpy(data.data(), errPtr.get(), errSize);
		rp.reply(403, data);
	}

	/** Get Data */
	auto [ptr, size] = this->temp->get(path);

	/** 404 */
	if (!ptr) {
		/** Get 404 Path */
		std::string errPath = this->config->get404Page();
		errPath = FileServerModule::replaceString(errPath, "\\$hostname\\$", rp.addr);
		errPath = FileServerModule::replaceString(errPath, "\\$port\\$", std::to_string(rp.port));
		errPath = FileServerModule::replaceString(errPath, "\\$root\\$", root);

		/** Get 404 Page */
		auto [errPtr, errSize] = this->temp->get(errPath);
		if (!errPtr) {
			rp.reply(500, std::vector<char>{});
		}

		/** Reply 404 */
		std::vector<char> data;
		data.resize(errSize);
		std::memcpy(data.data(), errPtr.get(), errSize);
		rp.reply(404, data);
	}

	/** Set MIME Type */
	rp.addHeader("Content-Type", FileServerModule::getMIMIType(path));

	/** Reply 200 */
	std::vector<char> data;
	data.resize(size);
	std::memcpy(data.data(), ptr.get(), size);
	rp.reply(200, data);
}

const std::string FileServerModule::replaceString(const std::string& input,
	const std::string& what, const std::string& replaceTo) {
	return std::regex_replace(input, std::regex(what), replaceTo);
}

bool FileServerModule::isSubpath(const std::string& base, const std::string& path) {
	std::filesystem::path absoluteBase = std::filesystem::weakly_canonical(base);
	std::filesystem::path absolutePath = std::filesystem::weakly_canonical(path);

	auto itBase = absoluteBase.begin();
	auto itPath = absolutePath.begin();

	for (; itBase != absoluteBase.end() && itPath != absolutePath.end(); ++itBase, ++itPath) {
		if (*itBase != *itPath) {
			return false;
		}
	}

	return itBase == absoluteBase.end();
}

const std::string FileServerModule::getMIMIType(const std::string& path) {
	static std::map<std::string, std::string> mimeTypes = {
		/** Text Files */
		{".html", "text/html"},
		{".htm", "text/html"},
		{".css", "text/css"},
		{".csv", "text/csv"},
		{".txt", "text/plain"},
		{".xml", "application/xml"},
		{".json", "application/json"},

		/** Image Files */
		{".png", "image/png"},
		{".jpg", "image/jpeg"},
		{".jpeg", "image/jpeg"},
		{".gif", "image/gif"},
		{".bmp", "image/bmp"},
		{".svg", "image/svg+xml"},
		{".ico", "image/x-icon"},
		{".tiff", "image/tiff"},
		{".tif", "image/tiff"},
		{".webp", "image/webp"},

		/** Audio Files */
		{".mp3", "audio/mpeg"},
		{".wav", "audio/wav"},
		{".ogg", "audio/ogg"},
		{".flac", "audio/flac"},
		{".aac", "audio/aac"},
		{".m4a", "audio/mp4"},

		/** Video Files */
		{".mp4", "video/mp4"},
		{".avi", "video/x-msvideo"},
		{".mov", "video/quicktime"},
		{".mkv", "video/x-matroska"},
		{".webm", "video/webm"},
		{".wmv", "video/x-ms-wmv"},

		/** Application Files */
		{".pdf", "application/pdf"},
		{".zip", "application/zip"},
		{".gz", "application/gzip"},
		{".tar", "application/x-tar"},
		{".rar", "application/vnd.rar"},
		{".7z", "application/x-7z-compressed"},
		{".exe", "application/vnd.microsoft.portable-executable"},
		{".msi", "application/x-msdownload"},
		{".doc", "application/msword"},
		{".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
		{".xls", "application/vnd.ms-excel"},
		{".xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
		{".ppt", "application/vnd.ms-powerpoint"},
		{".pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},

		/** Fonts */
		{".ttf", "font/ttf"},
		{".otf", "font/otf"},
		{".woff", "font/woff"},
		{".woff2", "font/woff2"},

		/** JavaScript */
		{".js", "application/javascript"},

		/** Others */
		{".swf", "application/x-shockwave-flash"},
		{".rtf", "application/rtf"}
	};

	std::string::size_type idx = path.rfind('.');
	if (idx != std::string::npos) {
		std::string ext = path.substr(idx);
		auto it = mimeTypes.find(ext);
		if (it != mimeTypes.end()) {
			return it->second;
		}
	}
	return "application/octet-stream";
}

LITEHTTPD_MODULE(FileServerModule)
