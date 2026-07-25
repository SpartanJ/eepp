#ifndef EE_UICUITHEME_HPP
#define EE_UICUITHEME_HPP

#include <eepp/graphics/resourcecatalog.hpp>
#include <eepp/ui/base.hpp>
#include <eepp/ui/css/stylesheet.hpp>
#include <eepp/ui/uifontstyleconfig.hpp>
#include <eepp/ui/uihelper.hpp>
#include <eepp/ui/uiicontheme.hpp>
#include <eepp/ui/uiskin.hpp>

namespace EE { namespace Graphics {
class Sprite;
class TextureAtlas;
class Font;
class Drawable;
}} // namespace EE::Graphics

namespace EE { namespace UI {

class UIIcon;
class UIIconTheme;

class UITheme;
using UIThemePtr = ResourcePtr<UITheme>;
using UIThemeWeakPtr = ResourceWeakPtr<UITheme>;
using UISkinPtr = ResourcePtr<UISkin>;

class EE_API UITheme {
  public:
	static UIThemePtr New( const std::string& name, const std::string& abbr,
						   Graphics::Font* defaultFont = NULL );

	static UIThemePtr load( const std::string& name, const std::string& abbr,
							const std::string& textureAtlasPath, Graphics::Font* defaultFont,
							const std::string& styleSheetPath );

	static UIThemePtr loadFromString( const std::string& name, const std::string& abbr,
									  const std::string& textureAtlasPath,
									  Graphics::Font* defaultFont,
									  const std::string& styleSheetString );

	static UIThemePtr loadFromTextureAtlas( UIThemePtr theme,
											Graphics::TextureAtlasPtr textureAtlas );

	static UIThemePtr loadFromTextureAtlas( Graphics::TextureAtlasPtr textureAtlas,
											const std::string& Name, const std::string& NameAbbr );

	static UIThemePtr loadFromDirectory( UIThemePtr theme, const std::string& Path,
										 const Float& pixelDensity = 1 );

	static UIThemePtr loadFromDirectory( const std::string& Path, const std::string& Name,
										 const std::string& NameAbbr,
										 const Float& pixelDensity = 1 );

	virtual ~UITheme();

	const std::string& getName() const;

	void setName( const std::string& name );

	const String::HashType& getId() const;

	const std::string& getAbbr() const;

	UISkin* add( UISkinPtr skin );

	UISkin* getById( const String::HashType& id ) const;

	UISkin* getByName( const std::string& name ) const;

	bool exists( const std::string& name ) const;

	bool existsId( const String::HashType& id ) const;

	Graphics::TextureAtlas* getTextureAtlas() const;

	UIIcon* getIconByName( const std::string& name );

	UISkin* getSkin( const std::string& widgetName );

	Font* getDefaultFont() const;

	void setDefaultFont( Font* font );

	CSS::StyleSheet& getStyleSheet();

	const CSS::StyleSheet& getStyleSheet() const;

	void setStyleSheet( CSS::StyleSheet&& styleSheet );

	void setStyleSheet( const CSS::StyleSheet& styleSheet );

	const Float& getDefaultFontSize() const;

	void setDefaultFontSize( const Float& defaultFontSize );

	UIIconTheme* getIconTheme() const;

	const Graphics::ResourceCatalogPtr& getResourceCatalog() const;

	const std::string& getStyleSheetPath() const;

	void setStyleSheetPath( const std::string& styleSheetPath );

	bool reloadStyleSheet();

  protected:
	std::string mName;
	String::HashType mNameHash;
	std::string mAbbr;
	Graphics::TextureAtlasPtr mTextureAtlas;
	Font* mDefaultFont;
	Float mDefaultFontSize;
	CSS::StyleSheet mStyleSheet;
	std::string mStyleSheetPath;
	UIIconThemePtr mIconTheme;
	UnorderedMap<String::HashType, std::vector<UISkinPtr>> mSkins;
	Graphics::ResourceCatalogPtr mResourceCatalog;

	void setTextureAtlas( Graphics::TextureAtlasPtr textureAtlas );

	UITheme( const std::string& name, const std::string& abbr, Graphics::Font* defaultFont = NULL );
};

}} // namespace EE::UI

#endif
