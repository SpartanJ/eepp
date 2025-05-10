#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/iostreammemory.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/system/packmanager.hpp>
#include <eepp/ui/doc/languages/c.hpp>
#include <eepp/ui/doc/languages/configfile.hpp>
#include <eepp/ui/doc/languages/cpp.hpp>
#include <eepp/ui/doc/languages/css.hpp>
#include <eepp/ui/doc/languages/html.hpp>
#include <eepp/ui/doc/languages/javascript.hpp>
#include <eepp/ui/doc/languages/json.hpp>
#include <eepp/ui/doc/languages/lua.hpp>
#include <eepp/ui/doc/languages/markdown.hpp>
#include <eepp/ui/doc/languages/python.hpp>
#include <eepp/ui/doc/languages/xml.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <nlohmann/json.hpp>

using namespace EE::System;
using namespace EE::UI::Doc::Language;

using json = nlohmann::json;

namespace EE { namespace UI { namespace Doc {

class TextMateScopeMapper {
  private:
	// Define the mapping from TM scope prefixes to ecode types.
	// Use string_view for efficiency.
	// **IMPORTANT**: This vector MUST be sorted by the length of the TM prefix
	//                in DESCENDING order to ensure more specific matches are
	//                checked first (e.g., "keyword.control" before "keyword").
	//                We initialize it sorted here.
	inline static const std::vector<std::pair<std::string_view, std::string_view>> scope_map_ = {
		// -- Most Specific --
		{ "markup.underline.link", "link" },	   // Specific markup
		{ "constant.character.escape", "string" }, // Escapes within strings
		{ "variable.parameter", "keyword3" },	   // Function parameters
		{ "variable.language", "literal" },		 // Language constants like 'this', 'self', 'null'?
		{ "storage.type", "keyword2" },			 // Class, struct, int, bool etc. (declaration)
		{ "entity.name.function", "function" },	 // Function definition name
		{ "entity.name.type", "keyword2" },		 // Type name (class, struct, etc.) in definition
		{ "entity.name.class", "keyword2" },	 // Class name in definition
		{ "entity.name.struct", "keyword2" },	 // Struct name in definition
		{ "entity.name.interface", "keyword2" }, // Interface name in definition
		{ "entity.name.tag", "keyword2" },		 // HTML/XML tag name
		{ "keyword.control", "keyword" },		 // if, else, for, while, return etc.
		{ "keyword.operator", "operator" },		 // +, -, =, and, or, etc.
		{ "punctuation.definition.tag", "operator" }, // <, >, </ in HTML/XML
		{ "support.function", "function" },			  // Built-in functions (print, len)
		{ "support.type", "keyword2" },				  // Built-in types (string, list)
		{ "support.class", "keyword2" },			  // Built-in classes
		{ "storage.modifier", "keyword" },			  // public, private, static, const etc.
		{ "constant.numeric", "number" },			  // Numbers
		{ "constant.language", "literal" },			  // true, false, null etc.
		{ "comment.unused", "normal" },				  // unused comments pattern

		// -- General Categories --
		{ "comment", "comment" },	   // Comments
		{ "string", "string" },		   // Strings
		{ "keyword", "keyword" },	   // Any other keyword
		{ "storage", "keyword" },	   // Fallback for storage (like storage.type)
		{ "operator", "operator" },	   // Fallback for operators (less common)
		{ "punctuation", "operator" }, // General punctuation -> operator is often suitable
		{ "constant", "literal" },	   // Any other constant (fallback)
		{ "entity", "normal" },		   // General entities (fallback, often unstyled)
		{ "variable", "normal" },	   // General variables (fallback, often unstyled)
		{ "support", "normal" },	   // General support scopes (fallback)
		{ "markup", "normal" }		   // General markup (fallback)
		// "meta" scopes are intentionally left out. They define structure,
		// not usually the token type itself. If no inner scope matches,
		// it will eventually fall back to "normal".
	};

