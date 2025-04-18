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
			json pattern;
			auto ptrnType =
				ptrn.matchType == SyntaxPatternMatchType::RegEx
					? "regex"
					: ( ptrn.matchType == SyntaxPatternMatchType::Parser ? "parser" : "pattern" );

			// Do not export injected patterns
			if ( ptrn.matchType == SyntaxPatternMatchType::LuaPattern &&
				 ptrn.patterns.size() == 1 &&
				 ( ptrn.patterns[0] == "%s+" || ptrn.patterns[0] == "%w+%f[%s]" ) )
				continue;

			if ( ptrn.patterns.size() == 1 ) {
				pattern[ptrnType] = ptrn.patterns[0];
			} else {
				pattern[ptrnType] = ptrn.patterns;
			}
			if ( ptrn.typesNames.size() == 1 ) {
				pattern["type"] = ptrn.typesNames[0];
			} else {
				pattern["type"] = ptrn.typesNames;
			}
			if ( !ptrn.syntax.empty() )
				pattern["syntax"] = ptrn.syntax;
			j["patterns"].emplace_back( std::move( pattern ) );
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

static std::string join( std::vector<std::string> const& vec, bool createCont = true,
						 bool allowReduce = false, bool setType = false,
						 std::string delim = ", " ) {
	if ( vec.empty() )
		return "{}";
	if ( vec.size() == 1 && allowReduce )
		return str( vec[0] );
	std::string accum = std::accumulate(
		vec.begin() + 1, vec.end(), str( vec[0] ),
		[&delim]( const std::string& a, const std::string& b ) { return a + delim + str( b ); } );
	return createCont
			   ? ( std::string( setType ? "std::vector<std::string>" : "" ) + "{ " + accum + " }" )
			   : accum;
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
	for ( auto& style : mDefinitions ) {
		if ( style.getLanguageName() == name )
			return style;
	}
	return mDefinitions[0];
}

const SyntaxDefinition& SyntaxDefinitionManager::getByLanguageIndex( const Uint32& index ) const {
	eeASSERT( index < mDefinitions.size() );
	return mDefinitions[index];
}

const SyntaxDefinition&
SyntaxDefinitionManager::getByLanguageNameInsensitive( const std::string_view& name ) const {
	for ( auto& style : mDefinitions ) {
		if ( String::iequals( style.getLanguageName(), name ) )
			return style;
	}
	return mDefinitions[0];
}

const SyntaxDefinition&
SyntaxDefinitionManager::getByLSPName( const std::string_view& name ) const {
	for ( auto& style : mDefinitions ) {
		if ( style.getLSPName() == name )
			return style;
	}
	return mDefinitions[0];
}

const SyntaxDefinition&
SyntaxDefinitionManager::getByLanguageId( const String::HashType& id ) const {
	for ( auto& style : mDefinitions ) {
		if ( style.getLanguageId() == id )
			return style;
	}
	return mDefinitions[0];
}

SyntaxDefinition& SyntaxDefinitionManager::getByLanguageNameRef( const std::string& name ) {
	return const_cast<SyntaxDefinition&>( getByLanguageName( name ) );
}

std::vector<std::string> SyntaxDefinitionManager::getLanguageNames() const {
	std::vector<std::string> names;
	for ( auto& style : mDefinitions ) {
		if ( style.isVisible() )
			names.push_back( style.getLanguageName() );
	}
	std::sort( names.begin(), names.end() );
	return names;
}

std::vector<std::string> SyntaxDefinitionManager::getExtensionsPatternsSupported() const {
	std::vector<std::string> exts;
	exts.reserve( mDefinitions.size() );
	for ( auto& style : mDefinitions )
		for ( auto& pattern : style.getFiles() )
			exts.emplace_back( pattern );
	return exts;
}

const SyntaxDefinition*
SyntaxDefinitionManager::getPtrByLanguageName( const std::string& name ) const {
	for ( const auto& style : mDefinitions ) {
		if ( style.getLanguageName() == name )
			return &style;
	}
	return nullptr;
}

