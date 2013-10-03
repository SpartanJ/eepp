#include <eepp/ui/cuitheme.hpp>
#include <eepp/ui/cuiskinsimple.hpp>
#include <eepp/ui/cuiskincomplex.hpp>
#include <eepp/graphics/csprite.hpp>
#include <eepp/graphics/ctextureatlas.hpp>
#include <eepp/graphics/cfont.hpp>
#include <eepp/graphics/ctexturefactory.hpp>
#include <eepp/graphics/ctextureatlasmanager.hpp>
#include <eepp/system/filesystem.hpp>

#include <eepp/ui/cuicheckbox.hpp>
#include <eepp/ui/cuicombobox.hpp>
#include <eepp/ui/cuidropdownlist.hpp>
#include <eepp/ui/cuilistbox.hpp>
#include <eepp/ui/cuipopupmenu.hpp>
#include <eepp/ui/cuiprogressbar.hpp>
#include <eepp/ui/cuipushbutton.hpp>
#include <eepp/ui/cuiradiobutton.hpp>
#include <eepp/ui/cuiscrollbar.hpp>
#include <eepp/ui/cuislider.hpp>
#include <eepp/ui/cuispinbox.hpp>
#include <eepp/ui/cuitextbox.hpp>
#include <eepp/ui/cuitextedit.hpp>
#include <eepp/ui/cuitextinput.hpp>
#include <eepp/ui/cuitooltip.hpp>
#include <eepp/ui/cuiwindow.hpp>
#include <eepp/ui/cuiwinmenu.hpp>
#include <eepp/ui/cuigfx.hpp>
#include <eepp/ui/cuisprite.hpp>
#include <eepp/ui/cuicommondialog.hpp>
#include <eepp/ui/cuimessagebox.hpp>
#include <eepp/ui/cuitabwidget.hpp>

