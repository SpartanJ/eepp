#ifndef EE_UICUITHEME_HPP
#define EE_UICUITHEME_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/uithemeconfig.hpp>
#include <eepp/ui/uihelper.hpp>
#include <eepp/ui/uiskin.hpp>
#include <eepp/system/resourcemanager.hpp>

namespace EE { namespace Graphics {
class Sprite;
class TextureAtlas;
class Font;
}}

namespace EE { namespace UI {

class UIControl;
class UICheckBox;
class UIComboBox;
class UIDropDownList;
class UIListBox;
class UIPopUpMenu;
class UIProgressBar;
class UIPushButton;
class UISelectButton;
class UIRadioButton;
class UIScrollBar;
class UISlider;
class UISpinBox;
class UITextView;
class UITextEdit;
class UITextInput;
class UITextInputPassword;
class UITooltip;
class UIWindow;
class UIWinMenu;
class UIImage;
class UISprite;
class UIMenu;
class UICommonDialog;
class UIMessageBox;
class UITabWidget;

class EE_API UITheme : protected ResourceManager<UISkin> {
	public:
		using ResourceManager<UISkin>::getById;
		using ResourceManager<UISkin>::getByName;
		using ResourceManager<UISkin>::exists;
		using ResourceManager<UISkin>::existsId;

		static UITheme * loadFromTextureAtlas( UITheme * tTheme, Graphics::TextureAtlas * getTextureAtlas );

		static UITheme * loadFromFile( UITheme * tTheme, const std::string& Path, const std::string ImgExt = "png" );

		static UITheme * loadFromTextureAtlas( Graphics::TextureAtlas * getTextureAtlas, const std::string& Name, const std::string NameAbbr );

		static UITheme * loadFromFile( const std::string& Path, const std::string& Name, const std::string& NameAbbr, const std::string ImgExt = "png" );

		void addThemeElement( const std::string& Element );

		void addThemeIcon( const std::string& Icon );

		UITheme( const std::string& name, const std::string& abbr, Graphics::Font * defaultFont = NULL );

		virtual ~UITheme();

		const std::string& getName() const;

		void setName( const std::string& name );

		const Uint32& getId() const;

		const std::string& getAbbr() const;

		virtual UISkin * add( UISkin * Resource );

		Graphics::TextureAtlas * getTextureAtlas() const;

		SubTexture * getIconByName( const std::string& name );

		TooltipStyleConfig getFontStyleConfig() const;

		void setFontStyleConfig(const TooltipStyleConfig & fontConfig);

		virtual TabWidgetStyleConfig getTabWidgetStyleConfig();

		virtual ProgressBarStyleConfig getProgressBarStyleConfig();

		virtual WinMenuStyleConfig getWinMenuStyleConfig();

		virtual DropDownListStyleConfig getDropDownListStyleConfig();

		virtual WindowStyleConfig getWindowStyleConfig();

		virtual MenuStyleConfig getMenuStyleConfig();

		virtual PushButtonStyleConfig getPushButtonStyleConfig();

		virtual SliderStyleConfig getSliderStyleConfig();

		virtual TooltipStyleConfig getTooltipStyleConfig();
	protected:
		std::string				mName;
		Uint32					mNameHash;
		std::string				mAbbr;
		Graphics::TextureAtlas *mTextureAtlas;
		TooltipStyleConfig			mFontStyleConfig;
		std::list<std::string>	mUIElements;
		std::list<std::string>	mUIIcons;

		void getTextureAtlas( Graphics::TextureAtlas * SG );

		static bool searchFilesOfElement( Graphics::TextureAtlas * SG, const std::string& Path, std::string Element, Uint32& IsComplex, const std::string ImgExt );

		static bool searchFilesInAtlas( Graphics::TextureAtlas * SG, std::string Element, Uint32& IsComplex );
};

}}

#endif