const SyntaxDefinition* SyntaxDefinitionManager::getPtrByLSPName( const std::string& name ) const {
	for ( const auto& style : mDefinitions ) {
		if ( style.getLSPName() == name )
			return &style;
	}
	return nullptr;
}

const SyntaxDefinition*
SyntaxDefinitionManager::getPtrByLanguageId( const String::HashType& id ) const {
	for ( const auto& style : mDefinitions ) {
		if ( style.getLanguageId() == id )
			return &style;
	}
	return nullptr;
}

static SyntaxDefinition loadLanguage( const nlohmann::json& json ) {
	SyntaxDefinition def;
	try {
		def.setLanguageName( json.value( "name", "" ) );
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
		if ( json.contains( "patterns" ) && json["patterns"].is_array() ) {
			const auto& patterns = json["patterns"];
			for ( const auto& pattern : patterns ) {
				std::vector<std::string> type;
				if ( pattern.contains( "type" ) ) {
					if ( pattern["type"].is_array() ) {
						for ( const auto& t : pattern["type"] ) {
							if ( t.is_string() )
								type.push_back( t.get<std::string>() );
						}
					} else if ( pattern["type"].is_string() ) {
						type.push_back( pattern["type"] );
					}
				} else {
					type.push_back( "normal" );
				}
				auto syntax = !pattern.contains( "syntax" ) || !pattern["syntax"].is_string()
								  ? ""
								  : pattern.value( "syntax", "" );
				std::vector<std::string> ptrns;
				auto ctype = SyntaxPatternMatchType::LuaPattern;
				if ( pattern.contains( "pattern" ) ) {
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
				def.addPattern(
					SyntaxPattern( std::move( ptrns ), std::move( type ), syntax, ctype ) );
			}
		}
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
				hds.push_back( headers.get<std::string>() );
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

	nlohmann::json j = nlohmann::json::parse( buffer, nullptr, false, true );

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

	for ( const auto& style : mDefinitions ) {
		for ( const auto& ext : style.getFiles() ) {
			if ( String::startsWith( ext, "%." ) || String::startsWith( ext, "^" ) ||
				 String::endsWith( ext, "$" ) ) {
				LuaPattern words( ext );
				int start, end;
				if ( words.find( extension, start, end ) )
					langs.push_back( &style );
			} else if ( extension == ext ) {
				langs.push_back( &style );
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
	for ( const auto& style : mDefinitions ) {
		for ( const auto& ext : style.getFiles() ) {
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
		for ( const auto& style : mDefinitions ) {
			if ( &style == &mDefinitions[0] ) // Ignore Plain text
				continue;

			for ( const auto& ext : style.getFiles() ) {
				if ( String::startsWith( ext, "%." ) || String::startsWith( ext, "^" ) ||
					 String::endsWith( ext, "$" ) ) {
					LuaPattern words( ext );
					int start, end;
					if ( words.find( fileName, start, end ) ) {
						if ( hFileAsCPP && style.getLSPName() == "c" && ext == "%.h$" )
							return getByLSPName( "cpp" );

						if ( extHasMultipleLangs && !style.hasExtensionPriority() ) {
							def = &style;
							continue;
						}

						return style;
					}
				} else if ( extension == ext ) {
					if ( hFileAsCPP && style.getLSPName() == "c" && ext == ".h" )
						return getByLSPName( "cpp" );

					if ( extHasMultipleLangs && !style.hasExtensionPriority() ) {
						def = &style;
						continue;
					}

					return style;
				}
			}
		}
	}

	return def != nullptr ? *def : mDefinitions[0];
}

const SyntaxDefinition& SyntaxDefinitionManager::getByHeader( const std::string& header,
															  bool /*hFileAsCPP*/ ) const {
	if ( !header.empty() ) {
		for ( auto style = mDefinitions.rbegin(); style != mDefinitions.rend(); ++style ) {
			for ( const auto& hdr : style->getHeaders() ) {
				LuaPattern words( hdr );
				int start, end;
				if ( words.find( header, start, end ) ) {
					return *style;
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