namespace EE { namespace UI {

static void LoadThemeElements( std::list<std::string>& UI_THEME_ELEMENTS, std::list<std::string>& UI_THEME_ICONS ) {
		UI_THEME_ELEMENTS.push_back( "control" );
		UI_THEME_ELEMENTS.push_back( "button" );
		UI_THEME_ELEMENTS.push_back( "textinput" );
		UI_THEME_ELEMENTS.push_back( "checkbox" );
		UI_THEME_ELEMENTS.push_back( "checkbox_active" );
		UI_THEME_ELEMENTS.push_back( "checkbox_inactive" );
		UI_THEME_ELEMENTS.push_back( "button" );
		UI_THEME_ELEMENTS.push_back( "radiobutton" );
		UI_THEME_ELEMENTS.push_back( "radiobutton_active" );
		UI_THEME_ELEMENTS.push_back( "radiobutton_inactive" );
		UI_THEME_ELEMENTS.push_back( "hslider" );
		UI_THEME_ELEMENTS.push_back( "hslider_bg" );
		UI_THEME_ELEMENTS.push_back( "hslider_button" );
		UI_THEME_ELEMENTS.push_back( "vslider" );
		UI_THEME_ELEMENTS.push_back( "vslider_bg" );
		UI_THEME_ELEMENTS.push_back( "vslider_button" );
		UI_THEME_ELEMENTS.push_back( "spinbox" );
		UI_THEME_ELEMENTS.push_back( "spinbox_input" );
		UI_THEME_ELEMENTS.push_back( "spinbox_btnup" );
		UI_THEME_ELEMENTS.push_back( "spinbox_btndown" );
		UI_THEME_ELEMENTS.push_back( "hscrollbar" );
		UI_THEME_ELEMENTS.push_back( "hscrollbar_slider" );
		UI_THEME_ELEMENTS.push_back( "hscrollbar_bg" );
		UI_THEME_ELEMENTS.push_back( "hscrollbar_button" );
		UI_THEME_ELEMENTS.push_back( "hscrollbar_btnup" );
		UI_THEME_ELEMENTS.push_back( "hscrollbar_btndown" );
		UI_THEME_ELEMENTS.push_back( "vscrollbar" );
		UI_THEME_ELEMENTS.push_back( "vscrollbar_slider" );
		UI_THEME_ELEMENTS.push_back( "vscrollbar_bg" );
		UI_THEME_ELEMENTS.push_back( "vscrollbar_button" );
		UI_THEME_ELEMENTS.push_back( "vscrollbar_btnup" );
		UI_THEME_ELEMENTS.push_back( "vscrollbar_btndown" );
		UI_THEME_ELEMENTS.push_back( "progressbar" );
		UI_THEME_ELEMENTS.push_back( "progressbar_filler" );
		UI_THEME_ELEMENTS.push_back( "listbox" );
		UI_THEME_ELEMENTS.push_back( "listboxitem" );
		UI_THEME_ELEMENTS.push_back( "dropdownlist" );
		UI_THEME_ELEMENTS.push_back( "combobox" );
		UI_THEME_ELEMENTS.push_back( "menu" );
		UI_THEME_ELEMENTS.push_back( "menuitem" );
		UI_THEME_ELEMENTS.push_back( "separator" );
		UI_THEME_ELEMENTS.push_back( "menucheckbox_active" );
		UI_THEME_ELEMENTS.push_back( "menucheckbox_inactive" );
		UI_THEME_ELEMENTS.push_back( "menuarrow" );
		UI_THEME_ELEMENTS.push_back( "textedit" );
		UI_THEME_ELEMENTS.push_back( "textedit_box" );
		UI_THEME_ELEMENTS.push_back( "tooltip" );
		UI_THEME_ELEMENTS.push_back( "genericgrid" );
		UI_THEME_ELEMENTS.push_back( "gridcell" );
		UI_THEME_ELEMENTS.push_back( "windeco" );
		UI_THEME_ELEMENTS.push_back( "winback" );
		UI_THEME_ELEMENTS.push_back( "winborderleft" );
		UI_THEME_ELEMENTS.push_back( "winborderright" );
		UI_THEME_ELEMENTS.push_back( "winborderbottom" );
		UI_THEME_ELEMENTS.push_back( "winclose" );
		UI_THEME_ELEMENTS.push_back( "winmax" );
		UI_THEME_ELEMENTS.push_back( "winmin" );
		UI_THEME_ELEMENTS.push_back( "winshade" );
		UI_THEME_ELEMENTS.push_back( "winmenu" );
		UI_THEME_ELEMENTS.push_back( "winmenubutton" );
		UI_THEME_ELEMENTS.push_back( "tabwidget" );
		UI_THEME_ELEMENTS.push_back( "tabcontainer" );
		UI_THEME_ELEMENTS.push_back( "tab" );
		UI_THEME_ELEMENTS.push_back( "tab_left" );
		UI_THEME_ELEMENTS.push_back( "tab_right" );

		UI_THEME_ICONS.push_back( "ok" );
		UI_THEME_ICONS.push_back( "cancel" );
		UI_THEME_ICONS.push_back( "remove" );
		UI_THEME_ICONS.push_back( "go-up" );
		UI_THEME_ICONS.push_back( "quit" );
		UI_THEME_ICONS.push_back( "add" );
		UI_THEME_ICONS.push_back( "document-open" );
		UI_THEME_ICONS.push_back( "document-close" );
		UI_THEME_ICONS.push_back( "document-new" );
		UI_THEME_ICONS.push_back( "document-save" );
		UI_THEME_ICONS.push_back( "document-save-as" );
}

void cUITheme::AddThemeElement( const std::string& Element ) {
	mUIElements.push_back( Element );
}

void cUITheme::AddThemeIcon( const std::string& Icon ) {
	mUIIcons.push_back( Icon );
}

cUITheme * cUITheme::LoadFromTextureAtlas( cUITheme * tTheme, cTextureAtlas * TextureAtlas ) {
	eeASSERT( NULL != tTheme && NULL != TextureAtlas );

	/** Themes use nearest filter by default, force the filter to the textures. */
	for ( Uint32 tC = 0; tC < TextureAtlas->GetTexturesCount(); tC++ ) {
		TextureAtlas->GetTexture( tC )->Filter( TEX_FILTER_NEAREST );
	}

	cClock TE;

	LoadThemeElements( tTheme->mUIElements, tTheme->mUIIcons );

	Uint32 i;
	bool Found;
	std::string Element;
	std::vector<std::string> 	ElemFound;
	std::vector<Uint32> 		ElemType;

	tTheme->TextureAtlas( TextureAtlas );

	for ( std::list<std::string>::iterator it = tTheme->mUIElements.begin() ; it != tTheme->mUIElements.end(); it++ ) {
		Uint32 IsComplex = 0;

		Element = std::string( tTheme->Abbr() + "_" + *it );

		Found 	= SearchFilesInAtlas( TextureAtlas, Element, IsComplex );

		if ( Found ) {
			ElemFound.push_back( Element );
			ElemType.push_back( IsComplex );
		}
	}

	for ( i = 0; i < ElemFound.size(); i++ ) {
		if ( ElemType[i] )
			tTheme->Add( eeNew( cUISkinComplex, ( ElemFound[i] ) ) );
		else
			tTheme->Add( eeNew( cUISkinSimple, ( ElemFound[i] ) ) );
	}

	eePRINTL( "UI Theme Loaded in: %4.3f ms ( from TextureAtlas )", TE.Elapsed().AsMilliseconds() );

	return tTheme;
}

cUITheme * cUITheme::LoadFromPath( cUITheme * tTheme, const std::string& Path, const std::string ImgExt ) {
	cClock TE;

	LoadThemeElements( tTheme->mUIElements, tTheme->mUIIcons );

	Uint32 i;
	bool Found;
	std::string Element;
	std::string ElemName;
	std::string RPath( Path );

	FileSystem::DirPathAddSlashAtEnd( RPath );

	if ( !FileSystem::IsDirectory( RPath ) )
		return NULL;

	std::vector<std::string> 	ElemFound;
	std::vector<Uint32> 		ElemType;

	cTextureAtlas * tSG = eeNew( cTextureAtlas, ( tTheme->Abbr() ) );

	tTheme->TextureAtlas( tSG );

	for ( std::list<std::string>::iterator it = tTheme->mUIElements.begin() ; it != tTheme->mUIElements.end(); it++ ) {
		Uint32 IsComplex = 0;

		Element = tTheme->Abbr() + "_" + *it;

		Found 	= SearchFilesOfElement( tSG, RPath, Element, IsComplex, ImgExt );

		if ( Found ) {
			ElemFound.push_back( Element );
			ElemType.push_back( IsComplex );
		}
	}

	// Load the icons from path.
	for ( std::list<std::string>::iterator it = tTheme->mUIIcons.begin() ; it != tTheme->mUIIcons.end(); it++ ) {
		ElemName	= tTheme->Abbr() + "_icon_" + *it;
		Element		= RPath + ElemName + "." + ImgExt;

		if ( FileSystem::FileExists( Element ) ) {
			tSG->Add( eeNew( cSubTexture, ( cTextureFactory::instance()->Load( Element ), ElemName ) ) );
		}
	}

	if ( tSG->Count() )
		cTextureAtlasManager::instance()->Add( tSG );
	else
		eeSAFE_DELETE( tSG );

	for ( i = 0; i < ElemFound.size(); i++ ) {
		if ( ElemType[i] )
			tTheme->Add( eeNew( cUISkinComplex, ( ElemFound[i] ) ) );
		else
			tTheme->Add( eeNew( cUISkinSimple, ( ElemFound[i] ) ) );
	}

	eePRINTL( "UI Theme Loaded in: %4.3f ms ( from path )", TE.Elapsed().AsMilliseconds() );

	return tTheme;
}

cUITheme * cUITheme::LoadFromPath( const std::string& Path, const std::string& Name, const std::string& NameAbbr, const std::string ImgExt ) {
	return LoadFromPath( eeNew( cUITheme, ( Name, NameAbbr ) ), Path, ImgExt );
}

cUITheme * cUITheme::LoadFromTextureAtlas( cTextureAtlas * TextureAtlas, const std::string& Name, const std::string NameAbbr ) {
	return LoadFromTextureAtlas( eeNew( cUITheme, ( Name, NameAbbr ) ), TextureAtlas );
}

bool cUITheme::SearchFilesInAtlas( cTextureAtlas * SG, std::string Element, Uint32& IsComplex ) {
	bool Found = false;
	Uint32 i = 0, s = 0;
	std::string ElemName;
	IsComplex = 0;

	// Search Complex Skin
	for ( i = 0; i < cUISkinState::StateCount; i++ ) {
		for ( s = 0; s < cUISkinComplex::SideCount; s++ ) {
			ElemName = Element + "_" + cUISkin::GetSkinStateName( i ) + "_" + cUISkinComplex::GetSideSuffix( s );

			if ( SG->GetByName( ElemName ) ) {
				IsComplex = 1;
				Found = true;
				break;
			}
		}

		if ( Found ) {
			break;
		}
	}

	// Search Simple Skin
	if ( !IsComplex ) {
		for ( i = 0; i < cUISkinState::StateCount; i++ ) {
			ElemName = Element + "_" + cUISkin::GetSkinStateName( i );

			if ( SG->GetByName( ElemName ) ) {
				Found = true;
				break;
			}
		}
	}

	return Found;
}

bool cUITheme::SearchFilesOfElement( cTextureAtlas * SG, const std::string& Path, std::string Element, Uint32& IsComplex, const std::string ImgExt ) {
	bool Found = false;
	Uint32 i = 0, s = 0;
	std::string ElemPath;
	std::string ElemFullPath;
	std::string ElemName;
	IsComplex = 0;

	// Search Complex Skin
	for ( i = 0; i < cUISkinState::StateCount; i++ ) {
		for ( s = 0; s < cUISkinComplex::SideCount; s++ ) {
			ElemName = Element + "_" + cUISkin::GetSkinStateName( i ) + "_" + cUISkinComplex::GetSideSuffix( s );
			ElemPath = Path + ElemName;
			ElemFullPath = ElemPath + "." + ImgExt;

			if ( FileSystem::FileExists( ElemFullPath ) ) {
				SG->Add( eeNew( cSubTexture, ( cTextureFactory::instance()->Load( ElemFullPath ), ElemName ) ) );

				IsComplex = 1;
				Found = true;
				break;
			}
		}
	}

	// Seach Simple Skin
	if ( !IsComplex ) {
		for ( i = 0; i < cUISkinState::StateCount; i++ ) {
			ElemName = Element + "_" + cUISkin::GetSkinStateName( i );
			ElemPath = Path + ElemName;
			ElemFullPath = ElemPath + "." + ImgExt;

			if ( FileSystem::FileExists( ElemFullPath ) ) {
				SG->Add( eeNew( cSubTexture, ( cTextureFactory::instance()->Load( ElemFullPath ), ElemName ) ) );

				Found = true;
			}
		}
	}

	return Found;
}

cUITheme::cUITheme( const std::string& Name, const std::string& Abbr, cFont * DefaultFont ) :
	tResourceManager<cUISkin> ( false ),
	mName( Name ),
	mNameHash( String::Hash( mName ) ),
	mAbbr( Abbr ),
	mTextureAtlas( NULL ),
	mFont( DefaultFont ),
	mFontColor( 0, 0, 0, 255 ),
	mFontShadowColor( 255, 255, 255, 200 ),
	mFontOverColor( 0, 0, 0, 255 ),
	mFontSelectedColor( 0, 0, 0, 255 ),
	mUseDefaultThemeValues( true )
{
}

cUITheme::~cUITheme() {

}

const std::string& cUITheme::Name() const {
	return mName;
}

void cUITheme::Name( const std::string& name ) {
	mName = name;
	mNameHash = String::Hash( mName );
}

const Uint32& cUITheme::Id() const {
	return mNameHash;
}

const std::string& cUITheme::Abbr() const {
	return mAbbr;
}

cUISkin * cUITheme::Add( cUISkin * Resource ) {
	Resource->Theme( this );

	return tResourceManager<cUISkin>::Add( Resource );
}

void cUITheme::Font( cFont * Font ) {
	mFont = Font;
}

cFont * cUITheme::Font() const {
	return mFont;
}

const eeColorA& cUITheme::FontColor() const {
	return mFontColor;
}

const eeColorA& cUITheme::FontShadowColor() const {
	return mFontShadowColor;
}

const eeColorA& cUITheme::FontOverColor() const {
	return mFontOverColor;
}

const eeColorA& cUITheme::FontSelectedColor() const {
	return mFontSelectedColor;
}

void cUITheme::FontColor( const eeColorA& Color ) {
	mFontColor = Color;
}

void cUITheme::FontShadowColor( const eeColorA& Color ) {
	mFontShadowColor = Color;
}

void cUITheme::FontOverColor( const eeColorA& Color ) {
	mFontOverColor = Color;
}

void cUITheme::FontSelectedColor( const eeColorA& Color ) {
	mFontSelectedColor = Color;
}

void cUITheme::UseDefaultThemeValues( const bool& Use ) {
	mUseDefaultThemeValues = Use;
}

const bool& cUITheme::UseDefaultThemeValues() const {
	return mUseDefaultThemeValues;
}

cTextureAtlas * cUITheme::TextureAtlas() const {
	return mTextureAtlas;
}

void cUITheme::TextureAtlas( cTextureAtlas * SG ) {
	mTextureAtlas = SG;
}

cSubTexture * cUITheme::GetIconByName( const std::string& name ) {
	if ( NULL != mTextureAtlas )
		return mTextureAtlas->GetByName( mAbbr + "_icon_" + name );

	return NULL;
}

cUIGfx * cUITheme::CreateGfx( cSubTexture * SubTexture, cUIControl * Parent, const eeSize& Size, const eeVector2i& Pos, const Uint32& Flags, eeColorA SubTextureColor, EE_RENDER_MODE SubTextureRender ) {
	cUIGfx::CreateParams GfxParams;
	GfxParams.Parent( Parent );
	GfxParams.PosSet( Pos );
	GfxParams.SizeSet( Size );
	GfxParams.Flags = Flags;
	GfxParams.SubTexture = SubTexture;
	GfxParams.SubTextureColor = SubTextureColor;
	GfxParams.SubTextureRender = SubTextureRender;
	cUIGfx * Gfx = eeNew( cUIGfx, ( GfxParams ) );
	Gfx->Visible( true );
	Gfx->Enabled( true );
	return Gfx;
}

cUISprite * cUITheme::CreateSprite( cSprite * Sprite, cUIControl * Parent, const eeSize& Size, const eeVector2i& Pos, const Uint32& Flags, bool DeallocSprite, EE_RENDER_MODE SpriteRender ) {
	cUISprite::CreateParams SpriteParams;
	SpriteParams.Parent( Parent );
	SpriteParams.PosSet( Pos );
	SpriteParams.SizeSet( Size );
	SpriteParams.Flags = Flags;
	SpriteParams.Sprite = Sprite;
	SpriteParams.SpriteRender = SpriteRender;
	SpriteParams.DeallocSprite = DeallocSprite;
	cUISprite * Spr = eeNew( cUISprite, ( SpriteParams ) );
	Spr->Visible( true );
	Spr->Enabled( true );
	return Spr;
}

cUICheckBox * cUITheme::CreateCheckBox( cUIControl * Parent, const eeSize& Size, const eeVector2i& Pos, const Uint32& Flags ) {
	cUICheckBox::CreateParams CheckBoxParams;
	CheckBoxParams.Parent( Parent );
	CheckBoxParams.PosSet( Pos );
	CheckBoxParams.SizeSet( Size );
	CheckBoxParams.Flags = Flags;
	cUICheckBox * Ctrl = eeNew( cUICheckBox, ( CheckBoxParams ) );
	Ctrl->Visible( true );
	Ctrl->Enabled( true );
	return Ctrl;
}

cUIRadioButton * cUITheme::CreateRadioButton( cUIControl * Parent, const eeSize& Size, const eeVector2i& Pos, const Uint32& Flags ) {
	cUIRadioButton::CreateParams RadioButtonParams;
	RadioButtonParams.Parent( Parent );
	RadioButtonParams.PosSet( Pos );
	RadioButtonParams.SizeSet( Size );
	RadioButtonParams.Flags = Flags;
	cUIRadioButton * Ctrl = eeNew( cUIRadioButton, ( RadioButtonParams ) );
	Ctrl->Visible( true );
	Ctrl->Enabled( true );
	return Ctrl;
}

cUITextBox * cUITheme::CreateTextBox( const String& Text, cUIControl * Parent, const eeSize& Size, const eeVector2i& Pos, const Uint32& Flags ) {
	cUITextBox::CreateParams TextBoxParams;
	TextBoxParams.Parent( Parent );
	TextBoxParams.PosSet( Pos );
	TextBoxParams.SizeSet( Size );
	TextBoxParams.Flags = Flags;
	cUITextBox * Ctrl = eeNew( cUITextBox, ( TextBoxParams ) );
	Ctrl->Visible( true );
	Ctrl->Enabled( false );
	Ctrl->Text( Text );
	return Ctrl;
}

cUITooltip * cUITheme::CreateTooltip( cUIControl * TooltipOf, cUIControl * Parent, const eeSize& Size, const eeVector2i& Pos, const Uint32& Flags ) {
	cUITooltip::CreateParams TooltipParams;
	TooltipParams.Parent( Parent );
	TooltipParams.PosSet( Pos );
	TooltipParams.SizeSet( Size );
	TooltipParams.Flags = Flags;
	cUITooltip * Ctrl = eeNew( cUITooltip, ( TooltipParams, TooltipOf ) );
	Ctrl->Visible( true );
	Ctrl->Enabled( true );
	return Ctrl;
}

cUITextEdit * cUITheme::CreateTextEdit( cUIControl * Parent, const eeSize& Size, const eeVector2i& Pos, const Uint32& Flags, UI_SCROLLBAR_MODE HScrollBar, UI_SCROLLBAR_MODE VScrollBar, bool WordWrap ) {
	cUITextEdit::CreateParams TextEditParams;
	TextEditParams.Parent( Parent );
	TextEditParams.PosSet( Pos );
	TextEditParams.SizeSet( Size );
	TextEditParams.Flags = Flags;
	TextEditParams.HScrollBar = HScrollBar;
	TextEditParams.VScrollBar = VScrollBar;
	TextEditParams.WordWrap = WordWrap;
	cUITextEdit * Ctrl = eeNew( cUITextEdit, ( TextEditParams ) );
	Ctrl->Visible( true );
	Ctrl->Enabled( true );
	return Ctrl;
}

cUITextInput * cUITheme::CreateTextInput( cUIControl * Parent, const eeSize& Size, const eeVector2i& Pos, const Uint32& Flags, bool SupportFreeEditing, Uint32 MaxLength ) {
	cUITextInput::CreateParams TextInputParams;
	TextInputParams.Parent( Parent );
	TextInputParams.PosSet( Pos );
	TextInputParams.SizeSet( Size );
	TextInputParams.Flags = Flags;
	TextInputParams.SupportFreeEditing = SupportFreeEditing;
	TextInputParams.MaxLength = MaxLength;
	cUITextInput * Ctrl = eeNew( cUITextInput, ( TextInputParams ) );
	Ctrl->Visible( true );
	Ctrl->Enabled( true );
	return Ctrl;
}

cUISpinBox * cUITheme::CreateSpinBox( cUIControl * Parent, const eeSize& Size, const eeVector2i& Pos, const Uint32& Flags, eeFloat DefaultValue, bool AllowDotsInNumbers ) {
	cUISpinBox::CreateParams SpinBoxParams;
	SpinBoxParams.Parent( Parent );
	SpinBoxParams.PosSet( Pos );
	SpinBoxParams.SizeSet( Size );
	SpinBoxParams.Flags = Flags;
	SpinBoxParams.DefaultValue = DefaultValue;
	SpinBoxParams.AllowDotsInNumbers = AllowDotsInNumbers;
	cUISpinBox * Ctrl = eeNew( cUISpinBox, ( SpinBoxParams ) );
	Ctrl->Visible( true );
	Ctrl->Enabled( true );
	return Ctrl;
}

cUIScrollBar * cUITheme::CreateScrollBar( cUIControl * Parent, const eeSize& Size, const eeVector2i& Pos, const Uint32& Flags, bool VerticalScrollBar ) {
	cUIScrollBar::CreateParams ScrollBarParams;
	ScrollBarParams.Parent( Parent );
	ScrollBarParams.PosSet( Pos );
	ScrollBarParams.SizeSet( Size );
	ScrollBarParams.Flags = Flags;
	ScrollBarParams.VerticalScrollBar = VerticalScrollBar;
	cUIScrollBar * Ctrl = eeNew( cUIScrollBar, ( ScrollBarParams ) );
	Ctrl->Visible( true );
	Ctrl->Enabled( true );
	return Ctrl;
}

cUISlider * cUITheme::CreateSlider( cUIControl * Parent, const eeSize& Size, const eeVector2i& Pos, const Uint32& Flags, bool VerticalSlider, bool AllowHalfSliderOut, bool ExpandBackground ) {
	cUISlider::CreateParams SliderParams;
	SliderParams.Parent( Parent );
	SliderParams.PosSet( Pos );
	SliderParams.SizeSet( Size );
	SliderParams.Flags = Flags;
	SliderParams.VerticalSlider = VerticalSlider;
	SliderParams.AllowHalfSliderOut = AllowHalfSliderOut;
	SliderParams.ExpandBackground = ExpandBackground;
	cUISlider * Ctrl = eeNew( cUISlider, ( SliderParams ) );
	Ctrl->Visible( true );
	Ctrl->Enabled( true );
	return Ctrl;
}

cUIComboBox * cUITheme::CreateComboBox( cUIControl * Parent, const eeSize& Size, const eeVector2i& Pos, const Uint32& Flags, Uint32 MinNumVisibleItems, bool PopUpToMainControl, cUIListBox * ListBox ) {
	cUIComboBox::CreateParams ComboParams;
	ComboParams.Parent( Parent );
	ComboParams.PosSet( Pos );
	ComboParams.SizeSet( Size );
	ComboParams.Flags = Flags;
	ComboParams.MinNumVisibleItems = MinNumVisibleItems;
	ComboParams.PopUpToMainControl = PopUpToMainControl;
	ComboParams.ListBox = ListBox;
	cUIComboBox * Ctrl = eeNew( cUIComboBox, ( ComboParams ) );
	Ctrl->Visible( true );
	Ctrl->Enabled( true );
	return Ctrl;
}

cUIDropDownList * cUITheme::CreateDropDownList( cUIControl * Parent, const eeSize& Size, const eeVector2i& Pos, const Uint32& Flags, Uint32 MinNumVisibleItems, bool PopUpToMainControl, cUIListBox * ListBox ) {
	cUIDropDownList::CreateParams DDLParams;
	DDLParams.Parent( Parent );
	DDLParams.PosSet( Pos );
	DDLParams.SizeSet( Size );
	DDLParams.Flags = Flags;
	DDLParams.MinNumVisibleItems = MinNumVisibleItems;
	DDLParams.PopUpToMainControl = PopUpToMainControl;
	DDLParams.ListBox = ListBox;
	cUIDropDownList * Ctrl = eeNew( cUIDropDownList, ( DDLParams ) );
	Ctrl->Visible( true );
	Ctrl->Enabled( true );
	return Ctrl;
}

cUIListBox * cUITheme::CreateListBox( cUIControl * Parent, const eeSize& Size, const eeVector2i& Pos, const Uint32& Flags, bool SmoothScroll, Uint32 RowHeight, UI_SCROLLBAR_MODE VScrollMode, UI_SCROLLBAR_MODE HScrollMode, eeRecti PaddingContainer ) {
	cUIListBox::CreateParams LBParams;
	LBParams.Parent( Parent );
	LBParams.PosSet( Pos );
	LBParams.SizeSet( Size );
	LBParams.Flags = Flags;
	LBParams.SmoothScroll = SmoothScroll;
	LBParams.RowHeight = RowHeight;
	LBParams.VScrollMode = VScrollMode;
	LBParams.HScrollMode = HScrollMode;
	LBParams.PaddingContainer = PaddingContainer;
	cUIListBox * Ctrl = eeNew( cUIListBox, ( LBParams ) );
	Ctrl->Visible( true );
	Ctrl->Enabled( true );
	return Ctrl;
}

cUIMenu * cUITheme::CreateMenu( cUIControl * Parent, const eeSize& Size, const eeVector2i& Pos, const Uint32& Flags, Uint32 RowHeight, eeRecti PaddingContainer, Uint32 MinWidth, Uint32 MinSpaceForIcons, Uint32 MinRightMargin ) {
	cUIMenu::CreateParams MenuParams;
	MenuParams.Parent( Parent );
	MenuParams.PosSet( Pos );
	MenuParams.SizeSet( Size );
	MenuParams.Flags = Flags;
	MenuParams.RowHeight = RowHeight;
	MenuParams.PaddingContainer = PaddingContainer;
	MenuParams.MinWidth = MinWidth;
	MenuParams.MinSpaceForIcons = MinSpaceForIcons;
	MenuParams.MinRightMargin = MinRightMargin;

	cUIMenu * Ctrl = eeNew( cUIMenu, ( MenuParams ) );
	Ctrl->Visible( true );
	Ctrl->Enabled( true );
	return Ctrl;
}

cUIPopUpMenu * cUITheme::CreatePopUpMenu( cUIControl * Parent, const eeSize& Size, const eeVector2i& Pos, const Uint32& Flags, Uint32 RowHeight, eeRecti PaddingContainer, Uint32 MinWidth, Uint32 MinSpaceForIcons, Uint32 MinRightMargin ) {
	cUIPopUpMenu::CreateParams MenuParams;
	MenuParams.Parent( Parent );
	MenuParams.PosSet( Pos );
	MenuParams.SizeSet( Size );
	MenuParams.Flags = Flags;
	MenuParams.RowHeight = RowHeight;
	MenuParams.PaddingContainer = PaddingContainer;
	MenuParams.MinWidth = MinWidth;
	MenuParams.MinSpaceForIcons = MinSpaceForIcons;
	MenuParams.MinRightMargin = MinRightMargin;
	return eeNew( cUIPopUpMenu, ( MenuParams ) );
}

cUIProgressBar * cUITheme::CreateProgressBar( cUIControl * Parent, const eeSize& Size, const eeVector2i& Pos, const Uint32& Flags, bool DisplayPercent, bool VerticalExpand, eeVector2f MovementSpeed, eeRectf FillerMargin ) {
	cUIProgressBar::CreateParams PBParams;
	PBParams.Parent( Parent );
	PBParams.PosSet( Pos );
	PBParams.SizeSet( Size );
	PBParams.Flags = Flags;
	PBParams.DisplayPercent = DisplayPercent;
	PBParams.VerticalExpand = VerticalExpand;
	PBParams.MovementSpeed = MovementSpeed;
	PBParams.FillerMargin = FillerMargin;

	cUIProgressBar * Ctrl = eeNew( cUIProgressBar, ( PBParams ) );
	Ctrl->Visible( true );
	Ctrl->Enabled( true );
	return Ctrl;
}

cUIPushButton * cUITheme::CreatePushButton( cUIControl * Parent, const eeSize& Size, const eeVector2i& Pos, const Uint32& Flags, cSubTexture * Icon, Int32 IconHorizontalMargin, bool IconAutoMargin ) {
	cUIPushButton::CreateParams ButtonParams;
	ButtonParams.Parent( Parent );
	ButtonParams.PosSet( Pos );
	ButtonParams.SizeSet( Size );
	ButtonParams.Flags = Flags;
	ButtonParams.Icon = Icon;
	ButtonParams.IconHorizontalMargin = IconHorizontalMargin;
	ButtonParams.IconAutoMargin = IconAutoMargin;

	if ( NULL != Icon )
		ButtonParams.SetIcon( Icon );

	cUIPushButton * Ctrl = eeNew( cUIPushButton, ( ButtonParams ) );
	Ctrl->Visible( true );
	Ctrl->Enabled( true );
	return Ctrl;
}

cUISelectButton * cUITheme::CreateSelectButton( cUIControl * Parent, const eeSize& Size, const eeVector2i& Pos, const Uint32& Flags, cSubTexture * Icon, Int32 IconHorizontalMargin, bool IconAutoMargin ) {
	cUIPushButton::CreateParams ButtonParams;
	ButtonParams.Parent( Parent );
	ButtonParams.PosSet( Pos );
	ButtonParams.SizeSet( Size );
	ButtonParams.Flags = Flags;
	ButtonParams.Icon = Icon;
	ButtonParams.IconHorizontalMargin = IconHorizontalMargin;
	ButtonParams.IconAutoMargin = IconAutoMargin;

	if ( NULL != Icon )
		ButtonParams.SetIcon( Icon );

	cUISelectButton * Ctrl = eeNew( cUISelectButton, ( ButtonParams ) );
	Ctrl->Visible( true );
	Ctrl->Enabled( true );
	return Ctrl;
}

cUIWinMenu * cUITheme::CreateWinMenu( cUIControl * Parent, const eeSize& Size, const eeVector2i& Pos, const Uint32& Flags, Uint32 MarginBetweenButtons, Uint32 ButtonMargin, Uint32 MenuHeight, Uint32 FirstButtonMargin ) {
	cUIWinMenu::CreateParams WinMenuParams;
	WinMenuParams.Parent( Parent );
	WinMenuParams.PosSet( Pos );
	WinMenuParams.SizeSet( Size );
	WinMenuParams.Flags = Flags;
	WinMenuParams.MarginBetweenButtons = MarginBetweenButtons;
	WinMenuParams.ButtonMargin = ButtonMargin;
	WinMenuParams.MenuHeight = MenuHeight;
	WinMenuParams.FirstButtonMargin = FirstButtonMargin;

	cUIWinMenu * Ctrl = eeNew( cUIWinMenu, ( WinMenuParams ) );
	Ctrl->Visible( true );
	Ctrl->Enabled( true );
	return Ctrl;
}

cUIWindow * cUITheme::CreateWindow( cUIControl * Parent, const eeSize& Size, const eeVector2i& Pos, const Uint32& Flags, Uint32 WinFlags, eeSize MinWindowSize, Uint8 BaseAlpha ) {
	cUIWindow::CreateParams WinParams;
	WinParams.Parent( Parent );
	WinParams.PosSet( Pos );
	WinParams.SizeSet( Size );
	WinParams.Flags = Flags;
	WinParams.WinFlags = WinFlags;
	WinParams.MinWindowSize = MinWindowSize;
	WinParams.BaseAlpha = BaseAlpha;
	return eeNew( cUIWindow, ( WinParams ) );
}

cUICommonDialog * cUITheme::CreateCommonDialog( cUIControl * Parent, const eeSize& Size, const eeVector2i& Pos, const Uint32& Flags, Uint32 WinFlags, eeSize MinWindowSize, Uint8 BaseAlpha, Uint32 CDLFlags, std::string DefaultFilePattern, std::string DefaultDirectory ) {
	cUICommonDialog::CreateParams DLGParams;
	DLGParams.Parent( Parent );
	DLGParams.PosSet( Pos );
	DLGParams.SizeSet( Size );
	DLGParams.Flags = Flags;
	DLGParams.WinFlags = WinFlags;
	DLGParams.MinWindowSize = MinWindowSize;
	DLGParams.BaseAlpha = BaseAlpha;
	DLGParams.DefaultDirectory = DefaultDirectory;
	DLGParams.DefaultFilePattern = DefaultFilePattern;
	DLGParams.CDLFlags = CDLFlags;
	return eeNew( cUICommonDialog, ( DLGParams ) );
}

cUIMessageBox * cUITheme::CreateMessageBox( UI_MSGBOX_TYPE Type, const String& Message, Uint32 WinFlags, cUIControl * Parent, const eeSize& Size, const eeVector2i& Pos, const Uint32& Flags, eeSize MinWindowSize, Uint8 BaseAlpha ) {
	cUIMessageBox::CreateParams MsgBoxParams;
	MsgBoxParams.Parent( Parent );
	MsgBoxParams.PosSet( Pos );
	MsgBoxParams.SizeSet( Size );
	MsgBoxParams.Flags = Flags;
	MsgBoxParams.WinFlags = WinFlags;
	MsgBoxParams.MinWindowSize = MinWindowSize;
	MsgBoxParams.BaseAlpha = BaseAlpha;
	MsgBoxParams.Type = Type;
	MsgBoxParams.Message = Message;
	return eeNew( cUIMessageBox, ( MsgBoxParams ) );
}

cUITabWidget * cUITheme::CreateTabWidget( cUIControl *Parent, const eeSize &Size, const eeVector2i &Pos, const Uint32 &Flags, const bool &TabsClosable, const bool &SpecialBorderTabs, const Int32 &TabSeparation, const Uint32 &MaxTextLength, const Uint32 &TabWidgetHeight, const Uint32 &TabTextAlign, const Uint32 &MinTabWidth, const Uint32 &MaxTabWidth ) {
	cUITabWidget::CreateParams TabWidgetParams;
	TabWidgetParams.Parent( Parent );
	TabWidgetParams.PosSet( Pos );
	TabWidgetParams.SizeSet( Size );
	TabWidgetParams.Flags = Flags;
	TabWidgetParams.TabsClosable = TabsClosable;
	TabWidgetParams.SpecialBorderTabs = SpecialBorderTabs;
	TabWidgetParams.TabSeparation = TabSeparation;
	TabWidgetParams.MaxTextLength = MaxTextLength;
	TabWidgetParams.TabWidgetHeight = TabWidgetHeight;
	TabWidgetParams.TabTextAlign = TabTextAlign;
	TabWidgetParams.MinTabWidth = MinTabWidth;
	TabWidgetParams.MaxTabWidth = MaxTabWidth;

	cUITabWidget * Ctrl = eeNew( cUITabWidget, ( TabWidgetParams ) );
	Ctrl->Visible( true );
	Ctrl->Enabled( true );
	return Ctrl;
}

}}
