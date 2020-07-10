#include <eepp/graphics/drawablesearcher.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>

namespace EE { namespace UI {

UIPushButton* UIPushButton::New() {
	return eeNew( UIPushButton, () );
}

UIPushButton* UIPushButton::NewWithTag( const std::string& tag ) {
	return eeNew( UIPushButton, ( tag ) );
}

UIPushButton::UIPushButton( const std::string& tag ) :
	UIWidget( tag ), mIcon( NULL ), mTextBox( NULL ) {
	mFlags |= ( UI_AUTO_SIZE | UI_VALIGN_CENTER | UI_HALIGN_CENTER );

	mIcon = UIImage::NewWithTag( "pushbutton::icon" );
	mIcon->setScaleType( UIScaleType::FitInside )
		->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed )
		->setFlags( UI_VALIGN_CENTER | UI_HALIGN_CENTER )
		->setParent( this )
		->setVisible( true )
		->setEnabled( false );

	auto cb = [&]( const Event* ) { onSizeChange(); };

	mIcon->addEventListener( Event::OnPaddingChange, cb );
	mIcon->addEventListener( Event::OnMarginChange, cb );
	mIcon->addEventListener( Event::OnSizeChange, cb );

	mTextBox = UITextView::NewWithTag( "pushbutton::text" );
	mTextBox->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent )
		->setFlags( UI_VALIGN_CENTER | UI_HALIGN_CENTER )
		->setParent( this )
		->setVisible( true )
		->setEnabled( false );
	mTextBox->addEventListener( Event::OnFontChanged, cb );
	mTextBox->addEventListener( Event::OnFontStyleChanged, cb );
	mTextBox->addEventListener( Event::OnTextChanged, cb );
	mTextBox->addEventListener( Event::OnVisibleChange, cb );

	if ( NULL != getExtraInnerWidget() ) {
		getExtraInnerWidget()->addEventListener( Event::OnPaddingChange, cb );
		getExtraInnerWidget()->addEventListener( Event::OnMarginChange, cb );
		getExtraInnerWidget()->addEventListener( Event::OnSizeChange, cb );
	}

	onSizeChange();

	applyDefaultTheme();
}

UIPushButton::UIPushButton() : UIPushButton( "pushbutton" ) {}

UIPushButton::~UIPushButton() {}

Uint32 UIPushButton::getType() const {
	return UI_TYPE_PUSHBUTTON;
}

bool UIPushButton::isType( const Uint32& type ) const {
	return UIPushButton::getType() == type ? true : UIWidget::isType( type );
}

void UIPushButton::onAutoSize() {
	if ( ( ( mFlags & UI_AUTO_SIZE ) && 0 == getSize().getHeight() ) ||
		 mHeightPolicy == SizePolicy::WrapContent ) {
		Float sH = getSkinSize().getHeight();
		Float sHS = getSkinSize( UIState::StateFlagSelected ).getHeight();
		Float tH = mTextBox->getPixelsSize().getHeight();
		Float eH =
			NULL != getExtraInnerWidget() ? getExtraInnerWidget()->getPixelsSize().getHeight() : 0;
		Float h = eemax( eemax( PixelDensity::dpToPx( eemax( sH, sHS ) ), tH ), eH );
		setInternalPixelsHeight( h + mRealPadding.Top + mRealPadding.Bottom );
	} else if ( ( mFlags & UI_AUTO_SIZE ) && NULL != getSkin() ) {
		setInternalHeight( getSkinSize().getHeight() );
	}

	if ( ( mFlags & UI_AUTO_SIZE ) || mWidthPolicy == SizePolicy::WrapContent ) {
		Int32 txtW = mTextBox->getPixelsSize().getWidth();
		Int32 iconSize = mIcon->getPixelsSize().getWidth() > 0
							 ? mIcon->getPixelsSize().getWidth() +
								   PixelDensity::dpToPxI( mIcon->getLayoutMargin().Left +
														  mIcon->getLayoutMargin().Right )
							 : 0;

		UIWidget* eWidget = getExtraInnerWidget();
		Int32 eWidgetSize = NULL != eWidget
								? PixelDensity::dpToPxI( eWidget->getSize().getWidth() +
														 eWidget->getLayoutMargin().Left +
														 eWidget->getLayoutMargin().Right )
								: 0;

		Int32 minSize =
			txtW + iconSize + eWidgetSize + mRealPadding.Left + mRealPadding.Right +
			( NULL != getSkin() ? PixelDensity::dpToPxI( getSkin()->getBorderSize().Left +
														 getSkin()->getBorderSize().Right )
								: 0 );

		if ( minSize > mSize.getWidth() ) {
			setInternalPixelsWidth( minSize );
		}
	}
}

