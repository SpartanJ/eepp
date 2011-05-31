#ifndef EE_UICUITHEME_HPP
#define EE_UICUITHEME_HPP

#include "base.hpp"
#include "uihelper.hpp"
#include "../graphics/cshapegroup.hpp"
#include "../graphics/cfont.hpp"
#include "cuiskin.hpp"

namespace EE { namespace UI {

class cUIControl;
class cUICheckBox;
class cUIComboBox;
class cUIDropDownList;
class cUIListBox;
class cUIPopUpMenu;
class cUIProgressBar;
class cUIPushButton;
class cUIRadioButton;
class cUIScrollBar;
class cUISlider;
class cUISpinBox;
class cUITextBox;
class cUITextEdit;
class cUITextInput;
class cUITooltip;
class cUIWindow;
class cUIWinMenu;
class cUIGfx;
class cUISprite;

class EE_API cUITheme : public tResourceManager<cUISkin> {
	public:
		static cUITheme * LoadFromPath( const std::string& Path, const std::string& Name, const std::string& NameAbbr, const std::string ImgExt = "png" );

		static cUITheme * LoadFromShapeGroup( cShapeGroup * ShapeGroup, const std::string& Name, const std::string NameAbbr );

		static void AddThemeElement( const std::string& Element );

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

		virtual cUIGfx * CreateGfx( cShape * Shape, cUIControl * Parent, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_AUTO_SIZE | UI_VALIGN_CENTER | UI_HALIGN_CENTER, eeColorA ShapeColor = eeColorA(255,255,255,255), EE_RENDERTYPE ShapeRender = RN_NORMAL );

		virtual cUISprite * CreateSprite( cSprite * Sprite, cUIControl * Parent, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_AUTO_SIZE | UI_VALIGN_CENTER | UI_HALIGN_CENTER, bool DeallocSprite = true, EE_RENDERTYPE SpriteRender = RN_NORMAL );

		virtual cUICheckBox * CreateCheckBox( cUIControl * Parent, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_VALIGN_CENTER | UI_HALIGN_LEFT );

		virtual cUIRadioButton * CreateRadioButton( cUIControl * Parent, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_VALIGN_CENTER | UI_HALIGN_LEFT );

		virtual cUITextBox * CreateTextBox( cUIControl * Parent, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_HALIGN_LEFT | UI_VALIGN_CENTER );

		virtual cUITextEdit * CreateTextEdit( cUIControl * Parent, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_HALIGN_LEFT | UI_VALIGN_CENTER | UI_AUTO_PADDING | UI_CLIP_ENABLE, UI_SCROLLBAR_MODE HScrollBar = UI_SCROLLBAR_AUTO, UI_SCROLLBAR_MODE VScrollBar = UI_SCROLLBAR_AUTO, bool WordWrap = true );

		virtual cUITextInput * CreateTextInput( cUIControl * Parent, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_HALIGN_LEFT | UI_VALIGN_CENTER | UI_CLIP_ENABLE | UI_AUTO_PADDING, bool SupportFreeEditing = true, Uint32 MaxLenght = 256 );

		virtual cUITooltip * CreateTooltip( cUIControl * TooltipOf, cUIControl * Parent, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_VALIGN_CENTER | UI_HALIGN_CENTER | UI_AUTO_PADDING | UI_AUTO_SIZE );

		virtual cUIScrollBar * CreateScrollBar( cUIControl * Parent, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_VALIGN_CENTER | UI_HALIGN_LEFT | UI_AUTO_SIZE, bool VerticalScrollBar = false );

		virtual cUISlider * CreateSlider( cUIControl * Parent, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_VALIGN_CENTER | UI_HALIGN_LEFT, bool VerticalSlider = false, bool AllowHalfSliderOut = true, bool ExpandBackground = false );

		virtual cUISpinBox * CreateSpinBox( cUIControl * Parent, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_VALIGN_CENTER | UI_HALIGN_LEFT | UI_CLIP_ENABLE, eeFloat DefaultValue = 0.f, bool AllowDotsInNumbers = true );

		virtual cUIComboBox * CreateComboBox( cUIControl * Parent, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_VALIGN_CENTER | UI_HALIGN_LEFT, Uint32 MinNumVisibleItems = 6, bool PopUpToMainControl = false, cUIListBox * ListBox = NULL );

		virtual cUIDropDownList * CreateDropDownList( cUIControl * Parent, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_VALIGN_CENTER | UI_HALIGN_LEFT, Uint32 MinNumVisibleItems = 6, bool PopUpToMainControl = false, cUIListBox * ListBox = NULL );

		virtual cUIListBox * CreateListBox( cUIControl * Parent, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CLIP_ENABLE | UI_AUTO_PADDING, bool SmoothScroll = true, Uint32 RowHeight = 0, UI_SCROLLBAR_MODE VScrollMode = UI_SCROLLBAR_AUTO, UI_SCROLLBAR_MODE HScrollMode = UI_SCROLLBAR_AUTO, eeRecti PaddingContainer = eeRecti() );

		virtual cUIPopUpMenu * CreatePopUpMenu( cUIControl * Parent, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_AUTO_SIZE | UI_AUTO_PADDING, Uint32 RowHeight = 0, eeRecti PaddingContainer = eeRecti(), Uint32 MinWidth = 0, Uint32 MinSpaceForIcons = 0, Uint32 MinRightMargin = 0 );

		virtual cUIProgressBar * CreateProgressBar( cUIControl * Parent, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_HALIGN_LEFT | UI_VALIGN_CENTER, bool DisplayPercent = false, bool VerticalExpand = false, eeVector2f MovementSpeed = eeVector2f( 64, 0 ), eeRectf FillerMargin = eeRectf() );

		virtual cUIPushButton * CreatePushButton( cUIControl * Parent, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_VALIGN_CENTER | UI_HALIGN_CENTER, cShape * Icon = NULL, Int32 IconHorizontalMargin = 0, bool IconAutoMargin = true );

		virtual cUIWinMenu * CreateWinMenu( cUIControl * Parent, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_VALIGN_CENTER | UI_HALIGN_LEFT, Uint32 MarginBetweenButtons = 0, Uint32 ButtonMargin = 4, Uint32 MenuHeight = 0, Uint32 FirstButtonMargin = 1 );
	protected:
		std::string 		mName;
		Uint32				mNameHash;
		std::string			mAbbr;
		cFont * 			mFont;
		eeColorA			mFontColor;
		eeColorA			mFontShadowColor;
		eeColorA			mFontOverColor;
		eeColorA			mFontSelectedColor;

		static bool SearchFilesOfElement( cShapeGroup * SG, const std::string& Path, std::string Element, Uint32& IsComplex, const std::string ImgExt );

		static bool SearchFilesInGroup( cShapeGroup * SG, std::string Element, Uint32& IsComplex );

		virtual void PostInit();
};

}}

#endif