  public:
	/**
	 * @brief Maps a TextMate scope string to an ecode syntax highlighting type.
	 *
	 * This function takes a full TextMate scope string (e.g., "keyword.control.if.python")
	 * and finds the most specific matching prefix in its internal mapping table
	 * to determine the appropriate ecode type (e.g., "keyword").
	 *
	 * The matching prioritizes longer prefixes (more specific rules) over shorter ones.
	 * For example, "keyword.control.if" will match "keyword.control" -> "keyword"
	 * before it could match "keyword" -> "keyword".
	 *
	 * Scopes starting with "meta." generally describe code structure and do not
	 * directly map to a type themselves, unless a more specific inner rule matches.
	 * If no specific rule matches, the default type "normal" is returned.
	 *
	 * @param scopeName The TextMate scope string to map.
	 * @return The corresponding ecode type string (e.g., "keyword", "string", "comment", "normal").
	 */
	static std::string scopeToType( const std::string_view scopeName ) {
		if ( scopeName.empty() ) {
			return "normal"; // Default for empty scope
		}

		// Iterate through the pre-sorted map (longest prefix first)
		for ( const auto& mapping : scope_map_ ) {
			const std::string_view tmPrefix = mapping.first;
			// Check if scopeName starts with tmPrefix
			if ( scopeName.starts_with( tmPrefix ) ) {
				// Make sure it's either the full scope or followed by a '.'
				// (prevents "stringBuffer" matching "string")
				if ( scopeName.size() == tmPrefix.size() || scopeName[tmPrefix.size()] == '.' ) {
					return std::string( mapping.second ); // Return the corresponding ecode type
				}
			}
		}

		// If no prefix matched, return the default type
		return "normal";
	}
};

SINGLETON_DECLARE_IMPLEMENTATION( SyntaxDefinitionManager )

SyntaxDefinitionManager*
SyntaxDefinitionManager::createSingleton( std::size_t reserveSpaceForLanguages ) {
	if ( NULL == ms_singleton ) {
		ms_singleton = eeNew( SyntaxDefinitionManager, ( reserveSpaceForLanguages ) );
	}

	return ms_singleton;
}

static void addPlainText() {
	SyntaxDefinitionManager::instance()->add(
		{ "Plain Text", { "%.txt$" }, {}, {}, "", {}, "plaintext" } );
}

// Syntax definitions can be directly converted from the lite (https://github.com/rxi/lite) and
// lite-plugins (https://github.com/rxi/lite-plugins) supported languages.

SyntaxDefinitionManager::SyntaxDefinitionManager( std::size_t reserveSpaceForLanguages ) {
	if ( ms_singleton == nullptr )
		ms_singleton = this;

	mDefinitions.reserve( reserveSpaceForLanguages );

	// Register some languages support.
	addPlainText();
	addC();
	addConfigFile();
	addCPP();
	addCSS();
	addHTML();
	addJavaScript();
	addJSON();
	addLua();
	addMarkdown();
	addPython();
	addXML();
}

const std::vector<SyntaxDefinition>& SyntaxDefinitionManager::getDefinitions() const {
	return mDefinitions;
}

static json toJson( const SyntaxDefinition& def ) {
	const auto serializePattern =
		[&def]( const SyntaxPattern& ptrn ) -> std::optional<nlohmann::json> {
		json pattern;
		auto ptrnType =
			ptrn.matchType == SyntaxPatternMatchType::RegEx
				? "regex"
				: ( ptrn.matchType == SyntaxPatternMatchType::Parser ? "parser" : "pattern" );

		// Do not export injected patterns
		if ( ptrn.matchType == SyntaxPatternMatchType::LuaPattern && ptrn.patterns.size() == 1 &&
			 ( ptrn.patterns[0] == "%s+" || ptrn.patterns[0] == "%w+%f[%s]" ) )
			return {};

		bool hasInclude = false;
		if ( ptrn.patterns.size() == 2 && ptrn.patterns[0] == "include" ) {
			hasInclude = true;
			pattern["include"] = ptrn.patterns[1];
		} else if ( ptrn.patterns.size() == 1 ) {
			pattern[ptrnType] = ptrn.patterns[0];
		} else if ( ptrn.patterns.size() ) {
			pattern[ptrnType] = ptrn.patterns;
		}

		if ( !hasInclude ) {
			if ( ptrn.typesNames.size() == 1 ) {
				pattern["type"] = ptrn.typesNames[0];
			} else if ( ptrn.typesNames.size() ) {
				pattern["type"] = ptrn.typesNames;
			}
			if ( ptrn.endTypesNames.size() == 1 ) {
				pattern["end_type"] = ptrn.endTypesNames[0];
			} else if ( ptrn.endTypesNames.size() ) {
				pattern["end_type"] = ptrn.endTypesNames;
			}
			if ( !ptrn.syntax.empty() )
				pattern["syntax"] = ptrn.syntax == def.getLanguageName() ? "$self" : ptrn.syntax;
		}
		return pattern;
	};

	json j;
	j["name"] = def.getLanguageName();
	if ( def.getLSPName() != String::toLower( def.getLanguageName() ) )
		j["lsp_name"] = def.getLSPName();
	j["files"] = def.getFiles();
	if ( !def.getComment().empty() )
		j["comment"] = def.getComment();
	if ( !def.getPatterns().empty() ) {
		j["patterns"] = json::array();
		for ( const auto& ptrn : def.getPatterns() ) {
			auto pattern = serializePattern( ptrn );
			if ( pattern )
				j["patterns"].emplace_back( std::move( *pattern ) );
		}
	}
	if ( !def.getSymbols().empty() ) {
		j["symbols"] = json::array();
		for ( const auto& sym : def.getSymbolNames() )
			j["symbols"].emplace_back( json{ json{ sym.first, sym.second } } );
	}

	if ( !def.getHeaders().empty() )
		j["headers"] = def.getHeaders();

	if ( def.getAutoCloseXMLTags() )
		j["auto_close_xml_tags"] = true;

	if ( !def.isVisible() )
		j["visible"] = false;

	if ( def.getFoldRangeType() != FoldRangeType::Undefined )
		j["fold_range_type"] = FoldRangeTypeUtil::toString( def.getFoldRangeType() );

	if ( !def.getFoldBraces().empty() ) {
		j["fold_braces"] = json::array();

		for ( const auto& fb : def.getFoldBraces() ) {
			json braces;
			braces["start"] = String( static_cast<String::StringBaseType>( fb.first ) ).toUtf8();
			braces["end"] = String( static_cast<String::StringBaseType>( fb.second ) ).toUtf8();
			j["fold_braces"].push_back( braces );
		}
	}

	if ( !def.getRepositories().empty() ) {
		j["repository"] = json::object();
		auto& repository = j["repository"];

		for ( const auto& [hash, patterns] : def.getRepositories() ) {
			std::string name = def.getRepositoryName( hash );
			nlohmann::json repo;
			for ( const auto& pattern : patterns ) {
				auto ojptrn = serializePattern( pattern );
				if ( ojptrn )
					repo.emplace_back( std::move( *ojptrn ) );
			}
			repository.emplace( name, std::move( repo ) );
		}
	}

	return j;
}

bool SyntaxDefinitionManager::save( const std::string& path,
									const std::vector<SyntaxDefinition>& def ) {
	if ( def.size() == 1 ) {
		return FileSystem::fileWrite( path, toJson( def[0] ).dump( 2 ) );
	} else if ( !def.empty() ) {
		json j = json::array();
		for ( const auto& d : def )
			j.emplace_back( toJson( d ) );
		return FileSystem::fileWrite( path, j.dump( 2 ) );
	} else {
		json j = json::array();
		for ( const auto& d : mDefinitions )
			j.emplace_back( toJson( d ) );
		return FileSystem::fileWrite( path, j.dump( 2 ) );
	}
	return false;
}

void SyntaxDefinitionManager::setLanguageExtensionsPriority(
	const std::map<std::string, std::string>& priorities ) {
	mPriorities = priorities;
}

std::optional<size_t> SyntaxDefinitionManager::getLanguageIndex( const std::string& langName ) {
	size_t pos = 0;
	for ( const auto& def : mDefinitions ) {
		if ( def.getLanguageName() == langName ) {
			return pos;
		}
		++pos;
	}
	return {};
}

static std::string str( std::string s, const std::string& prepend = "",
						const std::string& append = "", bool allowEmptyString = true ) {
	if ( s.empty() && !allowEmptyString )
		return "";
	String::replaceAll( s, "\\", "\\\\" );
	String::replaceAll( s, "\"", "\\\"" );
	return prepend + "\"" + String::escape( s ) + "\"" + append;
}

static std::string join( const std::vector<std::string>& vec, bool createCont = true,
						 bool allowReduce = false, bool setType = false, bool allowSkip = false,
						 bool addSep = false, const std::string& delim = ", " ) {
	std::string sep = addSep ? ", " : "";
	if ( vec.empty() )
		return allowSkip ? "" : ( sep + "{}" );
	if ( vec.size() == 1 && allowReduce )
		return sep + str( vec[0] );
	std::string accum = std::accumulate(
		vec.begin() + 1, vec.end(), str( vec[0] ),
		[&delim]( const std::string& a, const std::string& b ) { return a + delim + str( b ); } );
	return sep + ( createCont ? ( std::string( setType ? "std::vector<std::string>" : "" ) + "{ " +
								  accum + " }" )
							  : accum );
}

static std::string funcName( std::string name ) {
	if ( name.empty() )
		return "";
	String::replaceAll( name, " ", "" );
	String::replaceAll( name, "+", "p" );
	String::replaceAll( name, "#", "sharp" );
	name[0] = std::toupper( name[0] );
	return name;
}

std::pair<std::string, std::string> SyntaxDefinitionManager::toCPP( const SyntaxDefinition& def ) {
	std::string lang( def.getLanguageNameForFileSystem() );
	std::string func( funcName( lang ) );
	std::string header = "#ifndef EE_UI_DOC_" + func + "\n#define EE_UI_DOC_" + func +
						 "\n\nnamespace EE { namespace UI { namespace "
						 "Doc { namespace Language {\n\nextern void add" +
						 func + "();\n\n}}}}\n\n#endif\n";
	std::string buf = String::format( R"cpp(#include <eepp/ui/doc/languages/%s.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {
)cpp",
									  lang.c_str() );
	buf += "\nvoid add" + func + "() {\n";
	buf += "\nSyntaxDefinitionManager::instance()->add(\n\n{";
	// lang name
	buf += str( def.getLanguageName() ) + ",\n";
	// file types
	buf += join( def.getFiles() ) + ",\n";
	// patterns
	buf += "{\n";
	for ( const auto& pattern : def.getPatterns() ) {
		buf += "{ " + join( pattern.patterns ) + ", " +
			   join( pattern.typesNames, true, true,
					 pattern.matchType != SyntaxPatternMatchType::LuaPattern ) +
			   join( pattern.endTypesNames, true, true,
					 pattern.matchType != SyntaxPatternMatchType::LuaPattern, true, true ) +
			   str( pattern.syntax, ", ", "", false );
		if ( pattern.matchType != SyntaxPatternMatchType::LuaPattern && pattern.syntax.empty() ) {
			if ( pattern.matchType == SyntaxPatternMatchType::RegEx )
				buf += ", \"\", SyntaxPatternMatchType::RegEx";
			else if ( pattern.matchType == SyntaxPatternMatchType::Parser )
				buf += ", \"\", SyntaxPatternMatchType::Parser";
		} else if ( pattern.matchType != SyntaxPatternMatchType::LuaPattern ) {
			if ( pattern.matchType == SyntaxPatternMatchType::RegEx )
				buf += ", SyntaxPatternMatchType::RegEx";
			else if ( pattern.matchType == SyntaxPatternMatchType::Parser )
				buf += ", SyntaxPatternMatchType::Parser";
		}
		buf += " },\n";
	}
	buf += "\n},\n";
	// symbols
	buf += "{\n";
	for ( const auto& symbol : def.getSymbolNames() )
		buf += "{ " + str( symbol.first ) + " , " + str( symbol.second ) + " },\n";
	buf += "\n},\n";
	buf += str( def.getComment(), "", "", true ) + ",\n";
	std::string lspName =
		def.getLSPName().empty() || def.getLSPName() == String::toLower( def.getLanguageName() )
			? ""
			: def.getLSPName();
	// headers
	buf += join( def.getHeaders() ) + ( lspName.empty() ? "" : "," ) + "\n";
	// lsp
	buf += lspName.empty() ? "" : str( def.getLSPName() );
	buf += "\n}";
	buf += ")";
	if ( !def.isVisible() )
		buf += ".setVisible( false )\n";
	if ( def.getAutoCloseXMLTags() )
		buf += ".setAutoCloseXMLTags( true )\n";

