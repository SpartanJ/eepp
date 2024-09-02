#include <eepp/ui/doc/languages/cmake.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addCMake() {

	SyntaxDefinitionManager::instance()->add(

		{ "CMake",
		  { "%.cmake$", "CMakeLists.txt$" },
		  {
			  { { "#", "\n" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "[%a_][%w_]*%s?%f[(]" }, "function" },
			  { { "CMAKE_[%w%d_]+" }, "keyword" },
			  { { "CTEST_[%w%d_]+" }, "keyword" },
			  { { "%u[%u%d_]*_[%u%d_]+" }, "keyword" },
			  { { "%${[%a_][%w_]*%}" }, "keyword2" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "MSVC90", "keyword2" },		 { "UNIX", "keyword2" },
			  { "REPLACE", "literal" },		 { "MATCHES", "operator" },
			  { "SOURCES", "keyword2" },	 { "REGEX", "literal" },
			  { "MSVC12", "keyword2" },		 { "RUNTIME", "literal" },
			  { "LANGUAGE", "keyword2" },	 { "BORLAND", "keyword2" },
			  { "MINGW", "keyword2" },		 { "C", "literal" },
			  { "TOLOWER", "literal" },		 { "FALSE", "literal" },
			  { "OR", "operator" },			 { "DEPRECATION", "keyword2" },
			  { "LIBTYPE", "literal" },		 { "COMMAND", "literal" },
			  { "DISABLED", "keyword2" },	 { "VARIABLES", "keyword2" },
			  { "PREFIX", "keyword2" },		 { "APPEND", "literal" },
			  { "OFF", "literal" },			 { "XCTEST", "keyword2" },
			  { "GHSMULTI", "keyword2" },	 { "HELPSTRING", "keyword2" },
			  { "MSVC14", "keyword2" },		 { "AUTORCC", "keyword2" },
			  { "TESTS", "keyword2" },		 { "WINCE", "keyword2" },
			  { "DEFINED", "literal" },		 { "TARGET", "literal" },
			  { "NAME", "keyword2" },		 { "LABELS", "keyword2" },
			  { "VALUE", "keyword2" },		 { "TYPE", "keyword2" },
			  { "TRUE", "literal" },		 { "EchoString", "keyword2" },
			  { "MSVC60", "keyword2" },		 { "ARCHIVE", "literal" },
			  { "IMMEDIATE", "literal" },	 { "MSVC71", "keyword2" },
			  { "DESTINATION", "literal" },	 { "INTERFACE", "literal" },
			  { "PROCESSORS", "keyword2" },	 { "CYGWIN", "keyword2" },
			  { "ENVIRONMENT", "keyword2" }, { "IOS", "keyword2" },
			  { "SUFFIX", "keyword2" },		 { "TOUPPER", "literal" },
			  { "ANDROID", "keyword2" },	 { "ENV", "keyword2" },
			  { "WIN32", "keyword2" },		 { "STATUS", "literal" },
			  { "BUNDLE", "keyword2" },		 { "STREQUAL", "literal" },
			  { "MSVC10", "keyword2" },		 { "AUTOUIC", "keyword2" },
			  { "GNUtoMS", "keyword2" },	 { "SOVERSION", "keyword2" },
			  { "AUTOMOC", "keyword2" },	 { "SUBDIRECTORIES", "keyword2" },
			  { "SYMBOLIC", "keyword2" },	 { "AND", "operator" },
			  { "FOLDER", "keyword2" },		 { "APPLE", "keyword2" },
			  { "MSVC70", "keyword2" },		 { "COST", "keyword2" },
			  { "ABSTRACT", "keyword2" },	 { "MSVC11", "keyword2" },
			  { "IMPORTED", "keyword2" },	 { "NOT", "operator" },
			  { "MODIFIED", "keyword2" },	 { "GLOB", "literal" },
			  { "MSVC80", "keyword2" },		 { "MACROS", "keyword2" },
			  { "LOCATION", "keyword2" },	 { "CXX", "literal" },
			  { "WARNING", "literal" },		 { "MSYS", "keyword2" },
			  { "TIMEOUT", "keyword2" },	 { "CACHE", "keyword2" },
			  { "EXPR", "literal" },		 { "FRAMEWORK", "keyword2" },
			  { "DEPENDS", "keyword2" },	 { "XCODE", "keyword2" },
			  { "LIBRARY", "literal" },		 { "MEASUREMENT", "keyword2" },
			  { "GENERATED", "keyword2" },	 { "STRINGS", "keyword2" },
			  { "VERSION", "keyword2" },	 { "KEEP_EXTENSION", "keyword2" },
			  { "EQUAL", "operator" },		 { "MSVC", "keyword2" },
			  { "RESOURCE", "keyword2" },	 { "ADVANCED", "keyword2" },
			  { "ON", "literal" },			 { "LANGUAGES", "keyword2" },

		  },
		  "#",
		  { "^cmake_minimum_required.*%c" }

		} );
}

}}}} // namespace EE::UI::Doc::Language
