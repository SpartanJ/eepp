#ifndef ECODE_LSPDEFINITION_HPP
#define ECODE_LSPDEFINITION_HPP
#include <string>
#include <vector>

namespace ecode {

struct LSPDefinition {
	std::string language;
	std::string name;
	std::vector<std::string> filePatterns;
	std::string command;
	std::vector<std::string> rootIndicationFileNames;
	std::string url;
};

} // namespace ecode

#endif // ECODE_LSPDEFINITION_HPP
