#ifndef EE_UICUITHEME_HPP
#define EE_UICUITHEME_HPP

#include <eepp/system/resourcemanager.hpp>
#include <eepp/ui/base.hpp>
#include <eepp/ui/css/stylesheet.hpp>
#include <eepp/ui/uifontstyleconfig.hpp>
#include <eepp/ui/uihelper.hpp>
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

class EE_API UITheme : protected ResourceManagerMulti<UISkin> {
  public:
	using ResourceManagerMulti<UISkin>::getById;
	using ResourceManagerMulti<UISkin>::getByName;
	using ResourceManagerMulti<UISkin>::exists;
	using ResourceManagerMulti<UISkin>::existsId;

	static UITheme* New( const std::string& name, const std::string& abbr,
						 Graphics::Font* defaultFont = NULL );

	static UITheme* load( const std::string& name, const std::string& abbr,
						  const std::string& textureAtlasPath, Graphics::Font* defaultFont,
						  const std::string& styleSheetPath );

	static UITheme* loadFromTextureAtlas( UITheme* tTheme,
										  Graphics::TextureAtlas* getTextureAtlas );

	static UITheme* loadFromTextureAtlas( Graphics::TextureAtlas* getTextureAtlas,
										  const std::string& Name, const std::string& NameAbbr );

	static UITheme* loadFromDirectroy( UITheme* tTheme, const std::string& Path,
									   const Float& pixelDensity = 1 );

	static UITheme* loadFromDirectroy( const std::string& Path, const std::string& Name,
									   const std::string& NameAbbr, const Float& pixelDensity = 1 );

	virtual ~UITheme();

	const std::string& getName() const;

	void setName( const std::string& name );

	const String::HashType& getId() const;

	const std::string& getAbbr() const;

	virtual UISkin* add( UISkin* Resource );

	Graphics::TextureAtlas* getTextureAtlas() const;

	UIIcon* getIconByName( const std::string& name );

	UISkin* getSkin( const std::string& widgetName );

	Font* getDefaultFont() const;

	void setDefaultFont( Font* font );

	CSS::StyleSheet& getStyleSheet();

	const CSS::StyleSheet& getStyleSheet() const;

	void setStyleSheet( const CSS::StyleSheet& styleSheet );

	const Float& getDefaultFontSize() const;

	void setDefaultFontSize( const Float& defaultFontSize );

	UIIconTheme* getIconTheme() const;

	const std::string& getStyleSheetPath() const;

	void setStyleSheetPath( const std::string& styleSheetPath );

	bool reloadStyleSheet();

  protected:
	std::string mName;
	String::HashType mNameHash;
	std::string mAbbr;
	Graphics::TextureAtlas* mTextureAtlas;
	Font* mDefaultFont;
	Float mDefaultFontSize;
	CSS::StyleSheet mStyleSheet;
	std::string mStyleSheetPath;
	UIIconTheme* mIconTheme;

	void setTextureAtlas( Graphics::TextureAtlas* SG );

	UITheme( const std::string& name, const std::string& abbr, Graphics::Font* defaultFont = NULL );
};

}} // namespace EE::UI

#endif
