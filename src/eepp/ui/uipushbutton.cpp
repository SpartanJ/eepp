#include <eepp/graphics/drawablesearcher.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uiicon.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>

namespace EE { namespace UI {

InnerWidgetOrientation UIPushButton::innerWidgetOrientationFromString( std::string iwo ) {
	String::toLowerInPlace( iwo );
	if ( iwo == "widgeticontextbox" )
		return InnerWidgetOrientation::WidgetIconTextBox;
	if ( iwo == "widgettextboxicon" )
		return InnerWidgetOrientation::WidgetTextBoxIcon;
	if ( iwo == "icontextboxwidget" )
		return InnerWidgetOrientation::IconTextBoxWidget;
	if ( iwo == "iconwidgettextbox" )
		return InnerWidgetOrientation::IconWidgetTextBox;
	if ( iwo == "textboxiconwidget" )
		return InnerWidgetOrientation::TextBoxIconWidget;
	if ( iwo == "textboxwidgeticon" )
		return InnerWidgetOrientation::TextBoxWidgetIcon;
	return InnerWidgetOrientation::WidgetIconTextBox;
}

std::string
UIPushButton::innerWidgetOrientationToString( const InnerWidgetOrientation& orientation ) {
	switch ( orientation ) {
		case InnerWidgetOrientation::WidgetIconTextBox:
			return "widgeticontextbox";
		case InnerWidgetOrientation::WidgetTextBoxIcon:
			return "widgettextboxicon";
		case InnerWidgetOrientation::IconTextBoxWidget:
			return "icontextboxwidget";
		case InnerWidgetOrientation::IconWidgetTextBox:
			return "iconwidgettextbox";
		case InnerWidgetOrientation::TextBoxIconWidget:
			return "textboxiconwidget";
		case InnerWidgetOrientation::TextBoxWidgetIcon:
			return "textboxwidgeticon";
	}
	return "widgeticontextbox";
}

UIPushButton* UIPushButton::New() {
	return eeNew( UIPushButton, () );
}

UIPushButton* UIPushButton::NewWithTag( const std::string& tag ) {
	return eeNew( UIPushButton, ( tag ) );
}

UIPushButton*
UIPushButton::NewWithOpt( const std::string& tag,
						  const std::function<UITextView*( UIPushButton* )>& newTextViewCb ) {
	return eeNew( UIPushButton, ( tag, newTextViewCb ) );
}

UIPushButton::UIPushButton( const std::string& tag ) : UIPushButton( tag, nullptr ) {}

UIPushButton::UIPushButton( const std::string& tag,
							const std::function<UITextView*( UIPushButton* )>& newTextViewCb ) :
	UIWidget( tag ), mIcon( NULL ), mTextBox( NULL ) {
	mFlags |= ( UI_AUTO_SIZE | UI_VALIGN_CENTER | UI_HALIGN_CENTER );

	auto cb = [this]( const Event* ) { onSizeChange(); };

	mTextBox = newTextViewCb ? newTextViewCb( this ) : UITextView::NewWithTag( tag + "::text" );
	mTextBox->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent )
		->setFlags( UI_VALIGN_CENTER | UI_HALIGN_CENTER )
		->setParent( this )
		->setVisible( true )
		->setEnabled( false );
	mTextBox->addEventListener( Event::OnFontChanged, cb );
	mTextBox->addEventListener( Event::OnFontStyleChanged, cb );
	mTextBox->addEventListener( Event::OnTextChanged, cb );
	mTextBox->addEventListener( Event::OnVisibleChange, cb );

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

