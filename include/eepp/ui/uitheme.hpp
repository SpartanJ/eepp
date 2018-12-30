#ifndef EE_UICUITHEME_HPP
#define EE_UICUITHEME_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/uithemeconfig.hpp>
#include <eepp/ui/uihelper.hpp>
#include <eepp/ui/uiskin.hpp>
#include <eepp/ui/css/stylesheet.hpp>
#include <eepp/system/resourcemanager.hpp>

namespace EE { namespace Graphics {
class Sprite;
class TextureAtlas;
class Font;
class Drawable;
}}

namespace EE { namespace UI {

class EE_API UITheme : protected ResourceManager<UISkin> {
	public:
		using ResourceManager<UISkin>::getById;
		using ResourceManager<UISkin>::getByName;
		using ResourceManager<UISkin>::exists;
		using ResourceManager<UISkin>::existsId;

		static UITheme * New( const std::string& name, const std::string& abbr, Graphics::Font * defaultFont = NULL );

		static UITheme * loadFromTextureAtlas( UITheme * tTheme, Graphics::TextureAtlas * getTextureAtlas );

		static UITheme * loadFromTextureAtlas( Graphics::TextureAtlas * getTextureAtlas, const std::string& Name, const std::string& NameAbbr );

		static UITheme * loadFromDirectroy( UITheme * tTheme, const std::string& Path, const Float& pixelDensity = 1 );

		static UITheme * loadFromDirectroy( const std::string& Path, const std::string& Name, const std::string& NameAbbr, const Float& pixelDensity = 1 );

		UITheme( const std::string& name, const std::string& abbr, Graphics::Font * defaultFont = NULL );

		virtual ~UITheme();

		const std::string& getName() const;

		void setName( const std::string& name );

		const Uint32& getId() const;

		const std::string& getAbbr() const;

		virtual UISkin * add( UISkin * Resource );

		Graphics::TextureAtlas * getTextureAtlas() const;

		EE::Graphics::Drawable * getIconByName( const std::string& name );

		UISkin * getSkin( const std::string& controlName );

		UIFontStyleConfig getFontStyleConfig() const;

		Font * getDefaultFont() const;

		void setFontStyleConfig(UIFontStyleConfig fontConfig);

		virtual UIDropDownListStyleConfig getDropDownListStyleConfig();

		virtual UIWindowStyleConfig getWindowStyleConfig();

		virtual UIPushButtonStyleConfig getPushButtonStyleConfig();

		virtual UITooltipStyleConfig getTooltipStyleConfig();

		const CSS::StyleSheet& getStyleSheet() const;

		void setStyleSheet(const CSS::StyleSheet & styleSheet);
	protected:
		std::string				mName;
		Uint32					mNameHash;
		std::string				mAbbr;
		Graphics::TextureAtlas *mTextureAtlas;
		UIFontStyleConfig		mFontStyleConfig;
		CSS::StyleSheet			mStyleSheet;

		void setTextureAtlas( Graphics::TextureAtlas * SG );
};

}}

#endif
