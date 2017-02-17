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
	InputParams.setPos( 0, 0 );
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

	mInput->visible		( true );
	mInput->enabled		( true );
	mPushUp->visible	( true );
	mPushUp->enabled	( true );
	mPushDown->visible	( true );
	mPushDown->enabled	( true );

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

	SubTexture * tSubTexture = NULL;
	UISkin * tSkin = NULL;

	tSkin = mPushUp->getSkin();

	if ( NULL != tSkin ) {
		tSubTexture = tSkin->getSubTexture( UISkinState::StateNormal );

		if ( NULL != tSubTexture ) {
			mPushUp->size( tSubTexture->realSize() );
		}
	}

	tSkin = mPushDown->getSkin();

	if ( NULL != tSkin ) {
		tSubTexture = tSkin->getSubTexture( UISkinState::StateNormal );

		if ( NULL != tSubTexture ) {
			mPushDown->size( tSubTexture->realSize() );
		}
	}

	if ( mFlags & UI_AUTO_SIZE ) {
		mSize.height( mInput->getSkinSize().height() );
	}

	adjustChilds();
}

void UISpinBox::adjustChilds() {
	mPushUp->position( mSize.width() - mPushUp->size().width(), 0 );
	mPushDown->position( mSize.width() - mPushDown->size().width(), mPushUp->size().height() );
	mInput->size( mSize.width() - mPushUp->size().width(), mSize.height() );
}

void UISpinBox::padding( const Recti& padding ) {
	mInput->padding( padding );
}

const Recti& UISpinBox::padding() const {
	return mInput->padding();
}

void UISpinBox::clickStep( const Float& step ) {
	mClickStep = step;
}

const Float& UISpinBox::clickStep() const {
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
	if ( !mInput->text().size() )
		mInput->text( String::toStr( static_cast<Int32>( mMinValue ) ) );

	this->value( mValue + value );
}

void UISpinBox::internalValue( const Float& Val, const bool& Force ) {
	if ( Force || Val != mValue ) {
		if ( Val >= mMinValue && Val <= mMaxValue ) {
			Float iValN	= (Float)(Int32) Val;
			Float fValN 	= (Float)iValN;

			if ( fValN == Val ) {
				mInput->text( String::toStr( iValN ) );
			} else {
				mInput->text( String::toStr( Val ) );
			}

			mValue = Val;

			onValueChange();
		}
	}
}

void UISpinBox::value( const Float& Val ) {
	internalValue( Val, false );
}

const Float& UISpinBox::value() const {
	return mValue;
}

void UISpinBox::minValue( const Float& MinVal ) {
	mMinValue = MinVal;

	if ( mValue < mMinValue )
		mValue = mMinValue;
}

const Float& UISpinBox::minValue() const {
	return mMinValue;
}

void UISpinBox::maxValue( const Float& MaxVal ) {
	mMaxValue = MaxVal;

	if ( mValue > mMaxValue )
		mValue = mMaxValue;
}

const Float& UISpinBox::maxValue() const {
	return mMaxValue;
}

void UISpinBox::update() {
	bool Changed = mInput->getInputTextBuffer()->changedSinceLastUpdate();

	UIControlAnim::update();

	if ( Changed ) {
		if ( !mInput->text().size() ) {
			value( 0 );
		} else {
			Float Val = mValue;

			if ( '.' == mInput->text()[ mInput->text().size() - 1 ] ) {
				Uint32 pos = (Uint32)mInput->text().find_first_of( "." );

				if ( pos != mInput->text().size() - 1 )
					mInput->text( mInput->text().substr( 0, mInput->text().size() - 1 ) );
			} else {
				bool Res 	= String::fromString<Float>( Val, mInput->text() );

				if ( Res )
					value( Val );
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
	
	mInput->alpha( mAlpha );
	mPushUp->alpha( mAlpha );
	mPushDown->alpha( mAlpha );
}

}}
