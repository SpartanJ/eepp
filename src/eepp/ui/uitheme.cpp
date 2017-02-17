#include <eepp/ui/uitheme.hpp>
#include <eepp/ui/uiskinsimple.hpp>
#include <eepp/ui/uiskincomplex.hpp>
#include <eepp/graphics/sprite.hpp>
#include <eepp/graphics/textureatlas.hpp>
#include <eepp/graphics/font.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/graphics/textureatlasmanager.hpp>
#include <eepp/system/filesystem.hpp>

#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uicombobox.hpp>
#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uilistbox.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uiprogressbar.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiradiobutton.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uislider.hpp>
#include <eepp/ui/uispinbox.hpp>
#include <eepp/ui/uitextbox.hpp>
#include <eepp/ui/uitextedit.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uitextinputpassword.hpp>
#include <eepp/ui/uitooltip.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/ui/uiwinmenu.hpp>
#include <eepp/ui/uigfx.hpp>
#include <eepp/ui/uisprite.hpp>
#include <eepp/ui/uicommondialog.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uitabwidget.hpp>

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

void UITheme::addThemeElement( const std::string& Element ) {
	mUIElements.push_back( Element );
}

void UITheme::addThemeIcon( const std::string& Icon ) {
	mUIIcons.push_back( Icon );
}

UITheme * UITheme::loadFromTextureAtlas( UITheme * tTheme, Graphics::TextureAtlas * TextureAtlas ) {
	eeASSERT( NULL != tTheme && NULL != TextureAtlas );

	/** Themes use nearest filter by default, force the filter to the textures. */
	for ( Uint32 tC = 0; tC < TextureAtlas->getTexturesCount(); tC++ ) {
		TextureAtlas->getTexture( tC )->filter( TEX_FILTER_NEAREST );
	}

	Clock TE;

	LoadThemeElements( tTheme->mUIElements, tTheme->mUIIcons );

	Uint32 i;
	bool Found;
	std::string Element;
	std::vector<std::string> 	ElemFound;
	std::vector<Uint32> 		ElemType;

	tTheme->getTextureAtlas( TextureAtlas );

	for ( std::list<std::string>::iterator it = tTheme->mUIElements.begin() ; it != tTheme->mUIElements.end(); it++ ) {
		Uint32 IsComplex = 0;

		Element = std::string( tTheme->abbr() + "_" + *it );

		Found 	= searchFilesInAtlas( TextureAtlas, Element, IsComplex );

		if ( Found ) {
			ElemFound.push_back( Element );
			ElemType.push_back( IsComplex );
		}
	}

	for ( i = 0; i < ElemFound.size(); i++ ) {
		if ( ElemType[i] )
			tTheme->add( eeNew( UISkinComplex, ( ElemFound[i] ) ) );
		else
			tTheme->add( eeNew( UISkinSimple, ( ElemFound[i] ) ) );
	}

	eePRINTL( "UI Theme Loaded in: %4.3f ms ( from TextureAtlas )", TE.getElapsed().asMilliseconds() );

	return tTheme;
}

UITheme * UITheme::loadFromPath( UITheme * tTheme, const std::string& Path, const std::string ImgExt ) {
	Clock TE;

	LoadThemeElements( tTheme->mUIElements, tTheme->mUIIcons );

	Uint32 i;
	bool Found;
	std::string Element;
	std::string ElemName;
	std::string RPath( Path );

	FileSystem::dirPathAddSlashAtEnd( RPath );

	if ( !FileSystem::isDirectory( RPath ) )
		return NULL;

	std::vector<std::string> 	ElemFound;
	std::vector<Uint32> 		ElemType;

	Graphics::TextureAtlas * tSG = eeNew( Graphics::TextureAtlas, ( tTheme->abbr() ) );

	tTheme->getTextureAtlas( tSG );

	for ( std::list<std::string>::iterator it = tTheme->mUIElements.begin() ; it != tTheme->mUIElements.end(); it++ ) {
		Uint32 IsComplex = 0;

		Element = tTheme->abbr() + "_" + *it;

		Found 	= searchFilesOfElement( tSG, RPath, Element, IsComplex, ImgExt );

		if ( Found ) {
			ElemFound.push_back( Element );
			ElemType.push_back( IsComplex );
		}
	}

	// Load the icons from path.
	for ( std::list<std::string>::iterator it = tTheme->mUIIcons.begin() ; it != tTheme->mUIIcons.end(); it++ ) {
		ElemName	= tTheme->abbr() + "_icon_" + *it;
		Element		= RPath + ElemName + "." + ImgExt;

		if ( FileSystem::fileExists( Element ) ) {
			tSG->add( eeNew( SubTexture, ( TextureFactory::instance()->load( Element ), ElemName ) ) );
		}
	}

	if ( tSG->getCount() )
		TextureAtlasManager::instance()->add( tSG );
	else
		eeSAFE_DELETE( tSG );

	for ( i = 0; i < ElemFound.size(); i++ ) {
		if ( ElemType[i] )
			tTheme->add( eeNew( UISkinComplex, ( ElemFound[i] ) ) );
		else
			tTheme->add( eeNew( UISkinSimple, ( ElemFound[i] ) ) );
	}

	eePRINTL( "UI Theme Loaded in: %4.3f ms ( from path )", TE.getElapsed().asMilliseconds() );

	return tTheme;
}