void UIPushButton::onPaddingChange() {
	onSizeChange();

	UIWidget::onPaddingChange();
}

void UIPushButton::onSizeChange() {
	onAutoSize();

	Rectf autoPadding;

	if ( mFlags & UI_AUTO_PADDING ) {
		autoPadding = makePadding( true, true, true, true );
	}

	if ( mRealPadding.Top > autoPadding.Top )
		autoPadding.Top = mRealPadding.Top;
	if ( mRealPadding.Bottom > autoPadding.Bottom )
		autoPadding.Bottom = mRealPadding.Bottom;
	if ( mRealPadding.Left > autoPadding.Left )
		autoPadding.Left = mRealPadding.Left;
	if ( mRealPadding.Right > autoPadding.Right )
		autoPadding.Right = mRealPadding.Right;

	Vector2f position;
	Vector2f iconPos;
	Vector2f ePos;
	UIWidget* eWidget = getExtraInnerWidget();

	Float textBoxWidth = mTextBox->getPixelsSize().getWidth();
	Float iconWidth = mIcon->getPixelsSize().getWidth() > 0
						  ? mIcon->getPixelsSize().getWidth() +
								PixelDensity::dpToPxI( mIcon->getLayoutMargin().Left +
													   mIcon->getLayoutMargin().Right )
						  : 0;
	Float eWidth =
		NULL != eWidget && eWidget->isVisible() && eWidget->getPixelsSize().getWidth() > 0
			? eWidget->getPixelsSize().getWidth() +
				  PixelDensity::dpToPxI( eWidget->getLayoutMargin().Left +
										 eWidget->getLayoutMargin().Right )
			: 0;

	if ( iconWidth > 0 && textBoxWidth <= 0 &&
		 Font::getHorizontalAlign( getFlags() ) == UI_HALIGN_CENTER && eWidth == 0 &&
		 mIcon->getLayoutMargin().Left != mIcon->getLayoutMargin().Right ) {
		iconWidth = mIcon->getPixelsSize().getWidth();
	}

	Float totalWidth = textBoxWidth + iconWidth + eWidth;

	switch ( Font::getVerticalAlign( getFlags() ) ) {
		case UI_VALIGN_CENTER:
			iconPos.y =
				eefloor( ( mSize.getHeight() - mIcon->getPixelsSize().getHeight() ) * 0.5f );
			position.y =
				eefloor( ( mSize.getHeight() - mTextBox->getPixelsSize().getHeight() ) * 0.5f );
			ePos.y =
				NULL != eWidget && eWidget->isVisible()
					? eefloor( ( mSize.getHeight() - eWidget->getPixelsSize().getHeight() ) * 0.5f )
					: 0;
			break;
		case UI_VALIGN_BOTTOM:
			iconPos.y = mSize.y - mIcon->getPixelsSize().getHeight() - autoPadding.Bottom;
			position.y = mSize.y - mTextBox->getPixelsSize().getHeight() - autoPadding.Bottom;
			ePos.y = NULL != eWidget && eWidget->isVisible()
						 ? mSize.y - eWidget->getPixelsSize().getHeight() - autoPadding.Bottom
						 : 0;
			break;
		case UI_VALIGN_TOP:
			iconPos.y = autoPadding.Top;
			position.y = autoPadding.Top;
			ePos.y = autoPadding.Top;
			break;
	}

	switch ( Font::getHorizontalAlign( getFlags() ) ) {
		case UI_HALIGN_RIGHT:
			position.x = mSize.getWidth() - autoPadding.Right;
			ePos.x = position.x;

			if ( NULL != eWidget && eWidget->isVisible() ) {
				ePos.x = position.x - PixelDensity::dpToPx( eWidget->getLayoutMargin().Right ) -
						 eWidget->getPixelsSize().getWidth();
				position.x = ePos.x - PixelDensity::dpToPx( eWidget->getLayoutMargin().Left );
			}

			position.x -= textBoxWidth;

			iconPos.x = position.x - PixelDensity::dpToPxI( mIcon->getLayoutMargin().Right ) -
						mIcon->getPixelsSize().getWidth();
			break;
		case UI_HALIGN_CENTER:
			position.x = ( mSize.getWidth() - totalWidth ) / 2 + iconWidth;
			iconPos.x = ( mSize.getWidth() - totalWidth ) / 2 +
						PixelDensity::dpToPxI( mIcon->getLayoutMargin().Left );

			if ( NULL != eWidget && eWidget->isVisible() ) {
				ePos.x = position.x + textBoxWidth +
						 PixelDensity::dpToPx( eWidget->getLayoutMargin().Left );
			}
			break;
		case UI_HALIGN_LEFT:
			position.x = autoPadding.Left + iconWidth;
			iconPos.x = autoPadding.Left + PixelDensity::dpToPxI( mIcon->getLayoutMargin().Left );

			if ( NULL != eWidget && eWidget->isVisible() ) {
				ePos.x = position.x + textBoxWidth +
						 PixelDensity::dpToPx( eWidget->getLayoutMargin().Left );
			}
			break;
	}

	mTextBox->setPixelsPosition( position );
	mIcon->setPixelsPosition( iconPos );
	if ( NULL != eWidget && eWidget->isVisible() ) {
		eWidget->setPixelsPosition( ePos );
	}

	UIWidget::onSizeChange();
}

