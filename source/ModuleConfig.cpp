#include "ModuleConfig.h"

#include <CJsonObject.hpp>
#include <fstream>
#include <sstream>

ModuleConfig::ModuleConfig(const std::string& path) {
	/** Read File */
	auto data = ModuleConfig::readFile(path);

	/** Parse Json */
	neb::CJsonObject object(ModuleConfig::removeBOM(data));
	if (!object.IsEmpty()) {
		/** Get Survival Time */
		if (object.KeyExist("survival")) {
			object.Get("survival", this->survivalTime);
		}
		
		/** Get Root */
		if (object.KeyExist("root")) {
			object.Get("root", this->root);
		}

		/** Get Error Page */
		if (object.KeyExist("page404")) {
			object.Get("page404", this->page404);
		}
		if (object.KeyExist("page403")) {
			object.Get("page403", this->page403);
		}

		/** Get Default Page */
		if (object.KeyExist("defaultPage")) {
			object.Get("defaultPage", this->defaultPage);
		}

		/** Get FPM Config */
		if (object.KeyExist("fpm")) {
			auto& fpmObj = object["fpm"];
			if (!fpmObj.IsEmpty()) {
				this->fpm = true;

				/** Get Surfix */
				if (fpmObj.KeyExist("surfix")) {
					fpmObj.Get("surfix", this->fpmConf.surfix);
				}

				/** Get Address */
				if (fpmObj.KeyExist("address")) {
					fpmObj.Get("address", this->fpmConf.address);
				}

				/** Get Port */
				if (fpmObj.KeyExist("port")) {
					int port = 0;
					fpmObj.Get("port", port);
					this->fpmConf.port = port;
				}

				/** Get Children */
				if (fpmObj.KeyExist("fcgi_children")) {
					fpmObj.Get("fcgi_children", this->fpmConf.children);
				}

				/** Get Children */
				if (fpmObj.KeyExist("fcgi_max_requests")) {
					fpmObj.Get("fcgi_max_requests", this->fpmConf.maxRequests);
				}
			}
		}
	}
}

time_t ModuleConfig::getSurvival() const {
	return this->survivalTime;
}

const std::string ModuleConfig::getRoot() const {
	return this->root;
}

const std::string ModuleConfig::get404Page() const {
	return this->page404;
}

const std::string ModuleConfig::get403Page() const {
	return this->page403;
}

const std::string ModuleConfig::getDefaultPage() const {
	return this->defaultPage;
}

bool ModuleConfig::getFPMOn() const {
	return this->fpm;
}

const ModuleConfig::FPMConfig& ModuleConfig::getFPMConf() const {
	return this->fpmConf;
}

const std::string ModuleConfig::readFile(const std::string& path) {
	std::stringstream result;

	std::ifstream inputStream;
	inputStream.open(path);
	result << inputStream.rdbuf();

	return result.str();
}

const std::string ModuleConfig::removeBOM(const std::string& src) {
	const std::string BOM = "\xEF\xBB\xBF";
	if (src.compare(0, BOM.size(), BOM) == 0) {
		return src.substr(BOM.size());
	}
	return src;
}
