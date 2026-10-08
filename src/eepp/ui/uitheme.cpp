#include <eepp/graphics/drawable.hpp>
#include <eepp/graphics/font.hpp>
#include <eepp/graphics/ninepatch.hpp>
#include <eepp/graphics/sprite.hpp>
#include <eepp/graphics/statelistdrawable.hpp>
#include <eepp/graphics/textureatlas.hpp>
#include <eepp/graphics/textureatlasloader.hpp>
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

UIThemePtr UITheme::New( const std::string& name, const std::string& abbr, Font* defaultFont ) {
	return UIThemePtr( eeNew( UITheme, ( name, abbr, defaultFont ) ), ResourceDeleter<UITheme>() );
}

UIThemePtr UITheme::load( const std::string& name, const std::string& abbr,
						  const std::string& textureAtlasPath, Font* defaultFont,
						  const std::string& styleSheetPath ) {
	UIThemePtr theme = UITheme::New( name, abbr, defaultFont );

	CSS::StyleSheetParser styleSheetParser;

	if ( styleSheetParser.loadFromFile( styleSheetPath ) ) {
		theme->setStyleSheet( styleSheetParser.getStyleSheet() );
		theme->setStyleSheetPath( styleSheetPath );
	}

	if ( textureAtlasPath.empty() )
		return theme;

	ResourceScopePtr resourceScope = ResourceScope::New();
	theme->mResourceCatalog = resourceScope->getLocalCatalog();
	TextureAtlasLoader tgl;
	tgl.setResourceScope( std::move( resourceScope ) );
	tgl.loadFromFile( textureAtlasPath );

	return loadFromTextureAtlas( theme, tgl.getTextureAtlas() );
}

UIThemePtr UITheme::loadFromString( const std::string& name, const std::string& abbr,
									const std::string& textureAtlasPath, Font* defaultFont,
									const std::string& styleSheetString ) {
	UIThemePtr theme = UITheme::New( name, abbr, defaultFont );

	CSS::StyleSheetParser styleSheetParser;

	if ( styleSheetParser.loadFromString( styleSheetString ) ) {
		theme->setStyleSheet( styleSheetParser.getStyleSheet() );
	}

	if ( textureAtlasPath.empty() )
		return theme;

	ResourceScopePtr resourceScope = ResourceScope::New();
	theme->mResourceCatalog = resourceScope->getLocalCatalog();
	TextureAtlasLoader tgl;
	tgl.setResourceScope( std::move( resourceScope ) );
	tgl.loadFromFile( textureAtlasPath );

	return loadFromTextureAtlas( theme, tgl.getTextureAtlas() );
}

UIThemePtr UITheme::loadFromTextureAtlas( UIThemePtr tTheme, TextureAtlasPtr textureAtlas ) {
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
		TextureRegion* textureRegion = it.second.get();

		std::string name( textureRegion->getName() );

		if ( String::startsWith( name, sAbbrIcon ) ) {
			auto icon = UIIcon::New( name.substr( sAbbrIcon.size() ) );
			icon->setSource( textureRegion->getPixelsSize().getWidth(), textureRegion->clone() );
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
					String::fromString( l, srcRect[0] );
					String::fromString( t, srcRect[1] );
					String::fromString( r, srcRect[2] );
					String::fromString( b, srcRect[3] );
				}

				std::string skinName( elemNameFromSkin( nameParts ) );

				NinePatchPtr drawable = NinePatch::New( textureRegion, l, t, r, b, realName );
				tTheme->getResourceCatalog()->publishDrawable( realName, drawable );

				if ( skins.find( skinName ) == skins.end() )
					skins[skinName] = tTheme->add( UISkin::New( skinName ) );

				int stateNum = UIState::getStateNumber( nameParts[nameParts.size() - 1] );

				if ( -1 != stateNum )
					skins[skinName]->setStateDrawable( stateNum, drawable->clone() );
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
							skins[skinName]->setStateDrawable( stateNum, textureRegion->clone() );
					}
				}
			}
		}
	}

	Log::info( "UI Theme Loaded in: %4.3f ms ( from TextureAtlas )",
			   TE.getElapsedTimeAndReset().asMilliseconds() );

	return tTheme;
}