void UIPushButton::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );
	setThemeSkin( Theme, "button" );

	onThemeLoaded();
}

void UIPushButton::onThemeLoaded() {
	onSizeChange();

	UIWidget::onThemeLoaded();
}

UIPushButton* UIPushButton::setIcon( Drawable* Icon ) {
	if ( mIcon->getDrawable() != Icon ) {
		mIcon->setDrawable( Icon );
		onSizeChange();
	}
	return this;
}

UIImage* UIPushButton::getIcon() const {
	return mIcon;
}

UIPushButton* UIPushButton::setText( const String& text ) {
	mTextBox->setText( text );
	onSizeChange();
	return this;
}

const String& UIPushButton::getText() {
	return mTextBox->getText();
}

UITextView* UIPushButton::getTextBox() const {
	return mTextBox;
}

void UIPushButton::onAlphaChange() {
	UIWidget::onAlphaChange();

	mIcon->setAlpha( mAlpha );
	mTextBox->setAlpha( mAlpha );

	if ( NULL != getExtraInnerWidget() ) {
		getExtraInnerWidget()->setAlpha( mAlpha );
	}
}

void UIPushButton::onStateChange() {
	mTextBox->setAlpha( mAlpha );

	UIWidget::onStateChange();
}

void UIPushButton::onAlignChange() {
	UIWidget::onAlignChange();

	mTextBox->setHorizontalAlign( getHorizontalAlign() );
	mTextBox->setVerticalAlign( getVerticalAlign() );
}

Uint32 UIPushButton::onKeyDown( const KeyEvent& Event ) {
	if ( Event.getKeyCode() == KEY_RETURN ) {
		NodeMessage Msg( this, NodeMessage::MouseClick, EE_BUTTON_LMASK );
		messagePost( &Msg );
		onMouseClick( Vector2i( 0, 0 ), EE_BUTTON_LMASK );
		pushState( UIState::StatePressed );
	}

	return UIWidget::onKeyDown( Event );
}

Uint32 UIPushButton::onKeyUp( const KeyEvent& Event ) {
	if ( Event.getKeyCode() == KEY_RETURN ) {
		popState( UIState::StatePressed );
	}

	return UIWidget::onKeyUp( Event );
}

void UIPushButton::setIconMinimumSize( const Sizei& minIconSize ) {
	if ( minIconSize != mIconMinSize ) {
		mIconMinSize = minIconSize;

		if ( mIconMinSize.x != 0 && mIconMinSize.y != 0 ) {
			mIcon->setMinSizeEq( String::fromFloat( mIconMinSize.x, "dp" ),
								 String::fromFloat( mIconMinSize.y, "dp" ) );
		}
	}
}

const Sizei& UIPushButton::getIconMinimumSize() const {
	return mIconMinSize;
}

UIWidget* UIPushButton::getExtraInnerWidget() const {
	return NULL;
}

void UIPushButton::setTextAlign( const Uint32& align ) {
	mFlags &= ~( UI_HALIGN_CENTER | UI_HALIGN_RIGHT );
	mFlags |= align;
	onAlignChange();
}

