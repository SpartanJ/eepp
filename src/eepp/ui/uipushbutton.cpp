#include <eepp/ui/uipushbutton.hpp>
#include <eepp/graphics/text.hpp>
#include <pugixml/pugixml.hpp>
#include <eepp/graphics/drawablesearcher.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uiscenenode.hpp>

namespace EE { namespace UI {

UIPushButton * UIPushButton::New() {
	return eeNew( UIPushButton, () );
}

UIPushButton::UIPushButton( const std::string& tag ) :
	UIWidget( tag ),
	mIcon( NULL ),
	mTextBox( NULL )
{
	mFlags |= ( UI_AUTO_SIZE | UI_VALIGN_CENTER | UI_HALIGN_CENTER );

	Uint32 GfxFlags;

	if ( mStyleConfig.IconMinSize.x != 0 && mStyleConfig.IconMinSize.y != 0 ) {
		GfxFlags = UI_VALIGN_CENTER | UI_HALIGN_CENTER;
	} else {
		GfxFlags = UI_AUTO_SIZE | UI_VALIGN_CENTER | UI_HALIGN_CENTER;
	}

	mIcon = UIImage::NewWithTag( "pushbutton::image" );
	mIcon->setParent( this );
	mIcon->setLayoutSizeRules( FIXED, FIXED );
	mIcon->setFlags( GfxFlags );
	mIcon->unsetFlags( UI_AUTO_SIZE );
	mIcon->setScaleType( UIScaleType::FitInside );

	if ( mStyleConfig.IconMinSize.x != 0 && mStyleConfig.IconMinSize.y != 0 ) {
		mIcon->setSize( mStyleConfig.IconMinSize.asFloat() );
	}

	mIcon->setVisible( true );
	mIcon->setEnabled( false );

	mTextBox = UITextView::NewWithTag( "pushbutton::text" );
	mTextBox->setLayoutSizeRules( WRAP_CONTENT, WRAP_CONTENT );
	mTextBox->setParent( this );
	mTextBox->setVisible( true );
	mTextBox->setEnabled( false );
	mTextBox->setFlags( UI_VALIGN_CENTER | UI_HALIGN_CENTER );

	if ( mStyleConfig.IconAutoMargin )
		mNodeFlags |= NODE_FLAG_FREE_USE;

	onSizeChange();

	applyDefaultTheme();
}

UIPushButton::UIPushButton() :
	UIPushButton( "pushbutton" )
{}

UIPushButton::~UIPushButton() {
}

Uint32 UIPushButton::getType() const {
	return UI_TYPE_PUSHBUTTON;
}

bool UIPushButton::isType( const Uint32& type ) const {
	return UIPushButton::getType() == type ? true : UIWidget::isType( type );
}

void UIPushButton::onAutoSize() {
	if ( ( mFlags & UI_AUTO_SIZE ) && NULL != getSkin() ) {
		setInternalHeight( getSkinSize().getHeight() );
	}

	if ( ( mFlags & UI_AUTO_SIZE ) && NULL != getSkin() && ( 0 == mDpSize.getHeight() || mLayoutHeightRules == WRAP_CONTENT ) ) {
		Float h = eemax<Float>( PixelDensity::dpToPx( getSkinSize().getHeight() ), mTextBox->getTextHeight() );

		setInternalPixelsHeight( h + mRealPadding.Top + mRealPadding.Bottom );
	}

	if ( ( mFlags & UI_AUTO_SIZE ) || mLayoutWidthRules == WRAP_CONTENT ) {
		Int32 txtW = NULL != mTextBox ? mTextBox->getTextWidth() : 0;

		Int32 minSize = txtW +
						( NULL != mIcon ? mIcon->getPixelsSize().getWidth() : 0 ) +
						PixelDensity::dpToPxI( mStyleConfig.IconHorizontalMargin ) + mRealPadding.Left + mRealPadding.Right +
						( NULL != getSkin() ? PixelDensity::dpToPxI( getSkin()->getBorderSize().Left + getSkin()->getBorderSize().Right ) : 0 );

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

	if ( mRealPadding.Top > autoPadding.Top ) autoPadding.Top = mRealPadding.Top;
	if ( mRealPadding.Bottom > autoPadding.Bottom ) autoPadding.Bottom = mRealPadding.Bottom;
	if ( mRealPadding.Left > autoPadding.Left ) autoPadding.Left = mRealPadding.Left;
	if ( mRealPadding.Right > autoPadding.Right ) autoPadding.Right = mRealPadding.Right;

	mIcon->setPixelsPosition( autoPadding.Left + mStyleConfig.IconHorizontalMargin, 0 );
	mIcon->centerVertical();

	if ( NULL != mTextBox ) {
		Vector2f position;

		switch ( fontVAlignGet( getFlags() ) ) {
			case UI_VALIGN_CENTER:
				position.y = ( mSize.getHeight() - mTextBox->getPixelsSize().getHeight() ) / 2;
				break;
			case UI_VALIGN_BOTTOM:
				position.y = mSize.y - mTextBox->getPixelsSize().getHeight() - autoPadding.Bottom;
				break;
			case UI_VALIGN_TOP:
				position.y = autoPadding.Top;
				break;
		}

		switch ( fontHAlignGet( getFlags() ) ) {
			case UI_HALIGN_RIGHT:
				position.x = mSize.getWidth() - mTextBox->getPixelsSize().getWidth() - autoPadding.Right;
				break;
			case UI_HALIGN_CENTER:
				position.x = ( mSize.getWidth() - mTextBox->getPixelsSize().getWidth() ) / 2;

				if ( NULL != mIcon->getDrawable() ) {
					Uint32 iconPos = mIcon->getPixelsPosition().x + mIcon->getPixelsSize().getWidth();

					if ( iconPos >= position.x ) {
						Float px = PixelDensity::dpToPx(1);

						position.x = iconPos + px;
					}
				}

				break;
			case UI_HALIGN_LEFT:
				position.x = mIcon->getPixelsPosition().x + mIcon->getPixelsSize().getWidth();
				break;
		}

		mTextBox->setPixelsPosition( position );
	}

	if ( NULL != mTextBox && mTextBox->getText().empty() ) {
		mIcon->center();
	}
}

void UIPushButton::setTheme( UITheme * Theme ) {
	UIWidget::setTheme( Theme );
	setThemeSkin( Theme, "button" );

	onThemeLoaded();
}

void UIPushButton::onThemeLoaded() {
	autoIconHorizontalMargin();

	onAutoSize();

	UIWidget::onThemeLoaded();
}

UIPushButton * UIPushButton::setIcon( Drawable * Icon ) {
	mIcon->setDrawable( Icon );
	onSizeChange();
	return this;
}

UIImage * UIPushButton::getIcon() const {
	return mIcon;
}

UIPushButton * UIPushButton::setText( const String& text ) {
	mTextBox->setText( text );
	onSizeChange();
	return this;
}

const String& UIPushButton::getText() {
	return mTextBox->getText();
}

void UIPushButton::setIconHorizontalMargin( Int32 margin ) {
	mStyleConfig.IconHorizontalMargin = margin;
	onSizeChange();
}

const Int32& UIPushButton::getIconHorizontalMargin() const {
	return mStyleConfig.IconHorizontalMargin;
}

UITextView * UIPushButton::getTextBox() const {
	return mTextBox;
}

void UIPushButton::onAlphaChange() {
	UIWidget::onAlphaChange();

	mIcon->setAlpha( mAlpha );
	mTextBox->setAlpha( mAlpha );
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
		NodeMessage Msg( this, NodeMessage::Click, EE_BUTTON_LMASK );
		messagePost( &Msg );
		onMouseClick( Vector2i(0,0), EE_BUTTON_LMASK );

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

void UIPushButton::autoIconHorizontalMargin() {
	if ( mNodeFlags & NODE_FLAG_FREE_USE ) {
		Rectf RMargin = makePadding( true, false, false, false, true );
		setIconHorizontalMargin( RMargin.Left );
	}
}

const UIPushButton::StyleConfig& UIPushButton::getStyleConfig() const {
	return mStyleConfig;
}

void UIPushButton::setIconMinimumSize( const Sizei & minIconSize ) {
	if ( minIconSize != mStyleConfig.IconMinSize ) {
		mStyleConfig.IconMinSize = minIconSize;

		if ( mStyleConfig.IconMinSize.x != 0 && mStyleConfig.IconMinSize.y != 0 ) {
			Sizef minSize( eemax( mDpSize.x, (Float)mStyleConfig.IconMinSize.x ), eemax( mDpSize.y, (Float)mStyleConfig.IconMinSize.y ) );

			if ( minSize != mDpSize ) {
				mIcon->setSize( minSize );
				onSizeChange();
			}
		}
	}
}

void UIPushButton::setStyleConfig(const StyleConfig & styleConfig) {
	setIconMinimumSize( styleConfig.IconMinSize );
	mStyleConfig = styleConfig;
	onStateChange();
}

bool UIPushButton::setAttribute( const NodeAttribute& attribute, const Uint32& state ) {
	const std::string& name = attribute.getName();

	bool attributeSet = true;

	if ( "text" == name ) {
		if ( NULL != mSceneNode && mSceneNode->isUISceneNode() )
			setText( static_cast<UISceneNode*>( mSceneNode )->getTranslatorString( attribute.asString() ) );
	} else if ( "icon" == name ) {
		std::string val = attribute.asString();
		Drawable * icon = NULL;

		if ( NULL != mTheme && NULL != ( icon = mTheme->getIconByName( val ) ) ) {
			setIcon( icon );
		} else if ( NULL != ( icon = DrawableSearcher::searchByName( val ) ) ) {
			setIcon( icon );
		}
	} else if ( "iconminsize" == name ) {
		setIconMinimumSize( attribute.asSizei() );
	} else if ( "iconhorizontalmargin" == name ) {
		setIconHorizontalMargin( attribute.asInt() );
	} else if ( "iconautomargin" == name ) {
		mStyleConfig.IconAutoMargin = attribute.asBool();

		if ( mStyleConfig.IconAutoMargin )
			mNodeFlags |= NODE_FLAG_FREE_USE;
	} else {
		attributeSet = UIWidget::setAttribute( attribute, state );
	}

	if ( !attributeSet && ( String::startsWith( name, "text" ) || String::startsWith( name, "font" ) ) )
		mTextBox->setAttribute( attribute );

	return attributeSet;
}

}}
