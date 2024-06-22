#include "FileServerModule.h"

#include <regex>
#include <filesystem>
#include <cstring>

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

	/** Log */
	rp.log(RequestParams::LogLevel::INFO, "Request file root: " + root);
	rp.log(RequestParams::LogLevel::INFO, "Request file path: " + path);

	/** Check Path In Root */
#if WIN32
	if (!FileServerModule::isSubpath(root + "/", path)) {
#else 
	if (!FileServerModule::isSubpath(root, path)) {
#endif
		rp.log(RequestParams::LogLevel::WARNING, "Request file out of root directory!");

		/** Get 403 Path */
		std::string errPath = this->config->get403Page();
		errPath = FileServerModule::replaceString(errPath, "\\$hostname\\$", rp.addr);
		errPath = FileServerModule::replaceString(errPath, "\\$port\\$", std::to_string(rp.port));
		errPath = FileServerModule::replaceString(errPath, "\\$root\\$", root);
		rp.log(RequestParams::LogLevel::INFO, "403 page path: " + errPath);

		/** Get 403 Page */
		auto [errPtr, errSize] = this->temp->get(errPath);
		if (!errPtr) {
			rp.log(RequestParams::LogLevel::ERROR_, "Can't load 403 page, send 500!");
			rp.reply(500, std::vector<char>{});
			return;
		}

		/** Reply 403 */
		std::vector<char> data;
		data.resize(errSize);
		std::memcpy(data.data(), errPtr.get(), errSize);
		rp.reply(403, data);
		return;
	}

	/** Get Data */
	auto [ptr, size] = this->temp->get(path);

	/** 404 */
	if (!ptr) {
		rp.log(RequestParams::LogLevel::WARNING, "Can't load file!");

		/** Get 404 Path */
		std::string errPath = this->config->get404Page();
		errPath = FileServerModule::replaceString(errPath, "\\$hostname\\$", rp.addr);
		errPath = FileServerModule::replaceString(errPath, "\\$port\\$", std::to_string(rp.port));
		errPath = FileServerModule::replaceString(errPath, "\\$root\\$", root);
		rp.log(RequestParams::LogLevel::INFO, "404 page path: " + errPath);

		/** Get 404 Page */
		auto [errPtr, errSize] = this->temp->get(errPath);
		if (!errPtr) {
			rp.log(RequestParams::LogLevel::ERROR_, "Can't load 404 page, send 500!");
			rp.reply(500, std::vector<char>{});
			return;
		}

		/** Reply 404 */
		std::vector<char> data;
		data.resize(errSize);
		std::memcpy(data.data(), errPtr.get(), errSize);
		rp.reply(404, data);
		return;
	}

	/** PHP */
	if (this->config->getFPMOn()) {
		/** FPM Config */
		auto& fpmConf = this->config->getFPMConf();

		/** Check Path Type */
		if (FileServerModule::isPathType(path, fpmConf.surfix)) {
			/** Log */
			rp.log(RequestParams::LogLevel::INFO, "Wait for FPM...");

			/** Create Param */
			RequestParams::ParamList fpmParam;
			FileServerModule::createFPMParam(fpmParam, rp, fpmConf,
				path, root, this->getDevKitVersion());

			/** Send Data */
			RequestParams::FPMResult fpmResult = 
				rp.callFPM(fpmConf.address, fpmConf.port, rp.data, fpmParam);

			/** Result Invalid */
			if (!std::get<1>(fpmResult)) {
				rp.reply(500, std::vector<char>{});
				rp.log(RequestParams::LogLevel::ERROR_, "FPM returned false, send 500!");
				return;
			}

			/** Headers */
			auto headers = FileServerModule::parseFPMHeader(std::get<0>(fpmResult));
			for (auto& i : headers) {
				rp.addHeader(i.first, i.second);
			}

			/** Reply 200 */
			auto data = FileServerModule::parseFPMContent(std::get<0>(fpmResult));
			rp.reply(200, data);
			rp.log(RequestParams::LogLevel::INFO, "Send 200 with data size: " + std::to_string(data.size()));
			return;
		}
	}

	/** Set MIME Type */
	{
		std::string mimeType = FileServerModule::getMIMEType(path);
		rp.addHeader("Content-Type", mimeType);
		rp.log(RequestParams::LogLevel::INFO, "Set MIME type: " + mimeType);
	}

	/** Reply 200 */
	std::vector<char> data;
	data.resize(size);
	std::memcpy(data.data(), ptr.get(), size);
	rp.reply(200, data);
	rp.log(RequestParams::LogLevel::INFO, "Send 200 with data size: " + std::to_string(size));
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

bool FileServerModule::isPathType(const std::string& path, const std::string& type) {
	std::string::size_type idx = path.rfind('.');
	if (idx != std::string::npos) {
		std::string ext = path.substr(idx);
		return type == ext;
	}
	return false;
}

const std::string FileServerModule::getMIMEType(const std::string& path) {
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

void FileServerModule::createFPMParam(RequestParams::ParamList& fpmParam,
	const RequestParams& rp, const ModuleConfig::FPMConfig& fpmConf,
	const std::string& path, const std::string& root,
	const std::tuple<int, int, int>& version) {
	fpmParam["SCRIPT_FILENAME"] = path;
	fpmParam["SCRIPT_NAME"] = rp.path;
	fpmParam["DOCUMENT_ROOT"] = root + "/";
	{
		fpmParam["REQUEST_URI"] = rp.path;
		if (!rp.query.empty()) {
			fpmParam["REQUEST_URI"] += ("?" + rp.query);
		}
	}
	fpmParam["PHP_SELF"] = rp.path;
	fpmParam["CONTENT_TYPE"] = "application/x-www-form-urlencoded";
	fpmParam["PATH"] = "";
	fpmParam["PHP_FCGI_CHILDREN"] = std::to_string(fpmConf.children);
	fpmParam["PHP_FCGI_MAX_REQUESTS"] = std::to_string(fpmConf.maxRequests);
	fpmParam["FCGI_ROLE"] = "RESPONDER";
	{
		auto& [major, minor, patch] = version;
		fpmParam["SERVER_SOFTWARE"] = "LiteHttpd.FileServer/sdk" + std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
	}
	fpmParam["SERVER_NAME"] = "LiteHttpd.FileServer";
	fpmParam["GATEWAY_INTERFACE"] = "CGI/1.1";
	fpmParam["SERVER_PORT"] = std::to_string(fpmConf.port);
	fpmParam["SERVER_ADDR"] = fpmConf.address;
	fpmParam["REMOTE_PORT"] = std::to_string(rp.peerPort);
	fpmParam["REMOTE_ADDR"] = rp.peerAddr;
	fpmParam["PATH_INFO"] = rp.path;
	fpmParam["PATH_TRANSLATED"] = path;
	fpmParam["QUERY_STRING"] = rp.query;
	switch (rp.method) {
	case RequestParams::MethodType::GET:
		fpmParam["REQUEST_METHOD"] = "GET";
		break;
	case RequestParams::MethodType::POST:
		fpmParam["REQUEST_METHOD"] = "POST";
		break;
	case RequestParams::MethodType::HEAD:
		fpmParam["REQUEST_METHOD"] = "HEAD";
		break;
	case RequestParams::MethodType::PUT:
		fpmParam["REQUEST_METHOD"] = "PUT";
		break;
	case RequestParams::MethodType::DELETE_:
		fpmParam["REQUEST_METHOD"] = "DELETE";
		break;
	case RequestParams::MethodType::OPTIONS:
		fpmParam["REQUEST_METHOD"] = "OPTIONS";
		break;
	case RequestParams::MethodType::TRACE:
		fpmParam["REQUEST_METHOD"] = "TRACE";
		break;
	case RequestParams::MethodType::CONNECT:
		fpmParam["REQUEST_METHOD"] = "CONNECT";
		break;
	case RequestParams::MethodType::PATCH:
		fpmParam["REQUEST_METHOD"] = "PATCH";
		break;
	case RequestParams::MethodType::PROPFIND:
		fpmParam["REQUEST_METHOD"] = "PROPFIND";
		break;
	case RequestParams::MethodType::PROPPATCH:
		fpmParam["REQUEST_METHOD"] = "PROPPATCH";
		break;
	case RequestParams::MethodType::MKCOL:
		fpmParam["REQUEST_METHOD"] = "MKCOL";
		break;
	case RequestParams::MethodType::LOCK:
		fpmParam["REQUEST_METHOD"] = "LOCK";
		break;
	case RequestParams::MethodType::UNLOCK:
		fpmParam["REQUEST_METHOD"] = "UNLOCK";
		break;
	case RequestParams::MethodType::COPY:
		fpmParam["REQUEST_METHOD"] = "COPY";
		break;
	case RequestParams::MethodType::MOVE:
		fpmParam["REQUEST_METHOD"] = "MOVE";
		break;
	}
	fpmParam["HTTPS"] = (rp.protocol == RequestParams::ProtocolType::HTTPS) ? "on" : "off";
	fpmParam["REDIRECT_STATUS"] = "200";
	fpmParam["SERVER_PROTOCOL"] = "HTTP/1.1";
	fpmParam["HTTP_HOST"] = rp.addr + ":" + std::to_string(rp.port);
	{
		auto it = rp.headers.find("Connection");
		if (it != rp.headers.end()) {
			fpmParam["HTTP_CONNECTION"] = it->second;
		}
	}
	{
		auto it = rp.headers.find("Accept");
		if (it != rp.headers.end()) {
			fpmParam["HTTP_ACCEPT"] = it->second;
		}
	}
	{
		auto it = rp.headers.find("Accept-Encoding");
		if (it != rp.headers.end()) {
			fpmParam["HTTP_ACCEPT_ENCODING"] = it->second;
		}
	}
	{
		auto it = rp.headers.find("Accept-Language");
		if (it != rp.headers.end()) {
			fpmParam["HTTP_ACCEPT_LANGUAGE"] = it->second;
		}
	}
	{
		auto it = rp.headers.find("User-Agent");
		if (it != rp.headers.end()) {
			fpmParam["HTTP_UAER_AGENT"] = it->second;
		}
	}
	{
		auto it = rp.headers.find("Referer");
		if (it != rp.headers.end()) {
			fpmParam["HTTP_REFERER"] = it->second;
		}
	}
	{
		auto it = rp.headers.find("Cookie");
		if (it != rp.headers.end()) {
			fpmParam["HTTP_COOKIE"] = it->second;
		}
	}
}

const RequestParams::ParamList FileServerModule::parseFPMHeader(const std::vector<char>& data) {
	RequestParams::ParamList headers;

	auto it = data.begin();
	while (it != data.end()) {
		auto line_end = std::search(it, data.end(), "\r\n", "\r\n" + 2);
		if (line_end == it) break;

		auto delimiter_pos = std::find(it, line_end, ':');
		if (delimiter_pos != line_end) {
			std::string key(it, delimiter_pos);
			std::string value(delimiter_pos + 2, line_end);
			headers[FileServerModule::trim(key)] = FileServerModule::trim(value);
		}

		it = line_end + 2;
	}

	return headers;
}

const std::vector<char> FileServerModule::parseFPMContent(const std::vector<char>& data) {
	auto it = std::search(data.begin(), data.end(), "\r\n\r\n", "\r\n\r\n" + 4);
	if (it != data.end()) {
		return std::vector<char>{it + 4, data.end()};
	}
	return {};
}

const std::string FileServerModule::trim(const std::string& str) {
	auto front = std::find_if_not(str.begin(), str.end(), [](int c) { return std::isspace(c); });
	auto back = std::find_if_not(str.rbegin(), str.rend(), [](int c) { return std::isspace(c); }).base();
	return (front < back) ? std::string{ front, back } : std::string{};
}

LITEHTTPD_MODULE(FileServerModule)