Sizef UIPushButton::getContentSize() const {
	Float sH = getSkinSize().getHeight();
	Float sHS = getSkinSize( UIState::StateFlagSelected ).getHeight();
	Float tH = mTextBox->getPixelsSize().getHeight();
	Float eH =
		NULL != getExtraInnerWidget() ? getExtraInnerWidget()->getPixelsSize().getHeight() : 0;
	Float minHeight = eeceil( eemax( eemax( PixelDensity::dpToPx( eemax( sH, sHS ) ), tH ), eH ) );
	Int32 txtW = mTextBox->getPixelsSize().getWidth();
	Int32 iconSize = mIcon->getPixelsSize().getWidth() > 0
						 ? mIcon->getPixelsSize().getWidth() +
							   PixelDensity::dpToPxI( mIcon->getLayoutMargin().Left +
													  mIcon->getLayoutMargin().Right )
						 : 0;
	UIWidget* eWidget = getExtraInnerWidget();
	Int32 eWidgetSize = NULL != eWidget ? PixelDensity::dpToPxI( eWidget->getSize().getWidth() +
																 eWidget->getLayoutMargin().Left +
																 eWidget->getLayoutMargin().Right )
										: 0;
	Int32 minWidth = txtW + iconSize + eWidgetSize + mRealPadding.Left + mRealPadding.Right +
					 ( NULL != getSkin() ? PixelDensity::dpToPxI( getSkin()->getBorderSize().Left +
																  getSkin()->getBorderSize().Right )
										 : 0 );
	return Sizef( minWidth, minHeight );
}

std::string UIPushButton::getPropertyString( const PropertyDefinition* propertyDef,
											 const Uint32& propertyIndex ) {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Text:
			return getText().toUtf8();
		case PropertyId::Icon:
			// TODO: Implement icon
			return "";
		case PropertyId::MinIconSize:
			return String::format( "%ddp", mIconMinSize.getWidth() ) + " " +
				   String::format( "%ddp", mIconMinSize.getHeight() );
		case PropertyId::TextAlign:
			return Font::getHorizontalAlign( getFlags() ) == UI_HALIGN_CENTER
					   ? "center"
					   : ( Font::getHorizontalAlign( getFlags() ) == UI_HALIGN_RIGHT ? "right"
																					 : "left" );
		case PropertyId::Color:
		case PropertyId::ShadowColor:
		case PropertyId::SelectionColor:
		case PropertyId::SelectionBackColor:
		case PropertyId::FontFamily:
		case PropertyId::FontSize:
		case PropertyId::FontStyle:
		case PropertyId::Wordwrap:
		case PropertyId::TextStrokeWidth:
		case PropertyId::TextStrokeColor:
		case PropertyId::TextSelection:
			return mTextBox->getPropertyString( propertyDef, propertyIndex );
		default:
			return UIWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

bool UIPushButton::applyProperty( const StyleSheetProperty& attribute ) {
	bool attributeSet = true;

	if ( attribute.getPropertyDefinition() == NULL ) {
		return false;
	}

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Text:
			if ( NULL != mSceneNode && mSceneNode->isUISceneNode() )
				setText( static_cast<UISceneNode*>( mSceneNode )
							 ->getTranslatorString( attribute.asString() ) );
			break;
		case PropertyId::Icon: {
			std::string val = attribute.asString();
			Drawable* icon = NULL;

			if ( NULL != mTheme && NULL != ( icon = getUISceneNode()->findIcon( val ) ) ) {
				setIcon( icon );
			} else if ( NULL != ( icon = DrawableSearcher::searchByName( val ) ) ) {
				setIcon( icon );
			}
			break;
		}
		case PropertyId::MinIconSize:
			setIconMinimumSize( attribute.asSizei() );
			break;
		case PropertyId::TextAlign: {
			std::string align = String::toLower( attribute.value() );
			if ( align == "center" )
				setTextAlign( UI_HALIGN_CENTER );
			else if ( align == "left" )
				setTextAlign( UI_HALIGN_LEFT );
			else if ( align == "right" )
				setTextAlign( UI_HALIGN_RIGHT );
			break;
		}
		case PropertyId::Color:
		case PropertyId::ShadowColor:
		case PropertyId::SelectionColor:
		case PropertyId::SelectionBackColor:
		case PropertyId::FontFamily:
		case PropertyId::FontSize:
		case PropertyId::FontStyle:
		case PropertyId::Wordwrap:
		case PropertyId::TextStrokeWidth:
		case PropertyId::TextStrokeColor:
		case PropertyId::TextSelection:
			attributeSet = mTextBox->applyProperty( attribute );
			break;
		default:
			attributeSet = UIWidget::applyProperty( attribute );
			break;
	}

	return attributeSet;
}

}} // namespace EE::UI
