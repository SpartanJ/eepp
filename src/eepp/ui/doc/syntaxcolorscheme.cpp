#include <eepp/core/string.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/functionstring.hpp>
#include <eepp/system/inifile.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/iostreammemory.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/packmanager.hpp>
#include <eepp/ui/doc/syntaxcolorscheme.hpp>
#include <eepp/ui/doc/syntaxdefinition.hpp>

using namespace EE::Graphics;

namespace EE { namespace UI { namespace Doc {

// Color schemes are compatible with the lite (https://github.com/rxi/lite) color schemes.
// But I also added:
// "link" (link style)
// "gutter_background" (the gutter background color)
// "whitespace" (the whitespace color)
// "line_break_column" (the right margin line column color)
// "matching_bracket" (the background color drawn in the matching brackets)
// "matching_selection" (the background color drawn in the text matching the current selected text)
// "matching_search" (the background color drawn in the text matching the current searched text)
// "suggestion" (the auto-complete suggestion box text and background color
// "suggestion_selected" (the auto-complete selected suggestion box text and background color
// "selection_region" (The background color of the region you select a region to find/replace text)
// "error" (error underline color)
// "warning" (warning underline color)
// "notice" (notice underline color)
// "minimap_background" (Minimap background)
// "minimap_current_line" (Minimap current cursor line background)
// "minimap_hover" (Minimap mouse hover color)
// "minimap_selection" (Minimap text selection color)
// "minimap_highlight" (Minimap text highlight color)
// "minimap_visible_area" (Minimap visible area marker color)

SyntaxColorScheme SyntaxColorScheme::getDefault() {
	return {
		"eepp",
		{
			{ "normal"_sst, Color( "#e1e1e6" ) },
			{ "symbol"_sst, Color( "#e1e1e6" ) },
			{ "comment"_sst, Color( "#cd8b00" ) },
			{ "keyword"_sst, { Color( "#ff79c6" ), Color::Transparent, Text::Shadow } },
			{ "keyword2"_sst, { Color( "#8be9fd" ), Color::Transparent, Text::Shadow } },
			{ "keyword3"_sst, { Color( "#ffb86c" ), Color::Transparent, Text::Shadow } },
			{ "number"_sst, Color( "#ffd24a" ) },
			{ "literal"_sst, { Color( "#f1fa8c" ), Color::Transparent, Text::Shadow } },
			{ "string"_sst, Color( "#ffcd8b" ) },
			{ "operator"_sst, Color( "#51f0e7" ) },
			{ "function"_sst, { Color( "#00dc7f" ), Color::Transparent, Text::Shadow } },
			{ "link"_sst, { Color( "#6ae0f9" ), Color::Transparent, Text::Shadow } },
			{ "link_hover"_sst, { Color::Transparent, Color::Transparent, Text::Underlined } },
		},
		{ { "background"_sst, Color( "#282a36" ) },
		  { "text"_sst, Color( "#e1e1e6" ) },
		  { "caret"_sst, Color( "#93DDFA" ) },
		  { "selection"_sst, Color( "#394484" ) },
		  { "line_highlight"_sst, Color( "#2d303d" ) },
		  { "line_number"_sst, Color( "#525259" ) },
		  { "line_number2"_sst, Color( "#83838f" ) },
		  // eepp colors
		  { "gutter_background"_sst, Color( "#282a36" ) },
		  { "whitespace"_sst, Color( "#394484" ) },
		  { "line_break_column"_sst, Color( "#54575b99" ) },
		  { "matching_bracket"_sst, Color( "#FFFFFF33" ) },
		  { "matching_selection"_sst, Color( "#3e596e" ) },
		  { "matching_search"_sst, Color( "#181b1e" ) },
		  { "suggestion"_sst, { Color( "#e1e1e6" ), Color( "#1d1f27" ), Text::Regular } },
		  { "suggestion_scrollbar"_sst, { Color( "#3daee9" ) } },
		  { "suggestion_selected"_sst, { Color( "#ffffff" ), Color( "#2f3240" ), Text::Regular } },
		  { "error"_sst, { Color::Red } },
		  { "warning"_sst, { Color::Yellow } },
		  { "notice"_sst, Color( "#8abdff" ) },
		  { "selection_region"_sst, Color( "#39448477" ) },
		  // minimap colors
		  { "minimap_background"_sst, Color( "#282a36AA" ) },
		  { "minimap_current_line"_sst, Color( "#93DDFA40" ) },
		  { "minimap_hover"_sst, Color( "#FFFFFF1A" ) },
		  { "minimap_selection"_sst, Color( "#8abdff80" ) },
		  { "minimap_highlight"_sst, Color( "#FFFF0040" ) },
		  { "minimap_visible_area"_sst, Color( "#FFFFFF0A" ) } } };
}

SyntaxColorScheme::Style parseStyle(
	const std::string& value, bool* colorWasSet = nullptr,
	const UnorderedMap<SyntaxStyleType, SyntaxColorScheme::Style>* syntaxColors = nullptr ) {
	static const std::string outline = "outline";
	auto values = String::split( value, ",", "", "()" );
	SyntaxColorScheme::Style style;
	bool colorSet = false;
	for ( auto& val : values ) {
		String::toLowerInPlace( val );
		String::trimInPlace( val );
		if ( Color::isColorString( val ) ) {
			if ( !colorSet ) {
				style.color = Color::fromString( val );
				colorSet = true;
			} else {
				style.background = Color::fromString( val );
			}
		} else {
			if ( "regular" == val )
				style.style |= Text::Regular;
			else if ( "bold" == val )
				style.style |= Text::Bold;
			else if ( "italic" == val )
				style.style |= Text::Italic;
			else if ( "underline" == val || "underlined" == val )
				style.style |= Text::Underlined;
			else if ( "strikethrough" == val )
				style.style |= Text::StrikeThrough;
			else if ( "shadow" == val )
				style.style |= Text::Shadow;
			else if ( String::startsWith( val, outline ) ) {
				auto res = FunctionString::parse( val );
				if ( !res.isEmpty() ) {
					for ( const auto& param : res.getParameters() ) {
						if ( Color::isColorString( param ) ) {
							style.outlineColor = Color::fromString( param );
						} else if ( String::isNumber( param, true ) ) {
							Float f;
							if ( String::fromString( f, param ) )
								style.outlineThickness = f;
						}
					}
				}
			} else if ( syntaxColors &&
						( "normal" == val || "symbol" == val || "comment" == val ||
						  "keyword" == val || "keyword2" == val || "number" == val ||
						  "literal" == val || "string" == val || "opetaror" == val ||
						  "function" == val || "link" == val || "link_hover" == val ) ) {
				auto styleIt = syntaxColors->find( toSyntaxStyleType( val ) );
				if ( styleIt != syntaxColors->end() ) {
					style = styleIt->second;
					colorSet = true;
				}
			}
		}
	}
	if ( colorWasSet )
		*colorWasSet = colorSet;
	return style;
}

std::vector<SyntaxColorScheme> SyntaxColorScheme::loadFromStream( IOStream& stream ) {
	Clock clock;
	std::vector<SyntaxColorScheme> colorSchemes;
	SyntaxColorScheme refColorScheme( getDefault() );
	IniFile ini( stream );
	for ( size_t keyIdx = 0; keyIdx < ini.getNumKeys(); keyIdx++ ) {
		SyntaxColorScheme colorScheme;
		std::string name( ini.getKeyName( keyIdx ) );
		colorScheme.setName( name );
		size_t numValues = ini.getNumValues( keyIdx );
		for ( size_t valueIdx = 0; valueIdx < numValues; valueIdx++ ) {
			std::string valueName( String::toLower( ini.getValueName( keyIdx, valueIdx ) ) );
			std::string value( ini.getValue( keyIdx, valueIdx ) );
			if ( !value.empty() ) {
				SyntaxColorScheme::Style style = parseStyle( value );
				if ( refColorScheme.mSyntaxColors.find( toSyntaxStyleType( valueName ) ) !=
					 refColorScheme.mSyntaxColors.end() ) {
					colorScheme.setSyntaxStyle( toSyntaxStyleType( valueName ), style );
				} else if ( refColorScheme.mEditorColors.find( toSyntaxStyleType( valueName ) ) !=
							refColorScheme.mEditorColors.end() ) {
					colorScheme.setEditorSyntaxStyle( toSyntaxStyleType( valueName ), style );
				}
			}
		}
		colorSchemes.emplace_back( colorScheme );
	}
	Log::info( "Color Schemes loaded in %.2fms.", clock.getElapsedTime().asMilliseconds() );
	return colorSchemes;
}

std::vector<SyntaxColorScheme> SyntaxColorScheme::loadFromFile( const std::string& path ) {
	if ( !FileSystem::fileExists( path ) && PackManager::instance()->isFallbackToPacksActive() ) {
		std::string pathFix( path );
		Pack* pack = PackManager::instance()->exists( pathFix );
		if ( NULL != pack ) {
			return loadFromPack( pack, pathFix );
		}
		return {};
	}
	IOStreamFile stream( path );
	return loadFromStream( stream );
}

std::vector<SyntaxColorScheme> SyntaxColorScheme::loadFromMemory( const void* data,
																  std::size_t sizeInBytes ) {
	IOStreamMemory stream( (const char*)data, sizeInBytes );
	return loadFromStream( stream );
}

std::vector<SyntaxColorScheme> SyntaxColorScheme::loadFromPack( Pack* pack,
																std::string filePackPath ) {
	if ( NULL == pack )
		return {};
	ScopedBuffer buffer;
	if ( pack->isOpen() && pack->extractFileToMemory( filePackPath, buffer ) ) {
		return loadFromMemory( buffer.get(), buffer.length() );
	}
	return {};
}

SyntaxColorScheme::SyntaxColorScheme() {}

SyntaxColorScheme::SyntaxColorScheme( const std::string& name,
									  const UnorderedMap<SyntaxStyleType, Style>& syntaxColors,
									  const UnorderedMap<SyntaxStyleType, Style>& editorColors ) :
	mName( name ), mSyntaxColors( syntaxColors ), mEditorColors( editorColors ) {}

static const SyntaxColorScheme::Style StyleEmpty = { Color::Transparent };
static const SyntaxColorScheme StyleDefault = SyntaxColorScheme::getDefault();

const SyntaxColorScheme::Style&
SyntaxColorScheme::getSyntaxStyle( const SyntaxStyleType& type ) const {
	auto it = mSyntaxColors.find( type );
	if ( it != mSyntaxColors.end() )
		return it->second;
	else if ( type == "keyword3"_sst )
		return getSyntaxStyle( "symbol"_sst );
	else if ( type == "link"_sst || type == "link_hover"_sst )
		return getSyntaxStyle( "function"_sst );
	else if ( type == "error"_sst || type == "warning"_sst || type == "notice"_sst )
		return getEditorSyntaxStyle( type );
	else {
		auto foundIt = mStyleCache.find( type );
		if ( foundIt != mStyleCache.end() )
			return foundIt->second;
		return getSyntaxStyleFromCache<SyntaxStyleType>( type );
	}
	return StyleEmpty;
}

bool SyntaxColorScheme::hasSyntaxStyle( const SyntaxStyleType& type ) const {
	return mSyntaxColors.find( type ) != mSyntaxColors.end();
}

void SyntaxColorScheme::setSyntaxStyles( const UnorderedMap<SyntaxStyleType, Style>& styles ) {
	mSyntaxColors.insert( styles.begin(), styles.end() );
}

void SyntaxColorScheme::setSyntaxStyle( const SyntaxStyleType& type,
										const SyntaxColorScheme::Style& style ) {
	mSyntaxColors[type] = style;
}

const SyntaxColorScheme::Style&
SyntaxColorScheme::getEditorSyntaxStyle( const SyntaxStyleType& type ) const {
	auto it = mEditorColors.find( type );
	if ( it != mEditorColors.end() )
		return it->second;
	if ( type == "gutter_background"_sst || type == "minimap_background"_sst )
		return getEditorSyntaxStyle( "background"_sst );
	else if ( type == "whitespace"_sst || type == "line_break_column"_sst ||
			  type == "matching_bracket"_sst || type == "matching_selection"_sst ||
			  type == "selection_region"_sst )
		return getEditorSyntaxStyle( "selection"_sst );
	else if ( type == "suggestion"_sst )
		return StyleDefault.getEditorSyntaxStyle( "suggestion"_sst );
	else if ( type == "suggestion_selected"_sst )
		return StyleDefault.getEditorSyntaxStyle( "suggestion_selected"_sst );
	else if ( type == "error"_sst )
		return StyleDefault.getEditorSyntaxStyle( "error"_sst );
	else if ( type == "warning"_sst )
		return StyleDefault.getEditorSyntaxStyle( "warning"_sst );
	else if ( type == "notice"_sst )
		return StyleDefault.getEditorSyntaxStyle( "notice"_sst );
	else if ( type == "minimap_current_line"_sst )
		return StyleDefault.getEditorSyntaxStyle( "minimap_current_line"_sst );
	else if ( type == "minimap_hover"_sst )
		return StyleDefault.getEditorSyntaxStyle( "minimap_hover"_sst );
	else if ( type == "minimap_selection"_sst )
		return StyleDefault.getEditorSyntaxStyle( "minimap_selection"_sst );
	else if ( type == "minimap_highlight"_sst )
		return StyleDefault.getEditorSyntaxStyle( "minimap_highlight"_sst );
	else if ( type == "minimap_visible_area"_sst )
		return StyleDefault.getEditorSyntaxStyle( "minimap_visible_area"_sst );
	else if ( type == "suggestion_scrollbar"_sst )
		return getEditorSyntaxStyle( "line_highlight"_sst );
	return StyleEmpty;
}

const Color& SyntaxColorScheme::getEditorColor( const SyntaxStyleType& type ) const {
	return getEditorSyntaxStyle( type ).color;
}

void SyntaxColorScheme::setEditorSyntaxStyles(
	const EE::UnorderedMap<SyntaxStyleType, SyntaxColorScheme::Style>& styles ) {
	mEditorColors.insert( styles.begin(), styles.end() );
}

void SyntaxColorScheme::setEditorSyntaxStyle( const SyntaxStyleType& type,
											  const SyntaxColorScheme::Style& style ) {
	mEditorColors[type] = style;
}

const std::string& SyntaxColorScheme::getName() const {
	return mName;
}

void SyntaxColorScheme::setName( const std::string& name ) {
	mName = name;
}

template <typename SyntaxStyleType>
const SyntaxColorScheme::Style&
SyntaxColorScheme::getSyntaxStyleFromCache( const SyntaxStyleType& type ) const {
	bool colorWasSet = false;
	if constexpr ( std::is_same_v<SyntaxStyleType, std::string> )
		mStyleCache[type] = parseStyle( type, &colorWasSet, &mSyntaxColors );
	else {
		auto cache = SyntaxPattern::SyntaxStyleTypeCache.find( type );
		if ( cache != SyntaxPattern::SyntaxStyleTypeCache.end() ) {
			mStyleCache[type] = parseStyle( cache->second, &colorWasSet, &mSyntaxColors );
		} else {
			return StyleEmpty;
		}
	}
	if ( !colorWasSet ) {
		auto normalStyle = mSyntaxColors.find( "normal"_sst );
		if ( normalStyle != mSyntaxColors.end() )
			mStyleCache[type].color = normalStyle->second.color;
	}
	return mStyleCache[type];
}

}}} // namespace EE::UI::Doc
