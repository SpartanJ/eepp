#include <eepp/ui/uitheme.hpp>
#include <eepp/ui/uiskinsimple.hpp>
#include <eepp/ui/uiskincomplex.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/graphics/sprite.hpp>
#include <eepp/graphics/drawable.hpp>
#include <eepp/graphics/textureatlas.hpp>
#include <eepp/graphics/font.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/graphics/textureatlasmanager.hpp>
#include <eepp/graphics/ninepatch.hpp>
#include <eepp/system/filesystem.hpp>

namespace EE { namespace UI {

static std::string elemNameFromSkinSimple( const std::vector<std::string>& nameParts ) {
	std::string str;
	int lPart = (int)nameParts.size() - 1;

	for ( int i = 0; i < lPart; i++ ) {
		str += nameParts[i];

		if ( i != lPart - 1 ) {
			str += "_";
		}
	}

	return str;
}

static std::string elemNameFromSkinComplex( const std::vector<std::string>& nameParts ) {
	std::string str;
	int lPart = (int)nameParts.size() - 2;

	for ( int i = 0; i < lPart; i++ ) {
		str += nameParts[i];

		if ( i != lPart - 1 ) {
			str += "_";
		}
	}

	return str;
}

UITheme * UITheme::New( const std::string & name, const std::string & abbr, Font * defaultFont ) {
	return eeNew( UITheme, ( name, abbr, defaultFont ) );
}

UITheme * UITheme::loadFromTextureAtlas( UITheme * tTheme, Graphics::TextureAtlas * TextureAtlas ) {
	eeASSERT( NULL != tTheme && NULL != TextureAtlas );

	/** Themes use nearest filter by default, force the filter to the textures. */
	for ( Uint32 tC = 0; tC < TextureAtlas->getTexturesCount(); tC++ ) {
		TextureAtlas->getTexture( tC )->setFilter( TEX_FILTER_NEAREST );
	}

	Clock TE;

	tTheme->setTextureAtlas( TextureAtlas );

	std::list<DrawableResource*>& resources = TextureAtlas->getResources();
	std::list<DrawableResource*>::iterator it;
	std::string sAbbr( tTheme->getAbbr() + "_" );
	std::map<std::string, bool> elemFound;

	for ( it = resources.begin(); it != resources.end(); it++ ) {
		DrawableResource* subTexture = *it;

		std::string name( subTexture->getName() );

		if ( String::startsWith( name, sAbbr ) ) {
			std::vector<std::string> nameParts = String::split( name, '_' );

			if ( nameParts.size() >= 3 ) {
				int lPart = nameParts.size() - 1;
				int llPart = nameParts.size() - 2;

				if ( UISkin::isStateName( nameParts[ lPart ] ) ) {
					elemFound[ elemNameFromSkinSimple( nameParts ) ] = false;
				} else if ( UISkin::isStateName( nameParts[ llPart ] ) && UISkinComplex::isSideSuffix( nameParts[ lPart ] ) ) {
					elemFound[ elemNameFromSkinComplex( nameParts ) ] = true;
				}
			}
		}
	}

	for ( std::map<std::string, bool>::iterator it = elemFound.begin(); it != elemFound.end(); it++ ) {
		if ( it->second )
			tTheme->add( UISkinComplex::New( it->first ) );
		else
			tTheme->add( UISkinSimple::New( it->first ) );
	}

	eePRINTL( "UI Theme Loaded in: %4.3f ms ( from TextureAtlas )", TE.getElapsed().asMilliseconds() );

	return tTheme;
}

UITheme * UITheme::loadFromFile( UITheme * tTheme, const std::string& Path ) {
	Clock TE;

	std::string RPath( Path );

	FileSystem::dirPathAddSlashAtEnd( RPath );

	if ( !FileSystem::isDirectory( RPath ) )
		return NULL;

	Graphics::TextureAtlas * tSG = eeNew( Graphics::TextureAtlas, ( tTheme->getAbbr() ) );

	tTheme->setTextureAtlas( tSG );

	std::vector<std::string> resources = FileSystem::filesGetInPath( RPath );
	std::vector<std::string>::iterator it;
	std::string sAbbr( tTheme->getAbbr() + "_" );
	std::string sAbbrIcon( tTheme->getAbbr() + "_icon_" );
	std::map<std::string, bool> elemFound;

	for ( it = resources.begin(); it != resources.end(); it++ ) {
		std::string fpath( RPath + (*it) );
		std::string name( FileSystem::fileRemoveExtension( *it ) );

		if ( !FileSystem::isDirectory( fpath ) ) {
			if ( String::startsWith( name, sAbbrIcon ) ) {
				tSG->add( eeNew( SubTexture, ( TextureFactory::instance()->loadFromFile( fpath ), name ) ) );
			} else if ( String::startsWith( name, sAbbr ) ) {
				std::vector<std::string> dotParts = String::split( name, '.' );

				if ( dotParts.size() >= 3 && dotParts[ dotParts.size() - 1 ] == "9" ) {
					std::vector<std::string> nameParts = String::split( dotParts[0], '_' );

					std::string<std::string> srcRect = String::split( dotParts[ dotParts.size() - 2 ], '_' );
					int l = 0, t = 0, r = 0, b = 0;

					if ( srcRect.size() == 4 ) {
						String::fromString<int>( &l, srcRect[0] );
						String::fromString<int>( &t, srcRect[1] );
						String::fromString<int>( &r, srcRect[2] );
						String::fromString<int>( &b, srcRect[3] );
					}

					elemFound[ elemNameFromSkinSimple( nameParts ) ] = false;

					tSG->add( eeNew( NinePatch, ( TextureFactory::instance()->loadFromFile( fpath ), l, t, r, b, name ) ) );
				} else {
					std::vector<std::string> nameParts = String::split( name, '_' );

					if ( nameParts.size() >= 3 ) {
						int lPart = nameParts.size() - 1;
						int llPart = nameParts.size() - 2;

						if ( UISkin::isStateName( nameParts[ lPart ] ) ) {
							elemFound[ elemNameFromSkinSimple( nameParts ) ] = false;

							tSG->add( eeNew( SubTexture, ( TextureFactory::instance()->loadFromFile( fpath ), name ) ) );
						} else if ( UISkin::isStateName( nameParts[ llPart ] ) && UISkinComplex::isSideSuffix( nameParts[ lPart ] ) ) {
							elemFound[ elemNameFromSkinComplex( nameParts ) ] = true;

							tSG->add( eeNew( SubTexture, ( TextureFactory::instance()->loadFromFile( fpath ), name ) ) );
						}
					}
				}
			}
		}
	}

	if ( tSG->getCount() )
		TextureAtlasManager::instance()->add( tSG );
	else
		eeSAFE_DELETE( tSG );

	for ( std::map<std::string, bool>::iterator it = elemFound.begin(); it != elemFound.end(); it++ ) {
		if ( it->second )
			tTheme->add( UISkinComplex::New( it->first ) );
		else
			tTheme->add( UISkinSimple::New( it->first ) );
	}

	eePRINTL( "UI Theme Loaded in: %4.3f ms ( from path )", TE.getElapsed().asMilliseconds() );

	return tTheme;
}

UITheme * UITheme::loadFromFile( const std::string& Path, const std::string& Name, const std::string& NameAbbr ) {
	return loadFromFile( UITheme::New( Name, NameAbbr ), Path );
}

UITheme * UITheme::loadFromTextureAtlas( Graphics::TextureAtlas * TextureAtlas, const std::string& Name, const std::string NameAbbr ) {
	return loadFromTextureAtlas( UITheme::New( Name, NameAbbr ), TextureAtlas );
}

UITheme::UITheme(const std::string& name, const std::string& Abbr, Graphics::Font * defaultFont ) :
	ResourceManager<UISkin> ( false ),
	mName( name ),
	mNameHash( String::hash( mName ) ),
	mAbbr( Abbr ),
	mTextureAtlas( NULL )
{
	mFontStyleConfig.Font = defaultFont;
	mFontStyleConfig.ShadowColor = Color( 255, 255, 255, 200 );
	mFontStyleConfig.FontColor = mFontStyleConfig.FontOverColor = mFontStyleConfig.FontSelectedColor = Color( 0, 0, 0, 255 );
	mFontStyleConfig.FontSelectionBackColor = Color( 150, 150, 150, 255 );

	if ( NULL == defaultFont ) {
		mFontStyleConfig.Font = UIThemeManager::instance()->getDefaultFont();
	}
}

UITheme::~UITheme() {

}

const std::string& UITheme::getName() const {
	return mName;
}

void UITheme::setName( const std::string& name ) {
	mName = name;
	mNameHash = String::hash( mName );
}

const Uint32& UITheme::getId() const {
	return mNameHash;
}

const std::string& UITheme::getAbbr() const {
	return mAbbr;
}

UISkin * UITheme::add( UISkin * Resource ) {
	Resource->setTheme( this );

	return ResourceManager<UISkin>::add( Resource );
}

Graphics::TextureAtlas * UITheme::getTextureAtlas() const {
	return mTextureAtlas;
}

void UITheme::setTextureAtlas( Graphics::TextureAtlas * SG ) {
	mTextureAtlas = SG;
}

Drawable * UITheme::getIconByName( const std::string& name ) {
	if ( NULL != mTextureAtlas )
		return mTextureAtlas->getByName( mAbbr + "_icon_" + name );

	return NULL;
}

UISkin * UITheme::getSkin(const std::string & controlName) {
	return getByName( mAbbr + "_" + controlName );
}

void UITheme::setFontStyleConfig(const UITooltipStyleConfig & fontConfig) {
	mFontStyleConfig = fontConfig;
}

UITabWidgetStyleConfig UITheme::getTabWidgetStyleConfig() {
	return UITabWidgetStyleConfig( getFontStyleConfig() );
}

UIProgressBarStyleConfig UITheme::getProgressBarStyleConfig() {
	return UIProgressBarStyleConfig( getFontStyleConfig() );;
}

UIWinMenuStyleConfig UITheme::getWinMenuStyleConfig() {
	return UIWinMenuStyleConfig( getFontStyleConfig() );
}

UIDropDownListStyleConfig UITheme::getDropDownListStyleConfig() {
	return UIDropDownListStyleConfig( getFontStyleConfig() );
}

UIWindowStyleConfig UITheme::getWindowStyleConfig() {
	return UIWindowStyleConfig( getFontStyleConfig() );
}

UIMenuStyleConfig UITheme::getMenuStyleConfig() {
	return UIMenuStyleConfig( getFontStyleConfig() );
}

UIPushButtonStyleConfig UITheme::getPushButtonStyleConfig() {
	return UIPushButtonStyleConfig( getFontStyleConfig() );
}

UISliderStyleConfig UITheme::getSliderStyleConfig() {
	return UISliderStyleConfig();
}

UITooltipStyleConfig UITheme::getTooltipStyleConfig() {
	return UITooltipStyleConfig( getFontStyleConfig() );
}

UITooltipStyleConfig UITheme::getFontStyleConfig() const {
	return mFontStyleConfig;
}

}}
