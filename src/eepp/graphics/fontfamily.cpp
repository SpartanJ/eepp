#include <eepp/graphics/fontfamily.hpp>
#include <eepp/system/filesystem.hpp>

using namespace std::literals;

namespace EE { namespace Graphics {

void FontFamily::loadFromRegular( FontTrueType* font, std::string overwriteFontName ) {
	if ( font == nullptr || font->getInfo().fontpath.empty() || font->getInfo().filename.empty() )
		return;

	std::string ext( FileSystem::fileExtension( font->getInfo().filename ) );
	if ( ext.empty() )
		return;

	std::string fontname( overwriteFontName.empty()
							  ? FileSystem::fileRemoveExtension( font->getInfo().filename )
							  : overwriteFontName );
	if ( String::endsWith( fontname, "-Regular" ) || String::endsWith( fontname, "-regular" ) ) {
		auto pos( fontname.find_last_of( '-' ) );
		fontname.resize( pos );
	} else if ( String::endsWith( fontname, "Rg" ) ) {
		fontname.resize( fontname.size() - 2 );
	}

	setFont( font,
			 findType( font->getInfo().fontpath, fontname, ext, { "Bold"sv, "bold"sv, "Bd"sv } ),
			 "bold"sv );

	setFont( font,
			 findType( font->getInfo().fontpath, fontname, ext,
					   { "Italic"sv, "Oblique"sv, "italic"sv, "oblique"sv, "It"sv, "it"sv } ),
			 "italic"sv );

	setFont(
		font,
		findType( font->getInfo().fontpath, fontname, ext, { "BoldItalic"sv, "BoldOblique"sv } ),
		"bolditalic"sv );
}

std::string FontFamily::findType( const std::string& fontpath, const std::string& fontname,
								  const std::string& ext,
								  const std::vector<std::string_view>& names ) {
	std::string path;
	for ( const auto& name : names ) {
		path = fontpath + fontname + "-" + name + "." + ext;
		if ( FileSystem::fileExists( path ) )
			return path;
		path = fontpath + fontname + name + "." + ext;
		if ( FileSystem::fileExists( path ) )
			return path;
	}
	return "";
}

FontTrueType* FontFamily::setFont( FontTrueType* font, const std::string& fontpath,
								   const std::string_view& fontType ) {
	if ( fontpath.empty() )
		return nullptr;
	FontTrueType* loadedFont = FontTrueType::New( font->getName() + "-" + fontType, fontpath );
	if ( fontType == "bold"sv )
		font->setBoldFont( loadedFont );
	else if ( fontType == "italic"sv )
		font->setItalicFont( loadedFont );
	else if ( fontType == "bolditalic"sv )
		font->setBoldItalicFont( loadedFont );
	loadedFont->setBoldAdvanceSameAsRegular( font->getBoldAdvanceSameAsRegular() );
	loadedFont->setEnableDynamicMonospace( font->getEnableDynamicMonospace() );
	return loadedFont;
}

}} // namespace EE::Graphics
