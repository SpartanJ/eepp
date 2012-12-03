#include <eepp/ui/cuispinbox.hpp>

namespace EE { namespace UI {

cUISpinBox::cUISpinBox( const cUISpinBox::CreateParams& Params ) :
	cUIComplexControl( Params ),
	mMinValue( 0.f ),
	mMaxValue( 1024.f ),
	mValue( Params.DefaultValue ),
	mClickStep( 1.f )
{
	cUITextInput::CreateParams InputParams( Params );
	InputParams.PosSet( 0, 0 );
	InputParams.Parent( this );

	if ( InputParams.Flags & UI_AUTO_SIZE )
		InputParams.Flags &= ~UI_AUTO_SIZE;

	InputParams.Flags |= UI_AUTO_PADDING;

	mInput		= eeNew( cUITextInput, ( InputParams ) );

	cUIControlAnim::CreateParams BtnParams( Params );
	BtnParams.Parent( this );
	BtnParams.Size = eeSize( 16, 16 );

	if ( BtnParams.Flags & UI_CLIP_ENABLE )
		BtnParams.Flags &= ~UI_CLIP_ENABLE;

	mPushUp		= eeNew( cUIControlAnim, ( BtnParams ) );
	mPushDown 	= eeNew( cUIControlAnim, ( BtnParams ) );

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

cUISpinBox::~cUISpinBox() {
}

Uint32 cUISpinBox::Type() const {
	return UI_TYPE_SPINBOX;
}

bool cUISpinBox::IsType( const Uint32& type ) const {
	return cUISpinBox::Type() == type ? true : cUIComplexControl::IsType( type );
}

void cUISpinBox::SetTheme( cUITheme * Theme ) {
	cUIControl::SetThemeControl		( Theme, "spinbox" );

	mInput->SetThemeControl		( Theme, "spinbox_input" );
	mPushUp->SetThemeControl		( Theme, "spinbox_btnup" );
	mPushDown->SetThemeControl	( Theme, "spinbox_btndown" );

	cSubTexture * tSubTexture = NULL;
	cUISkin * tSkin = NULL;

	tSkin = mPushUp->GetSkin();

	if ( NULL != tSkin ) {
		tSubTexture = tSkin->GetSubTexture( cUISkinState::StateNormal );

		if ( NULL != tSubTexture ) {
			mPushUp->Size( tSubTexture->RealSize() );
		}
	}

	tSkin = mPushDown->GetSkin();

	if ( NULL != tSkin ) {
		tSubTexture = tSkin->GetSubTexture( cUISkinState::StateNormal );

		if ( NULL != tSubTexture ) {
			mPushDown->Size( tSubTexture->RealSize() );
		}
	}

	if ( mFlags & UI_AUTO_SIZE ) {
		mSize.Height( mInput->GetSkinSize().Height() );
	}

	AdjustChilds();
}

void cUISpinBox::AdjustChilds() {
	mPushUp->Pos( mSize.Width() - mPushUp->Size().Width(), 0 );
	mPushDown->Pos( mSize.Width() - mPushDown->Size().Width(), mPushUp->Size().Height() );
	mInput->Size( mSize.Width() - mPushUp->Size().Width(), mSize.Height() );
}

void cUISpinBox::Padding( const eeRecti& padding ) {
	mInput->Padding( padding );
}

const eeRecti& cUISpinBox::Padding() const {
	return mInput->Padding();
}

void cUISpinBox::ClickStep( const eeFloat& step ) {
	mClickStep = step;
}

const eeFloat& cUISpinBox::ClickStep() const {
	return mClickStep;
}

Uint32 cUISpinBox::OnMessage( const cUIMessage * Msg ) {
	switch ( Msg->Msg() ) {
		case cUIMessage::MsgClick:
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

void cUISpinBox::AddValue( const eeFloat& value ) {
	if ( !mInput->Text().size() )
		mInput->Text( String::ToStr( static_cast<Int32>( mMinValue ) ) );

	Value( mValue + value );
}

void cUISpinBox::InternalValue( const eeFloat& Val, const bool& Force ) {
	if ( Force || Val != mValue ) {
		if ( Val >= mMinValue && Val <= mMaxValue ) {
			eeFloat iValN	= (eeFloat)(Int32) Val;
			eeFloat fValN 	= (eeFloat)iValN;

			if ( fValN == Val ) {
				mInput->Text( String::ToStr( iValN ) );
			} else {
				mInput->Text( String::ToStr( Val ) );
			}

			mValue = Val;

			OnValueChange();
		}
	}
}

void cUISpinBox::Value( const eeFloat& Val ) {
	InternalValue( Val, false );
}

const eeFloat& cUISpinBox::Value() const {
	return mValue;
}

void cUISpinBox::MinValue( const eeFloat& MinVal ) {
	mMinValue = MinVal;

	if ( mValue < mMinValue )
		mValue = mMinValue;
}

const eeFloat& cUISpinBox::MinValue() const {
	return mMinValue;
}

void cUISpinBox::MaxValue( const eeFloat& MaxVal ) {
	mMaxValue = MaxVal;

	if ( mValue > mMaxValue )
		mValue = mMaxValue;
}

const eeFloat& cUISpinBox::MaxValue() const {
	return mMaxValue;
}

void cUISpinBox::Update() {
	bool Changed = mInput->GetInputTextBuffer()->ChangedSinceLastUpdate();

	cUIControlAnim::Update();

	if ( Changed ) {
		if ( !mInput->Text().size() ) {
			Value( 0 );
		} else {
			eeFloat Val = mValue;

			if ( '.' == mInput->Text()[ mInput->Text().size() - 1 ] ) {
				Uint32 pos = (Uint32)mInput->Text().find_first_of( "." );

				if ( pos != mInput->Text().size() - 1 )
					mInput->Text( mInput->Text().substr( 0, mInput->Text().size() - 1 ) );
			} else {
				bool Res 	= String::FromString<eeFloat>( Val, mInput->Text() );

				if ( Res )
					Value( Val );
			}
		}
	}
}

cUIControlAnim * cUISpinBox::ButtonPushUp() const {
	return mPushUp;
}

cUIControlAnim * cUISpinBox::ButtonPushDown() const {
	return mPushDown;
}

cUITextInput * cUISpinBox::TextInput() const {
	return mInput;
}

void cUISpinBox::OnAlphaChange() {
	cUIControlAnim::OnAlphaChange();
	
	mInput->Alpha( mAlpha );
	mPushUp->Alpha( mAlpha );
	mPushDown->Alpha( mAlpha );
}

}}