UITheme * UITheme::loadFromPath( const std::string& Path, const std::string& Name, const std::string& NameAbbr, const std::string ImgExt ) {
	return loadFromPath( eeNew( UITheme, ( Name, NameAbbr ) ), Path, ImgExt );
}

UITheme * UITheme::loadFromTextureAtlas( Graphics::TextureAtlas * TextureAtlas, const std::string& Name, const std::string NameAbbr ) {
	return loadFromTextureAtlas( eeNew( UITheme, ( Name, NameAbbr ) ), TextureAtlas );
}

bool UITheme::searchFilesInAtlas( Graphics::TextureAtlas * SG, std::string Element, Uint32& IsComplex ) {
	bool Found = false;
	Uint32 i = 0, s = 0;
	std::string ElemName;
	IsComplex = 0;

	// Search Complex Skin
	for ( i = 0; i < UISkinState::StateCount; i++ ) {
		for ( s = 0; s < UISkinComplex::SideCount; s++ ) {
			ElemName = Element + "_" + UISkin::getSkinStateName( i ) + "_" + UISkinComplex::GetSideSuffix( s );

			if ( SG->getByName( ElemName ) ) {
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
		for ( i = 0; i < UISkinState::StateCount; i++ ) {
			ElemName = Element + "_" + UISkin::getSkinStateName( i );

			if ( SG->getByName( ElemName ) ) {
				Found = true;
				break;
			}
		}
	}

	return Found;
}

bool UITheme::searchFilesOfElement( Graphics::TextureAtlas * SG, const std::string& Path, std::string Element, Uint32& IsComplex, const std::string ImgExt ) {
	bool Found = false;
	Uint32 i = 0, s = 0;
	std::string ElemPath;
	std::string ElemFullPath;
	std::string ElemName;
	IsComplex = 0;

	// Search Complex Skin
	for ( i = 0; i < UISkinState::StateCount; i++ ) {
		for ( s = 0; s < UISkinComplex::SideCount; s++ ) {
			ElemName = Element + "_" + UISkin::getSkinStateName( i ) + "_" + UISkinComplex::GetSideSuffix( s );
			ElemPath = Path + ElemName;
			ElemFullPath = ElemPath + "." + ImgExt;

			if ( FileSystem::fileExists( ElemFullPath ) ) {
				SG->add( eeNew( SubTexture, ( TextureFactory::instance()->load( ElemFullPath ), ElemName ) ) );

				IsComplex = 1;
				Found = true;
				break;
			}
		}
	}

	// Seach Simple Skin
	if ( !IsComplex ) {
		for ( i = 0; i < UISkinState::StateCount; i++ ) {
			ElemName = Element + "_" + UISkin::getSkinStateName( i );
			ElemPath = Path + ElemName;
			ElemFullPath = ElemPath + "." + ImgExt;

			if ( FileSystem::fileExists( ElemFullPath ) ) {
				SG->add( eeNew( SubTexture, ( TextureFactory::instance()->load( ElemFullPath ), ElemName ) ) );

				Found = true;
			}
		}
	}

	return Found;
}

UITheme::UITheme( const std::string& Name, const std::string& Abbr, Graphics::Font * defaultFont ) :
	ResourceManager<UISkin> ( false ),
	mName( Name ),
	mNameHash( String::hash( mName ) ),
	mAbbr( Abbr ),
	mTextureAtlas( NULL ),
	mFont( defaultFont ),
	mFontColor( 0, 0, 0, 255 ),
	mFontShadowColor( 255, 255, 255, 200 ),
	mFontOverColor( 0, 0, 0, 255 ),
	mFontSelectedColor( 0, 0, 0, 255 ),
	mUsedefaultThemeValues( true )
{
}

UITheme::~UITheme() {

}

const std::string& UITheme::getName() const {
	return mName;
}

void UITheme::setName( const std::string& name ) {
	mName = name;
	mNameHash = String::hash( mName );
}

const Uint32& UITheme::getId() const {
	return mNameHash;
}

const std::string& UITheme::abbr() const {
	return mAbbr;
}

UISkin * UITheme::add( UISkin * Resource ) {
	Resource->theme( this );

	return ResourceManager<UISkin>::add( Resource );
}

void UITheme::font( Graphics::Font * Font ) {
	mFont = Font;
}

Graphics::Font * UITheme::font() const {
	return mFont;
}

const ColorA& UITheme::fontColor() const {
	return mFontColor;
}

const ColorA& UITheme::fontShadowColor() const {
	return mFontShadowColor;
}

const ColorA& UITheme::fontOverColor() const {
	return mFontOverColor;
}

const ColorA& UITheme::fontSelectedColor() const {
	return mFontSelectedColor;
}

void UITheme::fontColor( const ColorA& Color ) {
	mFontColor = Color;
}

void UITheme::fontShadowColor( const ColorA& Color ) {
	mFontShadowColor = Color;
}

void UITheme::fontOverColor( const ColorA& Color ) {
	mFontOverColor = Color;
}

void UITheme::fontSelectedColor( const ColorA& Color ) {
	mFontSelectedColor = Color;
}

void UITheme::useDefaultThemeValues( const bool& Use ) {
	mUsedefaultThemeValues = Use;
}

const bool& UITheme::useDefaultThemeValues() const {
	return mUsedefaultThemeValues;
}

Graphics::TextureAtlas * UITheme::getTextureAtlas() const {
	return mTextureAtlas;
}

void UITheme::getTextureAtlas( Graphics::TextureAtlas * SG ) {
	mTextureAtlas = SG;
}

SubTexture * UITheme::getIconByName( const std::string& name ) {
	if ( NULL != mTextureAtlas )
		return mTextureAtlas->getByName( mAbbr + "_icon_" + name );

	return NULL;
}

UIGfx * UITheme::createGfx( SubTexture * SubTexture, UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, ColorA SubTextureColor, EE_RENDER_MODE SubTextureRender ) {
	UIGfx::CreateParams GfxParams;
	GfxParams.setParent( Parent );
	GfxParams.setPos( Pos );
	GfxParams.setSize( Size );
	GfxParams.Flags = Flags;
	GfxParams.SubTexture = SubTexture;
	GfxParams.SubTextureColor = SubTextureColor;
	GfxParams.SubTextureRender = SubTextureRender;
	UIGfx * Gfx = eeNew( UIGfx, ( GfxParams ) );
	Gfx->visible( true );
	Gfx->enabled( true );
	return Gfx;
}

UISprite * UITheme::createSprite( Sprite * Sprite, UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, bool DealloSprite, EE_RENDER_MODE SpriteRender ) {
	UISprite::CreateParams SpriteParams;
	SpriteParams.setParent( Parent );
	SpriteParams.setPos( Pos );
	SpriteParams.setSize( Size );
	SpriteParams.Flags = Flags;
	SpriteParams.Sprite = Sprite;
	SpriteParams.SpriteRender = SpriteRender;
	SpriteParams.DealloSprite = DealloSprite;
	UISprite * Spr = eeNew( UISprite, ( SpriteParams ) );
	Spr->visible( true );
	Spr->enabled( true );
	return Spr;
}

UICheckBox * UITheme::createCheckBox( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags ) {
	UICheckBox::CreateParams CheckBoxParams;
	CheckBoxParams.setParent( Parent );
	CheckBoxParams.setPos( Pos );
	CheckBoxParams.setSize( Size );
	CheckBoxParams.Flags = Flags;
	UICheckBox * Ctrl = eeNew( UICheckBox, ( CheckBoxParams ) );
	Ctrl->visible( true );
	Ctrl->enabled( true );
	return Ctrl;
}

UIRadioButton * UITheme::createRadioButton( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags ) {
	UIRadioButton::CreateParams RadioButtonParams;
	RadioButtonParams.setParent( Parent );
	RadioButtonParams.setPos( Pos );
	RadioButtonParams.setSize( Size );
	RadioButtonParams.Flags = Flags;
	UIRadioButton * Ctrl = eeNew( UIRadioButton, ( RadioButtonParams ) );
	Ctrl->visible( true );
	Ctrl->enabled( true );
	return Ctrl;
}

UITextBox * UITheme::createTextBox( const String& Text, UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags ) {
	UITextBox::CreateParams TextBoxParams;
	TextBoxParams.setParent( Parent );
	TextBoxParams.setPos( Pos );
	TextBoxParams.setSize( Size );
	TextBoxParams.Flags = Flags;
	UITextBox * Ctrl = eeNew( UITextBox, ( TextBoxParams ) );
	Ctrl->visible( true );
	Ctrl->enabled( false );
	Ctrl->text( Text );
	return Ctrl;
}

UITooltip * UITheme::createTooltip( UIControl * TooltipOf, UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags ) {
	UITooltip::CreateParams TooltipParams;
	TooltipParams.setParent( Parent );
	TooltipParams.setPos( Pos );
	TooltipParams.setSize( Size );
	TooltipParams.Flags = Flags;
	UITooltip * Ctrl = eeNew( UITooltip, ( TooltipParams, TooltipOf ) );
	Ctrl->visible( true );
	Ctrl->enabled( true );
	return Ctrl;
}

UITextEdit * UITheme::createTextEdit( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, UI_SCROLLBAR_MODE HScrollBar, UI_SCROLLBAR_MODE VScrollBar, bool WordWrap ) {
	UITextEdit::CreateParams TextEditParams;
	TextEditParams.setParent( Parent );
	TextEditParams.setPos( Pos );
	TextEditParams.setSize( Size );
	TextEditParams.Flags = Flags;
	TextEditParams.HScrollBar = HScrollBar;
	TextEditParams.VScrollBar = VScrollBar;
	TextEditParams.WordWrap = WordWrap;
	UITextEdit * Ctrl = eeNew( UITextEdit, ( TextEditParams ) );
	Ctrl->visible( true );
	Ctrl->enabled( true );
	return Ctrl;
}

UITextInput * UITheme::createTextInput( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, bool SupportFreeEditing, Uint32 MaxLength ) {
	UITextInput::CreateParams TextInputParams;
	TextInputParams.setParent( Parent );
	TextInputParams.setPos( Pos );
	TextInputParams.setSize( Size );
	TextInputParams.Flags = Flags;
	TextInputParams.SupportFreeEditing = SupportFreeEditing;
	TextInputParams.MaxLength = MaxLength;
	UITextInput * Ctrl = eeNew( UITextInput, ( TextInputParams ) );
	Ctrl->visible( true );
	Ctrl->enabled( true );
	return Ctrl;
}

UITextInputPassword * UITheme::createTextInputPassword( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, bool SupportFreeEditing, Uint32 MaxLength ) {
	UITextInput::CreateParams TextInputParams;
	TextInputParams.setParent( Parent );
	TextInputParams.setPos( Pos );
	TextInputParams.setSize( Size );
	TextInputParams.Flags = Flags;
	TextInputParams.SupportFreeEditing = SupportFreeEditing;
	TextInputParams.MaxLength = MaxLength;
	UITextInputPassword * Ctrl = eeNew( UITextInputPassword, ( TextInputParams ) );
	Ctrl->visible( true );
	Ctrl->enabled( true );
	return Ctrl;
}

UISpinBox * UITheme::createSpinBox( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, Float DefaultValue, bool AllowDotsInNumbers ) {
	UISpinBox::CreateParams SpinBoxParams;
	SpinBoxParams.setParent( Parent );
	SpinBoxParams.setPos( Pos );
	SpinBoxParams.setSize( Size );
	SpinBoxParams.Flags = Flags;
	SpinBoxParams.DefaultValue = DefaultValue;
	SpinBoxParams.AllowDotsInNumbers = AllowDotsInNumbers;
	UISpinBox * Ctrl = eeNew( UISpinBox, ( SpinBoxParams ) );
	Ctrl->visible( true );
	Ctrl->enabled( true );
	return Ctrl;
}

UIScrollBar * UITheme::createScrollBar( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, bool VerticalScrollBar ) {
	UIScrollBar::CreateParams ScrollBarParams;
	ScrollBarParams.setParent( Parent );
	ScrollBarParams.setPos( Pos );
	ScrollBarParams.setSize( Size );
	ScrollBarParams.Flags = Flags;
	ScrollBarParams.VerticalScrollBar = VerticalScrollBar;
	UIScrollBar * Ctrl = eeNew( UIScrollBar, ( ScrollBarParams ) );
	Ctrl->visible( true );
	Ctrl->enabled( true );
	return Ctrl;
}

UISlider * UITheme::createSlider( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, bool VerticalSlider, bool AllowHalfSliderOut, bool ExpandBackground ) {
	UISlider::CreateParams SliderParams;
	SliderParams.setParent( Parent );
	SliderParams.setPos( Pos );
	SliderParams.setSize( Size );
	SliderParams.Flags = Flags;
	SliderParams.VerticalSlider = VerticalSlider;
	SliderParams.AllowHalfSliderOut = AllowHalfSliderOut;
	SliderParams.ExpandBackground = ExpandBackground;
	UISlider * Ctrl = eeNew( UISlider, ( SliderParams ) );
	Ctrl->visible( true );
	Ctrl->enabled( true );
	return Ctrl;
}

UIComboBox * UITheme::createComboBox( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, Uint32 MinNumVisibleItems, bool PopUpToMainControl, UIListBox * ListBox ) {
	UIComboBox::CreateParams ComboParams;
	ComboParams.setParent( Parent );
	ComboParams.setPos( Pos );
	ComboParams.setSize( Size );
	ComboParams.Flags = Flags;
	ComboParams.MinNumVisibleItems = MinNumVisibleItems;
	ComboParams.PopUpToMainControl = PopUpToMainControl;
	ComboParams.ListBox = ListBox;
	UIComboBox * Ctrl = eeNew( UIComboBox, ( ComboParams ) );
	Ctrl->visible( true );
	Ctrl->enabled( true );
	return Ctrl;
}

UIDropDownList * UITheme::createDropDownList( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, Uint32 MinNumVisibleItems, bool PopUpToMainControl, UIListBox * ListBox ) {
	UIDropDownList::CreateParams DDLParams;
	DDLParams.setParent( Parent );
	DDLParams.setPos( Pos );
	DDLParams.setSize( Size );
	DDLParams.Flags = Flags;
	DDLParams.MinNumVisibleItems = MinNumVisibleItems;
	DDLParams.PopUpToMainControl = PopUpToMainControl;
	DDLParams.ListBox = ListBox;
	UIDropDownList * Ctrl = eeNew( UIDropDownList, ( DDLParams ) );
	Ctrl->visible( true );
	Ctrl->enabled( true );
	return Ctrl;
}

UIListBox * UITheme::createListBox( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, bool SmoothScroll, Uint32 RowHeight, UI_SCROLLBAR_MODE VScrollMode, UI_SCROLLBAR_MODE HScrollMode, Recti PaddingContainer ) {
	UIListBox::CreateParams LBParams;
	LBParams.setParent( Parent );
	LBParams.setPos( Pos );
	LBParams.setSize( Size );
	LBParams.Flags = Flags;
	LBParams.SmoothScroll = SmoothScroll;
	LBParams.RowHeight = RowHeight;
	LBParams.VScrollMode = VScrollMode;
	LBParams.HScrollMode = HScrollMode;
	LBParams.PaddingContainer = PaddingContainer;
	UIListBox * Ctrl = eeNew( UIListBox, ( LBParams ) );
	Ctrl->visible( true );
	Ctrl->enabled( true );
	return Ctrl;
}

UIMenu * UITheme::createMenu( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, Uint32 RowHeight, Recti PaddingContainer, Uint32 MinWidth, Uint32 MinSpaceForIcons, Uint32 MinRightMargin ) {
	UIMenu::CreateParams MenuParams;
	MenuParams.setParent( Parent );
	MenuParams.setPos( Pos );
	MenuParams.setSize( Size );
	MenuParams.Flags = Flags;
	MenuParams.RowHeight = RowHeight;
	MenuParams.PaddingContainer = PaddingContainer;
	MenuParams.MinWidth = MinWidth;
	MenuParams.MinSpaceForIcons = MinSpaceForIcons;
	MenuParams.MinRightMargin = MinRightMargin;

	UIMenu * Ctrl = eeNew( UIMenu, ( MenuParams ) );
	Ctrl->visible( true );
	Ctrl->enabled( true );
	return Ctrl;
}

UIPopUpMenu * UITheme::createPopUpMenu( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, Uint32 RowHeight, Recti PaddingContainer, Uint32 MinWidth, Uint32 MinSpaceForIcons, Uint32 MinRightMargin ) {
	UIPopUpMenu::CreateParams MenuParams;
	MenuParams.setParent( Parent );
	MenuParams.setPos( Pos );
	MenuParams.setSize( Size );
	MenuParams.Flags = Flags;
	MenuParams.RowHeight = RowHeight;
	MenuParams.PaddingContainer = PaddingContainer;
	MenuParams.MinWidth = MinWidth;
	MenuParams.MinSpaceForIcons = MinSpaceForIcons;
	MenuParams.MinRightMargin = MinRightMargin;
	return eeNew( UIPopUpMenu, ( MenuParams ) );
}

UIProgressBar * UITheme::createProgressBar( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, bool DisplayPercent, bool VerticalExpand, Vector2f MovementSpeed, Rectf FillerMargin ) {
	UIProgressBar::CreateParams PBParams;
	PBParams.setParent( Parent );
	PBParams.setPos( Pos );
	PBParams.setSize( Size );
	PBParams.Flags = Flags;
	PBParams.DisplayPercent = DisplayPercent;
	PBParams.VerticalExpand = VerticalExpand;
	PBParams.MovementSpeed = MovementSpeed;
	PBParams.FillerMargin = FillerMargin;

	UIProgressBar * Ctrl = eeNew( UIProgressBar, ( PBParams ) );
	Ctrl->visible( true );
	Ctrl->enabled( true );
	return Ctrl;
}

UIPushButton * UITheme::createPushButton( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, SubTexture * Icon, Int32 IconHorizontalMargin, bool IconAutoMargin ) {
	UIPushButton::CreateParams ButtonParams;
	ButtonParams.setParent( Parent );
	ButtonParams.setPos( Pos );
	ButtonParams.setSize( Size );
	ButtonParams.Flags = Flags;
	ButtonParams.Icon = Icon;
	ButtonParams.IconHorizontalMargin = IconHorizontalMargin;
	ButtonParams.IconAutoMargin = IconAutoMargin;

	if ( NULL != Icon )
		ButtonParams.SetIcon( Icon );

	UIPushButton * Ctrl = eeNew( UIPushButton, ( ButtonParams ) );
	Ctrl->visible( true );
	Ctrl->enabled( true );
	return Ctrl;
}

UISelectButton * UITheme::createSelectButton( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, SubTexture * Icon, Int32 IconHorizontalMargin, bool IconAutoMargin ) {
	UIPushButton::CreateParams ButtonParams;
	ButtonParams.setParent( Parent );
	ButtonParams.setPos( Pos );
	ButtonParams.setSize( Size );
	ButtonParams.Flags = Flags;
	ButtonParams.Icon = Icon;
	ButtonParams.IconHorizontalMargin = IconHorizontalMargin;
	ButtonParams.IconAutoMargin = IconAutoMargin;

	if ( NULL != Icon )
		ButtonParams.SetIcon( Icon );

	UISelectButton * Ctrl = eeNew( UISelectButton, ( ButtonParams ) );
	Ctrl->visible( true );
	Ctrl->enabled( true );
	return Ctrl;
}

UIWinMenu * UITheme::createWinMenu( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, Uint32 MarginBetweenButtons, Uint32 ButtonMargin, Uint32 MenuHeight, Uint32 FirstButtonMargin ) {
	UIWinMenu::CreateParams WinMenuParams;
	WinMenuParams.setParent( Parent );
	WinMenuParams.setPos( Pos );
	WinMenuParams.setSize( Size );
	WinMenuParams.Flags = Flags;
	WinMenuParams.MarginBetweenButtons = MarginBetweenButtons;
	WinMenuParams.ButtonMargin = ButtonMargin;
	WinMenuParams.MenuHeight = MenuHeight;
	WinMenuParams.FirstButtonMargin = FirstButtonMargin;

	UIWinMenu * Ctrl = eeNew( UIWinMenu, ( WinMenuParams ) );
	Ctrl->visible( true );
	Ctrl->enabled( true );
	return Ctrl;
}

UIWindow * UITheme::createWindow( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, Uint32 WinFlags, Sizei MinWindowSize, Uint8 BaseAlpha ) {
	UIWindow::CreateParams WinParams;
	WinParams.setParent( Parent );
	WinParams.setPos( Pos );
	WinParams.setSize( Size );
	WinParams.Flags = Flags;
	WinParams.WinFlags = WinFlags;
	WinParams.MinWindowSize = MinWindowSize;
	WinParams.BaseAlpha = BaseAlpha;
	return eeNew( UIWindow, ( WinParams ) );
}

UICommonDialog * UITheme::createCommonDialog( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, Uint32 WinFlags, Sizei MinWindowSize, Uint8 BaseAlpha, Uint32 CDLFlags, std::string DefaultFilePattern, std::string DefaultDirectory ) {
	UICommonDialog::CreateParams DLGParams;
	DLGParams.setParent( Parent );
	DLGParams.setPos( Pos );
	DLGParams.setSize( Size );
	DLGParams.Flags = Flags;
	DLGParams.WinFlags = WinFlags;
	DLGParams.MinWindowSize = MinWindowSize;
	DLGParams.BaseAlpha = BaseAlpha;
	DLGParams.DefaultDirectory = DefaultDirectory;
	DLGParams.DefaultFilePattern = DefaultFilePattern;
	DLGParams.CDLFlags = CDLFlags;
	return eeNew( UICommonDialog, ( DLGParams ) );
}

UIMessageBox * UITheme::createMessageBox( UI_MSGBOX_TYPE Type, const String& Message, Uint32 WinFlags, UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, Sizei MinWindowSize, Uint8 BaseAlpha ) {
	UIMessageBox::CreateParams MsgBoxParams;
	MsgBoxParams.setParent( Parent );
	MsgBoxParams.setPos( Pos );
	MsgBoxParams.setSize( Size );
	MsgBoxParams.Flags = Flags;
	MsgBoxParams.WinFlags = WinFlags;
	MsgBoxParams.MinWindowSize = MinWindowSize;
	MsgBoxParams.BaseAlpha = BaseAlpha;
	MsgBoxParams.Type = Type;
	MsgBoxParams.Message = Message;
	return eeNew( UIMessageBox, ( MsgBoxParams ) );
}

UITabWidget * UITheme::createTabWidget( UIControl *Parent, const Sizei &Size, const Vector2i &Pos, const Uint32 &Flags, const bool &TabsClosable, const bool &SpecialBorderTabs, const Int32 &TabSeparation, const Uint32 &MaxTextLength, const Uint32 &TabWidgetHeight, const Uint32 &TabTextAlign, const Uint32 &MinTabWidth, const Uint32 &MaxTabWidth ) {
	UITabWidget::CreateParams TabWidgetParams;
	TabWidgetParams.setParent( Parent );
	TabWidgetParams.setPos( Pos );
	TabWidgetParams.setSize( Size );
	TabWidgetParams.Flags = Flags;
	TabWidgetParams.TabsClosable = TabsClosable;
	TabWidgetParams.SpecialBorderTabs = SpecialBorderTabs;
	TabWidgetParams.TabSeparation = TabSeparation;
	TabWidgetParams.MaxTextLength = MaxTextLength;
	TabWidgetParams.TabWidgetHeight = TabWidgetHeight;
	TabWidgetParams.TabTextAlign = TabTextAlign;
	TabWidgetParams.MinTabWidth = MinTabWidth;
	TabWidgetParams.MaxTabWidth = MaxTabWidth;

	UITabWidget * Ctrl = eeNew( UITabWidget, ( TabWidgetParams ) );
	Ctrl->visible( true );
	Ctrl->enabled( true );
	return Ctrl;
}

}}