	if ( mWidthPolicy == SizePolicy::WrapContent || ( mFlags & UI_AUTO_SIZE ) ) {
		Sizef size = getContentSize();

		if ( mWidthPolicy != SizePolicy::WrapContent && ( mFlags & UI_AUTO_SIZE ) &&
			 size.getWidth() < mSize.getWidth() ) {
			return;
		}

		Sizef fsize = fitMinMaxSizePx( size );

		if ( size.getWidth() != fsize.getWidth() ) {
			UIWidget* eiw = getExtraInnerWidget();
			Float nonTextW =
				( NULL != mIcon ? mIcon->getSize().getWidth() + mIcon->getLayoutMargin().Left +
									  mIcon->getLayoutMargin().Right
								: 0 ) +
				( NULL != eiw && eiw->isVisible()
					  ? eiw->getSize().getWidth() + eiw->getLayoutMargin().Left +
							eiw->getLayoutMargin().Right
					  : 0 ) +
				getSkinSize().getWidth();

			Float textW = mTextBox->getSize().getWidth();

			if ( textW > fsize.getWidth() - nonTextW ) {
				Float mw = eemax( 0.f, fsize.getWidth() - nonTextW );
				getTextBox()->setMaxWidthEq( String::format( "%.0fdp", mw ) );
			}
			if ( getTextBox()->getTextWidth() > getTextBox()->getSize().getWidth() )
				getTextBox()->setHorizontalAlign( UI_HALIGN_LEFT );
		}

		setInternalPixelsWidth( size.getWidth() );
	}
}

Vector2f UIPushButton::calcLayoutSize( const std::vector<UIWidget*>& widgets,
									   const Rectf& padding ) const {
	Vector2f totSize{ padding.Left, padding.Top + padding.Bottom };
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
	Vector2f totSize{ padding.Left, padding.Top + padding.Bottom };
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
		pos[i].x += 0 == i || widget->isVisible() ? widget->getLayoutPixelsMargin().Left : 0;
		if ( widget->isVisible() && widget->getPixelsSize().getWidth() > 0 ) {
			totSize.x += widget->getPixelsSize().getWidth();
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
		case InnerWidgetOrientation::WidgetIconTextBox:
			return getExtraInnerWidget() && getExtraInnerWidget()->isVisible()
					   ? getExtraInnerWidget()
					   : ( mIcon && mIcon->isVisible() ? mIcon->asType<UIWidget>()
													   : mTextBox->asType<UIWidget>() );
		case InnerWidgetOrientation::IconWidgetTextBox:
			return mIcon && mIcon->isVisible()
					   ? mIcon
					   : ( getExtraInnerWidget() && getExtraInnerWidget()->isVisible()
							   ? getExtraInnerWidget()
							   : mTextBox );
		case InnerWidgetOrientation::IconTextBoxWidget:
			return mIcon && mIcon->isVisible() ? mIcon->asType<UIWidget>() : mTextBox;
		case InnerWidgetOrientation::WidgetTextBoxIcon:
			return ( getExtraInnerWidget() && getExtraInnerWidget()->isVisible() )
					   ? getExtraInnerWidget()
					   : mTextBox;
		case InnerWidgetOrientation::TextBoxIconWidget:
			if ( mTextBox->isVisible() )
				return mTextBox;
			if ( mIcon && mIcon->isVisible() )
				return mIcon;
			return getExtraInnerWidget();
			break;
		case InnerWidgetOrientation::TextBoxWidgetIcon:
			if ( mTextBox->isVisible() )
				return mTextBox;

			break;
	}
	return mChild->isWidget() ? mChild->asType<UIWidget>() : nullptr;
}

Sizef UIPushButton::updateLayout() {
	Sizef size;
	Rectf autoPadding = calculatePadding();
	switch ( mInnerWidgetOrientation ) {
		case InnerWidgetOrientation::WidgetIconTextBox:
			size = packLayout( { getExtraInnerWidget(), mIcon, mTextBox }, autoPadding );
			break;
		case InnerWidgetOrientation::IconWidgetTextBox:
			size = packLayout( { mIcon, getExtraInnerWidget(), mTextBox }, autoPadding );
			break;
		case InnerWidgetOrientation::IconTextBoxWidget:
			size = packLayout( { mIcon, mTextBox, getExtraInnerWidget() }, autoPadding );
			break;
		case InnerWidgetOrientation::WidgetTextBoxIcon:
			size = packLayout( { getExtraInnerWidget(), mTextBox, mIcon }, autoPadding );
			break;
		case InnerWidgetOrientation::TextBoxIconWidget:
			size = packLayout( { mTextBox, mIcon, getExtraInnerWidget() }, autoPadding );
			break;
		case InnerWidgetOrientation::TextBoxWidgetIcon:
			size = packLayout( { mTextBox, getExtraInnerWidget(), mIcon }, autoPadding );
			break;
	}
	return size.ceil();
}

bool UIPushButton::isTextAsFallback() const {
	return mTextAsFallback;
}

void UIPushButton::setTextAsFallback( bool textAsFallback ) {
	if ( mTextAsFallback != textAsFallback ) {
		mTextAsFallback = textAsFallback;
		updateTextBox();
	}
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

void UIPushButton::updateTextBox() {
	bool mustBeVisible = ( !getText().empty() && !mTextAsFallback ) ||
						 ( nullptr == mIcon || nullptr == mIcon->getDrawable() );
	if ( mTextBox->isVisible() != mustBeVisible ) {
		mTextBox->setVisible( mustBeVisible );
		onAutoSize();
		updateLayout();
	}
}

UIPushButton* UIPushButton::setIcon( Drawable* icon, bool ownIt ) {
	if ( nullptr == mIcon || mIcon->getDrawable() != icon ) {
		if ( icon )
			getIcon()->setPixelsSize( icon->getPixelsSize() );
		if ( icon == nullptr && mIcon == nullptr )
			return this;
		getIcon()->setDrawable( icon, ownIt );
		updateTextBox();
	}
	return this;
}

UIImage* UIPushButton::getIcon() {
	if ( nullptr == mIcon ) {
		auto cb = [this]( const Event* ) { onSizeChange(); };

		mIcon = UIImage::NewWithTag( mTag + "::icon" );
		mIcon->setScaleType( UIScaleType::FitInside )
			->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed )
			->setFlags( UI_VALIGN_CENTER | UI_HALIGN_CENTER )
			->setParent( this )
			->setVisible( true )
			->setEnabled( false );

		mIcon->addEventListener( Event::OnPaddingChange, cb );
		mIcon->addEventListener( Event::OnMarginChange, cb );
		mIcon->addEventListener( Event::OnSizeChange, cb );
		mIcon->addEventListener( Event::OnVisibleChange, cb );

		if ( mIconMinSize != Sizei::Zero ) {
			auto iconMinSize = mIconMinSize;
			mIconMinSize = Sizei{ -1, -1 }; // force refresh
			setIconMinimumSize( iconMinSize );
		}
	}
	return mIcon;
}

bool UIPushButton::hasIcon() const {
	return mIcon != nullptr;
}

UIPushButton* UIPushButton::setText( const String& text ) {
	if ( text != mTextBox->getText() ) {
		mTextBox->setVisible( !text.empty() );
		mTextBox->setText( text );
		onAutoSize();
		updateLayout();
	}
	return this;
}

const String& UIPushButton::getText() const {
	return mTextBox->getText();
}

UITextView* UIPushButton::getTextBox() const {
	return mTextBox;
}

void UIPushButton::onAlphaChange() {
	UIWidget::onAlphaChange();

	if ( mIcon )
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
			getIcon()->setMinSizeEq( String::fromFloat( mIconMinSize.x, "dp" ),
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

UIWidget* UIPushButton::setTextAlign( const Uint32& align ) {
	mFlags &= ~( UI_HALIGN_CENTER | UI_HALIGN_RIGHT );
	mFlags |= align;
	onAlignChange();
	return this;
}

Sizef UIPushButton::getContentSize() const {
	Sizef size;
	Rectf autoPadding = calculatePadding();
	switch ( mInnerWidgetOrientation ) {
		case InnerWidgetOrientation::WidgetIconTextBox:
			size = calcLayoutSize( { getExtraInnerWidget(), mIcon, mTextBox }, autoPadding );
			break;
		case InnerWidgetOrientation::IconWidgetTextBox:
			size = calcLayoutSize( { mIcon, getExtraInnerWidget(), mTextBox }, autoPadding );
			break;
		case InnerWidgetOrientation::IconTextBoxWidget:
			size = calcLayoutSize( { mIcon, mTextBox, getExtraInnerWidget() }, autoPadding );
			break;
		case InnerWidgetOrientation::TextBoxIconWidget:
			size = calcLayoutSize( { mIcon, mTextBox, getExtraInnerWidget() }, autoPadding );
			break;
		case InnerWidgetOrientation::TextBoxWidgetIcon:
			size = calcLayoutSize( { mTextBox, getExtraInnerWidget(), mIcon }, autoPadding );
			break;
		case InnerWidgetOrientation::WidgetTextBoxIcon:
			size = calcLayoutSize( { getExtraInnerWidget(), mTextBox, mIcon }, autoPadding );
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
											 const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::InnerWidgetOrientation:
			return innerWidgetOrientationToString( mInnerWidgetOrientation );
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
		case PropertyId::TextAsFallback:
			return mTextAsFallback ? "true" : "false";
		case PropertyId::Tint: {
			if ( mIcon )
				return mIcon->getColor().toHexString();
			return Color::Transparent.toHexString();
		}
		case PropertyId::Color:
		case PropertyId::TextShadowColor:
		case PropertyId::TextShadowOffset:
		case PropertyId::SelectionColor:
		case PropertyId::SelectionBackColor:
		case PropertyId::FontFamily:
		case PropertyId::FontSize:
		case PropertyId::FontStyle:
		case PropertyId::Wordwrap:
		case PropertyId::TextStrokeWidth:
		case PropertyId::TextStrokeColor:
		case PropertyId::TextSelection:
		case PropertyId::TextTransform:
		case PropertyId::TextOverflow:
			return mTextBox->getPropertyString( propertyDef, propertyIndex );
		default:
			return UIWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UIPushButton::getPropertiesImplemented() const {
	auto props = UIWidget::getPropertiesImplemented();
	auto local = { PropertyId::Text,
				   PropertyId::Icon,
				   PropertyId::MinIconSize,
				   PropertyId::TextAlign,
				   PropertyId::TextAsFallback,
				   PropertyId::Tint,
				   PropertyId::Color,
				   PropertyId::TextShadowColor,
				   PropertyId::TextShadowOffset,
				   PropertyId::SelectionColor,
				   PropertyId::SelectionBackColor,
				   PropertyId::FontFamily,
				   PropertyId::FontSize,
				   PropertyId::FontStyle,
				   PropertyId::Wordwrap,
				   PropertyId::TextStrokeWidth,
				   PropertyId::TextStrokeColor,
				   PropertyId::TextSelection,
				   PropertyId::TextTransform,
				   PropertyId::TextOverflow };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

bool UIPushButton::applyProperty( const StyleSheetProperty& attribute ) {
	bool attributeSet = true;

	if ( attribute.getPropertyDefinition() == NULL ) {
		return false;
	}

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::InnerWidgetOrientation:
			setInnerWidgetOrientation( innerWidgetOrientationFromString( attribute.value() ) );
			break;
		case PropertyId::Text:
			if ( NULL != mSceneNode && mSceneNode->isUISceneNode() )
				setText( getUISceneNode()->getTranslatorString( attribute.value() ) );
			break;
		case PropertyId::Icon: {
			const std::string& val = attribute.value();
			Drawable* icon = NULL;
			bool ownIt;
			UIIcon* iconF = getUISceneNode()->findIcon( val );
			if ( iconF ) {
				setIcon( iconF->getSize(
					eemax<size_t>( mSize.getHeight() - mPaddingPx.Top - mPadding.Bottom,
								   PixelDensity::dpToPxI( 16 ) ) ) );
			} else if ( NULL !=
						( icon = StyleSheetSpecification::instance()
									 ->getDrawableImageParser()
									 .createDrawable( val, getPixelsSize(), ownIt, this ) ) ) {
				setIcon( icon, ownIt );
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
		case PropertyId::TextAsFallback:
			setTextAsFallback( attribute.asBool() );
			break;
		case PropertyId::Tint:
			getIcon()->setColor( attribute.asColor() );
			break;
		case PropertyId::Color:
		case PropertyId::TextShadowColor:
		case PropertyId::TextShadowOffset:
		case PropertyId::SelectionColor:
		case PropertyId::SelectionBackColor:
		case PropertyId::FontFamily:
		case PropertyId::FontSize:
		case PropertyId::FontStyle:
		case PropertyId::Wordwrap:
		case PropertyId::TextStrokeWidth:
		case PropertyId::TextStrokeColor:
		case PropertyId::TextSelection:
		case PropertyId::TextTransform:
		case PropertyId::TextOverflow:
			attributeSet = mTextBox->applyProperty( attribute );
			break;
		default:
			attributeSet = UIWidget::applyProperty( attribute );
			break;
	}

	return attributeSet;
}

}} // namespace EE::UI
