#include <eepp/graphics/drawable.hpp>
#include <eepp/graphics/font.hpp>
#include <eepp/graphics/ninepatch.hpp>
#include <eepp/graphics/ninepatchmanager.hpp>
#include <eepp/graphics/sprite.hpp>
#include <eepp/graphics/statelistdrawable.hpp>
#include <eepp/graphics/textureatlas.hpp>
#include <eepp/graphics/textureatlasloader.hpp>
#include <eepp/graphics/textureatlasmanager.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/log.hpp>
#include <eepp/ui/css/stylesheetparser.hpp>
#include <eepp/ui/uiicontheme.hpp>
#include <eepp/ui/uistate.hpp>
#include <eepp/ui/uitheme.hpp>
#include <eepp/ui/uithememanager.hpp>

namespace EE { namespace UI {

static std::string elemNameFromSkin( const std::vector<std::string>& nameParts ) {
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

UITheme* UITheme::New( const std::string& name, const std::string& abbr, Font* defaultFont ) {
	return eeNew( UITheme, ( name, abbr, defaultFont ) );
}

UITheme* UITheme::load( const std::string& name, const std::string& abbr,
						const std::string& textureAtlasPath, Font* defaultFont,
						const std::string& styleSheetPath ) {
	UITheme* theme = UITheme::New( name, abbr, defaultFont );

	CSS::StyleSheetParser styleSheetParser;

	if ( styleSheetParser.loadFromFile( styleSheetPath ) ) {
		theme->setStyleSheet( styleSheetParser.getStyleSheet() );
		theme->setStyleSheetPath( styleSheetPath );
	}

	if ( textureAtlasPath.empty() )
		return theme;

	TextureAtlasLoader tgl( textureAtlasPath );

	return loadFromTextureAtlas( theme, tgl.getTextureAtlas() );
}

UITheme* UITheme::loadFromTextureAtlas( UITheme* tTheme, Graphics::TextureAtlas* textureAtlas ) {
	eeASSERT( NULL != tTheme && NULL != textureAtlas );

	/** Themes use nearest filter by default, force the filter to the textures. */
	for ( Uint32 tC = 0; tC < textureAtlas->getTexturesCount(); tC++ ) {
		textureAtlas->getTexture( tC )->setFilter( Texture::Filter::Nearest );
	}

	Clock TE;

	tTheme->setTextureAtlas( textureAtlas );

	auto& resources = textureAtlas->getResources();
	std::string sAbbr( tTheme->getAbbr() + "_" );
	std::string sAbbrIcon( tTheme->getAbbr() + "_icon_" );
	std::map<std::string, UISkin*> skins;

	for ( auto& it : resources ) {
		TextureRegion* textureRegion = it.second;

		std::string name( textureRegion->getName() );

		if ( String::startsWith( name, sAbbrIcon ) ) {
			auto* icon = UIIcon::New( name.substr( sAbbrIcon.size() ) );
			icon->setSize( textureRegion->getPixelsSize().getWidth(), textureRegion );
			tTheme->getIconTheme()->add( icon );
		} else if ( String::startsWith( name, sAbbr ) ) {
			std::vector<std::string> dotParts = String::split( name, '.' );

			if ( dotParts.size() >= 3 && dotParts[dotParts.size() - 1] == "9" ) {
				std::string realName;

				for ( size_t i = 0; i < dotParts.size() - 2; i++ ) {
					realName += dotParts[i];

					if ( i != dotParts.size() - 3 ) {
						realName += ".";
					}
				}

				std::vector<std::string> nameParts = String::split( realName, '_' );

				std::vector<std::string> srcRect =
					String::split( dotParts[dotParts.size() - 2], '_' );
				int l = 0, t = 0, r = 0, b = 0;

				if ( srcRect.size() == 4 ) {
					String::fromString<int>( l, srcRect[0] );
					String::fromString<int>( t, srcRect[1] );
					String::fromString<int>( r, srcRect[2] );
					String::fromString<int>( b, srcRect[3] );
				}

				std::string skinName( elemNameFromSkin( nameParts ) );

				Drawable* drawable = NinePatchManager::instance()->add(
					NinePatch::New( textureRegion, l, t, r, b, realName ) );

				if ( skins.find( skinName ) == skins.end() )
					skins[skinName] = tTheme->add( UISkin::New( skinName ) );

				int stateNum = UIState::getStateNumber( nameParts[nameParts.size() - 1] );

				if ( -1 != stateNum )
					skins[skinName]->setStateDrawable( stateNum, drawable );
			} else {
				std::vector<std::string> nameParts = String::split( name, '_' );

				if ( nameParts.size() >= 3 ) {
					int lPart = nameParts.size() - 1;

					if ( UIState::isStateName( nameParts[lPart] ) ) {
						std::string skinName( elemNameFromSkin( nameParts ) );
						int stateNum = UIState::getStateNumber( nameParts[lPart] );

						if ( skins.find( skinName ) == skins.end() )
							skins[skinName] = tTheme->add( UISkin::New( skinName ) );

						if ( -1 != stateNum )
							skins[skinName]->setStateDrawable( stateNum, textureRegion );
					}
				}
			}
		}
	}

	Log::info( "UI Theme Loaded in: %4.3f ms ( from TextureAtlas )",
			   TE.getElapsedTimeAndReset().asMilliseconds() );

	return tTheme;
}

UITheme* UITheme::loadFromDirectroy( UITheme* tTheme, const std::string& Path,
									 const Float& pixelDensity ) {
	Clock TE;

	std::string RPath( Path );

	FileSystem::dirAddSlashAtEnd( RPath );

	if ( !FileSystem::isDirectory( RPath ) )
		return NULL;

	Graphics::TextureAtlas* tSG = Graphics::TextureAtlas::New( tTheme->getAbbr() );

	tTheme->setTextureAtlas( tSG );

	std::vector<std::string> resources = FileSystem::filesGetInPath( RPath );
	std::vector<std::string>::iterator it;
	std::string sAbbr( tTheme->getAbbr() + "_" );
	std::string sAbbrIcon( tTheme->getAbbr() + "_icon_" );
	std::map<std::string, UISkin*> skins;

	for ( it = resources.begin(); it != resources.end(); ++it ) {
		std::string fpath( RPath + ( *it ) );
		std::string name( FileSystem::fileRemoveExtension( *it ) );

		if ( !FileSystem::isDirectory( fpath ) ) {
			if ( String::startsWith( name, sAbbrIcon ) ) {
				auto* drawable =
					TextureRegion::New( TextureFactory::instance()->loadFromFile( fpath ), name );
				tSG->add( drawable );
				auto* icon = UIIcon::New( name.substr( sAbbrIcon.size() ) );
				icon->setSize( drawable->getPixelsSize().getWidth(), drawable );
				tTheme->getIconTheme()->add( icon );
			} else if ( String::startsWith( name, sAbbr ) ) {
				std::vector<std::string> dotParts = String::split( name, '.' );

				if ( dotParts.size() >= 3 && dotParts[dotParts.size() - 1] == "9" ) {
					std::string realName;

					for ( size_t i = 0; i < dotParts.size() - 2; i++ ) {
						realName += dotParts[i];

						if ( i != dotParts.size() - 3 ) {
							realName += ".";
						}
					}

					std::vector<std::string> nameParts = String::split( realName, '_' );

					std::vector<std::string> srcRect =
						String::split( dotParts[dotParts.size() - 2], '_' );
					int l = 0, t = 0, r = 0, b = 0;

					if ( srcRect.size() == 4 ) {
						String::fromString<int>( l, srcRect[0] );
						String::fromString<int>( t, srcRect[1] );
						String::fromString<int>( r, srcRect[2] );
						String::fromString<int>( b, srcRect[3] );
					}

					std::string skinName( elemNameFromSkin( nameParts ) );

					Drawable* drawable = NinePatchManager::instance()->add(
						NinePatch::New( TextureFactory::instance()->loadFromFile( fpath ), l, t, r,
										b, pixelDensity, realName ) );

					if ( skins.find( skinName ) == skins.end() )
						skins[skinName] = tTheme->add( UISkin::New( skinName ) );

					int stateNum = UIState::getStateNumber( nameParts[nameParts.size() - 1] );

					if ( -1 != stateNum )
						skins[skinName]->setStateDrawable( stateNum, drawable );
				} else {
					std::vector<std::string> nameParts = String::split( name, '_' );

					if ( nameParts.size() >= 3 ) {
						int lPart = nameParts.size() - 1;

						if ( UIState::isStateName( nameParts[lPart] ) ) {
							TextureRegion* textureRegion = tSG->add( TextureRegion::New(
								TextureFactory::instance()->loadFromFile( fpath ), name ) );

							std::string skinName( elemNameFromSkin( nameParts ) );
							int stateNum = UIState::getStateNumber( nameParts[lPart] );

							if ( skins.find( skinName ) == skins.end() )
								skins[skinName] = tTheme->add( UISkin::New( skinName ) );

							if ( -1 != stateNum )
								skins[skinName]->setStateDrawable( stateNum, textureRegion );
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

	Log::info( "UI Theme Loaded in: %4.3f ms ( from path )",
			   TE.getElapsedTimeAndReset().asMilliseconds() );

	return tTheme;
}

UITheme* UITheme::loadFromDirectroy( const std::string& Path, const std::string& Name,
									 const std::string& NameAbbr, const Float& pixelDensity ) {
	return loadFromDirectroy( UITheme::New( Name, NameAbbr ), Path, pixelDensity );
}

UITheme* UITheme::loadFromTextureAtlas( Graphics::TextureAtlas* TextureAtlas,
										const std::string& Name, const std::string& NameAbbr ) {
	return loadFromTextureAtlas( UITheme::New( Name, NameAbbr ), TextureAtlas );
}

UITheme::UITheme( const std::string& name, const std::string& Abbr, Graphics::Font* defaultFont ) :
	ResourceManagerMulti<UISkin>(),
	mName( name ),
	mNameHash( String::hash( mName ) ),
	mAbbr( Abbr ),
	mTextureAtlas( NULL ),
	mDefaultFont( defaultFont ),
	mDefaultFontSize( PixelDensity::dpToPx( PixelDensity::getPixelDensity() > 1.4 ? 11 : 12 ) ),
	mIconTheme( UIIconTheme::New( name ) ) {}

UITheme::~UITheme() {
	eeSAFE_DELETE( mIconTheme );
}

const std::string& UITheme::getName() const {
	return mName;
}

void UITheme::setName( const std::string& name ) {
	mName = name;
	mNameHash = String::hash( mName );
}

const String::HashType& UITheme::getId() const {
	return mNameHash;
}

const std::string& UITheme::getAbbr() const {
	return mAbbr;
}

UISkin* UITheme::add( UISkin* Resource ) {
	return ResourceManagerMulti<UISkin>::add( Resource );
}

Graphics::TextureAtlas* UITheme::getTextureAtlas() const {
	return mTextureAtlas;
}

void UITheme::setTextureAtlas( Graphics::TextureAtlas* SG ) {
	mTextureAtlas = SG;
}

UIIcon* UITheme::getIconByName( const std::string& name ) {
	return mIconTheme->getIcon( name );
}

UISkin* UITheme::getSkin( const std::string& widgetName ) {
	return getByName( mAbbr + "_" + widgetName );
}

CSS::StyleSheet& UITheme::getStyleSheet() {
	return mStyleSheet;
}

const CSS::StyleSheet& UITheme::getStyleSheet() const {
	return mStyleSheet;
}

void UITheme::setStyleSheet( const CSS::StyleSheet& styleSheet ) {
	mStyleSheet = styleSheet;
}

const Float& UITheme::getDefaultFontSize() const {
	return mDefaultFontSize;
}

void UITheme::setDefaultFontSize( const Float& defaultFontSize ) {
	mDefaultFontSize = defaultFontSize;
}

UIIconTheme* UITheme::getIconTheme() const {
	return mIconTheme;
}

const std::string& UITheme::getStyleSheetPath() const {
	return mStyleSheetPath;
}

void UITheme::setStyleSheetPath( const std::string& styleSheetPath ) {
	mStyleSheetPath = styleSheetPath;
}

bool UITheme::reloadStyleSheet() {
	if ( mStyleSheetPath.empty() )
		return false;

	CSS::StyleSheetParser styleSheetParser;

	if ( styleSheetParser.loadFromFile( mStyleSheetPath ) ) {
		setStyleSheet( styleSheetParser.getStyleSheet() );
		return true;
	}

	return false;
}

Font* UITheme::getDefaultFont() const {
	return mDefaultFont;
}

void UITheme::setDefaultFont( Font* font ) {
	mDefaultFont = font;
}

}} // namespace EE::UI
