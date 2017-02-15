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
	InputParams.PosSet( 0, 0 );
	InputParams.Parent( this );

	if ( InputParams.Flags & UI_AUTO_SIZE )
		InputParams.Flags &= ~UI_AUTO_SIZE;

	if ( InputParams.Flags & UI_TEXT_SELECTION_ENABLED )
		InputParams.Flags |= UI_TEXT_SELECTION_ENABLED;

	InputParams.Flags |= UI_AUTO_PADDING;

	mInput		= eeNew( UITextInput, ( InputParams ) );

	UIControlAnim::CreateParams BtnParams( Params );
	BtnParams.Parent( this );
	BtnParams.Size = Sizei( 16, 16 );

	if ( BtnParams.Flags & UI_CLIP_ENABLE )
		BtnParams.Flags &= ~UI_CLIP_ENABLE;

	mPushUp		= eeNew( UIControlAnim, ( BtnParams ) );
	mPushDown 	= eeNew( UIControlAnim, ( BtnParams ) );

	mInput->Visible		( true );
	mInput->Enabled		( true );
	mPushUp->Visible	( true );
	mPushUp->Enabled	( true );
	mPushDown->Visible	( true );
	mPushDown->Enabled	( true );

	mInput->GetInputTextBuffer()->AllowOnlyNumbers( true, Params.AllowDotsInNumbers );

	InternalValue( mValue, true );

	AdjustChilds();

	ApplyDefaultTheme();
}

UISpinBox::~UISpinBox() {
}

Uint32 UISpinBox::Type() const {
	return UI_TYPE_SPINBOX;
}

bool UISpinBox::IsType( const Uint32& type ) const {
	return UISpinBox::Type() == type ? true : UIComplexControl::IsType( type );
}

void UISpinBox::SetTheme( UITheme * Theme ) {
	UIControl::SetThemeControl		( Theme, "spinbox" );

	mInput->SetThemeControl		( Theme, "spinbox_input" );
	mPushUp->SetThemeControl		( Theme, "spinbox_btnup" );
	mPushDown->SetThemeControl	( Theme, "spinbox_btndown" );

	SubTexture * tSubTexture = NULL;
	UISkin * tSkin = NULL;

	tSkin = mPushUp->GetSkin();

	if ( NULL != tSkin ) {
		tSubTexture = tSkin->GetSubTexture( UISkinState::StateNormal );

		if ( NULL != tSubTexture ) {
			mPushUp->Size( tSubTexture->RealSize() );
		}
	}

	tSkin = mPushDown->GetSkin();

	if ( NULL != tSkin ) {
		tSubTexture = tSkin->GetSubTexture( UISkinState::StateNormal );

		if ( NULL != tSubTexture ) {
			mPushDown->Size( tSubTexture->RealSize() );
		}
	}

	if ( mFlags & UI_AUTO_SIZE ) {
		mSize.height( mInput->GetSkinSize().height() );
	}

	AdjustChilds();
}

void UISpinBox::AdjustChilds() {
	mPushUp->Pos( mSize.width() - mPushUp->Size().width(), 0 );
	mPushDown->Pos( mSize.width() - mPushDown->Size().width(), mPushUp->Size().height() );
	mInput->Size( mSize.width() - mPushUp->Size().width(), mSize.height() );
}

void UISpinBox::Padding( const Recti& padding ) {
	mInput->Padding( padding );
}

const Recti& UISpinBox::Padding() const {
	return mInput->Padding();
}

void UISpinBox::ClickStep( const Float& step ) {
	mClickStep = step;
}

const Float& UISpinBox::ClickStep() const {
	return mClickStep;
}

Uint32 UISpinBox::OnMessage( const UIMessage * Msg ) {
	switch ( Msg->Msg() ) {
		case UIMessage::MsgClick:
		{
			if ( Msg->Flags() & EE_BUTTON_LMASK ) {
				if ( Msg->Sender() == mPushUp ) {
					AddValue( mClickStep );
				} else if ( Msg->Sender() == mPushDown ) {
					AddValue( -mClickStep );
				}
			} else if ( Msg->Flags() & EE_BUTTONS_WUWD ) {
				if ( Msg->Flags() & EE_BUTTON_WUMASK )
					AddValue( mClickStep );
				else
					AddValue( -mClickStep );
			}

			return 1;
		}
	}

	return 0;
}

void UISpinBox::AddValue( const Float& value ) {
	if ( !mInput->Text().size() )
		mInput->Text( String::toStr( static_cast<Int32>( mMinValue ) ) );

	Value( mValue + value );
}

void UISpinBox::InternalValue( const Float& Val, const bool& Force ) {
	if ( Force || Val != mValue ) {
		if ( Val >= mMinValue && Val <= mMaxValue ) {
			Float iValN	= (Float)(Int32) Val;
			Float fValN 	= (Float)iValN;

			if ( fValN == Val ) {
				mInput->Text( String::toStr( iValN ) );
			} else {
				mInput->Text( String::toStr( Val ) );
			}

			mValue = Val;

			OnValueChange();
		}
	}
}

void UISpinBox::Value( const Float& Val ) {
	InternalValue( Val, false );
}

const Float& UISpinBox::Value() const {
	return mValue;
}

void UISpinBox::MinValue( const Float& MinVal ) {
	mMinValue = MinVal;

	if ( mValue < mMinValue )
		mValue = mMinValue;
}

const Float& UISpinBox::MinValue() const {
	return mMinValue;
}

void UISpinBox::MaxValue( const Float& MaxVal ) {
	mMaxValue = MaxVal;

	if ( mValue > mMaxValue )
		mValue = mMaxValue;
}

const Float& UISpinBox::MaxValue() const {
	return mMaxValue;
}

void UISpinBox::Update() {
	bool Changed = mInput->GetInputTextBuffer()->ChangedSinceLastUpdate();

	UIControlAnim::Update();

	if ( Changed ) {
		if ( !mInput->Text().size() ) {
			Value( 0 );
		} else {
			Float Val = mValue;

			if ( '.' == mInput->Text()[ mInput->Text().size() - 1 ] ) {
				Uint32 pos = (Uint32)mInput->Text().find_first_of( "." );

				if ( pos != mInput->Text().size() - 1 )
					mInput->Text( mInput->Text().substr( 0, mInput->Text().size() - 1 ) );
			} else {
				bool Res 	= String::fromString<Float>( Val, mInput->Text() );

				if ( Res )
					Value( Val );
			}
		}
	}
}

UIControlAnim * UISpinBox::ButtonPushUp() const {
	return mPushUp;
}

UIControlAnim * UISpinBox::ButtonPushDown() const {
	return mPushDown;
}

UITextInput * UISpinBox::TextInput() const {
	return mInput;
}

void UISpinBox::OnAlphaChange() {
	UIControlAnim::OnAlphaChange();
	
	mInput->Alpha( mAlpha );
	mPushUp->Alpha( mAlpha );
	mPushDown->Alpha( mAlpha );
}

}}