	if ( def.getFoldRangeType() != FoldRangeType::Undefined ) {
		std::string fdtn;
		switch ( def.getFoldRangeType() ) {
			case FoldRangeType::Braces:
				fdtn = "FoldRangeType::Braces";
			case FoldRangeType::Indentation:
				fdtn = "FoldRangeType::Indentation";
			case FoldRangeType::Tag:
				fdtn = "FoldRangeType::Tag";
			case FoldRangeType::Markdown:
				fdtn = "FoldRangeType::Markdown";
			case FoldRangeType::Undefined:
				break;
		}
		if ( !fdtn.empty() )
			buf += String::format( ".setFoldRangeType( %s )\n", fdtn );
	}

	if ( !def.getFoldBraces().empty() ) {
		buf += ".setFoldBraces( { ";
		for ( const auto& brace : def.getFoldBraces() ) {
			buf += String::format(
				"{ '%s', '%s' },",
				String( static_cast<String::StringBaseType>( brace.first ) ).toUtf8(),
				String( static_cast<String::StringBaseType>( brace.second ) ).toUtf8() );
		}
		buf += " } );";
	}

	buf += ";\n}\n";
	buf += "\n}}}} // namespace EE::UI::Doc::Language\n";
	return std::make_pair( std::move( header ), std::move( buf ) );
}

SyntaxDefinition& SyntaxDefinitionManager::add( SyntaxDefinition&& syntaxStyle ) {
	syntaxStyle.mLanguageIndex = mDefinitions.size();
	mDefinitions.emplace_back( std::move( syntaxStyle ) );
	return mDefinitions.back();
}

const SyntaxDefinition& SyntaxDefinitionManager::getPlainDefinition() const {
	return mDefinitions[0];
}

SyntaxDefinition& SyntaxDefinitionManager::getByExtensionRef( const std::string& filePath ) {
	return const_cast<SyntaxDefinition&>( getByExtension( filePath ) );
}

const SyntaxDefinition&
SyntaxDefinitionManager::getByLanguageName( const std::string_view& name ) const {
	for ( auto& definition : mDefinitions ) {
		if ( definition.getLanguageName() == name ||
			 std::find_if( definition.getAlternativeNames().begin(),
						   definition.getAlternativeNames().end(),
						   [name]( const std::string& altName ) { return altName == name; } ) !=
				 definition.getAlternativeNames().end() )
			return definition;
	}
	return mDefinitions[0];
}

const SyntaxDefinition& SyntaxDefinitionManager::getByLanguageIndex( const Uint32& index ) const {
	eeASSERT( index < mDefinitions.size() );
	return mDefinitions[index];
}

const SyntaxDefinition&
SyntaxDefinitionManager::getByLanguageNameInsensitive( const std::string_view& name ) const {
	for ( auto& definition : mDefinitions ) {
		if ( String::iequals( definition.getLanguageName(), name ) ||
			 std::find_if( definition.getAlternativeNames().begin(),
						   definition.getAlternativeNames().end(),
						   [name]( const std::string& altName ) {
							   return String::iequals( altName, name );
						   } ) != definition.getAlternativeNames().end() )
			return definition;
	}
	return mDefinitions[0];
}

const SyntaxDefinition&
SyntaxDefinitionManager::getByLSPName( const std::string_view& name ) const {
	for ( auto& definition : mDefinitions ) {
		if ( definition.getLSPName() == name )
			return definition;
	}
	return mDefinitions[0];
}

const SyntaxDefinition&
SyntaxDefinitionManager::getByLanguageId( const String::HashType& id ) const {
	for ( auto& definition : mDefinitions ) {
		if ( definition.getLanguageId() == id )
			return definition;
	}
	return mDefinitions[0];
}

SyntaxDefinition& SyntaxDefinitionManager::getByLanguageNameRef( const std::string& name ) {
	return const_cast<SyntaxDefinition&>( getByLanguageName( name ) );
}

std::vector<std::string> SyntaxDefinitionManager::getLanguageNames() const {
	std::vector<std::string> names;
	for ( auto& definition : mDefinitions ) {
		if ( definition.isVisible() )
			names.push_back( definition.getLanguageName() );
	}
	std::sort( names.begin(), names.end() );
	return names;
}

std::vector<std::string> SyntaxDefinitionManager::getExtensionsPatternsSupported() const {
	std::vector<std::string> exts;
	exts.reserve( mDefinitions.size() );
	for ( auto& definition : mDefinitions )
		for ( auto& pattern : definition.getFiles() )
			exts.emplace_back( pattern );
	return exts;
}

const SyntaxDefinition*
SyntaxDefinitionManager::getPtrByLanguageName( const std::string& name ) const {
	for ( const auto& definition : mDefinitions ) {
		if ( definition.getLanguageName() == name )
			return &definition;
	}
	return nullptr;
}

const SyntaxDefinition* SyntaxDefinitionManager::getPtrByLSPName( const std::string& name ) const {
	for ( const auto& definition : mDefinitions ) {
		if ( definition.getLSPName() == name )
			return &definition;
	}
	return nullptr;
}

const SyntaxDefinition*
SyntaxDefinitionManager::getPtrByLanguageId( const String::HashType& id ) const {
	for ( const auto& definition : mDefinitions ) {
		if ( definition.getLanguageId() == id )
			return &definition;
	}
	return nullptr;
}

static SyntaxPattern parsePattern( const nlohmann::json& pattern ) {
	std::vector<std::string> type;
	std::vector<std::string> endType;
	std::vector<std::string> ptrns;
	std::vector<SyntaxPattern> subPatterns;
	auto ctype = SyntaxPatternMatchType::LuaPattern;
	std::string syntax;

	const auto fillTypes = []( const nlohmann::json& captures, std::vector<std::string>& type ) {
		Uint64 totalCaptures = 0;
		for ( const auto& [capNumStr, _] : captures.items() ) {
			Uint64 num;
			if ( String::fromString( num, capNumStr ) )
				totalCaptures = eemax( totalCaptures, num + 1 );
		}

		for ( Uint64 i = 0; i < totalCaptures; i++ ) {
			auto capNumStr = String::toString( i );
			if ( captures.contains( capNumStr ) && captures[capNumStr].contains( "name" ) ) {
				type.emplace_back(
					TextMateScopeMapper::scopeToType( captures[capNumStr].value( "name", "" ) ) );
			} else {
				type.emplace_back( "normal" );
			}
		}
	};

	// Assume TextMate pattern
	if ( pattern.contains( "name" ) || pattern.contains( "begin" ) ) {
		ctype = SyntaxPatternMatchType::RegEx;

		if ( pattern.contains( "beginCaptures" ) )
			fillTypes( pattern["beginCaptures"], type );

		if ( pattern.contains( "endCaptures" ) )
			fillTypes( pattern["endCaptures"], endType );

		if ( type.empty() && pattern.contains( "captures" ) )
			fillTypes( pattern["captures"], type );

		if ( type.empty() && pattern.contains( "name" ) ) {
			type.emplace_back( TextMateScopeMapper::scopeToType( pattern.value( "name", "" ) ) );
		}

		if ( pattern.contains( "match" ) && pattern["match"].is_string() ) {
			ptrns.emplace_back( pattern.value( "match", "" ) );
		} else if ( pattern.contains( "include" ) ) {
			ptrns.emplace_back( "include" );
			ptrns.emplace_back( pattern.value( "include", "" ) );
		}

		if ( pattern.contains( "begin" ) )
			ptrns.emplace_back( pattern.value( "begin", "" ) );

		if ( pattern.contains( "end" ) )
			ptrns.emplace_back( pattern.value( "end", "" ) );

		// Sub-languages / Sub patterns?
		if ( pattern.contains( "patterns" ) && !pattern["patterns"].empty() &&
			 pattern["patterns"].is_array() ) {
			const auto& patterns = pattern["patterns"];
			if ( patterns.size() == 1 && patterns[0].is_object() ) {
				if ( patterns[0].contains( "include" ) &&
					 patterns[0].value( "include", "" ) == "$self" ) {
					syntax = "$self";
				} else if ( patterns[0].contains( "name" ) && patterns[0].contains( "match" ) &&
					 patterns[0].value( "name", "" ).starts_with( "constant.character.escape" ) ) {
					ptrns.emplace_back( patterns[0].value( "match", "" ) );
				} else {
					subPatterns.push_back( parsePattern( patterns[0] ) );
				}
			} else {
				subPatterns.reserve( patterns.size() );
				for ( const auto& subPattern : patterns )
					subPatterns.push_back( parsePattern( subPattern ) );
			}
		}

	} else {
		if ( pattern.contains( "syntax" ) && pattern["syntax"].is_string() )
			syntax = pattern.value( "syntax", "" );

		if ( pattern.contains( "type" ) ) {
			if ( pattern["type"].is_array() ) {
				for ( const auto& t : pattern["type"] ) {
					if ( t.is_string() )
						type.emplace_back( t.get<std::string>() );
				}
			} else if ( pattern["type"].is_string() ) {
				type.emplace_back( pattern["type"] );
			}
		} else {
			type.emplace_back( "normal" );
		}

		if ( pattern.contains( "end_type" ) ) {
			if ( pattern["end_type"].is_array() ) {
				for ( const auto& t : pattern["end_type"] ) {
					if ( t.is_string() )
						endType.emplace_back( t.get<std::string>() );
				}
			} else if ( pattern["end_type"].is_string() ) {
				endType.emplace_back( pattern["end_type"] );
			}
		}

		if ( pattern.contains( "include" ) ) {
			ptrns.emplace_back( "include" );
			ptrns.emplace_back( pattern.value( "include", "" ) );
		} else if ( pattern.contains( "pattern" ) ) {
			if ( pattern["pattern"].is_array() ) {
				const auto& ptrnIt = pattern["pattern"];
				for ( const auto& ptrn : ptrnIt )
					ptrns.emplace_back( ptrn );
			} else if ( pattern["pattern"].is_string() ) {
				ptrns.emplace_back( pattern["pattern"] );
			}
		} else if ( pattern.contains( "regex" ) ) {
			ctype = SyntaxPatternMatchType::RegEx;
			if ( pattern["regex"].is_array() ) {
				const auto& ptrnIt = pattern["regex"];
				for ( const auto& ptrn : ptrnIt )
					ptrns.emplace_back( ptrn );
			} else if ( pattern["regex"].is_string() ) {
				ptrns.emplace_back( pattern["regex"] );
			}
		} else if ( pattern.contains( "parser" ) ) {
			ctype = SyntaxPatternMatchType::Parser;
			if ( pattern["parser"].is_array() ) {
				const auto& ptrnIt = pattern["parser"];
				for ( const auto& ptrn : ptrnIt )
					ptrns.emplace_back( ptrn );
			} else if ( pattern["parser"].is_string() ) {
				ptrns.emplace_back( pattern["parser"] );
			}
		}
	}

	eeASSERT( !ptrns.empty() );

	return SyntaxPattern( std::move( ptrns ), std::move( type ), std::move( endType ), syntax,
						  ctype, std::move( subPatterns ) );
}

static SyntaxDefinition loadTextMateLanguage( const nlohmann::json& json, SyntaxDefinition& def ) {
	if ( json.contains( "fileTypes" ) && json["fileTypes"].is_array() ) {
		const auto& files = json["fileTypes"];
		for ( const auto& file : files )
			if ( file.is_string() ) {
				auto ext( file.get<std::string>() );
				def.addFileType( ( !String::contains( ext, "." ) ? "%." : "" ) + ext + "$" );
			}
	} else if ( json.contains( "scopeName" ) && json["scopeName"].is_string() ) {
		const auto& scopeName = json.value( "scopeName", "" );
		def.addFileType( "%." + FileSystem::fileExtension( scopeName ) + "$" );
	}
	return def;
}

static SyntaxDefinition loadLanguage( const nlohmann::json& json ) {
	SyntaxDefinition def;

	try {
		def.setLanguageName( json.value( "name", "" ) );

		if ( json.contains( "patterns" ) && json["patterns"].is_array() ) {
			const auto& patterns = json["patterns"];
			for ( const auto& pattern : patterns )
				def.addPattern( parsePattern( pattern ) );
		}

		if ( json.contains( "repository" ) && json["repository"].is_object() ) {
			const auto& repository = json["repository"];
			for ( const auto& [name, repository] : repository.items() ) {
				std::vector<SyntaxPattern> ptrns;
				if ( repository.contains( "match" ) || repository.contains( "begin" ) ) {
					ptrns.emplace_back( parsePattern( repository ) );
				} else if ( repository.contains( "patterns" ) &&
							repository["patterns"].is_array() ) {
					const auto& patterns = repository["patterns"];
					for ( const auto& pattern : patterns )
						ptrns.emplace_back( parsePattern( pattern ) );
				}
				def.addRepository( name, std::move( ptrns ) );
			}
		}

		if ( ( json.contains( "$schema" ) && json["$schema"].is_string() &&
			   String::contains( json.value( "$schema", "" ), "tmlanguage.json" ) ) ||
			 json.contains( "scopeName" ) /* assume tmlanguage */ ) {
			return loadTextMateLanguage( json, def );
		}

		if ( json.contains( "lsp_name" ) && json["lsp_name"].is_string() )
			def.setLSPName( json["lsp_name"].get<std::string>() );

		if ( json.contains( "files" ) ) {
			if ( json["files"].is_array() ) {
				const auto& files = json["files"];
				for ( const auto& file : files ) {
					def.addFileType( file );
				}
			} else if ( json["files"].is_string() ) {
				def.addFileType( json["files"].get<std::string>() );
			}
		}

		def.setComment( json.value( "comment", "" ) );

		if ( json.contains( "symbols" ) ) {
			if ( json["symbols"].is_array() ) {
				const auto& symbols = json["symbols"];
				for ( const auto& symbol : symbols ) {
					for ( auto& el : symbol.items() ) {
						def.addSymbol( el.key(), el.value() );
					}
				}
			} else if ( json["symbols"].is_object() ) {
				for ( const auto& [key, value] : json["symbols"].items() ) {
					def.addSymbol( key, value );
				}
			}
		}

		if ( json.contains( "headers" ) && json["headers"].is_array() ) {
			const auto& headers = json["headers"];
			std::vector<std::string> hds;
			if ( headers.is_array() ) {
				for ( const auto& header : headers ) {
					if ( header.is_string() )
						hds.emplace_back( header.get<std::string>() );
				}
			} else if ( headers.is_string() ) {
				hds.emplace_back( headers.get<std::string>() );
			}
			if ( !hds.empty() )
				def.setHeaders( hds );
		}

		if ( json.contains( "visible" ) && json["visible"].is_boolean() )
			def.setVisible( json["visible"].get<bool>() );

		if ( json.contains( "auto_close_xml_tags" ) && json["auto_close_xml_tags"].is_boolean() )
			def.setAutoCloseXMLTags( json["auto_close_xml_tags"].get<bool>() );

		if ( json.contains( "extension_priority" ) && json["extension_priority"].is_boolean() )
			def.setExtensionPriority( json["extension_priority"].get<bool>() );

		if ( json.contains( "case_insensitive" ) && json["case_insensitive"].is_boolean() )
			def.setCaseInsensitive( json["case_insensitive"].get<bool>() );

		if ( json.contains( "fold_range_type" ) && json["fold_range_type"].is_string() ) {
			def.setFoldRangeType(
				FoldRangeTypeUtil::fromString( json["fold_range_type"].get<std::string>() ) );
		}

		if ( json.contains( "fold_braces" ) && json["fold_braces"].is_array() ) {
			const auto& foldBraces = json["fold_braces"];
			std::vector<std::pair<Int64, Int64>> folds;

			for ( const auto& fold : foldBraces ) {
				if ( fold.is_object() && fold.contains( "start" ) && fold.contains( "end" ) ) {
					auto start = String::fromUtf8( fold.value( "start", "" ) );
					auto end = String::fromUtf8( fold.value( "end", "" ) );
					if ( !start.empty() && !end.empty() )
						folds.emplace_back( start[0], end[0] );
				}
			}

			def.setFoldBraces( folds );
		}
	} catch ( const json::exception& e ) {
		Log::error( "SyntaxDefinition loadLanguage failed:\n%s", e.what() );
	}

	return def;
}

bool SyntaxDefinitionManager::loadFromStream( IOStream& stream,
											  std::vector<std::string>* addedLangs ) {
	if ( stream.getSize() == 0 )
		return false;
	std::string buffer;
	buffer.resize( stream.getSize() );
	stream.read( buffer.data(), buffer.size() );

	try {
		auto j = nlohmann::json::parse( buffer, nullptr, true, true );

		if ( j.is_array() ) {
			for ( const auto& lang : j ) {
				auto res = loadLanguage( lang );
				if ( !res.getLanguageName().empty() ) {
					auto pos = getLanguageIndex( res.getLanguageName() );
					if ( pos.has_value() ) {
						if ( addedLangs )
							addedLangs->push_back( res.getLanguageName() );
						mDefinitions[pos.value()] = std::move( res );
					} else {
						if ( addedLangs )
							addedLangs->push_back( res.getLanguageName() );
						res.mLanguageIndex = mDefinitions.size();
						mDefinitions.emplace_back( std::move( res ) );
					}
				}
			}
		} else {
			auto res = loadLanguage( j );
			if ( !res.getLanguageName().empty() ) {
				auto pos = getLanguageIndex( res.getLanguageName() );
				if ( pos.has_value() ) {
					if ( addedLangs )
						addedLangs->push_back( res.getLanguageName() );
					mDefinitions[pos.value()] = std::move( res );
				} else {
					if ( addedLangs )
						addedLangs->push_back( res.getLanguageName() );
					res.mLanguageIndex = mDefinitions.size();
					mDefinitions.emplace_back( std::move( res ) );
				}
			}
		}
	} catch ( const nlohmann::json::exception& e ) {
		Log::error( "SyntaxDefinition load failed:\n%s", e.what() );
		return false;
	}

	return true;
}

bool SyntaxDefinitionManager::loadFromStream( IOStream& stream ) {
	return loadFromStream( stream, nullptr );
}

bool SyntaxDefinitionManager::loadFromFile( const std::string& fpath ) {
	if ( FileSystem::fileExists( fpath ) ) {
		IOStreamFile IOS( fpath );

		return loadFromStream( IOS );
	} else if ( PackManager::instance()->isFallbackToPacksActive() ) {
		std::string tgPath( fpath );

		Pack* tPack = PackManager::instance()->exists( tgPath );

		if ( NULL != tPack ) {
			return loadFromPack( tPack, tgPath );
		}
	}
	return false;
}

bool SyntaxDefinitionManager::loadFromMemory( const Uint8* data, const Uint32& dataSize ) {
	IOStreamMemory IOS( (const char*)data, dataSize );
	return loadFromStream( IOS );
}

bool SyntaxDefinitionManager::loadFromPack( Pack* Pack, const std::string& filePackPath ) {
	if ( NULL != Pack && Pack->isOpen() && -1 != Pack->exists( filePackPath ) ) {
		ScopedBuffer buffer;
		Pack->extractFileToMemory( filePackPath, buffer );
		return loadFromMemory( buffer.get(), buffer.length() );
	}
	return false;
}

void SyntaxDefinitionManager::loadFromFolder( const std::string& folderPath ) {
	if ( !FileSystem::isDirectory( folderPath ) )
		return;
	auto files = FileSystem::filesInfoGetInPath( folderPath );
	if ( files.empty() )
		return;
	for ( const auto& file : files ) {
		if ( file.isRegularFile() && file.isReadable() && file.getExtension() == "json" )
			loadFromFile( file.getFilepath() );
	}
}

std::vector<const SyntaxDefinition*>
SyntaxDefinitionManager::languagesThatSupportExtension( std::string extension ) const {
	std::vector<const SyntaxDefinition*> langs;
	if ( extension.empty() )
		return {};

	if ( extension[0] != '.' )
		extension = '.' + extension;

	for ( const auto& definition : mDefinitions ) {
		for ( const auto& ext : definition.getFiles() ) {
			if ( String::startsWith( ext, "%." ) || String::startsWith( ext, "^" ) ||
				 String::endsWith( ext, "$" ) ) {
				LuaPattern words( ext );
				int start, end;
				if ( words.find( extension, start, end ) )
					langs.push_back( &definition );
			} else if ( extension == ext ) {
				langs.push_back( &definition );
			}
		}
	}
	return langs;
}

bool SyntaxDefinitionManager::extensionCanRepresentManyLanguages( std::string extension ) const {
	if ( extension.empty() )
		return false;
	if ( extension[0] != '.' )
		extension = '.' + extension;

	int count = 0;
	for ( const auto& definition : mDefinitions ) {
		for ( const auto& ext : definition.getFiles() ) {
			if ( String::startsWith( ext, "%." ) || String::startsWith( ext, "^" ) ||
				 String::endsWith( ext, "$" ) ) {
				LuaPattern words( ext );
				int start, end;
				if ( words.find( extension, start, end ) ) {
					count++;
					if ( count > 1 )
						return true;
				}
			} else if ( extension == ext ) {
				count++;
				if ( count > 1 )
					return true;
			}
		}
	}
	return false;
}

const SyntaxDefinition& SyntaxDefinitionManager::getByExtension( const std::string& filePath,
																 bool hFileAsCPP ) const {
	std::string extension( FileSystem::fileExtension( filePath ) );
	std::string fileName( FileSystem::fileNameFromPath( filePath ) );

	bool extHasMultipleLangs = extensionCanRepresentManyLanguages( extension );
	auto priorityLanguage = mPriorities.end();
	if ( extHasMultipleLangs ) {
		priorityLanguage = mPriorities.find( extension );
		const SyntaxDefinition* def = nullptr;
		if ( priorityLanguage != mPriorities.end() &&
			 ( def = getPtrByLSPName( priorityLanguage->second ) ) ) {
			return *def;
		}
	}

	// Use the filename instead
	if ( extension.empty() )
		extension = FileSystem::fileNameFromPath( filePath );

	const SyntaxDefinition* def = nullptr;

	if ( !extension.empty() ) {
		for ( const auto& definition : mDefinitions ) {
			if ( &definition == &mDefinitions[0] ) // Ignore Plain text
				continue;

			for ( const auto& ext : definition.getFiles() ) {
				if ( String::startsWith( ext, "%." ) || String::startsWith( ext, "^" ) ||
					 String::endsWith( ext, "$" ) ) {
					LuaPattern words( ext );
					int start, end;
					if ( words.find( fileName, start, end ) ) {
						if ( hFileAsCPP && definition.getLSPName() == "c" && ext == "%.h$" )
							return getByLSPName( "cpp" );

						if ( extHasMultipleLangs && !definition.hasExtensionPriority() ) {
							def = &definition;
							continue;
						}

						return definition;
					}
				} else if ( extension == ext ) {
					if ( hFileAsCPP && definition.getLSPName() == "c" && ext == ".h" )
						return getByLSPName( "cpp" );

					if ( extHasMultipleLangs && !definition.hasExtensionPriority() ) {
						def = &definition;
						continue;
					}

					return definition;
				}
			}
		}
	}

	return def != nullptr ? *def : mDefinitions[0];
}

const SyntaxDefinition& SyntaxDefinitionManager::getByHeader( const std::string& header,
															  bool /*hFileAsCPP*/ ) const {
	if ( !header.empty() ) {
		for ( auto definition = mDefinitions.rbegin(); definition != mDefinitions.rend();
			  ++definition ) {
			for ( const auto& hdr : definition->getHeaders() ) {
				LuaPattern words( hdr );
				int start, end;
				if ( words.find( header, start, end ) ) {
					return *definition;
				}
			}
		}
	}
	return mDefinitions[0];
}

const SyntaxDefinition& SyntaxDefinitionManager::find( const std::string& filePath,
													   const std::string& header,
													   bool hFileAsCPP ) {
	const SyntaxDefinition& def = getByHeader( header );
	if ( def.getLanguageName() == mDefinitions[0].getLanguageName() )
		return getByExtension( filePath, hFileAsCPP );
	return def;
}

const SyntaxDefinition&
SyntaxDefinitionManager::findFromString( const std::string_view& lang ) const {
	const auto& syn = getByLSPName( lang );
	if ( syn.getLSPName() != getPlainDefinition().getLSPName() )
		return syn;
	const auto& syn2 = getByLanguageName( lang );
	if ( syn2.getLSPName() != getPlainDefinition().getLSPName() )
		return syn2;
	const auto& syn3 = getByLanguageNameInsensitive( lang );
	if ( syn3.getLSPName() != getPlainDefinition().getLSPName() )
		return syn3;
	return getPlainDefinition();
}

const std::map<std::string, std::string>& SyntaxDefinitionManager::getLanguageExtensionsPriority() {
	return mPriorities;
}

std::size_t SyntaxDefinitionManager::count() const {
	return mDefinitions.size();
}

void SyntaxDefinitionManager::reserveSpaceForLanguages( std::size_t totalLangs ) {
	mDefinitions.reserve( totalLangs );
}

}}} // namespace EE::UI::Doc
