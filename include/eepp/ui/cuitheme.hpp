#ifndef EE_UICUITHEME_HPP
#define EE_UICUITHEME_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/uihelper.hpp>
#include <eepp/ui/cuiskin.hpp>
#include <eepp/system/tresourcemanager.hpp>

namespace EE { namespace Graphics {
class cSprite;
class cTextureAtlas;
class cFont;
}}

namespace EE { namespace UI {

class cUIControl;
class cUICheckBox;
class cUIComboBox;
class cUIDropDownList;
class cUIListBox;
class cUIPopUpMenu;
class cUIProgressBar;
class cUIPushButton;
class cUISelectButton;
class cUIRadioButton;
class cUIScrollBar;
class cUISlider;
class cUISpinBox;
class cUITextBox;
class cUITextEdit;
class cUITextInput;
class cUITextInputPassword;
class cUITooltip;
class cUIWindow;
class cUIWinMenu;
class cUIGfx;
class cUISprite;
class cUIMenu;
class cUICommonDialog;
class cUIMessageBox;
class cUITabWidget;

class EE_API cUITheme : protected tResourceManager<cUISkin> {
	public:
		using tResourceManager<cUISkin>::GetById;
		using tResourceManager<cUISkin>::GetByName;
		using tResourceManager<cUISkin>::Exists;
		using tResourceManager<cUISkin>::ExistsId;

		static cUITheme * LoadFromTextureAtlas( cUITheme * tTheme, cTextureAtlas * TextureAtlas );

		static cUITheme * LoadFromPath( cUITheme * tTheme, const std::string& Path, const std::string ImgExt = "png" );

		static cUITheme * LoadFromTextureAtlas( cTextureAtlas * TextureAtlas, const std::string& Name, const std::string NameAbbr );

		static cUITheme * LoadFromPath( const std::string& Path, const std::string& Name, const std::string& NameAbbr, const std::string ImgExt = "png" );

		void AddThemeElement( const std::string& Element );

		void AddThemeIcon( const std::string& Icon );

		cUITheme( const std::string& Name, const std::string& Abbr, cFont * DefaultFont = NULL );

		virtual ~cUITheme();

		const std::string& Name() const;

		void Name( const std::string& name );

		const Uint32& Id() const;

		const std::string& Abbr() const;

		virtual cUISkin * Add( cUISkin * Resource );

		void Font( cFont * Font );

		cFont * Font() const;

		const eeColorA& FontColor() const;

		const eeColorA& FontShadowColor() const;

		const eeColorA& FontOverColor() const;

		const eeColorA& FontSelectedColor() const;

		void FontColor( const eeColorA& Color );

		void FontShadowColor( const eeColorA& Color );

		void FontOverColor( const eeColorA& Color );

		void FontSelectedColor( const eeColorA& Color );

		void UseDefaultThemeValues( const bool& Use );

		const bool& UseDefaultThemeValues() const;

		cTextureAtlas * TextureAtlas() const;

		cSubTexture * GetIconByName( const std::string& name );

		virtual cUIGfx * CreateGfx( cSubTexture * SubTexture, cUIControl * Parent = NULL, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, eeColorA SubTextureColor = eeColorA(255,255,255,255), EE_RENDER_MODE SubTextureRender = RN_NORMAL );

		virtual cUISprite * CreateSprite( cSprite * Sprite, cUIControl * Parent = NULL, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, bool DeallocSprite = true, EE_RENDER_MODE SpriteRender = RN_NORMAL );

		virtual cUICheckBox * CreateCheckBox( cUIControl * Parent = NULL, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS );

		virtual cUIRadioButton * CreateRadioButton( cUIControl * Parent = NULL, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS );

		virtual cUITextBox * CreateTextBox( const String& Text = "", cUIControl * Parent = NULL, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );

		virtual cUITextEdit * CreateTextEdit( cUIControl * Parent = NULL, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_PADDING | UI_CLIP_ENABLE | UI_AUTO_SIZE | UI_TEXT_SELECTION_ENABLED, UI_SCROLLBAR_MODE HScrollBar = UI_SCROLLBAR_AUTO, UI_SCROLLBAR_MODE VScrollBar = UI_SCROLLBAR_AUTO, bool WordWrap = true );

		virtual cUITextInput * CreateTextInput( cUIControl * Parent = NULL, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_TEXT_SELECTION_ENABLED, bool SupportFreeEditing = true, Uint32 MaxLength = 256 );

		virtual cUITextInputPassword * CreateTextInputPassword( cUIControl * Parent = NULL, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_TEXT_SELECTION_ENABLED, bool SupportFreeEditing = true, Uint32 MaxLength = 256 );

		virtual cUITooltip * CreateTooltip( cUIControl * TooltipOf, cUIControl * Parent = NULL, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_PADDING | UI_AUTO_SIZE );

		virtual cUIScrollBar * CreateScrollBar( cUIControl * Parent = NULL, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE, bool VerticalScrollBar = false );

		virtual cUISlider * CreateSlider( cUIControl * Parent = NULL, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS, bool VerticalSlider = false, bool AllowHalfSliderOut = true, bool ExpandBackground = false );

