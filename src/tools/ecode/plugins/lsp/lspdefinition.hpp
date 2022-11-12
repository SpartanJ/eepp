#ifndef ECODE_LSPDEFINITION_HPP
#define ECODE_LSPDEFINITION_HPP
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace ecode {

struct LSPDefinition {
	std::string language;
	std::string name;
	std::vector<std::string> filePatterns;
	std::string command;
	std::string commandParameters;
	std::vector<std::string> rootIndicationFileNames;
	std::string url;
	nlohmann::json initializationOptions;

	bool disabled{ false };
};

} // namespace ecode

#endif // ECODE_LSPDEFINITION_HPP
