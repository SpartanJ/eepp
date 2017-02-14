#ifndef EE_UICUITHEME_HPP
#define EE_UICUITHEME_HPP

#include <eepp/ui/base.hpp>
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
class UITextBox;
class UITextEdit;
class UITextInput;
class UITextInputPassword;
class UITooltip;
class UIWindow;
class UIWinMenu;
class UIGfx;
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

		static UITheme * LoadFromTextureAtlas( UITheme * tTheme, Graphics::TextureAtlas * TextureAtlas );

		static UITheme * LoadFromPath( UITheme * tTheme, const std::string& Path, const std::string ImgExt = "png" );

		static UITheme * LoadFromTextureAtlas( Graphics::TextureAtlas * TextureAtlas, const std::string& Name, const std::string NameAbbr );

		static UITheme * LoadFromPath( const std::string& Path, const std::string& Name, const std::string& NameAbbr, const std::string ImgExt = "png" );

		void AddThemeElement( const std::string& Element );

		void AddThemeIcon( const std::string& Icon );

		UITheme( const std::string& Name, const std::string& Abbr, Graphics::Font * DefaultFont = NULL );

		virtual ~UITheme();

		const std::string& Name() const;

		void Name( const std::string& name );

		const Uint32& Id() const;

		const std::string& Abbr() const;

		virtual UISkin * add( UISkin * Resource );

		void Font( Graphics::Font * Font );

		Graphics::Font * Font() const;

		const ColorA& FontColor() const;

		const ColorA& FontShadowColor() const;

		const ColorA& FontOverColor() const;

		const ColorA& FontSelectedColor() const;

		void FontColor( const ColorA& Color );

		void FontShadowColor( const ColorA& Color );

		void FontOverColor( const ColorA& Color );

		void FontSelectedColor( const ColorA& Color );

		void UseDefaultThemeValues( const bool& Use );

		const bool& UseDefaultThemeValues() const;

		Graphics::TextureAtlas * TextureAtlas() const;

		SubTexture * GetIconByName( const std::string& name );

		virtual UIGfx * CreateGfx( SubTexture * SubTexture, UIControl * Parent = NULL, const Sizei& Size = Sizei(), const Vector2i& Pos = Vector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, ColorA SubTextureColor = ColorA(255,255,255,255), EE_RENDER_MODE SubTextureRender = RN_NORMAL );

		virtual UISprite * CreateSprite( Sprite * Sprite, UIControl * Parent = NULL, const Sizei& Size = Sizei(), const Vector2i& Pos = Vector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, bool DealloSprite = true, EE_RENDER_MODE SpriteRender = RN_NORMAL );

		virtual UICheckBox * CreateCheckBox( UIControl * Parent = NULL, const Sizei& Size = Sizei(), const Vector2i& Pos = Vector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS );

		virtual UIRadioButton * CreateRadioButton( UIControl * Parent = NULL, const Sizei& Size = Sizei(), const Vector2i& Pos = Vector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS );

		virtual UITextBox * CreateTextBox( const String& Text = "", UIControl * Parent = NULL, const Sizei& Size = Sizei(), const Vector2i& Pos = Vector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );

		virtual UITextEdit * CreateTextEdit( UIControl * Parent = NULL, const Sizei& Size = Sizei(), const Vector2i& Pos = Vector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_PADDING | UI_CLIP_ENABLE | UI_AUTO_SIZE | UI_TEXT_SELECTION_ENABLED, UI_SCROLLBAR_MODE HScrollBar = UI_SCROLLBAR_AUTO, UI_SCROLLBAR_MODE VScrollBar = UI_SCROLLBAR_AUTO, bool WordWrap = true );

		virtual UITextInput * CreateTextInput( UIControl * Parent = NULL, const Sizei& Size = Sizei(), const Vector2i& Pos = Vector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_TEXT_SELECTION_ENABLED, bool SupportFreeEditing = true, Uint32 MaxLength = 256 );

		virtual UITextInputPassword * CreateTextInputPassword( UIControl * Parent = NULL, const Sizei& Size = Sizei(), const Vector2i& Pos = Vector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_TEXT_SELECTION_ENABLED, bool SupportFreeEditing = true, Uint32 MaxLength = 256 );

		virtual UITooltip * CreateTooltip( UIControl * TooltipOf, UIControl * Parent = NULL, const Sizei& Size = Sizei(), const Vector2i& Pos = Vector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_PADDING | UI_AUTO_SIZE );

		virtual UIScrollBar * CreateScrollBar( UIControl * Parent = NULL, const Sizei& Size = Sizei(), const Vector2i& Pos = Vector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE, bool VerticalScrollBar = false );

		virtual UISlider * CreateSlider( UIControl * Parent = NULL, const Sizei& Size = Sizei(), const Vector2i& Pos = Vector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS, bool VerticalSlider = false, bool AllowHalfSliderOut = true, bool ExpandBackground = false );

		virtual UISpinBox * CreateSpinBox( UIControl * Parent = NULL, const Sizei& Size = Sizei(), const Vector2i& Pos = Vector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_SIZE | UI_TEXT_SELECTION_ENABLED, Float DefaultValue = 0.f, bool AllowDotsInNumbers = true );

		virtual UIComboBox * CreateComboBox( UIControl * Parent = NULL, const Sizei& Size = Sizei(), const Vector2i& Pos = Vector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_TEXT_SELECTION_ENABLED, Uint32 MinNumVisibleItems = 6, bool PopUpToMainControl = false, UIListBox * ListBox = NULL );

		virtual UIDropDownList * CreateDropDownList( UIControl * Parent = NULL, const Sizei& Size = Sizei(), const Vector2i& Pos = Vector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING, Uint32 MinNumVisibleItems = 6, bool PopUpToMainControl = false, UIListBox * ListBox = NULL );

		virtual UIListBox * CreateListBox( UIControl * Parent = NULL, const Sizei& Size = Sizei(), const Vector2i& Pos = Vector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING, bool SmoothScroll = true, Uint32 RowHeight = 0, UI_SCROLLBAR_MODE VScrollMode = UI_SCROLLBAR_AUTO, UI_SCROLLBAR_MODE HScrollMode = UI_SCROLLBAR_AUTO, Recti PaddingContainer = Recti() );

		virtual UIMenu * CreateMenu( UIControl * Parent = NULL, const Sizei& Size = Sizei(), const Vector2i& Pos = Vector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE | UI_AUTO_PADDING, Uint32 RowHeight = 0, Recti PaddingContainer = Recti(), Uint32 MinWidth = 0, Uint32 MinSpaceForIcons = 0, Uint32 MinRightMargin = 0 );

		virtual UIPopUpMenu * CreatePopUpMenu( UIControl * Parent = NULL, const Sizei& Size = Sizei(), const Vector2i& Pos = Vector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE | UI_AUTO_PADDING, Uint32 RowHeight = 0, Recti PaddingContainer = Recti(), Uint32 MinWidth = 0, Uint32 MinSpaceForIcons = 0, Uint32 MinRightMargin = 0 );

		virtual UIProgressBar * CreateProgressBar( UIControl * Parent = NULL, const Sizei& Size = Sizei(), const Vector2i& Pos = Vector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS, bool DisplayPercent = false, bool VerticalExpand = false, Vector2f MovementSpeed = Vector2f( 64, 0 ), Rectf FillerMargin = Rectf() );

		virtual UIPushButton * CreatePushButton( UIControl * Parent = NULL, const Sizei& Size = Sizei(), const Vector2i& Pos = Vector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, SubTexture * Icon = NULL, Int32 IconHorizontalMargin = 0, bool IconAutoMargin = true );

		virtual UISelectButton * CreateSelectButton( UIControl * Parent = NULL, const Sizei& Size = Sizei(), const Vector2i& Pos = Vector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, SubTexture * Icon = NULL, Int32 IconHorizontalMargin = 0, bool IconAutoMargin = true );

		virtual UIWinMenu * CreateWinMenu( UIControl * Parent = NULL, const Sizei& Size = Sizei(), const Vector2i& Pos = Vector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS, Uint32 MarginBetweenButtons = 0, Uint32 ButtonMargin = 4, Uint32 MenuHeight = 0, Uint32 FirstButtonMargin = 1 );

		virtual UIWindow * CreateWindow( UIControl * Parent = NULL, const Sizei& Size = Sizei(), const Vector2i& Pos = Vector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS_CENTERED, Uint32 WinFlags = UI_WIN_DEFAULT_FLAGS, Sizei MinWindowSize = Sizei(0,0), Uint8 BaseAlpha = 255 );

		virtual UICommonDialog * CreateCommonDialog( UIControl * Parent = NULL, const Sizei& Size = Sizei(), const Vector2i& Pos = Vector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS_CENTERED, Uint32 WinFlags = UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON, Sizei MinWindowSize = Sizei(0,0), Uint8 BaseAlpha = 255, Uint32 CDLFlags = UI_CDL_DEFAULT_FLAGS, std::string DefaultFilePattern = "*", std::string DefaultDirectory = Sys::getProcessPath() );

		virtual UIMessageBox * CreateMessageBox( UI_MSGBOX_TYPE Type = MSGBOX_OKCANCEL, const String& Message = String(), Uint32 WinFlags = UI_WIN_DEFAULT_FLAGS | UI_WIN_MODAL, UIControl * Parent = NULL, const Sizei& Size = Sizei(), const Vector2i& Pos = Vector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS_CENTERED, Sizei MinWindowSize = Sizei(0,0), Uint8 BaseAlpha = 255 );

		virtual UITabWidget * CreateTabWidget( UIControl * Parent = NULL, const Sizei& Size = Sizei(), const Vector2i& Pos = Vector2i(), const Uint32& Flags = UI_HALIGN_CENTER | UI_VALIGN_BOTTOM | UI_CONTROL_DEFAULT_ANCHOR, const bool& TabsClosable = false, const bool& SpecialBorderTabs = false , const Int32& TabSeparation = 0, const Uint32& MaxTextLength = 30, const Uint32& TabWidgetHeight = 0, const Uint32& TabTextAlign = UI_HALIGN_CENTER | UI_VALIGN_CENTER, const Uint32& MinTabWidth = 32, const Uint32& MaxTabWidth = 210 );
	protected:
		std::string				mName;
		Uint32					mNameHash;
		std::string				mAbbr;
		Graphics::TextureAtlas *			mTextureAtlas;
		Graphics::Font *					mFont;
		ColorA				mFontColor;
		ColorA				mFontShadowColor;
		ColorA				mFontOverColor;
		ColorA				mFontSelectedColor;
		bool					mUseDefaultThemeValues;
		std::list<std::string>	mUIElements;
		std::list<std::string>	mUIIcons;

		void TextureAtlas( Graphics::TextureAtlas * SG );

		static bool SearchFilesOfElement( Graphics::TextureAtlas * SG, const std::string& Path, std::string Element, Uint32& IsComplex, const std::string ImgExt );

		static bool SearchFilesInAtlas( Graphics::TextureAtlas * SG, std::string Element, Uint32& IsComplex );
};

}}

#endif
