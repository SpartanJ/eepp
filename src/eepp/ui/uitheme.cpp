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

static void loadThemeElements( std::list<std::string>& UI_THEME_ELEMENTS, std::list<std::string>& UI_THEME_ICONS ) {
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
		UI_THEME_ELEMENTS.push_back( "combobox_button" );
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
		TextureAtlas->getTexture( tC )->setFilter( TEX_FILTER_NEAREST );
	}

	Clock TE;

	loadThemeElements( tTheme->mUIElements, tTheme->mUIIcons );

	Uint32 i;
	bool Found;
	std::string Element;
	std::vector<std::string> 	ElemFound;
	std::vector<Uint32> 		ElemType;

	tTheme->getTextureAtlas( TextureAtlas );

	for ( std::list<std::string>::iterator it = tTheme->mUIElements.begin() ; it != tTheme->mUIElements.end(); it++ ) {
		Uint32 IsComplex = 0;

		Element = std::string( tTheme->getAbbr() + "_" + *it );

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

	loadThemeElements( tTheme->mUIElements, tTheme->mUIIcons );

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

	Graphics::TextureAtlas * tSG = eeNew( Graphics::TextureAtlas, ( tTheme->getAbbr() ) );

	tTheme->getTextureAtlas( tSG );

	for ( std::list<std::string>::iterator it = tTheme->mUIElements.begin() ; it != tTheme->mUIElements.end(); it++ ) {
		Uint32 IsComplex = 0;

		Element = tTheme->getAbbr() + "_" + *it;

		Found 	= searchFilesOfElement( tSG, RPath, Element, IsComplex, ImgExt );

		if ( Found ) {
			ElemFound.push_back( Element );
			ElemType.push_back( IsComplex );
		}
	}

	// Load the icons from path.
	for ( std::list<std::string>::iterator it = tTheme->mUIIcons.begin() ; it != tTheme->mUIIcons.end(); it++ ) {
		ElemName	= tTheme->getAbbr() + "_icon_" + *it;
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
			ElemName = Element + "_" + UISkin::getSkinStateName( i ) + "_" + UISkinComplex::getSideSuffix( s );

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
			ElemName = Element + "_" + UISkin::getSkinStateName( i ) + "_" + UISkinComplex::getSideSuffix( s );
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

UITheme::UITheme(const std::string& name, const std::string& Abbr, Graphics::Font * defaultFont ) :
	ResourceManager<UISkin> ( false ),
	mName( name ),
	mNameHash( String::hash( mName ) ),
	mAbbr( Abbr ),
	mTextureAtlas( NULL ),
	mUseDefaultThemeValues( true )
{
	mFontStyleConfig.font = defaultFont;
	mFontStyleConfig.fontShadowColor = ColorA( 255, 255, 255, 200 );
	mFontStyleConfig.fontColor = mFontStyleConfig.fontOverColor = mFontStyleConfig.fontSelectedColor = ColorA( 0, 0, 0, 255 );
	mFontStyleConfig.fontSelectionBackColor = ColorA( 150, 150, 150, 255 );

	if ( NULL == defaultFont ) {
		mFontStyleConfig.font = UIThemeManager::instance()->getDefaultFont();
	}
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

const std::string& UITheme::getAbbr() const {
	return mAbbr;
}

UISkin * UITheme::add( UISkin * Resource ) {
	Resource->setTheme( this );

	return ResourceManager<UISkin>::add( Resource );
}

void UITheme::setUseDefaultThemeValues( const bool& Use ) {
	mUseDefaultThemeValues = Use;
}

const bool& UITheme::getUseDefaultThemeValues() const {
	return mUseDefaultThemeValues;
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
	GfxParams.setPosition( Pos );
	GfxParams.setSize( Size );
	GfxParams.Flags = Flags;
	GfxParams.SubTexture = SubTexture;
	GfxParams.SubTextureColor = SubTextureColor;
	GfxParams.SubTextureRender = SubTextureRender;
	UIGfx * Gfx = eeNew( UIGfx, ( GfxParams ) );
	Gfx->setVisible( true );
	Gfx->setEnabled( true );
	return Gfx;
}

UISprite * UITheme::createSprite( Sprite * Sprite, UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, bool DealloSprite, EE_RENDER_MODE SpriteRender ) {
	UISprite::CreateParams SpriteParams;
	SpriteParams.setParent( Parent );
	SpriteParams.setPosition( Pos );
	SpriteParams.setSize( Size );
	SpriteParams.Flags = Flags;
	SpriteParams.Sprite = Sprite;
	SpriteParams.SpriteRender = SpriteRender;
	SpriteParams.DeallocSprite = DealloSprite;
	UISprite * Spr = eeNew( UISprite, ( SpriteParams ) );
	Spr->setVisible( true );
	Spr->setEnabled( true );
	return Spr;
}

UICheckBox * UITheme::createCheckBox( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags ) {
	UICheckBox::CreateParams CheckBoxParams;
	CheckBoxParams.setParent( Parent );
	CheckBoxParams.setPosition( Pos );
	CheckBoxParams.setSize( Size );
	CheckBoxParams.Flags = Flags;
	UICheckBox * Ctrl = eeNew( UICheckBox, ( CheckBoxParams ) );
	Ctrl->setVisible( true );
	Ctrl->setEnabled( true );
	return Ctrl;
}

UIRadioButton * UITheme::createRadioButton( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags ) {
	UIRadioButton::CreateParams RadioButtonParams;
	RadioButtonParams.setParent( Parent );
	RadioButtonParams.setPosition( Pos );
	RadioButtonParams.setSize( Size );
	RadioButtonParams.Flags = Flags;
	UIRadioButton * Ctrl = eeNew( UIRadioButton, ( RadioButtonParams ) );
	Ctrl->setVisible( true );
	Ctrl->setEnabled( true );
	return Ctrl;
}

UITextBox * UITheme::createTextBox( const String& Text, UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags ) {
	UITextBox::CreateParams TextBoxParams;
	TextBoxParams.setParent( Parent );
	TextBoxParams.setPosition( Pos );
	TextBoxParams.setSize( Size );
	TextBoxParams.Flags = Flags;
	UITextBox * Ctrl = eeNew( UITextBox, ( TextBoxParams ) );
	Ctrl->setVisible( true );
	Ctrl->setEnabled( false );
	Ctrl->setText( Text );
	return Ctrl;
}

UITextEdit * UITheme::createTextEdit( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, UI_SCROLLBAR_MODE HScrollBar, UI_SCROLLBAR_MODE VScrollBar, bool WordWrap ) {
	UITextEdit::CreateParams TextEditParams;
	TextEditParams.setParent( Parent );
	TextEditParams.setPosition( Pos );
	TextEditParams.setSize( Size );
	TextEditParams.Flags = Flags;
	TextEditParams.HScrollBar = HScrollBar;
	TextEditParams.VScrollBar = VScrollBar;
	TextEditParams.WordWrap = WordWrap;
	UITextEdit * Ctrl = eeNew( UITextEdit, ( TextEditParams ) );
	Ctrl->setVisible( true );
	Ctrl->setEnabled( true );
	return Ctrl;
}

UITextInput * UITheme::createTextInput( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, bool SupportFreeEditing, Uint32 MaxLength ) {
	UITextInput::CreateParams TextInputParams;
	TextInputParams.setParent( Parent );
	TextInputParams.setPosition( Pos );
	TextInputParams.setSize( Size );
	TextInputParams.Flags = Flags;
	TextInputParams.SupportFreeEditing = SupportFreeEditing;
	TextInputParams.MaxLength = MaxLength;
	UITextInput * Ctrl = eeNew( UITextInput, ( TextInputParams ) );
	Ctrl->setVisible( true );
	Ctrl->setEnabled( true );
	return Ctrl;
}

UITextInputPassword * UITheme::createTextInputPassword( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, bool SupportFreeEditing, Uint32 MaxLength ) {
	UITextInput::CreateParams TextInputParams;
	TextInputParams.setParent( Parent );
	TextInputParams.setPosition( Pos );
	TextInputParams.setSize( Size );
	TextInputParams.Flags = Flags;
	TextInputParams.SupportFreeEditing = SupportFreeEditing;
	TextInputParams.MaxLength = MaxLength;
	UITextInputPassword * Ctrl = eeNew( UITextInputPassword, ( TextInputParams ) );
	Ctrl->setVisible( true );
	Ctrl->setEnabled( true );
	return Ctrl;
}

UISpinBox * UITheme::createSpinBox( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, Float DefaultValue, bool AllowDotsInNumbers ) {
	UISpinBox::CreateParams SpinBoxParams;
	SpinBoxParams.setParent( Parent );
	SpinBoxParams.setPosition( Pos );
	SpinBoxParams.setSize( Size );
	SpinBoxParams.Flags = Flags;
	SpinBoxParams.DefaultValue = DefaultValue;
	SpinBoxParams.AllowDotsInNumbers = AllowDotsInNumbers;
	UISpinBox * Ctrl = eeNew( UISpinBox, ( SpinBoxParams ) );
	Ctrl->setVisible( true );
	Ctrl->setEnabled( true );
	return Ctrl;
}

UIScrollBar * UITheme::createScrollBar( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, bool VerticalScrollBar ) {
	UIScrollBar::CreateParams ScrollBarParams;
	ScrollBarParams.setParent( Parent );
	ScrollBarParams.setPosition( Pos );
	ScrollBarParams.setSize( Size );
	ScrollBarParams.Flags = Flags;
	ScrollBarParams.VerticalScrollBar = VerticalScrollBar;
	UIScrollBar * Ctrl = eeNew( UIScrollBar, ( ScrollBarParams ) );
	Ctrl->setVisible( true );
	Ctrl->setEnabled( true );
	return Ctrl;
}

UISlider * UITheme::createSlider( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, bool VerticalSlider, bool AllowHalfSliderOut, bool ExpandBackground ) {
	UISlider::CreateParams SliderParams;
	SliderParams.setParent( Parent );
	SliderParams.setPosition( Pos );
	SliderParams.setSize( Size );
	SliderParams.Flags = Flags;
	SliderParams.VerticalSlider = VerticalSlider;
	SliderParams.AllowHalfSliderOut = AllowHalfSliderOut;
	SliderParams.ExpandBackground = ExpandBackground;
	UISlider * Ctrl = eeNew( UISlider, ( SliderParams ) );
	Ctrl->setVisible( true );
	Ctrl->setEnabled( true );
	return Ctrl;
}

UIDropDownList * UITheme::createDropDownList( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, Uint32 MinNumVisibleItems, bool PopUpToMainControl, UIListBox * ListBox ) {
	UIDropDownList::CreateParams DDLParams;
	DDLParams.setParent( Parent );
	DDLParams.setPosition( Pos );
	DDLParams.setSize( Size );
	DDLParams.Flags = Flags;
	DDLParams.MinNumVisibleItems = MinNumVisibleItems;
	DDLParams.PopUpToMainControl = PopUpToMainControl;
	DDLParams.ListBox = ListBox;
	UIDropDownList * Ctrl = eeNew( UIDropDownList, ( DDLParams ) );
	Ctrl->setVisible( true );
	Ctrl->setEnabled( true );
	return Ctrl;
}

UIListBox * UITheme::createListBox( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, bool SmoothScroll, Uint32 RowHeight, UI_SCROLLBAR_MODE VScrollMode, UI_SCROLLBAR_MODE HScrollMode, Recti PaddingContainer ) {
	UIListBox::CreateParams LBParams;
	LBParams.setParent( Parent );
	LBParams.setPosition( Pos );
	LBParams.setSize( Size );
	LBParams.Flags = Flags;
	LBParams.SmoothScroll = SmoothScroll;
	LBParams.RowHeight = RowHeight;
	LBParams.VScrollMode = VScrollMode;
	LBParams.HScrollMode = HScrollMode;
	LBParams.PaddingContainer = PaddingContainer;
	UIListBox * Ctrl = eeNew( UIListBox, ( LBParams ) );
	Ctrl->setVisible( true );
	Ctrl->setEnabled( true );
	return Ctrl;
}

UIMenu * UITheme::createMenu( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, Recti PaddingContainer, Uint32 MinWidth, Uint32 MinSpaceForIcons, Uint32 MinRightMargin ) {
	UIMenu::CreateParams MenuParams;
	MenuParams.setParent( Parent );
	MenuParams.setPosition( Pos );
	MenuParams.setSize( Size );
	MenuParams.Flags = Flags;
	MenuParams.PaddingContainer = PaddingContainer;
	MenuParams.MinWidth = MinWidth;
	MenuParams.MinSpaceForIcons = MinSpaceForIcons;
	MenuParams.MinRightMargin = MinRightMargin;

	UIMenu * Ctrl = eeNew( UIMenu, ( MenuParams ) );
	Ctrl->setVisible( true );
	Ctrl->setEnabled( true );
	return Ctrl;
}

UIPopUpMenu * UITheme::createPopUpMenu( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, Recti PaddingContainer, Uint32 MinWidth, Uint32 MinSpaceForIcons, Uint32 MinRightMargin ) {
	UIPopUpMenu::CreateParams MenuParams;
	MenuParams.setParent( Parent );
	MenuParams.setPosition( Pos );
	MenuParams.setSize( Size );
	MenuParams.Flags = Flags;
	MenuParams.PaddingContainer = PaddingContainer;
	MenuParams.MinWidth = MinWidth;
	MenuParams.MinSpaceForIcons = MinSpaceForIcons;
	MenuParams.MinRightMargin = MinRightMargin;
	return eeNew( UIPopUpMenu, ( MenuParams ) );
}

UIPushButton * UITheme::createPushButton( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, SubTexture * Icon, Int32 IconHorizontalMargin, bool IconAutoMargin ) {
	UIPushButton::CreateParams ButtonParams;
	ButtonParams.setParent( Parent );
	ButtonParams.setPosition( Pos );
	ButtonParams.setSize( Size );
	ButtonParams.Flags = Flags;
	ButtonParams.Icon = Icon;
	ButtonParams.IconHorizontalMargin = IconHorizontalMargin;
	ButtonParams.IconAutoMargin = IconAutoMargin;

	if ( NULL != Icon )
		ButtonParams.setIcon( Icon );

	UIPushButton * Ctrl = eeNew( UIPushButton, ( ButtonParams ) );
	Ctrl->setVisible( true );
	Ctrl->setEnabled( true );
	return Ctrl;
}

UISelectButton * UITheme::createSelectButton( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, SubTexture * Icon, Int32 IconHorizontalMargin, bool IconAutoMargin ) {
	UIPushButton::CreateParams ButtonParams;
	ButtonParams.setParent( Parent );
	ButtonParams.setPosition( Pos );
	ButtonParams.setSize( Size );
	ButtonParams.Flags = Flags;
	ButtonParams.Icon = Icon;
	ButtonParams.IconHorizontalMargin = IconHorizontalMargin;
	ButtonParams.IconAutoMargin = IconAutoMargin;

	if ( NULL != Icon )
		ButtonParams.setIcon( Icon );

	UISelectButton * Ctrl = eeNew( UISelectButton, ( ButtonParams ) );
	Ctrl->setVisible( true );
	Ctrl->setEnabled( true );
	return Ctrl;
}

UIWindow * UITheme::createWindow( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, Uint32 WinFlags, Sizei MinWindowSize, Uint8 BaseAlpha ) {
	UIWindow::CreateParams WinParams;
	WinParams.setParent( Parent );
	WinParams.setPosition( Pos );
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
	DLGParams.setPosition( Pos );
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
	MsgBoxParams.setPosition( Pos );
	MsgBoxParams.setSize( Size );
	MsgBoxParams.Flags = Flags;
	MsgBoxParams.WinFlags = WinFlags;
	MsgBoxParams.MinWindowSize = MinWindowSize;
	MsgBoxParams.BaseAlpha = BaseAlpha;
	MsgBoxParams.Type = Type;
	MsgBoxParams.Message = Message;
	return eeNew( UIMessageBox, ( MsgBoxParams ) );
}

ColorA UITheme::getMenuFontColor() const {
	return mMenuFontColor;
}

void UITheme::setMenuFontColor(const ColorA & menuFontColor) {
	mMenuFontColor = menuFontColor;
}

ColorA UITheme::getMenuFontColorOver() const {
	return mMenuFontColorOver;
}

void UITheme::setMenuFontColorOver(const ColorA & menuFontColorOver) {
	mMenuFontColorOver = menuFontColorOver;
}

ColorA UITheme::getTooltipFontColor() const {
	return mTooltipFontColor;
}

void UITheme::setTooltipFontColor(const ColorA & tooltipFontColor) {
	mTooltipFontColor = tooltipFontColor;
}

Recti UITheme::getTooltipPadding() const {
	return mTooltipPadding;
}

void UITheme::setTooltipPadding(const Recti & tooltipPadding) {
	mTooltipPadding = tooltipPadding;
}

Int32 UITheme::getTabSeparation() const {
	return mTabSeparation;
}

void UITheme::setTabSeparation(const Int32 & tabSeparation) {
	mTabSeparation = tabSeparation;
}

void UITheme::setFontStyleConfig(const FontStyleConfig & fontConfig) {
	mFontStyleConfig = fontConfig;
}

TabWidgetStyleConfig UITheme::getTabWidgetStyleConfig() {
	return TabWidgetStyleConfig( getFontStyleConfig() );
}

ProgressBarStyleConfig UITheme::getProgressBarStyleConfig() {
	return ProgressBarStyleConfig( getFontStyleConfig() );;
}

WinMenuStyleConfig UITheme::getWinMenuStyleConfig() {
	return WinMenuStyleConfig( getFontStyleConfig() );
}

FontStyleConfig UITheme::getFontStyleConfig() const {
	return mFontStyleConfig;
}

}}