UIThemePtr UITheme::loadFromDirectory( UIThemePtr tTheme, const std::string& Path,
									   const Float& pixelDensity ) {
	Clock TE;

	std::string RPath( Path );

	FileSystem::dirAddSlashAtEnd( RPath );

	if ( !FileSystem::isDirectory( RPath ) )
		return NULL;

	TextureAtlasPtr tSG = TextureAtlas::New( tTheme->getAbbr() );

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
				auto drawable =
					TextureRegion::New( TextureFactory::instance()->loadFromFile( fpath ), name );
				tSG->add( drawable );
				auto icon = UIIcon::New( name.substr( sAbbrIcon.size() ) );
				icon->setSource( drawable->getPixelsSize().getWidth(), drawable->clone() );
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
						String::fromString( l, srcRect[0] );
						String::fromString( t, srcRect[1] );
						String::fromString( r, srcRect[2] );
						String::fromString( b, srcRect[3] );
					}

					std::string skinName( elemNameFromSkin( nameParts ) );

					NinePatchPtr drawable =
						NinePatch::New( TextureFactory::instance()->loadFromFile( fpath ), l, t, r,
										b, pixelDensity, realName );
					tTheme->getResourceCatalog()->publishDrawable( realName, drawable );

					if ( skins.find( skinName ) == skins.end() )
						skins[skinName] = tTheme->add( UISkin::New( skinName ) );

					int stateNum = UIState::getStateNumber( nameParts[nameParts.size() - 1] );

					if ( -1 != stateNum )
						skins[skinName]->setStateDrawable( stateNum, drawable->clone() );
				} else {
					std::vector<std::string> nameParts = String::split( name, '_' );

					if ( nameParts.size() >= 3 ) {
						int lPart = nameParts.size() - 1;

						if ( UIState::isStateName( nameParts[lPart] ) ) {
							TextureRegionPtr textureRegion = tSG->add( TextureRegion::New(
								TextureFactory::instance()->loadFromFile( fpath ), name ) );

							std::string skinName( elemNameFromSkin( nameParts ) );
							int stateNum = UIState::getStateNumber( nameParts[lPart] );

							if ( skins.find( skinName ) == skins.end() )
								skins[skinName] = tTheme->add( UISkin::New( skinName ) );

							if ( -1 != stateNum )
								skins[skinName]->setStateDrawable( stateNum,
																   textureRegion->clone() );
						}
					}
				}
			}
		}
	}

	if ( tSG->getCount() )
		tTheme->setTextureAtlas( tSG );
	else
		tTheme->setTextureAtlas( {} );

	Log::info( "UI Theme Loaded in: %4.3f ms ( from path )",
			   TE.getElapsedTimeAndReset().asMilliseconds() );

	return tTheme;
}

UIThemePtr UITheme::loadFromDirectory( const std::string& Path, const std::string& Name,
									   const std::string& NameAbbr, const Float& pixelDensity ) {
	return loadFromDirectory( UITheme::New( Name, NameAbbr ), Path, pixelDensity );
}

UIThemePtr UITheme::loadFromTextureAtlas( TextureAtlasPtr textureAtlas, const std::string& Name,
										  const std::string& NameAbbr ) {
	return loadFromTextureAtlas( UITheme::New( Name, NameAbbr ), std::move( textureAtlas ) );
}

UITheme::UITheme( const std::string& name, const std::string& Abbr, Graphics::Font* defaultFont ) :
	mName( name ),
	mNameHash( String::hash( mName ) ),
	mAbbr( Abbr ),
	mTextureAtlas(),
	mDefaultFont( defaultFont ),
	mDefaultFontSize( PixelDensity::dpToPx( PixelDensity::getPixelDensity() > 1.4 ? 11 : 12 ) ),
	mIconTheme( UIIconTheme::New( name ) ),
	mResourceCatalog( ResourceCatalog::New() ) {}

UITheme::~UITheme() = default;

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

UISkin* UITheme::add( UISkinPtr skin ) {
	if ( !skin )
		return nullptr;
	UISkin* result = skin.get();
	mSkins[skin->getId()].emplace_back( std::move( skin ) );
	return result;
}

UISkin* UITheme::getById( const String::HashType& id ) const {
	auto it = mSkins.find( id );
	return it != mSkins.end() && !it->second.empty() ? it->second.front().get() : nullptr;
}

UISkin* UITheme::getByName( const std::string& name ) const {
	return getById( String::hash( name ) );
}

bool UITheme::exists( const std::string& name ) const {
	return existsId( String::hash( name ) );
}

bool UITheme::existsId( const String::HashType& id ) const {
	return mSkins.find( id ) != mSkins.end();
}

Graphics::TextureAtlas* UITheme::getTextureAtlas() const {
	return mTextureAtlas.get();
}

void UITheme::setTextureAtlas( TextureAtlasPtr textureAtlas ) {
	mTextureAtlas = std::move( textureAtlas );
	if ( !mTextureAtlas )
		return;
	mResourceCatalog->publishAtlas( mTextureAtlas->getName(), mTextureAtlas );
	for ( const auto& resource : mTextureAtlas->getResources() )
		mResourceCatalog->publishDrawable( resource.second->getName(), resource.second );
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

void UITheme::setStyleSheet( CSS::StyleSheet&& styleSheet ) {
	mStyleSheet = std::move( styleSheet );
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
	return mIconTheme.get();
}

const ResourceCatalogPtr& UITheme::getResourceCatalog() const {
	return mResourceCatalog;
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
