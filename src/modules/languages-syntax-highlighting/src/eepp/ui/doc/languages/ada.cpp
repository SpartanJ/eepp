#include <eepp/ui/doc/languages/ada.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addAda() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "Ada",
		  { "%.adb$", "%.ads$", "%.ada$" },
		  {
			  { { "\"", "\"", "\\" }, "string" },
			  { { "%-%-.-\n" }, "comment" },
			  { { "[%:%;%=%<%>%&%+%-%*%/%.%(%)]" }, "operator" },
			  { { "-?%d+[%d%.eE_]*" }, "number" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "-?%d+[%d%.eE_]*" }, "number" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "abort", "keyword" },
			  { "abs", "keyword" },
			  { "abstract", "keyword" },
			  { "accept", "keyword" },
			  { "access", "keyword" },
			  { "aliased", "keyword" },
			  { "all", "keyword" },
			  { "and", "keyword" },
			  { "array", "keyword" },
			  { "at", "keyword" },
			  { "begin", "keyword" },
			  { "body", "keyword" },
			  { "case", "keyword" },
			  { "constant", "keyword" },
			  { "declare", "keyword" },
			  { "delay", "keyword" },
			  { "delta", "keyword" },
			  { "digits", "keyword" },
			  { "do", "keyword" },
			  { "else", "keyword" },
			  { "elsif", "keyword" },
			  { "end", "keyword" },
			  { "entry", "keyword" },
			  { "exception", "keyword" },
			  { "exit", "keyword" },
			  { "for", "keyword" },
			  { "function", "keyword" },
			  { "generic", "keyword" },
			  { "goto", "keyword" },
			  { "if", "keyword" },
			  { "in", "keyword" },
			  { "interface", "keyword" },
			  { "is", "keyword" },
			  { "limited", "keyword" },
			  { "loop", "keyword" },
			  { "mod", "keyword" },
			  { "new", "keyword" },
			  { "not", "keyword" },
			  { "null", "keyword" },
			  { "of", "keyword" },
			  { "or", "keyword" },
			  { "others", "keyword" },
			  { "out", "keyword" },
			  { "overriding", "keyword" },
			  { "package", "keyword" },
			  { "parallel", "keyword" },
			  { "pragma", "keyword" },
			  { "private", "keyword" },
			  { "procedure", "keyword" },
			  { "protected", "keyword" },
			  { "raise", "keyword" },
			  { "range", "keyword" },
			  { "record", "keyword" },
			  { "rem", "keyword" },
			  { "renames", "keyword" },
			  { "requeue", "keyword" },
			  { "return", "keyword" },
			  { "reverse", "keyword" },
			  { "select", "keyword" },
			  { "separate", "keyword" },
			  { "some", "keyword" },
			  { "subtype", "keyword" },
			  { "synchronized", "keyword" },
			  { "tagged", "keyword" },
			  { "task", "keyword" },
			  { "terminate", "keyword" },
			  { "then", "keyword" },
			  { "type", "keyword" },
			  { "until", "keyword" },
			  { "use", "keyword" },
			  { "when", "keyword" },
			  { "while", "keyword" },
			  { "with", "keyword" },
			  { "xor", "keyword" },
			  { "true", "keyword" },
			  { "false", "keyword" },
			  { "boolean", "keyword2" },
			  { "character", "keyword2" },
			  { "count", "keyword2" },
			  { "duration", "keyword2" },
			  { "float", "keyword2" },
			  { "integer", "keyword2" },
			  { "long_float", "keyword2" },
			  { "long_integer", "keyword2" },
			  { "priority", "keyword2" },
			  { "short_float", "keyword2" },
			  { "short_integer", "keyword2" },
			  { "string", "keyword2" },
		  },
		  "--",
		  {}

		} );

	sd.setCaseInsensitive( true );
}

}}}} // namespace EE::UI::Doc::Language
