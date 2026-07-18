#ifndef ECODE_SNIPPETPARSER_HPP
#define ECODE_SNIPPETPARSER_HPP

#include <eepp/core/containers.hpp>
#include <string>
#include <string_view>
#include <vector>

using namespace EE;

namespace ecode {

class SnippetParser {
  public:
	struct TabStop {
		Uint32 index{ 0 };
		size_t start{ 0 };
		size_t end{ 0 };
		std::vector<std::string> choices;
		bool synthetic{ false };
	};

	struct Result {
		std::string text;
		std::vector<TabStop> tabStops;
		size_t codepointLength{ 0 };
		bool hasExplicitTabStops{ false };

		bool hasTabStops() const { return hasExplicitTabStops; }
	};

	using VariableMap = UnorderedMap<std::string, std::string>;

	static Result parse( std::string_view snippet, const VariableMap& variables = {} );
};

} // namespace ecode

#endif // ECODE_SNIPPETPARSER_HPP
