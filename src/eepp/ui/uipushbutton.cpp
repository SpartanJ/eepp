#include <eepp/graphics/drawablesearcher.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uiicon.hpp>
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
	mIcon->addEventListener( Event::OnVisibleChange, cb );

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

	onSizeChange();

	applyDefaultTheme();
}

Rectf UIPushButton::calculatePadding() const {
	Rectf autoPadding;
	if ( mFlags & UI_AUTO_PADDING ) {
		autoPadding = makePadding( true, true, true, true );
		if ( autoPadding != Rectf() )
			autoPadding = PixelDensity::dpToPx( autoPadding );
	}
	if ( mPaddingPx.Top > autoPadding.Top )
		autoPadding.Top = mPaddingPx.Top;
	if ( mPaddingPx.Bottom > autoPadding.Bottom )
		autoPadding.Bottom = mPaddingPx.Bottom;
	if ( mPaddingPx.Left > autoPadding.Left )
		autoPadding.Left = mPaddingPx.Left;
	if ( mPaddingPx.Right > autoPadding.Right )
		autoPadding.Right = mPaddingPx.Right;
	return autoPadding;
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
		Float eH = getExtraInnerWidget() ? getExtraInnerWidget()->getPixelsSize().getHeight() : 0;
		Float h = eemax( eemax( PixelDensity::dpToPx( eemax( sH, sHS ) ), tH ), eH );
		setInternalPixelsHeight( h + mPaddingPx.Top + mPaddingPx.Bottom );
	} else if ( ( mFlags & UI_AUTO_SIZE ) && NULL != getSkin() ) {
		setInternalHeight( getSkinSize().getHeight() );
	}

	if ( ( mFlags & UI_AUTO_SIZE ) || mWidthPolicy == SizePolicy::WrapContent ) {
		Float minSize = getContentSize().getWidth();
		if ( minSize > mSize.getWidth() )
			setInternalPixelsWidth( minSize );
	}
}

Vector2f UIPushButton::calcLayoutSize( const std::vector<UIWidget*>& widgets,
									   const Rectf& padding ) const {
	Vector2f totSize{padding.Left, padding.Top + padding.Bottom};
	UIWidget* widget;
	for ( size_t i = 0; i < widgets.size(); i++ ) {
		if ( !widgets[i] || !widgets[i]->isVisible() )
			continue;
		widget = widgets[i];
		if ( widget->getPixelsSize().getWidth() > 0 ) {
			totSize.x += widget->getLayoutPixelsMargin().Left + widget->getPixelsSize().getWidth();
			if ( ( i + 1 < widgets.size() && widgets[i + 1] && widgets[i + 1]->isVisible() ) ||
				 ( i + 2 < widgets.size() && widgets[i + 2] && widgets[i + 2]->isVisible() ) ) {
				totSize.x += widget->getLayoutPixelsMargin().Right;
			}
		}
		totSize.y = eemax<Float>( totSize.y, padding.Top + padding.Bottom +
												 widget->getPixelsSize().getHeight() );
	}
	totSize.x += padding.Right;
	return totSize;
}

Vector2f UIPushButton::packLayout( const std::vector<UIWidget*>& widgets, const Rectf& padding ) {
	std::vector<Vector2f> pos( widgets.size() );
	Vector2f totSize{padding.Left, padding.Top + padding.Bottom};
	UIWidget* widget;
	for ( size_t i = 0; i < widgets.size(); i++ ) {
		if ( !widgets[i] || !widgets[i]->isVisible() )
			continue;
		widget = widgets[i];
		pos[i].x = totSize.x;
		switch ( Font::getVerticalAlign( getFlags() ) ) {
			case UI_VALIGN_CENTER:
				pos[i].y =
					eefloor( ( mSize.getHeight() - widget->getPixelsSize().getHeight() ) * 0.5f );
				break;
			case UI_VALIGN_BOTTOM:
				pos[i].y = mSize.y - widget->getPixelsSize().getHeight() - padding.Bottom;
				break;
			case UI_VALIGN_TOP:
				pos[i].y = padding.Top;
				break;
		}
		if ( 0 == i )
			pos[i].x += widget->getLayoutPixelsMargin().Left;
		if ( widget->getPixelsSize().getWidth() > 0 ) {
			totSize.x += widget->getLayoutPixelsMargin().Left + widget->getPixelsSize().getWidth();
			if ( ( i + 1 < widgets.size() && widgets[i + 1] && widgets[i + 1]->isVisible() ) ||
				 ( i + 2 < widgets.size() && widgets[i + 2] && widgets[i + 2]->isVisible() ) ) {
				totSize.x += widget->getLayoutPixelsMargin().Right;
			}
		}
		totSize.y = eemax<Float>( totSize.y, padding.Top + padding.Bottom +
												 widget->getPixelsSize().getHeight() );
	}
	totSize.x += padding.Right;
	Vector2f align;
	switch ( Font::getHorizontalAlign( getFlags() ) ) {
		case UI_HALIGN_RIGHT:
			align.x = mSize.getWidth() - totSize.x;
			break;
		case UI_HALIGN_CENTER:
			if ( mSize.getWidth() > totSize.x ) {
				align.x = ( mSize.getWidth() - totSize.x ) * 0.5f;
			} else {
				align.x = ( totSize.x - mSize.getWidth() ) * 0.5f;
			}
			break;
		case UI_HALIGN_LEFT:
			break;
	}
	for ( size_t i = 0; i < widgets.size(); i++ ) {
		if ( widgets[i] && widgets[i]->isVisible() )
			widgets[i]->setPixelsPosition( pos[i] + align );
	}
	return totSize;
}

