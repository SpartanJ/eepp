#include <algorithm>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/luapatternmatcher.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/uiwidgetcreator.hpp>

using namespace EE::System;

namespace EE { namespace UI { namespace Doc {

SINGLETON_DECLARE_IMPLEMENTATION( SyntaxDefinitionManager )

// Syntax definitions can be directly converted from the lite (https://github.com/rxi/lite) and
// lite-plugins (https://github.com/rxi/lite-plugins) supported languages.

SyntaxDefinitionManager::SyntaxDefinitionManager() {
	// Register some languages support.

	// XML - HTML
	add( {{"%.xml$", "%.html?$", "%.svg?$"},
		  {
			  {{"<!%-%-", "%-%->"}, "comment"},
			  {{"%f[^>][^<]", "%f[<]"}, "normal"},
			  {{"\"", "\"", "\\"}, "string"},
			  {{"'", "'", "\\"}, "string"},
			  {{"0x[%da-fA-F]+"}, "number"},
			  {{"-?%d+[%d%.]*f?"}, "number"},
			  {{"-?%.?%d+f?"}, "number"},
			  {{"%f[^<]![%a_][%w_]*"}, "keyword2"},
			  {{"%f[^<][%a_][%w_]*"}, "function"},
			  {{"%f[^<]/[%a_][%w_]*"}, "function"},
			  {{"[%a_][%w_]*"}, "keyword"},
			  {{"[/<>=]"}, "operator"},
		  }} );

	// CSS
	add( {{"%.css$"},
		  {
			  {{"\\."}, "normal"},
			  {{"//.-\n"}, "comment"},
			  {{"/%*", "%*/"}, "comment"},
			  {{"\"", "\"", "\\"}, "string"},
			  {{"'", "'", "\\"}, "string"},
			  {{"[%a][%w-]*%s*%f[:]"}, "keyword"},
			  {{"#%x+"}, "string"},
			  {{"-?%d+[%d%.]*p[xt]"}, "number"},
			  {{"-?%d+[%d%.]*deg"}, "number"},
			  {{"-?%d+[%d%.]*"}, "number"},
			  {{"[%a_][%w_]*"}, "symbol"},
			  {{"#[%a][%w_-]*"}, "keyword2"},
			  {{"@[%a][%w_-]*"}, "keyword2"},
			  {{"%.[%a][%w_-]*"}, "keyword2"},
			  {{"[{}:]"}, "operator"},
		  }} )
		.addSymbols( UIWidgetCreator::getWidgetNames(), "keyword2" );

	// Markdown
	add( {{"%.md$", "%.markdown$"},
		  {
			  {{"\\."}, "normal"},
			  {{"<!%-%-", "%-%->"}, "comment"},
			  {{"```", "```"}, "string"},
			  {{"``", "``", "\\"}, "string"},
			  {{"`", "`", "\\"}, "string"},
			  {{"~~", "~~", "\\"}, "keyword2"},
			  {{"%-%-%-+"}, "comment"},
			  {{"%*%s+"}, "operator"},
			  {{"%*", "[%*\n]", "\\"}, "operator"},
			  {{"%_", "[%_\n]", "\\"}, "keyword2"},
			  {{"#.-\n"}, "keyword"},
			  {{"!?%[.-%]%(.-%)"}, "function"},
			  {{"https?://%S+"}, "function"},
		  }} );

	// C
	add( {{"%.c$", "%.h$"},
		  {
			  {{"//.-\n"}, "comment"},
			  {{"/%*", "%*/"}, "comment"},
			  {{"#", "[^\\]\n"}, "comment"},
			  {{"\"", "\"", "\\"}, "string"},
			  {{"'", "'", "\\"}, "string"},
			  {{"-?0x%x+"}, "number"},
			  {{"-?%d+[%d%.eE]*f?"}, "number"},
			  {{"-?%.?%d+f?"}, "number"},
			  {{"[%+%-=/%*%^%%<>!~|&]"}, "operator"},
			  {{"[%a_][%w_]*%f[(]"}, "function"},
			  {{"[%a_][%w_]*"}, "symbol"},
		  },
		  {
			  {"if", "keyword"},	  {"then", "keyword"},	  {"else", "keyword"},
			  {"elseif", "keyword"},  {"do", "keyword"},	  {"while", "keyword"},
			  {"for", "keyword"},	  {"break", "keyword"},	  {"continue", "keyword"},
			  {"return", "keyword"},  {"goto", "keyword"},	  {"struct", "keyword"},
			  {"union", "keyword"},	  {"typedef", "keyword"}, {"enum", "keyword"},
			  {"extern", "keyword"},  {"static", "keyword"},  {"volatile", "keyword"},
			  {"const", "keyword"},	  {"inline", "keyword"},  {"switch", "keyword"},
			  {"case", "keyword"},	  {"default", "keyword"}, {"auto", "keyword"},
			  {"const", "keyword"},	  {"void", "keyword"},	  {"int", "keyword2"},
			  {"short", "keyword2"},  {"long", "keyword2"},	  {"float", "keyword2"},
			  {"double", "keyword2"}, {"char", "keyword2"},	  {"unsigned", "keyword2"},
			  {"bool", "keyword2"},	  {"true", "literal"},	  {"false", "literal"},
			  {"NULL", "literal"},
		  },
		  "//"} );

	// Lua
	add( {{"%.lua$"},
		  {
			  {{"\"", "\"", "\\"}, "string"},
			  {{"'", "'", "\\"}, "string"},
			  {{"%[%{", "%]%]"}, "string"},
			  {{"%-%-%[%{", "%]%]"}, "comment"},
			  {{"%-%-.-\n"}, "comment"},
			  {{"-?0x%x+"}, "number"},
			  {{"-?%d+[%d%.eE]*"}, "number"},
			  {{"-?%.?%d+"}, "number"},
			  {{"%.%.%.?"}, "operator"},
			  {{"[<>~=]="}, "operator"},
			  {{"[%+%-=/%*%^%%#<>]"}, "operator"},
			  {{"[%a_][%w_]*%s*%f[(\"{]"}, "function"},
			  {{"[%a_][%w_]*"}, "symbol"},
			  {{"::[%a_][%w_]*::"}, "function"},
		  },
		  {
			  {"if", "keyword"},	   {"then", "keyword"},	  {"else", "keyword"},
			  {"elseif", "keyword"},   {"end", "keyword"},	  {"do", "keyword"},
			  {"function", "keyword"}, {"repeat", "keyword"}, {"until", "keyword"},
			  {"while", "keyword"},	   {"for", "keyword"},	  {"break", "keyword"},
			  {"return", "keyword"},   {"local", "keyword"},  {"in", "keyword"},
			  {"not", "keyword"},	   {"and", "keyword"},	  {"or", "keyword"},
			  {"goto", "keyword"},	   {"self", "keyword2"},  {"true", "literal"},
			  {"false", "literal"},	   {"nil", "literal"},
		  },
		  "--"} );

	// JavaScript
	add( {{"%.js$", "%.json$", "%.cson$"},
		  {
			  {{"//.-\n"}, "comment"},
			  {{"/%*", "%*/"}, "comment"},
			  {{"\"", "\"", "\\"}, "string"},
			  {{"'", "'", "\\"}, "string"},
			  {{"0x[%da-fA-F]+"}, "number"},
			  {{"-?%d+[%d%.eE]*"}, "number"},
			  {{"-?%.?%d+"}, "number"},
			  {{"[%+%-=/%*%^%%<>!~|&]"}, "operator"},
			  {{"[%a_][%w_]*%f[(]"}, "function"},
			  {{"[%a_][%w_]*"}, "symbol"},
		  },
		  {
			  {"arguments", "keyword2"}, {"async", "keyword"},		{"await", "keyword"},
			  {"break", "keyword"},		 {"case", "keyword"},		{"catch", "keyword"},
			  {"class", "keyword"},		 {"const", "keyword"},		{"continue", "keyword"},
			  {"debugger", "keyword"},	 {"default", "keyword"},	{"delete", "keyword"},
			  {"do", "keyword"},		 {"else", "keyword"},		{"export", "keyword"},
			  {"extends", "keyword"},	 {"false", "literal"},		{"finally", "keyword"},
			  {"for", "keyword"},		 {"function", "keyword"},	{"get", "keyword"},
			  {"if", "keyword"},		 {"import", "keyword"},		{"in", "keyword"},
			  {"Infinity", "keyword2"},	 {"instanceof", "keyword"}, {"let", "keyword"},
			  {"NaN", "keyword2"},		 {"new", "keyword"},		{"null", "literal"},
			  {"return", "keyword"},	 {"set", "keyword"},		{"super", "keyword"},
			  {"switch", "keyword"},	 {"this", "keyword2"},		{"throw", "keyword"},
			  {"true", "literal"},		 {"try", "keyword"},		{"typeof", "keyword"},
			  {"undefined", "literal"},	 {"var", "keyword"},		{"void", "keyword"},
			  {"while", "keyword"},		 {"with", "keyword"},		{"yield", "keyword"},
		  },
		  "//"} );

	// Python
	add( {{"%.py$"},
		  {
			  {{"#", "\n"}, "comment"},
			  {{"[ruU]?\"", "\"", "\\"}, "string"},
			  {{"[ruU]?'", "'", "\\"}, "string"},
			  {{"\"\"\"", "\"\"\""}, "string"},
			  {{"0x[%da-fA-F]+"}, "number"},
			  {{"-?%d+[%d%.eE]*"}, "number"},
			  {{"-?%.?%d+"}, "number"},
			  {{"[%+%-=/%*%^%%<>!~|&]"}, "operator"},
			  {{"[%a_][%w_]*%f[(]"}, "function"},
			  {{"[%a_][%w_]*"}, "symbol"},
		  },
		  {
			  {"class", "keyword"},	 {"finally", "keyword"},  {"is", "keyword"},
			  {"return", "keyword"}, {"continue", "keyword"}, {"for", "keyword"},
			  {"lambda", "keyword"}, {"try", "keyword"},	  {"def", "keyword"},
			  {"from", "keyword"},	 {"nonlocal", "keyword"}, {"while", "keyword"},
			  {"and", "keyword"},	 {"global", "keyword"},	  {"not", "keyword"},
			  {"with", "keyword"},	 {"as", "keyword"},		  {"elif", "keyword"},
			  {"if", "keyword"},	 {"or", "keyword"},		  {"else", "keyword"},
			  {"import", "keyword"}, {"pass", "keyword"},	  {"break", "keyword"},
			  {"except", "keyword"}, {"in", "keyword"},		  {"del", "keyword"},
			  {"raise", "keyword"},	 {"yield", "keyword"},	  {"assert", "keyword"},
			  {"self", "keyword2"},	 {"None", "literal"},	  {"True", "literal"},
			  {"False", "literal"},
		  },
		  "#"} );

	// sh - bash
	add( {{"%.sh$", "%.bash$"},
		  {
			  {{"#.*\n"}, "comment"},
			  {{"[[\\.]]"}, "normal"},
			  {{"\"", "\"", "\\"}, "string"},
			  {{"'", "'", "\\"}, "string"},
			  {{"`", "`", "\\"}, "string"},
			  {{"%f[%w_][%d%.]+%f[^%w_]"}, "number"},
			  {{"[!<>|&%[%]=*]"}, "operator"},
			  {{"%f[%S]%-[%w%-_]+"}, "function"},
			  {{"${.*}"}, "keyword2"},
			  {{"$[%a_@*][%w_]*"}, "keyword2"},
			  {{"[%a_][%w_]*"}, "symbol"},
		  },
		  {
			  {"case", "keyword"},	{"do", "keyword"},	 {"done", "keyword"},
			  {"elif", "keyword"},	{"else", "keyword"}, {"esac", "keyword"},
			  {"fi", "keyword"},	{"for", "keyword"},	 {"function", "keyword"},
			  {"if", "keyword"},	{"in", "keyword"},	 {"select", "keyword"},
			  {"then", "keyword"},	{"time", "keyword"}, {"until", "keyword"},
			  {"while", "keyword"}, {"echo", "keyword"}, {"true", "literal"},
			  {"false", "literal"},
		  },
		  "#"} );

	// C++
	add( {{"%.h$", "%.inl$", "%.cpp$", "%.cc$", "%.C$", "%.cxx$", "%.c++$", "%.hh$", "%.H$",
		   "%.hxx$", "%.hpp$", "%.h++$"},
		  {
			  {{"//.-\n"}, "comment"},
			  {{"/%*", "%*/"}, "comment"},
			  {{"#", "[^\\]\n"}, "keyword2"},
			  {{"\"", "\"", "\\"}, "string"},
			  {{"'", "'", "\\"}, "string"},
			  {{"-?0x%x+"}, "number"},
			  {{"-?%d+[%d%.eE]*f?"}, "number"},
			  {{"-?%.?%d+f?"}, "number"},
			  {{"[%+%-=/%*%^%%<>!~|&]"}, "operator"},
			  {{"[%a_][%w_]*%f[(]"}, "function"},
			  {{"[%a_][%w_]*"}, "symbol"},
		  },
		  {
			  {"alignof", "keyword"},
			  {"alignas", "keyword"},
			  {"and", "keyword"},
			  {"and_eq", "keyword"},
			  {"not", "keyword"},
			  {"not_eq", "keyword"},
			  {"or", "keyword"},
			  {"or_eq", "keyword"},
			  {"xor", "keyword"},
			  {"xor_eq", "keyword"},
			  {"private", "keyword"},
			  {"protected", "keyword"},
			  {"public", "keyword"},
			  {"register", "keyword"},
			  {"nullptr", "keyword"},
			  {"operator", "keyword"},
			  {"asm", "keyword"},
			  {"bitand", "keyword"},
			  {"bitor", "keyword"},
			  {"catch", "keyword"},
			  {"throw", "keyword"},
			  {"try", "keyword"},
			  {"class", "keyword"},
			  {"compl", "keyword"},
			  {"explicit", "keyword"},
			  {"export", "keyword"},
			  {"concept", "keyword"},
			  {"consteval", "keyword"},
			  {"constexpr", "keyword"},
			  {"constinit", "keyword"},
			  {"const_cast", "keyword"},
			  {"dynamic_cast", "keyword"},
			  {"reinterpret_cast", "keyword"},
			  {"static_cast", "keyword"},
			  {"static_assert", "keyword"},
			  {"template", "keyword"},
			  {"this", "keyword"},
			  {"thread_local", "keyword"},
			  {"requires", "keyword"},
			  {"co_wait", "keyword"},
			  {"co_return", "keyword"},
			  {"co_yield", "keyword"},
			  {"decltype", "keyword"},
			  {"delete", "keyword"},
			  {"export", "keyword"},
			  {"friend", "keyword"},
			  {"typeid", "keyword"},
			  {"typename", "keyword"},
			  {"mutable", "keyword"},
			  {"virtual", "keyword"},
			  {"using", "keyword"},
			  {"namespace", "keyword"},
			  {"new", "keyword"},
			  {"noexcept", "keyword"},
			  {"if", "keyword"},
			  {"then", "keyword"},
			  {"else", "keyword"},
			  {"elseif", "keyword"},
			  {"do", "keyword"},
			  {"while", "keyword"},
			  {"for", "keyword"},
			  {"break", "keyword"},
			  {"continue", "keyword"},
			  {"return", "keyword"},
			  {"goto", "keyword"},
			  {"struct", "keyword"},
			  {"union", "keyword"},
			  {"typedef", "keyword"},
			  {"enum", "keyword"},
			  {"extern", "keyword"},
			  {"static", "keyword"},
			  {"volatile", "keyword"},
			  {"const", "keyword"},
			  {"inline", "keyword"},
			  {"switch", "keyword"},
			  {"case", "keyword"},
			  {"default", "keyword"},
			  {"auto", "keyword"},
			  {"const", "keyword"},
			  {"void", "keyword"},
			  {"int", "keyword2"},
			  {"short", "keyword2"},
			  {"long", "keyword2"},
			  {"float", "keyword2"},
			  {"double", "keyword2"},
			  {"char", "keyword2"},
			  {"unsigned", "keyword2"},
			  {"bool", "keyword2"},
			  {"true", "keyword2"},
			  {"false", "keyword2"},
			  {"wchar_t", "keyword2"},
			  {"char8_t", "keyword2"},
			  {"char16_t", "keyword2"},
			  {"char32_t", "keyword2"},
			  {"NULL", "literal"},
		  },
		  "//"} );

	// PHP
	add( {{"%.php$", "%.php3$", "%.php4$", "%.php5$", "%.phtml"},
		  {
			  {{"//.-\n"}, "comment"},
			  {{"/%*", "%*/"}, "comment"},
			  {{"\"", "\"", "\\"}, "string"},
			  {{"'", "'", "\\"}, "string"},
			  {{"%\\x[%da-fA-F]+"}, "number"},
			  {{"-?%d+[%d%.eE]*"}, "number"},
			  {{"-?%.?%d+"}, "number"},
			  {{"[%.%+%-=/%*%^%%<>!~|&]"}, "operator"},
			  {{"[%a_][%w_]*%f[(]"}, "function"},
			  {{"[%a_][%w_]*"}, "symbol"},
			  {{"%$[%a][%w_]*"}, "keyword2"},
		  },
		  {
			  {"return", "keyword"},	  {"if", "keyword"},
			  {"else", "keyword"},		  {"elseif", "keyword"},
			  {"endif", "keyword"},		  {"declare", "keyword"},
			  {"enddeclare", "keyword"},  {"switch", "keyword"},
			  {"endswitch", "keyword"},	  {"as", "keyword"},
			  {"do", "keyword"},		  {"for", "keyword"},
			  {"endfor", "keyword"},	  {"foreach", "keyword"},
			  {"endforeach", "keyword"},  {"while", "keyword"},
			  {"endwhile", "keyword"},	  {"switch", "keyword"},
			  {"case", "keyword"},		  {"continue", "keyword"},
			  {"default", "keyword"},	  {"break", "keyword"},
			  {"exit", "keyword"},		  {"goto", "keyword"},

			  {"catch", "keyword"},		  {"throw", "keyword"},
			  {"try", "keyword"},		  {"finally", "keyword"},

			  {"class", "keyword"},		  {"trait", "keyword"},
			  {"interface", "keyword"},	  {"public", "keyword"},
			  {"static", "keyword"},	  {"protected", "keyword"},
			  {"private", "keyword"},	  {"abstract", "keyword"},
			  {"final", "keyword"},

			  {"function", "keyword2"},	  {"global", "keyword2"},
			  {"var", "keyword2"},		  {"const", "keyword2"},
			  {"bool", "keyword2"},		  {"boolean", "keyword2"},
			  {"int", "keyword2"},		  {"integer", "keyword2"},
			  {"real", "keyword2"},		  {"double", "keyword2"},
			  {"float", "keyword2"},	  {"string", "keyword2"},
			  {"array", "keyword2"},	  {"object", "keyword2"},
			  {"callable", "keyword2"},	  {"iterable", "keyword2"},

			  {"namespace", "keyword2"},  {"extends", "keyword2"},
			  {"implements", "keyword2"}, {"instanceof", "keyword2"},
			  {"require", "keyword2"},	  {"require_once", "keyword2"},
			  {"include", "keyword2"},	  {"include_once", "keyword2"},
			  {"use", "keyword2"},		  {"new", "keyword2"},
			  {"clone", "keyword2"},

			  {"true", "literal"},		  {"false", "literal"},
			  {"NULL", "literal"},		  {"parent", "literal"},
			  {"self", "literal"},
		  },
		  "//"} );
}

SyntaxDefinition& SyntaxDefinitionManager::add( SyntaxDefinition&& syntaxStyle ) {
	mStyles.emplace_back( std::move( syntaxStyle ) );
	return mStyles.back();
}

const SyntaxDefinition& SyntaxDefinitionManager::getPlainStyle() const {
	return mEmptyDefinition;
}

SyntaxDefinition& SyntaxDefinitionManager::getStyleByExtensionRef( const std::string& filePath ) {
	return const_cast<SyntaxDefinition&>( getStyleByExtension( filePath ) );
}

const SyntaxDefinition&
SyntaxDefinitionManager::getStyleByExtension( const std::string& filePath ) const {
	std::string extension( FileSystem::fileExtension( filePath ) );
	if ( !extension.empty() ) {
		for ( auto style = mStyles.rbegin(); style != mStyles.rend(); ++style ) {
			for ( auto ext : style->getFiles() ) {
				if ( String::startsWith( ext, "%." ) || String::endsWith( ext, "$" ) ) {
					LuaPatternMatcher words( ext );
					int start, end;
					if ( words.find( filePath, 0, start, end ) ) {
						return *style;
					}
				} else if ( extension == ext ) {
					return *style;
				}
			}
		}
	}
	return mEmptyDefinition;
}

}}} // namespace EE::UI::Doc
