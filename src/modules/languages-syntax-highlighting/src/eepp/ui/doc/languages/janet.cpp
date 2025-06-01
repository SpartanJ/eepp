#include <eepp/ui/doc/languages/janet.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addJanet() {

	return SyntaxDefinitionManager::instance()
		->add(

			{ "Janet",
			  { "%.janet$" },
			  {
				  { { "(@?)```", "```" }, "string" },
				  { { "(@?)``", "``" }, "string" },
				  { { "(@?)`", "`" }, "string" },
				  { { "\"", "\"", "\\" }, "string" },
				  { { "0x[%da-fA-F]+" }, "number" },
				  { { "-?%d+[%d%.eE]*f?" }, "number" },
				  { { "#.-\n" }, "comment" },
				  { { "^\\((def|defglobal|defdyn)\\-?\\s+([a-zA-Z0-9!$%&*+-./:<?=>@^_\"]+)" },
					std::vector<std::string>{ "operator", "keyword", "literal" },
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "[';~,|]*\\((defn|defmacro)\\-?\\s+([a-zA-Z0-9!$%&*+-./:<?=>@^_\"]+)" },
					std::vector<std::string>{ "operator", "keyword", "literal" },
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "[';~,|]*\\(([';~,|]*)(break|def|do|fn|if|quasiquote|quote|set|splice|"
					  "unquote|upscope|var|while|call|maker|array|tuple|tablector|bufferctor|asm|"
					  "disasm|compile|dyn|setdyn|native|describe|string|symbol|keyword|buffer|"
					  "abstract\\?|scan-number|tuple|array|slice|table|getproto|struct|gensym|"
					  "gccollect|gcsetinterval|gcinterval|type|hash|getline|trace|untrace|int\\?|"
					  "nat\\?|signal|memcmp|sandbox|print|prin|eprint|eprin|xprint|xprin|printf|"
					  "prinf|eprintf|eprinf|xprintf|xprinf|flush|eflush|env-lookup|marshal|"
					  "unmarshal|not|debug|error|apply|yield|resume|in|put|length|add|sub|mul|div|"
					  "band|bor|bxor|lshift|rshift|rshiftu|bnot|gt|lt|gte|lte|eq|neq|propagate|get|"
					  "next|modulo|remainder|cmp|cancel|mod|sandbox|defmacro|defglobal|varglobal|"
					  "nan\\?|number\\?|fiber\\?|string\\?|symbol\\?|keyword\\?|buffer\\?|"
					  "function\\?|cfunction\\?|table\\?|struct\\?|array\\?|tuple\\?|boolean\\?|"
					  "bytes\\?|dictionary\\?|indexed\\?|truthy\\?|true\\?|false\\?|nil\\?|empty\\?"
					  "|odd\\?|inc|dec|errorf|return|sum|mean|product|comp|identity|complement|"
					  "extreme|max|min|max-of|min-of|first|last|compare|compare=|compare<|compare<="
					  "|compare>|compare>=|zero\\?|pos\\?|neg\\?|one\\?|even\\?|odd\\?|sort|sort-"
					  "by|sorted|sorted-by|reduce|reduce2|accumulate|accumulate2|map|mapcat|filter|"
					  "count|keep|range|find-index|find|index-of|take|take-until|take-while|drop|"
					  "drop-until|drop-while|juxt\\*|walk|postwalk|prewalk|partial|every\\?|any\\?|"
					  "reverse!|reverse|invert|zipcoll|get-in|update-in|put-in|update|merge-into|"
					  "merge|keys|values|pairs|frequencies|group-by|partition-by|interleave|"
					  "distinct|flatten-into|flatten|kvs|from-pairs|interpose|partition|slurp|spit|"
					  "pp|maclintf|macex1|all|some|not=|deep-not=|deep=|freeze|macex|make-env|bad-"
					  "parse|warn-compile|bad-compile|curenv|run-context|quit|eval|parse|parse-all|"
					  "eval-string|make-image|load-image|debugger|debugger-on-status|dofile|"
					  "require|merge-module|import\\*|all-bindings|all-dynamics|doc-format|doc\\*|"
					  "doc-of|\\.fiber|\\.signal|\\.stack|\\.frame|\\.locals|\\.fn|\\.slots|\\."
					  "slot|\\.source|\\.break|\\.clear|\\.next|\\.nextc|\\.step|\\.locals|repl|"
					  "flycheck|cli-main|as-macro|defmacro-|defn-|def-|var-|toggle|assert|default|"
					  "comment|if-not|when|unless|cond|case|let|try|protect|and|or|with-syms|defer|"
					  "edefer|prompt|chr|label|with|when-with|if-with|forv|for|eachk|eachp|repeat|"
					  "forever|each|loop|seq|catseq|tabseq|generate|coro|fiber-fn|if-let|when-let|"
					  "juxt|defdyn|tracev|with-dyns|with-vars|match|varfn|short-fn|comptime|compif|"
					  "compwhen|import|use|doc|delay|keep-syntax|keep-syntax!|->|->>|-?>|-?>>|as->|"
					  "as?->|--|\\+=|\\+\\+|-=|\\*=|/=|%=|\\+|-|\\*|%|/"
					  "|>=|<=|=|<|>)((?=$|[\\s,()\\[\\]{}\\\";])+)" },
					std::vector<std::string>{ "operator", "keyword", "literal" },
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "%(?(%:[%a_][%w_]*)" }, { "operator", "parameter", "parameter" } },
				  { { "[';~,|]*\\(([a-zA-Z!$%&*+-./:<?=>@^_\"]+/?[a-zA-Z!$%&*+-./:<?=>@^_\"]+)" },
					std::vector<std::string>{ "operator", "function", "literal" },
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "(nil|true|false)(?=$|[\\s,()\\[\\]{}\";])+" },
					"literal",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "[a-zA-Z0-9!$%&*+-./:<?=>@^_\"]+" },
					"type",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "([';~,|]*)@?(\\()" },
					std::vector<std::string>{ "operator", "operator", "operator" },
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "[%)%[%]%{%}]" }, "operator" },

			  },
			  {

			  },
			  "#",
			  {}

			} )
		.setFoldRangeType( FoldRangeType::Markdown )
		.setFoldBraces( {
			{ '(', ')' },
			{ '{', '}' },
			{ '[', ']' },
		} );
	;
}

}}}} // namespace EE::UI::Doc::Language
