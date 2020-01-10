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

class EE_API UITheme : protected ResourceManager<UISkin> {
  public:
	using ResourceManager<UISkin>::getById;
	using ResourceManager<UISkin>::getByName;
	using ResourceManager<UISkin>::exists;
	using ResourceManager<UISkin>::existsId;

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

	UITheme( const std::string& name, const std::string& abbr, Graphics::Font* defaultFont = NULL );

	virtual ~UITheme();

	const std::string& getName() const;

	void setName( const std::string& name );

	const Uint32& getId() const;

	const std::string& getAbbr() const;

	virtual UISkin* add( UISkin* Resource );

	Graphics::TextureAtlas* getTextureAtlas() const;

	EE::Graphics::Drawable* getIconByName( const std::string& name );

	UISkin* getSkin( const std::string& controlName );

	Font* getDefaultFont() const;

	void setDefaultFont( Font* font );

	const CSS::StyleSheet& getStyleSheet() const;

	void setStyleSheet( const CSS::StyleSheet& styleSheet );

  protected:
	std::string mName;
	Uint32 mNameHash;
	std::string mAbbr;
	Graphics::TextureAtlas* mTextureAtlas;
	Font* mDefaultFont;
	CSS::StyleSheet mStyleSheet;

	void setTextureAtlas( Graphics::TextureAtlas* SG );
};

}} // namespace EE::UI

#endif
