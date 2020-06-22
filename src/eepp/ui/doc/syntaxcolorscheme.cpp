#include <eepp/core/string.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/inifile.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/iostreammemory.hpp>
#include <eepp/system/packmanager.hpp>
#include <eepp/ui/doc/syntaxcolorscheme.hpp>

namespace EE { namespace UI { namespace Doc {

// Color schemes are compatible with the lite (https://github.com/rxi/lite) color schemes.
// But I also added:
// "line_number_background" (the gutter background color)
// "whitespace" (the whitespace color)
// "line_break_column" (the right margin line column color)
// "matching_bracket" (the background color drawn in the matching brackets)
// "matching_selection" (the background color drawn in the text matching the current selected text)

SyntaxColorScheme SyntaxColorScheme::getDefault() {
	return {"lite-theme",
			{
				{"normal", Color( "#e1e1e6" )},
				{"symbol", Color( "#e1e1e6" )},
				{"comment", Color( "#676b6f" )},
				{"keyword", Color( "#E58AC9" )},
				{"keyword2", Color( "#F77483" )},
				{"number", Color( "#FFA94D" )},
				{"literal", Color( "#FFA94D" )},
				{"string", Color( "#f7c95c" )},
				{"operator", Color( "#93DDFA" )},
				{"function", Color( "#93DDFA" )},
			},
			{
				{"background", Color( "#2e2e32" )},
				{"text", Color( "#e1e1e6" )},
				{"caret", Color( "#93DDFA" )},
				{"selection", Color( "#48484f" )},
				{"line_highlight", Color( "#343438" )},
				{"line_number", Color( "#525259" )},
				{"line_number2", Color( "#83838f" )},
				// eepp colors
				{"line_number_background", Color( "#2e2e32" )},
				{"whitespace", Color( "#54575b" )},
				{"line_break_column", Color( "#54575b99" )},
				{"matching_bracket", Color( "#FFFFFF33" )},
				{"matching_selection", Color( "#FFFFFF33" )},
			}};
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
				for ( auto& val : values ) {
					String::toLowerInPlace( val );
					String::trimInPlace( val );
					if ( Color::isColorString( val ) ) {
						style.color = Color::fromString( val );
					} else {
						if ( "bold" == val )
							style.style |= Graphics::Text::Bold;
						else if ( "italic" == val )
							style.style |= Graphics::Text::Italic;
						else if ( "underline" == val || "underlined" == val )
							style.style |= Graphics::Text::Underlined;
						else if ( "strikethrough" == val )
							style.style |= Graphics::Text::StrikeThrough;
						else if ( "shadow" == val )
							style.style |= Graphics::Text::Shadow;
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
	eePRINTL( "Color Schemes loaded in %.2fms.", clock.getElapsedTime().asMilliseconds() );
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

static const SyntaxColorScheme::Style StyleEmpty = {Color::White};

const SyntaxColorScheme::Style& SyntaxColorScheme::getSyntaxStyle( const std::string& type ) const {
	auto it = mSyntaxColors.find( type );
	if ( it != mSyntaxColors.end() )
		return it->second;
	return StyleEmpty;
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
	if ( type == "line_number_background" )
		return getEditorSyntaxStyle( "background" );
	else if ( type == "guide" || type == "line_break_column" || type == "matching_bracket" ||
			  type == "matching_selection" )
		return getEditorSyntaxStyle( "selection" );
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
