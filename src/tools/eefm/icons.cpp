#include "icons.hpp"
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/graphics/textureregion.hpp>
#include <eepp/ui/iconmanager.hpp>
#include <eepp/ui/uiicontheme.hpp>
#include <eepp/ui/uiiconthememanager.hpp>

using namespace EE;
using namespace EE::Graphics;
using namespace EE::UI;

namespace eefm {

namespace {

struct GlyphIcon {
	const char* name;
	Uint32 codePoint;
};

// Code points of the fonts shipped in bin/assets/fonts (glyph names in comments).
constexpr GlyphIcon RemixIcons[] = {
	{ "go-previous", 0xea60 },		 // arrow-left-line
	{ "go-next", 0xea6c },			 // arrow-right-line
	{ "go-parent", 0xea76 },		 // arrow-up-line
	{ "view-refresh", 0xf064 },		 // refresh-line
	{ "view-split", 0xee8d },		 // layout-column-line
	{ "places-panel", 0xf45c },		 // sidebar-fold-line
	{ "place-home", 0xee1d },		 // home-4-line
	{ "place-trash", 0xec2a },		 // delete-bin-line
	{ "place-desktop", 0xebca },	 // computer-line
	{ "place-documents", 0xed0f },	 // file-text-line
	{ "place-downloads", 0xec5a },	 // download-line
	{ "place-pictures", 0xee4b },	 // image-line
	{ "place-music", 0xef85 },		 // music-line
	{ "place-videos", 0xef81 },		 // movie-line
	{ "place-folder", 0xed6a },		 // folder-line
	{ "place-remote", 0xf0e0 },		 // server-line
	{ "place-network", 0xedcf },	 // global-line
	{ "device-drive", 0xedf9 },		 // hard-drive-2-line
	{ "device-root", 0xedfb },		 // hard-drive-line
	{ "device-usb", 0xf252 },		 // usb-line
	{ "device-phone", 0xf15a },		 // smartphone-line
	{ "mime-folder", 0xed6a },		 // folder-line
	{ "mime-generic", 0xeceb },		 // file-line
	{ "mime-text", 0xed0f },		 // file-text-line
	{ "mime-markdown", 0xef1e },	 // markdown-line
	{ "mime-code", 0xecd1 },		 // file-code-line
	{ "mime-pdf", 0xecfd },			 // file-pdf-line
	{ "mime-spreadsheet", 0xecdd },	 // file-excel-2-line
	{ "mime-document", 0xed1b },	 // file-word-2-line
	{ "mime-presentation", 0xecff }, // file-ppt-2-line
	{ "mime-image", 0xf3c5 },		 // file-image-line
	{ "mime-video", 0xf3c9 },		 // file-video-line
	{ "mime-audio", 0xecf7 },		 // file-music-line
	{ "mime-archive", 0xed1f },		 // file-zip-line
	{ "mime-executable", 0xf1f6 },	 // terminal-box-line
	{ "action-open-with", 0xecaf },	 // external-link-line
	{ "action-restore", 0xf080 },	 // restart-line
	{ "action-properties", 0xee59 }, // information-line
	{ "action-add-place", 0xf18b },	 // star-line
	{ "action-terminal", 0xf1f6 },	 // terminal-box-line
	{ "location-clear", 0xec1a },	 // delete-back-2-line
	{ "location-history", 0xea4e },	 // arrow-down-s-line
};

constexpr GlyphIcon CodIcons[] = {
	{ "action-new-tab", 0xea60 },		// add
	{ "action-close", 0xea76 },			// close
	{ "action-accept", 0xeab2 },		// check
	{ "app-menu", 0xeb94 },				// menu
	{ "filter-lock", 0xea75 },			// lock
	{ "filter-unlock", 0xeb74 },		// unlock
	{ "filter-case", 0xeab1 },			// case-sensitive
	{ "action-copy-location", 0xeb15 }, // link
};

FontTrueType* loadFont( UISceneNode* ui, const std::string& name, const std::string& path ) {
	FontTrueType* font = FontTrueType::New( name, path, *ui->getResourceScope() ).get();
	if ( font && font->loaded() )
		font->setIsEmojiFont( true );
	return font;
}

} // namespace

void Icons::load( UISceneNode* ui, const std::string& assetsPath,
				  const std::string& windowIconPath ) {
	FontTrueType* remix = loadFont( ui, "icon", assetsPath + "fonts/remixicon.ttf" );
	FontTrueType* nonicons = loadFont( ui, "nonicons", assetsPath + "fonts/nonicons.ttf" );
	FontTrueType* codicons = loadFont( ui, "codicon", assetsPath + "fonts/codicon.ttf" );
	auto theme = IconManager::init( "eefm", remix, nonicons, codicons );
	if ( remix && remix->loaded() ) {
		for ( const auto& icon : RemixIcons )
			theme->add( UIGlyphIcon::New( icon.name, remix, icon.codePoint ) );
	}
	if ( codicons && codicons->loaded() ) {
		for ( const auto& icon : CodIcons )
			theme->add( UIGlyphIcon::New( icon.name, codicons, icon.codePoint ) );
	}
	// The scene owns the same logo used for the native window. Upload once at startup;
	// context menus borrow the cached drawable instead of decoding the PNG on each open.
	if ( auto texture = TextureFactory::instance()->loadFromFile( windowIconPath, true ) ) {
		const int size = PixelDensity::dpToPxI( 16 );
		const Rect source( 0, 0, texture->getImageWidth(), texture->getImageHeight() );
		auto drawable = TextureRegion::New( std::move( texture ), source, Sizef( size, size ) );
		drawable->setPixelDensity( PixelDensity::getPixelDensity() );
		auto appIcon = UIIcon::New( "eefm" );
		appIcon->setSource( size, std::move( drawable ) );
		theme->add( std::move( appIcon ) );
	}
	ui->getUIIconThemeManager()->setCurrentTheme( theme );
}

} // namespace eefm
