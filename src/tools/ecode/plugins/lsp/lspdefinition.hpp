#ifndef ECODE_LSPDEFINITION_HPP
#define ECODE_LSPDEFINITION_HPP
#include <eepp/core/string.hpp>
#include <eepp/system/sys.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using namespace EE;
using namespace EE::System;

namespace ecode {

struct LSPDefinition {
	std::string language;
	std::string name;
	std::vector<std::string> filePatterns;
	std::string command;
	std::string commandParameters;
	std::vector<std::string> rootIndicationFileNames;
	std::string url;
	std::string host;
	int port{ 0 };
	nlohmann::json initializationOptions;

	bool disabled{ false };

	bool commandAvailable() const {
		auto cmdp( String::split( command, ' ' ) );
		return !cmdp.empty() && !Sys::which( cmdp[0] ).empty();
	}
};

} // namespace ecode

#endif // ECODE_LSPDEFINITION_HPP
