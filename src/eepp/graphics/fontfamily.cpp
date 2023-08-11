#include <eepp/graphics/fontfamily.hpp>
#include <eepp/system/filesystem.hpp>

namespace EE { namespace Graphics {

void FontFamily::loadFromRegular( FontTrueType* font ) {
	if ( font == nullptr || font->getInfo().fontpath.empty() || font->getInfo().filename.empty() )
		return;

	std::string ext( FileSystem::fileExtension( font->getInfo().filename ) );
	if ( ext.empty() )
		return;

	std::string fontname( FileSystem::fileRemoveExtension( font->getInfo().filename ) );
	if ( String::endsWith( fontname, "-Regular" ) ) {
		auto pos( fontname.find_last_of( '-' ) );
		fontname.resize( pos );
	}

	setFont( font, findType( font->getInfo().fontpath, fontname, ext, { "Bold", "bold" } ),
			 "bold" );

	setFont( font,
			 findType( font->getInfo().fontpath, fontname, ext,
					   { "Italic", "Oblique", "italic", "oblique", "It", "it" } ),
			 "italic" );

	setFont( font,
			 findType( font->getInfo().fontpath, fontname, ext, { "BoldItalic", "BoldOblique" } ),
			 "bolditalic" );
}

std::string FontFamily::findType( const std::string& fontpath, const std::string& fontname,
								  const std::string& ext, const std::vector<std::string>& names ) {
	std::string path;
	for ( const auto& name : names ) {
		path = fontpath + fontname + "-" + name + "." + ext;
		if ( FileSystem::fileExists( path ) )
			return path;
	}
	return "";
}

FontTrueType* FontFamily::setFont( FontTrueType* font, const std::string& fontpath,
								   const std::string& fontType ) {
	if ( fontpath.empty() )
		return nullptr;
	FontTrueType* loadedFont = FontTrueType::New( font->getName() + "-" + fontType, fontpath );
	if ( fontType == "bold" )
		font->setBoldFont( loadedFont );
	else if ( fontType == "italic" )
		font->setItalicFont( loadedFont );
	else if ( fontType == "bolditalic" )
		font->setBoldItalicFont( loadedFont );
	loadedFont->setBoldAdvanceSameAsRegular( font->getBoldAdvanceSameAsRegular() );
	loadedFont->setEnableDynamicMonospace( font->getEnableDynamicMonospace() );
	return loadedFont;
}

}} // namespace EE::Graphics
