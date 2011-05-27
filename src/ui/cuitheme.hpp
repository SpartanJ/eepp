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

		virtual cUICheckBox * CreateCheckBox( cUIControl * Parent, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_VALIGN_CENTER | UI_HALIGN_LEFT );

		virtual cUIComboBox * CreateComboBox( cUIControl * Parent, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_VALIGN_CENTER | UI_HALIGN_LEFT, Uint32 MinNumVisibleItems = 6, bool PopUpToMainControl = false, cUIListBox * ListBox = NULL );

		virtual cUIDropDownList * CreateDropDownList( cUIControl * Parent, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_VALIGN_CENTER | UI_HALIGN_LEFT, Uint32 MinNumVisibleItems = 6, bool PopUpToMainControl = false, cUIListBox * ListBox = NULL );

		virtual cUIListBox * CreateListBox( cUIControl * Parent, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CLIP_ENABLE | UI_AUTO_PADDING, bool SmoothScroll = true, Uint32 RowHeight = 0, UI_SCROLLBAR_MODE VScrollMode = UI_SCROLLBAR_AUTO, UI_SCROLLBAR_MODE HScrollMode = UI_SCROLLBAR_AUTO, eeRecti PaddingContainer = eeRecti() );

		virtual cUIPopUpMenu * CreatePopUpMenu( cUIControl * Parent, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_AUTO_SIZE | UI_AUTO_PADDING );

		virtual cUIProgressBar * CreateProgressBar( cUIControl * Parent, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_HALIGN_LEFT | UI_VALIGN_CENTER, bool DisplayPercent = true );

		virtual cUIPushButton * CreatePushButton( cUIControl * Parent, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_VALIGN_CENTER | UI_HALIGN_CENTER, cShape * Icon = NULL );
	protected:
		std::string 		mName;
		Uint32				mNameHash;
		std::string			mAbbr;
		cFont * 			mFont;
		eeColorA			mFontColor;
		eeColorA			mFontShadowColor;
		eeColorA			mFontOverColor;
		eeColorA			mFontSelectedColor;
	private:
		static bool SearchFilesOfElement( cShapeGroup * SG, const std::string& Path, std::string Element, Uint32& IsComplex, const std::string ImgExt );

		static bool SearchFilesInGroup( cShapeGroup * SG, std::string Element, Uint32& IsComplex );

		virtual void PostInit();
};

}}

#endif
