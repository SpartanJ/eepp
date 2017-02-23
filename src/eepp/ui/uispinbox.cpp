#include <eepp/ui/uispinbox.hpp>
#include <eepp/graphics/subtexture.hpp>

namespace EE { namespace UI {

UISpinBox::UISpinBox( const UISpinBox::CreateParams& Params ) :
	UIComplexControl( Params ),
	mMinValue( 0.f ),
	mMaxValue( 1024.f ),
	mValue( Params.DefaultValue ),
	mClickStep( 1.f )
{
	UITextInput::CreateParams InputParams( Params );
	InputParams.setPosition( 0, 0 );
	InputParams.setParent( this );

	if ( InputParams.Flags & UI_AUTO_SIZE )
		InputParams.Flags &= ~UI_AUTO_SIZE;

	if ( InputParams.Flags & UI_TEXT_SELECTION_ENABLED )
		InputParams.Flags |= UI_TEXT_SELECTION_ENABLED;

	InputParams.Flags |= UI_AUTO_PADDING;

	mInput		= eeNew( UITextInput, ( InputParams ) );

	UIControlAnim::CreateParams BtnParams( Params );
	BtnParams.setParent( this );
	BtnParams.Size = Sizei( 16, 16 );

	if ( BtnParams.Flags & UI_CLIP_ENABLE )
		BtnParams.Flags &= ~UI_CLIP_ENABLE;

	mPushUp		= eeNew( UIControlAnim, ( BtnParams ) );
	mPushDown 	= eeNew( UIControlAnim, ( BtnParams ) );

	mInput->setVisible		( true );
	mInput->setEnabled		( true );
	mPushUp->setVisible	( true );
	mPushUp->setEnabled	( true );
	mPushDown->setVisible	( true );
	mPushDown->setEnabled	( true );

	mInput->getInputTextBuffer()->setAllowOnlyNumbers( true, Params.AllowDotsInNumbers );

	internalValue( mValue, true );

	adjustChilds();

	applyDefaultTheme();
}

UISpinBox::~UISpinBox() {
}

Uint32 UISpinBox::getType() const {
	return UI_TYPE_SPINBOX;
}

bool UISpinBox::isType( const Uint32& type ) const {
	return UISpinBox::getType() == type ? true : UIComplexControl::isType( type );
}

void UISpinBox::setTheme( UITheme * Theme ) {
	UIControl::setThemeControl		( Theme, "spinbox" );

	mInput->setThemeControl		( Theme, "spinbox_input" );
	mPushUp->setThemeControl		( Theme, "spinbox_btnup" );
	mPushDown->setThemeControl	( Theme, "spinbox_btndown" );

	UISkin * tSkin = NULL;

	tSkin = mPushUp->getSkin();

	if ( NULL != tSkin ) {
		mPushUp->setPixelsSize( tSkin->getSize() );
	}

	tSkin = mPushDown->getSkin();

	if ( NULL != tSkin ) {
		mPushDown->setPixelsSize( tSkin->getSize() );
	}

	if ( mFlags & UI_AUTO_SIZE ) {
		setInternalPixelsHeight( mInput->getSkinSize().getHeight() );
	}

	adjustChilds();
}

void UISpinBox::adjustChilds() {
	mPushUp->setPosition( mSize.getWidth() - mPushUp->getSize().getWidth(), 0 );
	mPushDown->setPosition( mSize.getWidth() - mPushDown->getSize().getWidth(), mPushUp->getSize().getHeight() );
	mInput->setSize( mSize.getWidth() - mPushUp->getSize().getWidth(), mSize.getHeight() );
}

void UISpinBox::setPadding( const Recti& padding ) {
	mInput->setPadding( padding );
}

const Recti& UISpinBox::getPadding() const {
	return mInput->getPadding();
}

void UISpinBox::setClickStep( const Float& step ) {
	mClickStep = step;
}

const Float& UISpinBox::getClickStep() const {
	return mClickStep;
}

Uint32 UISpinBox::onMessage( const UIMessage * Msg ) {
	switch ( Msg->getMsg() ) {
		case UIMessage::MsgClick:
		{
			if ( Msg->getFlags() & EE_BUTTON_LMASK ) {
				if ( Msg->getSender() == mPushUp ) {
					addValue( mClickStep );
				} else if ( Msg->getSender() == mPushDown ) {
					addValue( -mClickStep );
				}
			} else if ( Msg->getFlags() & EE_BUTTONS_WUWD ) {
				if ( Msg->getFlags() & EE_BUTTON_WUMASK )
					addValue( mClickStep );
				else
					addValue( -mClickStep );
			}

			return 1;
		}
	}

	return 0;
}

void UISpinBox::addValue( const Float& value ) {
	if ( !mInput->getText().size() )
		mInput->setText( String::toStr( static_cast<Int32>( mMinValue ) ) );

	this->setValue( mValue + value );
}

void UISpinBox::internalValue( const Float& Val, const bool& Force ) {
	if ( Force || Val != mValue ) {
		if ( Val >= mMinValue && Val <= mMaxValue ) {
			Float iValN	= (Float)(Int32) Val;
			Float fValN 	= (Float)iValN;

			if ( fValN == Val ) {
				mInput->setText( String::toStr( iValN ) );
			} else {
				mInput->setText( String::toStr( Val ) );
			}

			mValue = Val;

			onValueChange();
		}
	}
}

void UISpinBox::onSizeChange() {
	UIComplexControl::onSizeChange();

	adjustChilds();
}

void UISpinBox::setValue( const Float& Val ) {
	internalValue( Val, false );
}

const Float& UISpinBox::getValue() const {
	return mValue;
}

void UISpinBox::setMinValue( const Float& MinVal ) {
	mMinValue = MinVal;

	if ( mValue < mMinValue )
		mValue = mMinValue;
}

const Float& UISpinBox::getMinValue() const {
	return mMinValue;
}

void UISpinBox::setMaxValue( const Float& MaxVal ) {
	mMaxValue = MaxVal;

	if ( mValue > mMaxValue )
		mValue = mMaxValue;
}

const Float& UISpinBox::getMaxValue() const {
	return mMaxValue;
}

void UISpinBox::update() {
	bool Changed = mInput->getInputTextBuffer()->changedSinceLastUpdate();

	UIControlAnim::update();

	if ( Changed ) {
		if ( !mInput->getText().size() ) {
			setValue( 0 );
		} else {
			Float Val = mValue;

			if ( '.' == mInput->getText()[ mInput->getText().size() - 1 ] ) {
				Uint32 pos = (Uint32)mInput->getText().find_first_of( "." );

				if ( pos != mInput->getText().size() - 1 )
					mInput->setText( mInput->getText().substr( 0, mInput->getText().size() - 1 ) );
			} else {
				bool Res 	= String::fromString<Float>( Val, mInput->getText() );

				if ( Res )
					setValue( Val );
			}
		}
	}
}

UIControlAnim * UISpinBox::getButtonPushUp() const {
	return mPushUp;
}

UIControlAnim * UISpinBox::getButtonPushDown() const {
	return mPushDown;
}

UITextInput * UISpinBox::getTextInput() const {
	return mInput;
}

void UISpinBox::onAlphaChange() {
	UIControlAnim::onAlphaChange();
	
	mInput->setAlpha( mAlpha );
	mPushUp->setAlpha( mAlpha );
	mPushDown->setAlpha( mAlpha );
}

}}