UIWidget* UIPushButton::getFirstInnerItem() const {
	switch ( mInnerWidgetOrientation ) {
		case InnerWidgetOrientation::Left:
			return getExtraInnerWidget() && getExtraInnerWidget()->isVisible()
					   ? getExtraInnerWidget()
					   : mIcon;
		case InnerWidgetOrientation::Center:
			return mIcon->isVisible()
					   ? mIcon
					   : ( getExtraInnerWidget() && getExtraInnerWidget()->isVisible()
							   ? getExtraInnerWidget()
							   : mTextBox );
		case InnerWidgetOrientation::Right:
			if ( mIcon->isVisible() )
				return mIcon;
			else
				return mTextBox;
	}
	return mChild->isWidget() ? mChild->asType<UIWidget>() : nullptr;
}

Sizef UIPushButton::updateLayout() {
	Sizef size;
	Rectf autoPadding = calculatePadding();
	switch ( mInnerWidgetOrientation ) {
		case InnerWidgetOrientation::Left:
			size = packLayout( {getExtraInnerWidget(), mIcon, mTextBox}, autoPadding );
			break;
		case InnerWidgetOrientation::Center:
			size = packLayout( {mIcon, getExtraInnerWidget(), mTextBox}, autoPadding );
			break;
		case InnerWidgetOrientation::Right:
			size = packLayout( {mIcon, mTextBox, getExtraInnerWidget()}, autoPadding );
			break;
	}
	return size;
}

void UIPushButton::onPaddingChange() {
	onSizeChange();

	UIWidget::onPaddingChange();
}

void UIPushButton::onSizeChange() {
	onAutoSize();

	updateLayout();

	UIWidget::onSizeChange();
}

void UIPushButton::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );
	setThemeSkin( Theme, "button" );

	onThemeLoaded();
}

void UIPushButton::onThemeLoaded() {
	updateLayout();

	UIWidget::onThemeLoaded();
}

UIPushButton* UIPushButton::setIcon( Drawable* icon ) {
	if ( mIcon->getDrawable() != icon ) {
		mIcon->setPixelsSize( icon->getPixelsSize() );
		mIcon->setDrawable( icon );
		mTextBox->setVisible( !getText().empty() );
		updateLayout();
	}
	return this;
}

UIImage* UIPushButton::getIcon() const {
	return mIcon;
}

UIPushButton* UIPushButton::setText( const String& text ) {
	if ( text != mTextBox->getText() ) {
		mTextBox->setVisible( !text.empty() );
		mTextBox->setText( text );
		updateLayout();
	}
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
	Sizef size;
	Rectf autoPadding = calculatePadding();
	switch ( mInnerWidgetOrientation ) {
		case InnerWidgetOrientation::Left:
			size = calcLayoutSize( {getExtraInnerWidget(), mIcon, mTextBox}, autoPadding );
			break;
		case InnerWidgetOrientation::Center:
			size = calcLayoutSize( {mIcon, getExtraInnerWidget(), mTextBox}, autoPadding );
			break;
		case InnerWidgetOrientation::Right:
			size = calcLayoutSize( {mIcon, mTextBox, getExtraInnerWidget()}, autoPadding );
			break;
	}
	if ( getSkin() )
		size.x += PixelDensity::dpToPxI( getSkin()->getBorderSize().Left +
										 getSkin()->getBorderSize().Right );
	return size;
}

const InnerWidgetOrientation& UIPushButton::getInnerWidgetOrientation() const {
	return mInnerWidgetOrientation;
}

void UIPushButton::setInnerWidgetOrientation(
	const InnerWidgetOrientation& innerWidgetOrientation ) {
	if ( mInnerWidgetOrientation != innerWidgetOrientation ) {
		mInnerWidgetOrientation = innerWidgetOrientation;
		updateLayout();
	}
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
			UIIcon* iconF = getUISceneNode()->findIcon( val );
			if ( iconF ) {
				setIcon( iconF->getSize(
					eemax<size_t>( mSize.getHeight() - mPaddingPx.Top - mPadding.Bottom,
								   PixelDensity::dpToPxI( 16 ) ) ) );
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
