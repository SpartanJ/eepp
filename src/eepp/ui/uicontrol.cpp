#include <eepp/ui/uicontrol.hpp>
#include <eepp/ui/uitheme.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/subtexture.hpp>
#include <eepp/graphics/renderer/gl.hpp>

namespace EE { namespace UI {

UIControl::UIControl( const CreateParams& Params ) :
	mPos( Params.Pos ),
	mSize( Params.Size ),
	mFlags( Params.Flags ),
	mData( 0 ),
	mParentCtrl( Params.ParentCtrl ),
	mChild( NULL ),
	mChildLast( NULL ),
	mNext( NULL ),
	mPrev( NULL ),
	mSkinState( NULL ),
	mBackground( NULL ),
	mBorder( NULL ),
	mControlFlags( 0 ),
	mBlend( Params.Blend ),
	mNumCallBacks( 0 ),
	mVisible( false ),
	mEnabled( false )
{
	if ( NULL == mParentCtrl && NULL != UIManager::instance()->MainControl() ) {
		mParentCtrl = UIManager::instance()->MainControl();
	}

	if ( NULL != mParentCtrl )
		mParentCtrl->ChildAdd( this );

	if ( mFlags & UI_FILL_BACKGROUND )
		mBackground = eeNew( UIBackground, ( Params.Background ) );

	if ( mFlags & UI_BORDER )
		mBorder = eeNew( UIBorder, ( Params.Border ) );

	UpdateScreenPos();
	UpdateQuad();
}

UIControl::~UIControl() {
	SafeDeleteSkinState();
	eeSAFE_DELETE( mBackground );
	eeSAFE_DELETE( mBorder );

	ChildDeleteAll();

	if ( NULL != mParentCtrl )
		mParentCtrl->ChildRemove( this );

	if ( UIManager::instance()->FocusControl() == this && UIManager::instance()->MainControl() != this ) {
		UIManager::instance()->FocusControl( UIManager::instance()->MainControl() );
	}

	if ( UIManager::instance()->OverControl() == this && UIManager::instance()->MainControl() != this ) {
		UIManager::instance()->OverControl( UIManager::instance()->MainControl() );
	}
}

void UIControl::ScreenToControl( Vector2i& Pos ) const {
	UIControl * ParentLoop = mParentCtrl;

	Pos.x -= mPos.x;
	Pos.y -= mPos.y;

	while ( NULL != ParentLoop ) {
		const Vector2i& ParentPos = ParentLoop->Pos();

		Pos.x -= ParentPos.x;
		Pos.y -= ParentPos.y;

		ParentLoop = ParentLoop->Parent();
	}
}

void UIControl::ControlToScreen( Vector2i& Pos ) const {
	UIControl * ParentLoop = mParentCtrl;

	while ( NULL != ParentLoop ) {
		const Vector2i& ParentPos = ParentLoop->Pos();

		Pos.x += ParentPos.x;
		Pos.y += ParentPos.y;

		ParentLoop = ParentLoop->Parent();
	}
}

Uint32 UIControl::Type() const {
	return UI_TYPE_CONTROL;
}

bool UIControl::IsType( const Uint32& type ) const {
	return UIControl::Type() == type;
}

void UIControl::MessagePost( const UIMessage * Msg ) {
	UIControl * Ctrl = this;

	while( NULL != Ctrl ) {
		if ( Ctrl->OnMessage( Msg ) )
			break;

		Ctrl = Ctrl->Parent();
	}
}

Uint32 UIControl::OnMessage( const UIMessage * Msg ) {
	return 0;
}

bool UIControl::IsInside( const Vector2i& Pos ) const {
	return ( Pos.x >= 0 && Pos.y >= 0 && Pos.x < mSize.Width() && Pos.y < mSize.Height() );
}

void UIControl::Pos( const Vector2i& Pos ) {
	mPos = Pos;

	OnPosChange();
}

void UIControl::Pos( const Int32& x, const Int32& y ) {
	mPos = Vector2i( x, y );

	OnPosChange();
}

const Vector2i& UIControl::Pos() const {
	return mPos;
}

void UIControl::Size( const Sizei& Size ) {
	if ( Size != mSize ) {
		Vector2i sizeChange( Size.x - mSize.x, Size.y - mSize.y );

		mSize = Size;

		OnSizeChange();

		if ( mFlags & UI_REPORT_SIZE_CHANGE_TO_CHILDS ) {
			SendParentSizeChange( sizeChange );
		}
	}
}

void UIControl::Size( const Int32& Width, const Int32& Height ) {
	Size( Sizei( Width, Height ) );
}

Recti UIControl::Rect() const {
	return Recti( mPos, mSize );
}

const Sizei& UIControl::Size() {
	return mSize;
}

void UIControl::Visible( const bool& visible ) {
	mVisible = visible;
	OnVisibleChange();
}

bool UIControl::Visible() const {
	return mVisible;
}

bool UIControl::Hided() const {
	return !mVisible;
}

void UIControl::Enabled( const bool& enabled ) {
	mEnabled = enabled;
	OnEnabledChange();
}

bool UIControl::Enabled() const {
	return mEnabled;
}

bool UIControl::Disabled() const {
	return !mEnabled;
}

UIControl * UIControl::Parent() const {
	return mParentCtrl;
}

void UIControl::Parent( UIControl * parent ) {
	if ( parent == mParentCtrl )
		return;

	if ( NULL != mParentCtrl )
		mParentCtrl->ChildRemove( this );

	mParentCtrl = parent;

	if ( NULL != mParentCtrl )
		mParentCtrl->ChildAdd( this );
}

bool UIControl::IsParentOf( UIControl * Ctrl ) {
	eeASSERT( NULL != Ctrl );

	UIControl * tParent = Ctrl->Parent();

	while ( NULL != tParent ) {
		if ( this == tParent )
			return true;

		tParent = tParent->Parent();
	}

	return false;
}

void UIControl::CenterHorizontal() {
	UIControl * Ctrl = Parent();

	if ( NULL != Ctrl )
		Pos( Vector2i( ( Ctrl->Size().Width() / 2 ) - ( mSize.Width() / 2 ), mPos.y ) );
}

void UIControl::CenterVertical(){
	UIControl * Ctrl = Parent();

	if ( NULL != Ctrl )
		Pos( Vector2i( mPos.x, ( Ctrl->Size().Height() / 2 ) - ( mSize.Height() / 2 ) ) );
}

void UIControl::Center() {
	CenterHorizontal();
	CenterVertical();
}

void UIControl::Close() {
	mControlFlags |= UI_CTRL_FLAG_CLOSE;

	UIManager::instance()->AddToCloseQueue( this );
}

void UIControl::Draw() {
	if ( mVisible ) {
		if ( mFlags & UI_FILL_BACKGROUND )
			BackgroundDraw();

		if ( mFlags & UI_BORDER )
			BorderDraw();

		if ( NULL != mSkinState )
			mSkinState->Draw( mScreenPosf.x, mScreenPosf.y, (Float)mSize.Width(), (Float)mSize.Height(), 255 );

		if ( UIManager::instance()->HighlightFocus() && UIManager::instance()->FocusControl() == this ) {
			Primitives P;
			P.FillMode( DRAW_LINE );
			P.BlendMode( Blend() );
			P.SetColor( UIManager::instance()->HighlightFocusColor() );
			P.DrawRectangle( GetRectf() );
		}

		if ( UIManager::instance()->HighlightOver() && UIManager::instance()->OverControl() == this ) {
			Primitives P;
			P.FillMode( DRAW_LINE );
			P.BlendMode( Blend() );
			P.SetColor( UIManager::instance()->HighlightOverColor() );
			P.DrawRectangle( GetRectf() );
		}
	}
}

void UIControl::Update() {
	UIControl * ChildLoop = mChild;

	while ( NULL != ChildLoop ) {
		ChildLoop->Update();
		ChildLoop = ChildLoop->mNext;
	}

	if ( mControlFlags & UI_CTRL_FLAG_MOUSEOVER_ME_OR_CHILD )
		WriteCtrlFlag( UI_CTRL_FLAG_MOUSEOVER_ME_OR_CHILD, 0 );
}

void UIControl::SendMouseEvent( const Uint32& Event, const Vector2i& Pos, const Uint32& Flags ) {
	UIEventMouse MouseEvent( this, Event, Pos, Flags );
	SendEvent( &MouseEvent );
}

void UIControl::SendCommonEvent( const Uint32& Event ) {
	UIEvent CommonEvent( this, Event );
	SendEvent( &CommonEvent );
}

Uint32 UIControl::OnKeyDown( const UIEventKey& Event ) {
	SendEvent( &Event );
	return 0;
}

Uint32 UIControl::OnKeyUp( const UIEventKey& Event ) {
	SendEvent( &Event );
	return 0;
}

Uint32 UIControl::OnMouseMove( const Vector2i& Pos, const Uint32 Flags ) {
	SendMouseEvent( UIEvent::EventMouseMove, Pos, Flags );
	return 1;
}

Uint32 UIControl::OnMouseDown( const Vector2i& Pos, const Uint32 Flags ) {
	SendMouseEvent( UIEvent::EventMouseDown, Pos, Flags );

	SetSkinState( UISkinState::StateMouseDown );

	return 1;
}

Uint32 UIControl::OnMouseUp( const Vector2i& Pos, const Uint32 Flags ) {
	SendMouseEvent( UIEvent::EventMouseUp, Pos, Flags );

	SetPrevSkinState();

	return 1;
}

Uint32 UIControl::OnMouseClick( const Vector2i& Pos, const Uint32 Flags ) {
	SendMouseEvent( UIEvent::EventMouseClick, Pos, Flags );
	return 1;
}

bool UIControl::IsMouseOver() {
	return 0 != ( mControlFlags & UI_CTRL_FLAG_MOUSEOVER );
}

bool UIControl::IsMouseOverMeOrChilds() {
	return 0 != ( mControlFlags & UI_CTRL_FLAG_MOUSEOVER_ME_OR_CHILD );
}

Uint32 UIControl::OnMouseDoubleClick( const Vector2i& Pos, const Uint32 Flags ) {
	SendMouseEvent( UIEvent::EventMouseDoubleClick, Pos, Flags );
	return 1;
}

Uint32 UIControl::OnMouseEnter( const Vector2i& Pos, const Uint32 Flags ) {
	WriteCtrlFlag( UI_CTRL_FLAG_MOUSEOVER, 1 );

	SendMouseEvent( UIEvent::EventMouseEnter, Pos, Flags );

	SetSkinState( UISkinState::StateMouseEnter );

	return 1;
}

Uint32 UIControl::OnMouseExit( const Vector2i& Pos, const Uint32 Flags ) {
	WriteCtrlFlag( UI_CTRL_FLAG_MOUSEOVER, 0 );

	SendMouseEvent( UIEvent::EventMouseExit, Pos, Flags );

	SetSkinState( UISkinState::StateMouseExit );

	return 1;
}

Uint32 UIControl::OnFocus() {
	mControlFlags |= UI_CTRL_FLAG_HAS_FOCUS;

	SendCommonEvent( UIEvent::EventOnFocus );

	SetSkinState( UISkinState::StateFocus );

	return 1;
}

Uint32 UIControl::OnFocusLoss() {
	mControlFlags &= ~UI_CTRL_FLAG_HAS_FOCUS;

	SendCommonEvent( UIEvent::EventOnFocusLoss );

	return 1;
}

void UIControl::OnComplexControlFocusLoss() {
	SendCommonEvent( UIEvent::EventOnComplexControlFocusLoss );
}

bool UIControl::HasFocus() const {
	return 0 != ( mControlFlags & UI_CTRL_FLAG_HAS_FOCUS );
}

Uint32 UIControl::OnValueChange() {
	SendCommonEvent( UIEvent::EventOnValueChange );

	return 1;
}

void UIControl::OnClose() {
	SendCommonEvent( UIEvent::EventOnClose );
}

Uint32 UIControl::HAlign() const {
	return mFlags & UI_HALIGN_MASK;
}

void UIControl::HAlign( Uint32 halign ) {
	mFlags |= halign & UI_HALIGN_MASK;
}

Uint32 UIControl::VAlign() const {
	return mFlags & UI_VALIGN_MASK;
}

void UIControl::VAlign( Uint32 valign ) {
	mFlags |= valign & UI_VALIGN_MASK;
}

void UIControl::FillBackground( bool enabled ) {
	WriteFlag( UI_FILL_BACKGROUND, enabled ? 1 : 0 );

	if ( enabled && NULL == mBackground ) {
		mBackground = eeNew( UIBackground, () );
	}
}

void UIControl::Border( bool enabled ) {
	WriteFlag( UI_BORDER, enabled ? 1 : 0 );

	if ( enabled && NULL == mBorder ) {
		mBorder = eeNew( UIBorder, () );

		if ( NULL == mBackground ) {
			mBackground = eeNew( UIBackground, () );
		}
	}
}

UIControl * UIControl::NextGet() const {
	return mNext;
}

UIControl * UIControl::PrevGet() const {
	return mPrev;
}

UIControl * UIControl::NextGetLoop() const {
	if ( NULL == mNext )
		return Parent()->ChildGetFirst();
	else
		return mNext;
}

void UIControl::Data(const UintPtr& data ) {
	mData = data;
}

const UintPtr& UIControl::Data() const {
	return mData;
}

const Uint32& UIControl::Flags() const {
	return mFlags;
}

void UIControl::Flags( const Uint32& flags ) {
	mFlags |= flags;
}

void UIControl::Blend( const EE_BLEND_MODE& blend ) {
	mBlend = static_cast<Uint16> ( blend );
}

EE_BLEND_MODE UIControl::Blend() {
	return static_cast<EE_BLEND_MODE> ( mBlend );
}

void UIControl::ToFront() {
	if ( NULL != mParentCtrl ) {
		mParentCtrl->ChildRemove( this );
		mParentCtrl->ChildAdd( this );
	}
}

void UIControl::ToBack() {
	if ( NULL != mParentCtrl ) {
		mParentCtrl->ChildAddAt( this, 0 );
	}
}

void UIControl::ToPos( const Uint32& Pos ) {
	if ( NULL != mParentCtrl ) {
		mParentCtrl->ChildAddAt( this, Pos );
	}
}

void UIControl::OnVisibleChange() {
	SendCommonEvent( UIEvent::EventOnVisibleChange );
}

void UIControl::OnEnabledChange() {
	if ( !Enabled() && NULL != UIManager::instance()->FocusControl() ) {
		if ( IsChild( UIManager::instance()->FocusControl() ) ) {
			UIManager::instance()->FocusControl( NULL );
		}
	}

	SendCommonEvent( UIEvent::EventOnEnabledChange );
}

void UIControl::OnPosChange() {
	UpdateScreenPos();

	UpdateChildsScreenPos();

	SendCommonEvent( UIEvent::EventOnPosChange );
}

void UIControl::OnSizeChange() {
	UpdateCenter();

	SendCommonEvent( UIEvent::EventOnSizeChange );
}

Rectf UIControl::GetRectf() {
	return Rectf( mScreenPosf, Sizef( (Float)mSize.Width(), (Float)mSize.Height() ) );
}

void UIControl::BackgroundDraw() {
	Primitives P;
	Rectf R = GetRectf();
	P.BlendMode( mBackground->Blend() );
	P.SetColor( mBackground->Color() );

	if ( 4 == mBackground->Colors().size() ) {
		if ( mBackground->Corners() ) {
			P.DrawRoundedRectangle( R, mBackground->Colors()[0], mBackground->Colors()[1], mBackground->Colors()[2], mBackground->Colors()[3], mBackground->Corners() );
		} else {
			P.DrawRectangle( R, mBackground->Colors()[0], mBackground->Colors()[1], mBackground->Colors()[2], mBackground->Colors()[3] );
		}
	} else {
		if ( mBackground->Corners() ) {
			P.DrawRoundedRectangle( R, 0.f, Vector2f::One, mBackground->Corners() );
		} else {
			P.DrawRectangle( R );
		}
	}
}

void UIControl::BorderDraw() {
	Primitives P;
	P.FillMode( DRAW_LINE );
	P.BlendMode( Blend() );
	P.LineWidth( (Float)mBorder->Width() );
	P.SetColor( mBorder->Color() );

	//! @TODO: Check why was this +0.1f -0.1f?
	if ( mFlags & UI_CLIP_ENABLE ) {
		Rectf R( Vector2f( mScreenPosf.x + 0.1f, mScreenPosf.y + 0.1f ), Sizef( (Float)mSize.Width() - 0.1f, (Float)mSize.Height() - 0.1f ) );

		if ( mBackground->Corners() ) {
			P.DrawRoundedRectangle( GetRectf(), 0.f, Vector2f::One, mBackground->Corners() );
		} else {
			P.DrawRectangle( R );
		}
	} else {
		if ( mBackground->Corners() ) {
			P.DrawRoundedRectangle( GetRectf(), 0.f, Vector2f::One, mBackground->Corners() );
		} else {
			P.DrawRectangle( GetRectf() );
		}
	}
}

const Uint32& UIControl::ControlFlags() const {
	return mControlFlags;
}

void UIControl::ControlFlags( const Uint32& Flags ) {
	mControlFlags = Flags;
}

void UIControl::DrawChilds() {
	UIControl * ChildLoop = mChild;

	while ( NULL != ChildLoop ) {
		if ( ChildLoop->mVisible ) {
			ChildLoop->InternalDraw();
		}

		ChildLoop = ChildLoop->mNext;
	}
}

void UIControl::InternalDraw() {
	if ( mVisible ) {
		MatrixSet();

		ClipMe();

		Draw();

		DrawChilds();

		ClipDisable();

		MatrixUnset();
	}
}

void UIControl::ClipMe() {
	if ( mFlags & UI_CLIP_ENABLE ) {
		if ( mFlags & UI_BORDER )
			UIManager::instance()->ClipEnable( mScreenPos.x, mScreenPos.y, mSize.Width(), mSize.Height() + 1 );
		else
			UIManager::instance()->ClipEnable( mScreenPos.x, mScreenPos.y, mSize.Width(), mSize.Height() );
	}
}

void UIControl::ClipDisable() {
	if ( mFlags & UI_CLIP_ENABLE )
		UIManager::instance()->ClipDisable();
}

void UIControl::MatrixSet() {
}

void UIControl::MatrixUnset() {
}

void UIControl::ChildDeleteAll() {
	while( NULL != mChild ) {
		eeDelete( mChild );
	}
}

void UIControl::ChildAdd( UIControl * ChildCtrl ) {
	if ( NULL == mChild ) {
		mChild 		= ChildCtrl;
		mChildLast 	= ChildCtrl;
	} else {
		mChildLast->mNext 		= ChildCtrl;
		ChildCtrl->mPrev		= mChildLast;
		ChildCtrl->mNext		= NULL;
		mChildLast 				= ChildCtrl;
	}
}

void UIControl::ChildAddAt( UIControl * ChildCtrl, Uint32 Pos ) {
	eeASSERT( NULL != ChildCtrl );

	UIControl * ChildLoop = mChild;
	
	ChildCtrl->Parent( this );

	ChildRemove( ChildCtrl );
	ChildCtrl->mParentCtrl = this;
	
	if ( ChildLoop == NULL ) {
		mChild 				= ChildCtrl;
		mChildLast			= ChildCtrl;
		ChildCtrl->mNext 	= NULL;
		ChildCtrl->mPrev 	= NULL;
	} else {
		if( Pos == 0 ) {
			if ( NULL != mChild ) {
				mChild->mPrev		= ChildCtrl;
			}

			ChildCtrl->mNext 	= mChild;
			ChildCtrl->mPrev	= NULL;
			mChild 				= ChildCtrl;
		} else {
			Uint32 i = 0;

			while ( NULL != ChildLoop->mNext && i < Pos ) {
				ChildLoop = ChildLoop->mNext;
				i++;
			}

			UIControl * ChildTmp = ChildLoop->mNext;
			ChildLoop->mNext 	= ChildCtrl;
			ChildCtrl->mPrev 	= ChildLoop;
			ChildCtrl->mNext 	= ChildTmp;

			if ( NULL != ChildTmp ) {
				ChildTmp->mPrev = ChildCtrl;
			} else {
				mChildLast		= ChildCtrl;
			}
		}
	}
}

void UIControl::ChildRemove( UIControl * ChildCtrl ) {
	if ( ChildCtrl == mChild ) {
		mChild 			= mChild->mNext;

		if ( NULL != mChild ) {
			mChild->mPrev 	= NULL;

			if ( ChildCtrl == mChildLast )
				mChildLast		= mChild;
		} else {
			mChildLast		= NULL;
		}
	} else {
		if ( mChildLast == ChildCtrl )
			mChildLast = mChildLast->mPrev;

		ChildCtrl->mPrev->mNext = ChildCtrl->mNext;

		if ( NULL != ChildCtrl->mNext ) {
			ChildCtrl->mNext->mPrev = ChildCtrl->mPrev;
			ChildCtrl->mNext = NULL;
		}

		ChildCtrl->mPrev = NULL;
	}
}

void UIControl::ChildsCloseAll() {
	UIControl * ChildLoop = mChild;

	while ( NULL != ChildLoop ) {
		ChildLoop->Close();
		ChildLoop = ChildLoop->mNext;
	}
}

bool UIControl::IsChild( UIControl * ChildCtrl ) const {
	UIControl * ChildLoop = mChild;

	while ( NULL != ChildLoop ) {
		if ( ChildCtrl == ChildLoop )
			return true;

		ChildLoop = ChildLoop->mNext;
	}

	return false;
}

bool UIControl::InParentTreeOf( UIControl * Child ) const {
	UIControl * ParentLoop = Child->mParentCtrl;

	while ( NULL != ParentLoop ) {
		if ( ParentLoop == this )
			return true;

		ParentLoop = ParentLoop->mParentCtrl;
	}

	return false;
}

Uint32 UIControl::ChildCount() const {
	UIControl * ChildLoop = mChild;
	Uint32 Count = 0;

	while( NULL != ChildLoop ) {
		ChildLoop = ChildLoop->mNext;
		Count++;
	}

	return Count;
}

UIControl * UIControl::ChildAt( Uint32 Index ) const {
	UIControl * ChildLoop = mChild;

	while( NULL != ChildLoop && Index ) {
		ChildLoop = ChildLoop->mNext;
		Index--;
	}

	return ChildLoop;
}

UIControl * UIControl::ChildPrev( UIControl * Ctrl, bool Loop ) const {
	if ( Loop && Ctrl == mChild && NULL != mChild->mNext )
		return mChildLast;

	return Ctrl->mPrev;
}

UIControl * UIControl::ChildNext( UIControl * Ctrl, bool Loop ) const {
	if ( NULL == Ctrl->mNext && Loop )
		return mChild;

	return Ctrl->mNext;
}

UIControl * UIControl::ChildGetFirst() const {
	return mChild;
}

UIControl * UIControl::ChildGetLast() const {
	return mChildLast;
}

UIControl * UIControl::OverFind( const Vector2f& Point ) {
	UIControl * pOver = NULL;

	if ( mEnabled && mVisible ) {
		UpdateQuad();

		if ( mPoly.PointInside( Point ) ) {
			WriteCtrlFlag( UI_CTRL_FLAG_MOUSEOVER_ME_OR_CHILD, 1 );

			UIControl * ChildLoop = mChildLast;

			while ( NULL != ChildLoop ) {
				UIControl * ChildOver = ChildLoop->OverFind( Point );

				if ( NULL != ChildOver ) {
					pOver = ChildOver;

					break; // Search from top to bottom, so the first over will be the topmost
				}

				ChildLoop = ChildLoop->mPrev;
			}


			if ( NULL == pOver )
				pOver = this;
		}
	}

	return pOver;
}

UIControl * UIControl::ChildGetAt( Vector2i CtrlPos, unsigned int RecursiveLevel ) {
	UIControl * Ctrl = NULL;

	for( UIControl * pLoop = mChild; NULL != pLoop && NULL == Ctrl; pLoop = pLoop->mNext )
	{
		if ( !pLoop->Visible() )
			continue;

		if ( pLoop->Rect().Contains( CtrlPos ) ) {
			if ( RecursiveLevel )
				Ctrl = ChildGetAt( CtrlPos - pLoop->Pos(), RecursiveLevel - 1 );

			if ( NULL == Ctrl )
				Ctrl = pLoop;
		}
	}

	return Ctrl;
}

Uint32 UIControl::IsAnimated() {
	return mControlFlags & UI_CTRL_FLAG_ANIM;
}

Uint32 UIControl::IsDragable() {
	return mControlFlags & UI_CTRL_FLAG_DRAGABLE;
}

Uint32 UIControl::IsComplex() {
	return mControlFlags & UI_CTRL_FLAG_COMPLEX;
}

Uint32 UIControl::IsClipped() {
	return mFlags & UI_CLIP_ENABLE;
}

Polygon2f& UIControl::GetPolygon() {
	return mPoly;
}

const Vector2f& UIControl::GetPolygonCenter() const {
	return mCenter;
}

void UIControl::UpdateQuad() {
	mPoly 	= Polygon2f( eeAABB( mScreenPosf.x, mScreenPosf.y, mScreenPosf.x + mSize.Width(), mScreenPosf.y + mSize.Height() ) );

	UIControl * tParent = Parent();

	while ( tParent ) {
		if ( tParent->IsAnimated() ) {
			UIControlAnim * tP = reinterpret_cast<UIControlAnim *> ( tParent );

			mPoly.Rotate( tP->Angle(), tP->RotationCenter() );
			mPoly.Scale( tP->Scale(), tP->ScaleCenter() );
		}

		tParent = tParent->Parent();
	};
}

void UIControl::UpdateCenter() {
	mCenter = Vector2f( mScreenPosf.x + (Float)mSize.Width() * 0.5f, mScreenPosf.y + (Float)mSize.Height() * 0.5f );
}

Time UIControl::Elapsed() {
	return UIManager::instance()->Elapsed();
}

Uint32 UIControl::AddEventListener( const Uint32& EventType, const UIEventCallback& Callback ) {
	mNumCallBacks++;

	mEvents[ EventType ][ mNumCallBacks ] = Callback;

	return mNumCallBacks;
}

void UIControl::RemoveEventListener( const Uint32& CallbackId ) {
	UIEventsMap::iterator it;

	for ( it = mEvents.begin(); it != mEvents.end(); ++it ) {
		std::map<Uint32, UIEventCallback> event = it->second;

		if ( event.erase( CallbackId ) )
			break;
	}
}

void UIControl::SendEvent( const UIEvent * Event ) {
	if ( 0 != mEvents.count( Event->EventType() ) ) {
		std::map<Uint32, UIEventCallback>			event = mEvents[ Event->EventType() ];
		std::map<Uint32, UIEventCallback>::iterator it;

		if ( event.begin() != event.end() ) {
			for ( it = event.begin(); it != event.end(); ++it )
				it->second( Event );
		}
	}
}

UIBackground * UIControl::Background() {
	return mBackground;
}

UIBorder * UIControl::Border() {
	return mBorder;
}

void UIControl::SetThemeByName( const std::string& Theme ) {
	SetTheme( UIThemeManager::instance()->GetByName( Theme ) );
}

void UIControl::SetTheme( UITheme * Theme ) {
	SetThemeControl( Theme, "control" );
}

void UIControl::SafeDeleteSkinState() {
	if ( NULL != mSkinState && ( mControlFlags & UI_CTRL_FLAG_SKIN_OWNER ) ) {
		UISkin * tSkin = mSkinState->GetSkin();

		eeSAFE_DELETE( tSkin );
	}

	eeSAFE_DELETE( mSkinState );
}

void UIControl::SetThemeControl( UITheme * Theme, const std::string& ControlName ) {
	if ( NULL != Theme ) {
		UISkin * tSkin = Theme->GetByName( Theme->Abbr() + "_" + ControlName );

		if ( NULL != tSkin ) {
			Uint32 InitialState = UISkinState::StateNormal;

			if ( NULL != mSkinState ) {
				InitialState = mSkinState->GetState();
			}

			SafeDeleteSkinState();

			mSkinState = eeNew( UISkinState, ( tSkin ) );
			mSkinState->SetState( InitialState );
		}
	}
}

void UIControl::SetSkinFromTheme( UITheme * Theme, const std::string& ControlName ) {
	SetThemeControl( Theme, ControlName );
}

void UIControl::SetSkin( const UISkin& Skin ) {
	SafeDeleteSkinState();

	WriteCtrlFlag( UI_CTRL_FLAG_SKIN_OWNER, 1 );

	UISkin * SkinCopy = const_cast<UISkin*>( &Skin )->Copy();

	mSkinState = eeNew( UISkinState, ( SkinCopy ) );

	DoAfterSetTheme();
}

void UIControl::OnStateChange() {
}

void UIControl::SetSkinState( const Uint32& State ) {
	if ( NULL != mSkinState ) {
		mSkinState->SetState( State );

		OnStateChange();
	}
}

void UIControl::SetPrevSkinState() {
	if ( NULL != mSkinState ) {
		mSkinState->SetPrevState();

		OnStateChange();
	}
}

void UIControl::SetThemeToChilds( UITheme * Theme ) {
	UIControl * ChildLoop = mChild;

	while ( NULL != ChildLoop ) {
		ChildLoop->SetThemeToChilds( Theme );
		ChildLoop->SetTheme( Theme );	// First set the theme to childs to let the father override the childs forced themes

		ChildLoop = ChildLoop->mNext;
	}
}

void UIControl::UpdateChildsScreenPos() {
	UIControl * ChildLoop = mChild;

	while ( NULL != ChildLoop ) {
		ChildLoop->UpdateScreenPos();
		ChildLoop->UpdateChildsScreenPos();

		ChildLoop = ChildLoop->mNext;
	}
}

void UIControl::UpdateScreenPos() {
	Vector2i Pos( mPos );

	ControlToScreen( Pos );

	mScreenPos = Pos;
	mScreenPosf = Vector2f( Pos.x, Pos.y );

	UpdateCenter();
}

UISkin * UIControl::GetSkin() {
	if ( NULL != mSkinState )
		return mSkinState->GetSkin();

	return NULL;
}

void UIControl::WriteCtrlFlag( const Uint32& Flag, const Uint32& Val ) {
	BitOp::SetBitFlagValue( &mControlFlags, Flag, Val );
}

void UIControl::WriteFlag( const Uint32& Flag, const Uint32& Val ) {
	if ( Val )
		mFlags |= Flag;
	else {
		if ( mFlags & Flag )
			mFlags &= ~Flag;
	}
}

void UIControl::ApplyDefaultTheme() {
	UIThemeManager::instance()->ApplyDefaultTheme( this );
}

Recti UIControl::GetScreenRect() {
	return Recti( mScreenPos, mSize );
}

Recti UIControl::MakePadding( bool PadLeft, bool PadRight, bool PadTop, bool PadBottom, bool SkipFlags ) {
	Recti tPadding( 0, 0, 0, 0 );

	if ( mFlags & UI_AUTO_PADDING || SkipFlags ) {
		if ( NULL != mSkinState && NULL != mSkinState->GetSkin() ) {
			if ( mSkinState->GetSkin()->GetType() == UISkin::SkinComplex ) {
				UISkinComplex * tComplex = reinterpret_cast<UISkinComplex*> ( mSkinState->GetSkin() );

				SubTexture * tSubTexture = NULL;

				if ( PadLeft ) {
					tSubTexture = tComplex->GetSubTextureSide( UISkinState::StateNormal, UISkinComplex::Left );

					if ( NULL != tSubTexture )
						tPadding.Left = tSubTexture->RealSize().Width();
				}

				if ( PadRight ) {
					tSubTexture = tComplex->GetSubTextureSide( UISkinState::StateNormal, UISkinComplex::Right );

					if ( NULL != tSubTexture )
						tPadding.Right = tSubTexture->RealSize().Width();
				}

				if ( PadTop ) {
					tSubTexture = tComplex->GetSubTextureSide( UISkinState::StateNormal, UISkinComplex::Up );

					if ( NULL != tSubTexture )
						tPadding.Top = tSubTexture->RealSize().Height();
				}

				if ( PadBottom ) {
					tSubTexture = tComplex->GetSubTextureSide( UISkinState::StateNormal, UISkinComplex::Down );

					if ( NULL != tSubTexture )
						tPadding.Bottom = tSubTexture->RealSize().Height();
				}
			}
		}
	}

	return tPadding;
}

void UIControl::SetFocus() {
	UIManager::instance()->FocusControl( this );
}

void UIControl::SendParentSizeChange( const Vector2i& SizeChange ) {
	if ( mFlags & UI_REPORT_SIZE_CHANGE_TO_CHILDS )	{
		UIControl * ChildLoop = mChild;

		while( NULL != ChildLoop ) {
			ChildLoop->OnParentSizeChange( SizeChange );
			ChildLoop = ChildLoop->mNext;
		}
	}
}

void UIControl::OnParentSizeChange( const Vector2i& SizeChange ) {
	SendCommonEvent( UIEvent::EventOnParentSizeChange );
}

Sizei UIControl::GetSkinSize( UISkin * Skin, const Uint32& State ) {
	Sizei		tSize;

	if ( NULL != Skin ) {
		SubTexture * tSubTexture = Skin->GetSubTexture( State );

		if ( NULL != tSubTexture ) {
			tSize = tSubTexture->RealSize();
		}

		if ( Skin->GetType() == UISkin::SkinComplex ) {
			UISkinComplex * SkinC = reinterpret_cast<UISkinComplex*> ( Skin );

			tSubTexture = SkinC->GetSubTextureSide( State, UISkinComplex::Up );

			if ( NULL != tSubTexture ) {
				tSize.y += tSubTexture->RealSize().Height();
			}

			tSubTexture = SkinC->GetSubTextureSide( State, UISkinComplex::Down );

			if ( NULL != tSubTexture ) {
				tSize.y += tSubTexture->RealSize().Height();
			}

			tSubTexture = SkinC->GetSubTextureSide( State, UISkinComplex::Left );

			if ( NULL != tSubTexture ) {
				tSize.x += tSubTexture->RealSize().Width();
			}

			tSubTexture = SkinC->GetSubTextureSide( State, UISkinComplex::Right );

			if ( NULL != tSubTexture ) {
				tSize.x += tSubTexture->RealSize().Width();
			}
		}
	}

	return tSize;
}

Sizei UIControl::GetSkinSize() {
	return GetSkinSize( GetSkin(), UISkinState::StateNormal );
}

UIControl * UIControl::NextComplexControl() {
	UIControl * Found		= NULL;
	UIControl * ChildLoop	= mChild;

	while( NULL != ChildLoop ) {
		if ( ChildLoop->Visible() && ChildLoop->Enabled() ) {
			if ( ChildLoop->IsComplex() ) {
				return ChildLoop;
			} else {
				Found = ChildLoop->NextComplexControl();

				if ( NULL != Found ) {
					return Found;
				}
			}
		}

		ChildLoop = ChildLoop->mNext;
	}

	if ( NULL != mNext ) {
		if ( mNext->Visible() && mNext->Enabled() && mNext->IsComplex() ) {
			return mNext;
		} else {
			return mNext->NextComplexControl();
		}
	} else {
		ChildLoop = mParentCtrl;

		while ( NULL != ChildLoop ) {
			if ( NULL != ChildLoop->mNext ) {
				if ( ChildLoop->mNext->Visible() && ChildLoop->mNext->Enabled() && ChildLoop->mNext->IsComplex() ) {
					return ChildLoop->mNext;
				} else {
					return ChildLoop->mNext->NextComplexControl();
				}
			}

			ChildLoop = ChildLoop->mParentCtrl;
		}
	}

	return UIManager::instance()->MainControl();
}

void UIControl::DoAfterSetTheme() {
}

void UIControl::WorldToControl( Vector2i& pos ) const {
	Vector2f Pos( pos.x, pos.y );

	std::list<UIControl*> parents;

	UIControl * ParentLoop = mParentCtrl;

	while ( NULL != ParentLoop ) {
		parents.push_front( ParentLoop );
		ParentLoop = ParentLoop->Parent();
	}

	parents.push_back( const_cast<UIControl*>( reinterpret_cast<const UIControl*>( this ) ) );

	Vector2f scale(1,1);

	for ( std::list<UIControl*>::iterator it = parents.begin(); it != parents.end(); it++ ) {
		UIControl * tParent	= (*it);
		UIControlAnim * tP		= tParent->IsAnimated() ? reinterpret_cast<UIControlAnim *> ( tParent ) : NULL;
		Vector2f pPos			( tParent->mPos.x * scale.x			, tParent->mPos.y * scale.y			);
		Vector2f Center;

		if ( NULL != tP && 1.f != tP->Scale() ) {
			Center = tP->ScaleOriginPoint() * scale;
			scale *= tP->Scale();

			pPos.Scale( scale, pPos + Center );
		}

		Pos -= pPos;

		if ( NULL != tP && 0.f != tP->Angle() ) {
			Center = tP->RotationOriginPoint() * scale;
			Pos.Rotate( -tP->Angle(), Center );
		}
	}

	pos = Vector2i( Pos.x / scale.x, Pos.y / scale.y );
}

void UIControl::ControlToWorld( Vector2i& pos ) const {
	Vector2f Pos( pos.x, pos.y );

	std::list<UIControl*> parents;

	UIControl * ParentLoop = mParentCtrl;

	while ( NULL != ParentLoop ) {
		parents.push_back( ParentLoop );
		ParentLoop = ParentLoop->Parent();
	}

	parents.push_front( const_cast<UIControl*>( reinterpret_cast<const UIControl*>( this ) ) );

	for ( std::list<UIControl*>::iterator it = parents.begin(); it != parents.end(); it++ ) {
		UIControl * tParent	= (*it);
		UIControlAnim * tP		= tParent->IsAnimated() ? reinterpret_cast<UIControlAnim *> ( tParent ) : NULL;
		Vector2f pPos			( tParent->mPos.x					, tParent->mPos.y					);

		Pos += pPos;

		if ( NULL != tP ) {
			Vector2f CenterAngle( pPos.x + tP->mRotationOriginPoint.x, pPos.y + tP->mRotationOriginPoint.y );
			Vector2f CenterScale( pPos.x + tP->mScaleOriginPoint.x, pPos.y + tP->mScaleOriginPoint.y );

			Pos.Rotate( tP->Angle(), CenterAngle );
			Pos.Scale( tP->Scale(), CenterScale );
		}
	}

	pos = Vector2i( eeceil( Pos.x ), eeceil( Pos.y ) );
}

}}
