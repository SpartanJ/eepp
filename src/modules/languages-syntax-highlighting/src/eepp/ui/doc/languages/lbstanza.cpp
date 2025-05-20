#include <eepp/ui/doc/languages/lbstanza.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addLbstanza() {

	SyntaxDefinitionManager::instance()
		->add(

			{ "L.B. Stanza",
			  { "%.stanza$" },
			  {
				  { { "(?x) \\s* ; .*" }, "comment", "", SyntaxPatternMatchType::RegEx },
				  { { "^[ \t]*(#define|#if-defined|#if-not-defined|#else)" },
					"keyword",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "(?x) \\s* ( ` ) ( \\( )", "\\)" },
					{ "normal", "keyword", "operator" },
					{ "operator" },
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "$self" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::RegEx },

					} },
				  { { "(?x) \\s* (?<! ` ) \\(", "\\)" },
					{ "operator" },
					{ "operator" },
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "$self" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },
				  { { "include", "#tuples" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "(?x) (?<! \\s | < ) < (?! < | : | \\) | \\= )", "(?x) (?<!\\-) >" },
					{ "operator" },
					{ "operator" },
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#types" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },
				  { { "(?x) ^ \\s* (defpackage)", "\\s*$|:|(?=;)" },
					{ "literal", "keyword2" },
					{ "operator" },
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "[^ `\"'\t0-9~!%^*+-=/.:&|<>\\(\\{\\[][^ "
							"\t,.:&|<>\\[\\]\\{\\}\\(\\)\\*]*" },
						  "function",
						  "",
						  SyntaxPatternMatchType::RegEx },

					} },
				  { { "(?x) ^ \\s* (import) (?= \\s )", "\\s*$|:|(?=;)" },
					{ "literal", "keyword2" },
					{ "operator" },
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "(?x) (?<! \\w | \\- | \\$ ) (?: with | from | as ) (?! \\w | \\- | "
							"\\$ )" },
						  "keyword",
						  "",
						  SyntaxPatternMatchType::RegEx },
						{ { "[^ `\"'\t0-9~!%^*+-=/.:&|<>\\(\\{\\[][^ "
							"\t,.:&|<>\\[\\]\\{\\}\\(\\)\\*]*" },
						  "function",
						  "",
						  SyntaxPatternMatchType::RegEx },

					} },
				  { { "(?x) ^ \\s* (?: (public|protected) \\s+ )? ( def ( struct | type | "
					  "production | syntax ))",
					  "(?x) $ | : | (?= ; ) " },
					{ "literal", "keyword", "keyword2" },
					{ "operator" },
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#types" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },
				  { { "(?x) \\s* (new)", "\\s*$|:|(?=;)" },
					{ "literal", "keyword2" },
					{ "operator" },
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "[^ `\"'\t0-9~!%^*+-=/.:&|<>\\(\\{\\[][^ "
							"\t,.:&|<>\\[\\]\\{\\}\\(\\)\\*]*" },
						  "keyword2",
						  "",
						  SyntaxPatternMatchType::RegEx },

					} },
				  { { "(?x) ^ \\s* (?: (public|protected) \\s+ )? (?: (lostanza) \\s+ )? ( "
					  "defn|defmethod|defmulti ) \\*? \\s+ ([^ "
					  "`\"'\t0-9~!%^*+-=/.:&|<>\\(\\{\\[][^ \t,.:&|<>\\[\\]\\{\\}\\(\\)\\*]*) (?= "
					  "\\s | < )" },
					{ "literal", "keyword", "keyword", "keyword2", "function" },
					{},
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "include", "#return-types" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "{", "(?x) } \\s*" },
					{ "operator" },
					{ "operator" },
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "(?<!\\w)_(?!\\w)" }, "literal", "", SyntaxPatternMatchType::RegEx },
						{ { "include", "$self" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },
				  { { "(?x) \\s* (?: (var|val) \\s+ )? (\\?)? ([^ "
					  "`\"'\t0-9~!%^*+-=/.:&|<>\\(\\{\\[][^ \t,.:&|<>\\[\\]\\{\\}\\(\\)\\*]*) (:) "
					  "\\s*",
					  "(?x) (?= , | \\) | \\] | \\} | $ ) | \\s+ (?! -> | \\.\\.\\. | \\| | \\& "
					  ")" },
					{ "literal", "keyword2", "keyword", "normal", "operator" },
					{},
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#types" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },
				  { { "(?x) \\s* (var|val|defrule) \\s+", "(?x) ( = | (?=\\)) ) (?! > ) " },
					{ "literal", "keyword2", "normal" },
					{ "literal", "operator" },
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#identifiers" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },
						{ { "include", "#tuples" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },
				  { { "(?x) \\s* label(?=[ <])", ":" },
					{ "keyword2" },
					{ "operator" },
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "<", ">" },
						  { "operator" },
						  {},
						  "",
						  SyntaxPatternMatchType::RegEx,
						  {
							  { { "include", "#types" },
								{ "normal" },
								{},
								"",
								SyntaxPatternMatchType::LuaPattern },

						  } },
						{ { "[^ `\"'\t0-9~!%^*+-=/.:&|<>\\(\\{\\[][^ "
							"\t,.:&|<>\\[\\]\\{\\}\\(\\)\\*]*" },
						  "normal",
						  "",
						  SyntaxPatternMatchType::RegEx },

					} },
				  { { "(?<!\\w)-?[0-9][^ \t,.:&|<>\\[\\]{}()]*" },
					"number",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "(?x) \\s* ( \" )", "\"" },
					{ "string", "string" },
					{ "string" },
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "(?<!\\\\)%[_*,~@%]" }, "keyword", "", SyntaxPatternMatchType::RegEx },

					} },
				  { { "'(?:\\\\.|.)(.*?)'" },
					{ "string", "error" },
					{},
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "(?x) \\s* (?<! \\w | \\- | \\$ ) ( "
					  "to-(?:char|seq|string|array|tuple|list|symbol|byte|int|long|float|double) "
					  ")(?=\\(|\\{|\\s+\\$)" },
					"function",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "(?x) \\s* (?<! \\w | \\- | \\$ ) ( (?:not-)?equal\\?|compare| "
					  "less(?:-eq)?\\?|greater(?:-eq)?\\?| "
					  "max(?:imum)?|min(?:imum)?|hash|length|empty\\?|next|peek| "
					  "get\\??|set|map!?|reverse!|in-reverse| print(?:ln)?(?:-all)?| "
					  "(?:with|current)-output-stream|get-(?:char|byte)| "
					  "(?:do-)indented|put|close|with-output-file|spit| "
					  "write(?:-all)?|close|slurp|peek\\?|info| "
					  "bits(?:-as-(?:float|double))?|rand|fill(?:-template)?| "
					  "(?:ceil|floor)-log2|(?:next|prev)-pow2|sum|product| "
					  "complement|digit\\?|letter\\?|(?:upper|lower)-case\\??| "
					  "start|end|step|inclusive\\?| "
					  "matches\\?|prefix\\?|suffix\\?|append(?:-all)?| "
					  "string-join|(?:last-)?index-of-chars?|replace|trim| "
					  "add(?:-all)?|clear|(?:get|set)-chars| cons|headn?|tailn?|(?:in-)?reverse| "
					  "(?:but-)?last|transpose|seq-append|filename|line|column| "
					  "item|unwrap-(?:token|all)|key(?:\\?|s)?|value(?:\\?|!|s)?| "
					  "symbol-join|gensym|name|id|qualified\\?|qualifier| "
					  "throw|with-(?:exception-handler|finally)|try-catch-finally| "
					  "fatal|fail|with-attempt|attempt-else|generate| "
					  "resume|suspend|break|close|active\\?|open\\?| "
					  "dynamic-wind|find!?|first!?|seq\\??|filter| "
					  "index-when!?|split|take-(?:while|until)|seq-cat| "
					  "all\\?|none\\?|any\\?|count|repeat(?:-while)?|repeatedly| "
					  "take(?:-up-to)?-n|cat(?:-all)?|join|zip(?:-all)?| "
					  "contains\\?|index-of!?|reduce(?:-right)?|unique| "
					  "lookup\\??|parallel-seq|qsort!|lazy-qsort|marker!?| "
					  "add-gc-notifier|command-line-arguments|file-exists\\?| "
					  "delete-file|resolve-path|current-time-(?:ms|us)| "
					  "(?:get|set)-env|call-system|stop|time| "
					  "exp|log(?:10)?|pow|sin|cos|tan|asin|acos|atan|atan2| "
					  "sinh|cosh|tanh|ceil|floor|round|to-(?:radians|degrees)| "
					  "pop|peek|remove(?:-item|when)?|update|shorten| "
					  "lengthen|default\\?|read(?:-file|-all)?|tagged-list\\?| ) (?= \\( | \\{ | "
					  "\\s+ \\$ )" },
					"function",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "(?x) ^(\\s*)(?:(public|protected)\\s+)? "
					  "(lostanza|extern)(?![\\w\\-])(?=.*(?:=|:)$)",
					  "^(?!\\1\\s+|\\n)" },
					{ "function", "function", "keyword", "keyword2" },
					{},
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#lostanza" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },
						{ { "include", "$self" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },
				  { { "(lostanza)\\s+(deftype)\\s+(\\S+)\\s+" },
					{ "keyword2", "function", "keyword2", "keyword2" },
					{},
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "(?x)(lostanza)\\s+(?: (val|var)\\s+\\S+\\s*:| (defn)\\s+(\\S+)\\s+\\()",
					  "$" },
					{ "normal", "function", "keyword2", "keyword2", "function" },
					{},
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#lostanza" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },
						{ { "include", "$self" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },
				  { { "\\s*(extern)\\s+(\\S+)\\s*(:)", "$" },
					{ "keyword", "keyword2", "normal", "operator" },
					{},
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#lostanza" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },
						{ { "include", "$self" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },
				  { { "(?x) \\s* (?<! \\w | \\- | \\$ ) (?: "
					  "(?:fail-)?if|else|when|switch|match|let|let-var|where|for|from|while|label|"
					  "yield|try|catch|finally|throw|attempt|fn\\*?|multifn\\*?|qquote|do "
					  ")(?!\\w|\\-)" },
					"keyword",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "(?x) \\s* (?<! \\w | \\- | \\$ ) "
					  "(?:to|this|through|by|in|and|or|not|as\\??|is|seq) (?! \\w | \\- )" },
					"keyword",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "(?<!\\w|\\-)(true|false)(?!\\w|\\-)" },
					"literal",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "(?x) \\s* (`) ( [^\\s\\)\\]\\}]+ )" },
					{ "literal", "keyword", "literal" },
					{},
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "(?x) \\s* (?: ~ | ! | \\$ | % | \\^ | , | \\* | \\+ | - | != | => | ={1,2} "
					  "| \\/ | \\.{1,2} | : | & | <[=:] | << | < | >= | >{1,3} | \\| ) (?= \\s | $ "
					  ") \\s*" },
					"operator",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "(\\\\<)(.+?)(>)", "(<)(\\2)(>)" },
					{ "operator", "operator", "keyword2", "operator" },
					{ "operator", "operator", "keyword2", "operator" },
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "(?<=\\s|\\()\\[", "\\]" },
					{ "operator" },
					{ "operator" },
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "$self" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },
				  { { "(?x) (?<= \\S : \\s* ) ([^ `\"'\t0-9~!%^*+-=/.:&|<>\\(\\{\\[][^ "
					  "\t,.:&|<>\\[\\]\\{\\}\\(\\)\\*]*)" },
					"keyword2",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "(?x) \\s* ([^ `\"'\t0-9~!%^*+-=/.:&|<>\\(\\{\\[][^ "
					  "\t,.:&|<>\\[\\]\\{\\}\\(\\)\\*]*) (?= < | \\( | \\s+ \\$ )" },
					"function",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "include", "#identifiers" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::LuaPattern },

			  },
			  {

			  },
			  "",
			  {}

			} )
		.addRepositories( {

			{ "lostanza",
			  {
				  { { "(?<!\\w|\\-)(?:return|call-c|value|new)(?!\\w|\\-|\\:)" },
					"keyword",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "(ptr|ref)(\\<)", "(?<!\\-)\\>" },
					{ "keyword2", "keyword", "operator" },
					{ "operator" },
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#lostanza" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },
						{ { "include", "#types" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },
				  { { "\\baddr!?(?=\\(|\\{)" }, "function", "", SyntaxPatternMatchType::RegEx },
				  { { "(?<!\\w|\\-)(byte|int|long|float|double)(?!\\w|\\-)" },
					"keyword2",
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "types",
			  {
				  { { "(?x) \\s* (?: \\| | & | -> | , | <: | \\? (?! \\w | \\# | \\$ ) | \\.\\.\\. "
					  ") \\s*" },
					"operator",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "\\?[^ `\"'\t0-9~!%^*+-=/.:&|<>\\(\\{\\[][^ "
					  "\t,.:&|<>\\[\\]\\{\\}\\(\\)\\*]*" },
					"normal",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "[^ `\"'\t0-9~!%^*+-=/.:&|<>\\(\\{\\[][^ \t,.:&|<>\\[\\]\\{\\}\\(\\)\\*]*" },
					"keyword2",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "(?x) Void | Equalable | Comparable | Hashable | Lengthable | Seq(?:able)? | "
					  "(?:Indexed)?Collection | (?:Out|In)(?:put|dented)Stream | "
					  "(?:String|File)(?:Out|In)putStream |  RandomAccessFile | Byte | Int | Long "
					  "| Float | Double | True | False | Char | Range | String(?:Buffer)? | "
					  "(?:Char|Byte)?Array | Tuple | List | FileInfo | Token | KeyValue | "
					  "(?:String|Gen)?Symbol | Maybe | One | None | Exception | Coroutine | Vector "
					  "| Timer | Liveness(?:Tracker|Marker) | Queue | Table | HashTable | "
					  "(?:Splice|Nested|Plural|Choice)Template" },
					"keyword2",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "(?x) (?<! \\s ) < (?! : )", "(?x) (?<!\\-) >" },
					{ "operator" },
					{ "operator" },
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#types" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },
				  { { "\\[", "\\]" },
					{ "operator" },
					{ "operator" },
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#types" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },

			  } },
			{ "tuples",
			  {
				  { { "\\s*\\[", "\\]" },
					{ "operator" },
					{ "operator" },
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "$self" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },

			  } },
			{ "identifiers",
			  {
				  { { "(?x) \\s* ([^ `\"'\t0-9~!%^*+-=/.:&|<>\\(\\{\\[][^ "
					  "\t,.:&|<>\\[\\]\\{\\}\\(\\)\\*]*)" },
					"normal",
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "return-types",
			  {
				  { { "(?x) (?<=\\)) \\s+ (->) \\s+", "(?x) (?= $ | : | ; | \\) | \\] | \\} ) " },
					{ "literal", "keyword" },
					{},
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#types" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },

			  } },
		} );
}

}}}} // namespace EE::UI::Doc::Language