		virtual cUISpinBox * CreateSpinBox( cUIControl * Parent = NULL, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_SIZE | UI_TEXT_SELECTION_ENABLED, eeFloat DefaultValue = 0.f, bool AllowDotsInNumbers = true );

		virtual cUIComboBox * CreateComboBox( cUIControl * Parent = NULL, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_TEXT_SELECTION_ENABLED, Uint32 MinNumVisibleItems = 6, bool PopUpToMainControl = false, cUIListBox * ListBox = NULL );

		virtual cUIDropDownList * CreateDropDownList( cUIControl * Parent = NULL, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING, Uint32 MinNumVisibleItems = 6, bool PopUpToMainControl = false, cUIListBox * ListBox = NULL );

		virtual cUIListBox * CreateListBox( cUIControl * Parent = NULL, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING, bool SmoothScroll = true, Uint32 RowHeight = 0, UI_SCROLLBAR_MODE VScrollMode = UI_SCROLLBAR_AUTO, UI_SCROLLBAR_MODE HScrollMode = UI_SCROLLBAR_AUTO, eeRecti PaddingContainer = eeRecti() );

		virtual cUIMenu * CreateMenu( cUIControl * Parent = NULL, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE | UI_AUTO_PADDING, Uint32 RowHeight = 0, eeRecti PaddingContainer = eeRecti(), Uint32 MinWidth = 0, Uint32 MinSpaceForIcons = 0, Uint32 MinRightMargin = 0 );

		virtual cUIPopUpMenu * CreatePopUpMenu( cUIControl * Parent = NULL, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE | UI_AUTO_PADDING, Uint32 RowHeight = 0, eeRecti PaddingContainer = eeRecti(), Uint32 MinWidth = 0, Uint32 MinSpaceForIcons = 0, Uint32 MinRightMargin = 0 );

		virtual cUIProgressBar * CreateProgressBar( cUIControl * Parent = NULL, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS, bool DisplayPercent = false, bool VerticalExpand = false, eeVector2f MovementSpeed = eeVector2f( 64, 0 ), eeRectf FillerMargin = eeRectf() );

		virtual cUIPushButton * CreatePushButton( cUIControl * Parent = NULL, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, cSubTexture * Icon = NULL, Int32 IconHorizontalMargin = 0, bool IconAutoMargin = true );

		virtual cUISelectButton * CreateSelectButton( cUIControl * Parent = NULL, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, cSubTexture * Icon = NULL, Int32 IconHorizontalMargin = 0, bool IconAutoMargin = true );

		virtual cUIWinMenu * CreateWinMenu( cUIControl * Parent = NULL, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS, Uint32 MarginBetweenButtons = 0, Uint32 ButtonMargin = 4, Uint32 MenuHeight = 0, Uint32 FirstButtonMargin = 1 );

		virtual cUIWindow * CreateWindow( cUIControl * Parent = NULL, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS_CENTERED, Uint32 WinFlags = UI_WIN_DEFAULT_FLAGS, eeSize MinWindowSize = eeSize(0,0), Uint8 BaseAlpha = 255 );

		virtual cUICommonDialog * CreateCommonDialog( cUIControl * Parent = NULL, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS_CENTERED, Uint32 WinFlags = UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON, eeSize MinWindowSize = eeSize(0,0), Uint8 BaseAlpha = 255, Uint32 CDLFlags = UI_CDL_DEFAULT_FLAGS, std::string DefaultFilePattern = "*", std::string DefaultDirectory = Sys::GetProcessPath() );

		virtual cUIMessageBox * CreateMessageBox( UI_MSGBOX_TYPE Type = MSGBOX_OKCANCEL, const String& Message = String(), Uint32 WinFlags = UI_WIN_DEFAULT_FLAGS | UI_WIN_MODAL, cUIControl * Parent = NULL, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS_CENTERED, eeSize MinWindowSize = eeSize(0,0), Uint8 BaseAlpha = 255 );

		virtual cUITabWidget * CreateTabWidget( cUIControl * Parent = NULL, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_HALIGN_CENTER | UI_VALIGN_BOTTOM | UI_CONTROL_DEFAULT_ANCHOR, const bool& TabsClosable = false, const bool& SpecialBorderTabs = false , const Int32& TabSeparation = 0, const Uint32& MaxTextLength = 30, const Uint32& TabWidgetHeight = 0, const Uint32& TabTextAlign = UI_HALIGN_CENTER | UI_VALIGN_CENTER, const Uint32& MinTabWidth = 32, const Uint32& MaxTabWidth = 210 );
	protected:
		std::string				mName;
		Uint32					mNameHash;
		std::string				mAbbr;
		cTextureAtlas *			mTextureAtlas;
		cFont *					mFont;
		eeColorA				mFontColor;
		eeColorA				mFontShadowColor;
		eeColorA				mFontOverColor;
		eeColorA				mFontSelectedColor;
		bool					mUseDefaultThemeValues;
		std::list<std::string>	mUIElements;
		std::list<std::string>	mUIIcons;

		void TextureAtlas( cTextureAtlas * SG );

		static bool SearchFilesOfElement( cTextureAtlas * SG, const std::string& Path, std::string Element, Uint32& IsComplex, const std::string ImgExt );

		static bool SearchFilesInAtlas( cTextureAtlas * SG, std::string Element, Uint32& IsComplex );
};

}}

#endif
