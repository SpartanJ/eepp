#include <eepp/ui/uicomplexcontrol.hpp>
#include <eepp/ui/uimanager.hpp>

namespace EE { namespace UI {

UIComplexControl::UIComplexControl( const UIComplexControl::CreateParams& Params ) :
	UIControlAnim( Params ),
	mTooltip( NULL ),
	mMinControlSize( Params.MinControlSize )
{
	mControlFlags |= UI_CTRL_FLAG_COMPLEX;

	UpdateAnchorsDistances();

	TooltipText( Params.TooltipText );
}

UIComplexControl::~UIComplexControl() {
	eeSAFE_DELETE( mTooltip );
}

Uint32 UIComplexControl::Type() const {
	return UI_TYPE_CONTROL_COMPLEX;
}

bool UIComplexControl::IsType( const Uint32& type ) const {
	return UIComplexControl::Type() == type ? true : UIControlAnim::IsType( type );
}

void UIComplexControl::UpdateAnchorsDistances() {
	if ( NULL != mParentCtrl ) {
		mDistToBorder	= Recti( mPos.x, mPos.y, mParentCtrl->Size().x - ( mPos.x + mSize.x ), mParentCtrl->Size().y - ( mPos.y + mSize.y ) );
	}
}

void UIComplexControl::Update() {
	if ( mVisible && NULL != mTooltip && mTooltip->Text().size() ) {
		if ( IsMouseOverMeOrChilds() ) {
			Vector2i Pos = UIManager::instance()->GetMousePos();
			Pos.x += UIThemeManager::instance()->CursorSize().x;
			Pos.y += UIThemeManager::instance()->CursorSize().y;

			if ( Pos.x + mTooltip->Size().Width() > UIManager::instance()->MainControl()->Size().Width() ) {
				Pos.x = UIManager::instance()->GetMousePos().x - mTooltip->Size().Width();
			}

			if ( Pos.y + mTooltip->Size().Height() > UIManager::instance()->MainControl()->Size().Height() ) {
				Pos.y = UIManager::instance()->GetMousePos().y - mTooltip->Size().Height();
			}

			if ( Time::Zero == UIThemeManager::instance()->TooltipTimeToShow() ) {
				if ( !mTooltip->Visible() || UIThemeManager::instance()->TooltipFollowMouse() )
					mTooltip->Pos( Pos );

				mTooltip->Show();
			} else {
				if ( -1.f != mTooltip->TooltipTime().AsMilliseconds() ) {
					mTooltip->TooltipTimeAdd( UIManager::instance()->Elapsed() );
				}

				if ( mTooltip->TooltipTime() >= UIThemeManager::instance()->TooltipTimeToShow() ) {
					if ( mTooltip->TooltipTime().AsMilliseconds() != -1.f ) {
						mTooltip->Pos( Pos );

						mTooltip->Show();

						mTooltip->TooltipTime( Milliseconds( -1.f ) );
					}
				}
			}

			if ( UIThemeManager::instance()->TooltipFollowMouse() ) {
				mTooltip->Pos( Pos );
			}
		} else {
			mTooltip->TooltipTime( Milliseconds( 0.f ) );

			if ( mTooltip->Visible() )
				mTooltip->Hide();
		}
	}

	UIControlAnim::Update();
}

void UIComplexControl::CreateTooltip() {
	if ( NULL != mTooltip )
		return;

	UITheme * tTheme = UIThemeManager::instance()->DefaultTheme();

	if ( NULL != tTheme ) {
		mTooltip = tTheme->CreateTooltip( this, UIManager::instance()->MainControl() );
		mTooltip->Visible( false );
		mTooltip->Enabled( false );
	} else {
		UITooltip::CreateParams Params;
		Params.Parent( UIManager::instance()->MainControl() );
		Params.Flags = UI_VALIGN_CENTER | UI_HALIGN_CENTER | UI_AUTO_PADDING | UI_AUTO_SIZE;
		mTooltip = eeNew( UITooltip, ( Params, this ) );
	}
}

void UIComplexControl::TooltipText( const String& Text ) {
	if ( NULL == mTooltip ) {	// If the tooltip wasn't created it will avoid to create a new one if the string is ""
		if ( Text.size() ) {
			CreateTooltip();

			mTooltip->Text( Text );
		}
	} else { // but if it's created, i will allow it
		mTooltip->Text( Text );
	}
}

String UIComplexControl::TooltipText() {
	if ( NULL != mTooltip )
		return mTooltip->Text();

	return String();
}

void UIComplexControl::TooltipRemove() {
	mTooltip = NULL;
}

void UIComplexControl::Size( const Sizei &Size ) {
	Sizei s( Size );

	if ( s.x < mMinControlSize.x )
		s.x = mMinControlSize.x;

	if ( s.y < mMinControlSize.y )
		s.y = mMinControlSize.y;

	UIControlAnim::Size( s );
}

void UIComplexControl::Size( const Int32& Width, const Int32& Height ) {
	UIControlAnim::Size( Width, Height );
}

const Sizei& UIComplexControl::Size() {
	return UIControlAnim::Size();
}

void UIComplexControl::OnParentSizeChange( const Vector2i& SizeChange ) {
	Sizei newSize( mSize );

	if ( mFlags & UI_ANCHOR_LEFT ) {
		// Nothing ?
	} else {
		Pos( mPos.x += SizeChange.x, mPos.y );
	}

	if ( mFlags & UI_ANCHOR_RIGHT ) {
		if ( NULL != mParentCtrl ) {
			newSize.x = mParentCtrl->Size().Width() - mPos.x - mDistToBorder.Right;

			if ( newSize.x < mMinControlSize.Width() )
				newSize.x = mMinControlSize.Width();
		}
	}

	if ( mFlags & UI_ANCHOR_TOP ) {
		// Nothing ?
	} else {
		Pos( mPos.x, mPos.y += SizeChange.y );
	}

	if ( mFlags & UI_ANCHOR_BOTTOM ) {
		if ( NULL != mParentCtrl ) {
			newSize.y = mParentCtrl->Size().y - mPos.y - mDistToBorder.Bottom;

			if ( newSize.y < mMinControlSize.Height() )
				newSize.y = mMinControlSize.Height();
		}
	}

	if ( newSize != mSize )
		Size( newSize );

	UIControlAnim::OnParentSizeChange( SizeChange );
}

}}
