#include <eepp/core/string.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/inifile.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/iostreammemory.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/packmanager.hpp>
#include <eepp/ui/doc/syntaxcolorscheme.hpp>

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
	return { "eepp",
			 {
				 { "normal", Color( "#e1e1e6" ) },
				 { "symbol", Color( "#e1e1e6" ) },
				 { "comment", Color( "#cd8b00" ) },
				 { "keyword", { Color( "#ff79c6" ), Color::Transparent, Text::Shadow } },
				 { "keyword2", { Color( "#8be9fd" ), Color::Transparent, Text::Shadow } },
				 { "number", Color( "#ffd24a" ) },
				 { "literal", { Color( "#f1fa8c" ), Color::Transparent, Text::Shadow } },
				 { "string", Color( "#ffcd8b" ) },
				 { "operator", Color( "#51f0e7" ) },
				 { "function", { Color( "#00dc7f" ), Color::Transparent, Text::Shadow } },
				 { "link", { Color( "#6ae0f9" ), Color::Transparent, Text::Shadow } },
				 { "link_hover", { Color::Transparent, Color::Transparent, Text::Underlined } },
			 },
			 { { "background", Color( "#282a36" ) },
			   { "text", Color( "#e1e1e6" ) },
			   { "caret", Color( "#93DDFA" ) },
			   { "selection", Color( "#394484" ) },
			   { "line_highlight", Color( "#2d303d" ) },
			   { "line_number", Color( "#525259" ) },
			   { "line_number2", Color( "#83838f" ) },
			   // eepp colors
			   { "gutter_background", Color( "#282a36" ) },
			   { "whitespace", Color( "#394484" ) },
			   { "line_break_column", Color( "#54575b99" ) },
			   { "matching_bracket", Color( "#FFFFFF33" ) },
			   { "matching_selection", Color( "#3e596e" ) },
			   { "matching_search", Color( "#181b1e" ) },
			   { "suggestion", { Color( "#e1e1e6" ), Color( "#1d1f27" ), Text::Regular } },
			   { "suggestion_selected", { Color( "#ffffff" ), Color( "#2f3240" ), Text::Regular } },
			   { "error", { Color::Red } },
			   { "warning", { Color::Yellow } },
			   { "notice", Color( "#8abdff" ) },
			   { "selection_region", Color( "#39448477" ) },
			   // minimap colors
			   { "minimap_background", Color( "#282a36AA" ) },
			   { "minimap_current_line", Color( "#93DDFA40" ) },
			   { "minimap_hover", Color( "#FFFFFF1A" ) },
			   { "minimap_selection", Color( "#8abdff80" ) },
			   { "minimap_highlight", Color( "#FFFF0040" ) },
			   { "minimap_visible_area", Color( "#FFFFFF0A" ) } } };
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
				auto values = String::split( value, ',' );
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
					}

					if ( refColorScheme.mSyntaxColors.find( valueName ) !=
						 refColorScheme.mSyntaxColors.end() ) {
						colorScheme.setSyntaxStyle( valueName, style );
					} else if ( refColorScheme.mEditorColors.find( valueName ) !=
								refColorScheme.mEditorColors.end() ) {
						colorScheme.setEditorSyntaxStyle( valueName, style );
					}
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
									  const std::unordered_map<std::string, Style>& syntaxColors,
									  const std::unordered_map<std::string, Style>& editorColors ) :
	mName( name ), mSyntaxColors( syntaxColors ), mEditorColors( editorColors ) {}

static const SyntaxColorScheme::Style StyleEmpty = { Color::Transparent };
static const SyntaxColorScheme StyleDefault = SyntaxColorScheme::getDefault();

const SyntaxColorScheme::Style& SyntaxColorScheme::getSyntaxStyle( const std::string& type ) const {
	auto it = mSyntaxColors.find( type );
	if ( it != mSyntaxColors.end() )
		return it->second;
	else if ( type == "link" || type == "link_hover" )
		return getSyntaxStyle( "function" );
	return StyleEmpty;
}

bool SyntaxColorScheme::hasSyntaxStyle( const std::string& type ) const {
	return mSyntaxColors.find( type ) != mSyntaxColors.end();
}

void SyntaxColorScheme::setSyntaxStyles( const std::unordered_map<std::string, Style>& styles ) {
	mSyntaxColors.insert( styles.begin(), styles.end() );
}

void SyntaxColorScheme::setSyntaxStyle( const std::string& type,
										const SyntaxColorScheme::Style& style ) {
	mSyntaxColors[type] = style;
}

const SyntaxColorScheme::Style&
SyntaxColorScheme::getEditorSyntaxStyle( const std::string& type ) const {
	auto it = mEditorColors.find( type );
	if ( it != mEditorColors.end() )
		return it->second;
	if ( type == "gutter_background" || "minimap_background" )
		return getEditorSyntaxStyle( "background" );
	else if ( type == "whitespace" || type == "line_break_column" || type == "matching_bracket" ||
			  type == "matching_selection" || type == "selection_region" )
		return getEditorSyntaxStyle( "selection" );
	else if ( type == "suggestion" )
		return StyleDefault.getEditorSyntaxStyle( "suggestion" );
	else if ( type == "suggestion_selected" )
		return StyleDefault.getEditorSyntaxStyle( "suggestion_selected" );
	else if ( type == "error" )
		return StyleDefault.getEditorSyntaxStyle( "error" );
	else if ( type == "warning" )
		return StyleDefault.getEditorSyntaxStyle( "warning" );
	else if ( type == "notice" )
		return StyleDefault.getEditorSyntaxStyle( "notice" );
	else if ( type == "minimap_current_line" )
		return StyleDefault.getEditorSyntaxStyle( "minimap_current_line" );
	else if ( type == "minimap_hover" )
		return StyleDefault.getEditorSyntaxStyle( "minimap_hover" );
	else if ( type == "minimap_selection" )
		return StyleDefault.getEditorSyntaxStyle( "minimap_selection" );
	else if ( type == "minimap_highlight" )
		return StyleDefault.getEditorSyntaxStyle( "minimap_highlight" );
	else if ( type == "minimap_visible_area" )
		return StyleDefault.getEditorSyntaxStyle( "minimap_visible_area" );
	return StyleEmpty;
}

const Color& SyntaxColorScheme::getEditorColor( const std::string& type ) const {
	return getEditorSyntaxStyle( type ).color;
}

void SyntaxColorScheme::setEditorSyntaxStyles(
	const std::unordered_map<std::string, Style>& styles ) {
	mEditorColors.insert( styles.begin(), styles.end() );
}

void SyntaxColorScheme::setEditorSyntaxStyle( const std::string& type,
											  const SyntaxColorScheme::Style& style ) {
	mEditorColors[type] = style;
}

const std::string& SyntaxColorScheme::getName() const {
	return mName;
}

void SyntaxColorScheme::setName( const std::string& name ) {
	mName = name;
}

}}} // namespace EE::UI::Doc
