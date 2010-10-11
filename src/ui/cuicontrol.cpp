#include "cuicontrol.hpp"
#include "cuimanager.hpp"

namespace EE { namespace UI {

cUIControl::cUIControl( const CreateParams& Params ) :
	mVisible( false ),
	mEnabled( false ),
	mPos( Params.Pos ),
	mSize( Params.Size ),
	mParentCtrl( Params.ParentCtrl ),
	mFlags( Params.Flags ),
	mType( 0 ),
	mData( 0 ),
	mChild( NULL ),
	mNext( NULL ),
	mBackground( Params.Background ),
	mBorder( Params.Border ),
	mControlFlags( 0 ),
	mBlend( Params.Blend ),
	mNumCallBacks(0),
	mSkin(NULL)
{
	mType |= UI_TYPE_GET(UI_TYPE_CONTROL);

	if ( NULL != mParentCtrl )
		mParentCtrl->ChildAdd( this );

	UpdateScreenPos();
	UpdateQuad();
}

cUIControl::~cUIControl() {
	while( NULL != mChild )
		eeDelete( mChild );

	if ( NULL != mParentCtrl )
		mParentCtrl->ChildRemove( this );

	if ( cUIManager::instance()->FocusControl() == this )
		cUIManager::instance()->FocusControl( NULL );

	if ( cUIManager::instance()->OverControl() == this )
		cUIManager::instance()->OverControl( NULL );
}

void cUIControl::ScreenToControl( eeVector2i& Pos ) const {
	cUIControl * ParentLoop = mParentCtrl;

	Pos.x -= mPos.x;
	Pos.y -= mPos.y;

	while ( NULL != ParentLoop ) {
		const eeVector2i& ParentPos = ParentLoop->Pos();

		Pos.x -= ParentPos.x;
		Pos.y -= ParentPos.y;

		ParentLoop = ParentLoop->Parent();
	}
}

void cUIControl::ControlToScreen( eeVector2i& Pos ) const {
	cUIControl * ParentLoop = mParentCtrl;

	while ( NULL != ParentLoop ) {
		const eeVector2i& ParentPos = ParentLoop->Pos();

		Pos.x += ParentPos.x;
		Pos.y += ParentPos.y;

		ParentLoop = ParentLoop->Parent();
	}
}

Uint32 cUIControl::Type() const {
	return mType;
}

bool cUIControl::IsType( const Uint32& Type ) const {
	return ( mType & UI_TYPE_GET(Type) ) != 0;
}

void cUIControl::MessagePost( const cUIMessage * Msg ) {
	cUIControl * Ctrl = this;

	while( NULL != Ctrl ) {
		if ( Ctrl->OnMessage( Msg ) )
			break;

		Ctrl = Ctrl->Parent();
	}
}

Uint32 cUIControl::OnMessage( const cUIMessage * Msg ) {
	return 0;
}

bool cUIControl::IsInside( const eeVector2i& Pos ) const {
	if ( Pos.x >= 0 && Pos.y >= 0 && Pos.x < mSize.Width() && Pos.y < mSize.Height() )
		return true;

	return false;
}

void cUIControl::Pos( const eeVector2i& Pos ) {
	mPos = Pos;
	OnPosChange();
}

void cUIControl::Pos( const Int32& x, const Int32& y ) {
	mPos = eeVector2i( x, y );
	OnPosChange();
}

const eeVector2i& cUIControl::Pos() const {
	return mPos;
}

void cUIControl::Size( const eeSize& Size ) {
	mSize = Size;
	OnSizeChange();
}

void cUIControl::Size( const Int32 Width, const Int32 Height ) {
	Size( eeSize( Width, Height ) );
}

eeRecti cUIControl::Rect() const {
	return eeRecti( mPos, mSize );
}

const eeSize& cUIControl::Size() {
	return mSize;
}

void cUIControl::Visible( const bool& visible ) {
	mVisible = visible;
	OnVisibleChange();
}

bool cUIControl::Visible() const {
	return mVisible;
}

bool cUIControl::Hided() const {
	return !mVisible;
}

void cUIControl::Enabled( const bool& enabled ) {
	mEnabled = enabled;
	OnEnabledChange();
}

bool cUIControl::Enabled() const {
	return mEnabled;
}

bool cUIControl::Disabled() const {
	return !mEnabled;
}

cUIControl * cUIControl::Parent() const {
	return mParentCtrl;
}

void cUIControl::Parent( cUIControl * parent ) {
	if ( NULL != Parent() )
		Parent()->ChildRemove( this );

	mParentCtrl = parent;

	if ( NULL != mParentCtrl )
		mParentCtrl->ChildAdd( this );
}

void cUIControl::CenterHorizontal() {
	cUIControl * Ctrl = Parent();

	if ( NULL != Ctrl )
		Pos( eeVector2i( ( Ctrl->Size().Width() / 2 ) - ( mSize.Width() / 2 ), mPos.y ) );
}

void cUIControl::CenterVertical(){
	cUIControl * Ctrl = Parent();

	if ( NULL != Ctrl )
		Pos( eeVector2i( mPos.x, ( Ctrl->Size().Height() / 2 ) - ( mSize.Height() / 2 ) ) );
}

void cUIControl::Center() {
	CenterHorizontal();
	CenterVertical();
}

void cUIControl::Close() {
	mControlFlags |= UI_CTRL_FLAG_CLOSE;
}

void cUIControl::Draw() {
	if ( mVisible ) {
		if ( mFlags & UI_FILL_BACKGROUND )
			BackgroundDraw();

		if ( mFlags & UI_BORDER )
			BorderDraw();

		if ( NULL != mSkin )
			mSkin->Draw( (eeFloat)mScreenPos.x, (eeFloat)mScreenPos.y, (eeFloat)mSize.Width(), (eeFloat)mSize.Height() );
	}
}

void cUIControl::Update() {
	cUIControl * ChildLoop = mChild;

	while ( NULL != ChildLoop ) {
		ChildLoop->Update();
		ChildLoop = ChildLoop->NextGet();
	}
}

void cUIControl::SendMouseEvent( const Uint32& Event, const eeVector2i& Pos, const Uint32& Flags ) {
	cUIEventMouse MouseEvent( this, Event, Pos, Flags );
	SendEvent( &MouseEvent );
}

void cUIControl::SendCommonEvent( const Uint32& Event ) {
	cUIEvent CommonEvent( this, Event );
	SendEvent( &CommonEvent );
}

Uint32 cUIControl::OnKeyDown( const cUIEventKey& Event ) {
	SendEvent( &Event );
	return 1;
}

Uint32 cUIControl::OnKeyUp( const cUIEventKey& Event ) {
	SendEvent( &Event );
	return 1;
}

Uint32 cUIControl::OnMouseMove( const eeVector2i& Pos, const Uint32 Flags ) {
	SendMouseEvent( cUIEvent::EventMouseMove, Pos, Flags );
	return 1;
}

Uint32 cUIControl::OnMouseDown( const eeVector2i& Pos, const Uint32 Flags ) {
	SendMouseEvent( cUIEvent::EventMouseDown, Pos, Flags );

	SetSkinState( cUISkin::StateMouseDown );

	return 1;
}

Uint32 cUIControl::OnMouseUp( const eeVector2i& Pos, const Uint32 Flags ) {
	SendMouseEvent( cUIEvent::EventMouseUp, Pos, Flags );

	SetPrevSkinState();

	return 1;
}

Uint32 cUIControl::OnMouseClick( const eeVector2i& Pos, const Uint32 Flags ) {
	SendMouseEvent( cUIEvent::EventMouseClick, Pos, Flags );
	return 1;
}

bool cUIControl::IsMouseOver() {
	return 0 != Read32BitKey( &mControlFlags, UI_CTRL_FLAG_MOUSEOVER_POS );
}

Uint32 cUIControl::OnMouseDoubleClick( const eeVector2i& Pos, const Uint32 Flags ) {
	SendMouseEvent( cUIEvent::EventMouseDoubleClick, Pos, Flags );
	return 1;
}

Uint32 cUIControl::OnMouseEnter( const eeVector2i& Pos, const Uint32 Flags ) {
	Write32BitKey( &mControlFlags, UI_CTRL_FLAG_MOUSEOVER_POS, 1 );
	
	SendMouseEvent( cUIEvent::EventMouseEnter, Pos, Flags );

	SetSkinState( cUISkin::StateMouseEnter );

	return 1;
}

Uint32 cUIControl::OnMouseExit( const eeVector2i& Pos, const Uint32 Flags ) {
	Write32BitKey( &mControlFlags, UI_CTRL_FLAG_MOUSEOVER_POS, 0 );
	
	SendMouseEvent( cUIEvent::EventMouseExit, Pos, Flags );

	SetSkinState( cUISkin::StateMouseExit );

	return 1;
}

Uint32 cUIControl::OnFocus() {
	SendCommonEvent( cUIEvent::EventOnFocus );

	SetSkinState( cUISkin::StateFocus );

	return 1;
}

Uint32 cUIControl::OnFocusLoss() {
	SendCommonEvent( cUIEvent::EventOnFocusLoss );

	SetSkinState( cUISkin::StateLostFocus );

	return 1;
}

Uint32 cUIControl::HAlign() const {
	return mFlags & UI_HALIGN_MASK;
}

void cUIControl::HAlign( Uint32 halign ) {
	mFlags |= halign & UI_HALIGN_MASK;
}

Uint32 cUIControl::VAlign() const {
	return mFlags & UI_VALIGN_MASK;
}

void cUIControl::VAlign( Uint32 valign ) {
	mFlags |= valign & UI_VALIGN_MASK;
}

void cUIControl::FillBackground( bool enabled ) {
	if ( enabled )
		mFlags |= UI_FILL_BACKGROUND;
	else
		mFlags &= ~UI_FILL_BACKGROUND;
}

void cUIControl::Border( bool enabled ) {
	if ( enabled )
		mFlags |= UI_BORDER;
	else
		mFlags &= ~UI_BORDER;
}

cUIControl * cUIControl::NextGet() const {
	return mNext;
}

cUIControl * cUIControl::NextGetLoop() const {
	if ( NULL == mNext )
		return Parent()->ChildAt( 0 );
	else
		return mNext;
}

void cUIControl::Data( const Uint32& data ) {
	mData = data;
}

const Uint32& cUIControl::Data() const {
	return mData;
}

cUIControl * cUIControl::ChildGetAt( eeVector2i CtrlPos, eeUint RecursiveLevel ) {
	cUIControl * Ctrl = NULL;

	for( cUIControl * pLoop = mChild; NULL != pLoop && NULL == Ctrl; pLoop = pLoop->NextGet() )
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

const Uint32& cUIControl::Flags() const {
	return mFlags;
}

void cUIControl::Flags( const Uint32& flags ) {
	mFlags |= flags;
}

void cUIControl::Blend( const EE_PRE_BLEND_FUNC& blend ) {
	mBlend = blend;
}

EE_PRE_BLEND_FUNC& cUIControl::Blend() {
	return mBlend;
}

void cUIControl::ToFront() {
	Parent()->ChildRemove( this );
	mNext = NULL;
	Parent()->ChildAdd( this );
}

void cUIControl::ToBack() {
	Parent()->ChildRemove( this );
	Parent()->ChildAddAt( this, 0 );
}

void cUIControl::ToPos( const Uint32& Pos ) {
	Parent()->ChildRemove( this );
	Parent()->ChildAddAt( this, Pos );
}

void cUIControl::OnVisibleChange() {
	SendCommonEvent( cUIEvent::EventOnVisibleChange );
}

void cUIControl::OnEnabledChange() {
	if ( !Enabled() && NULL != cUIManager::instance()->FocusControl() ) {
		if ( IsChild( cUIManager::instance()->FocusControl() ) ) {
			cUIManager::instance()->FocusControl( NULL );
		}
	}

	SendCommonEvent( cUIEvent::EventOnEnabledChange );
}

void cUIControl::OnPosChange() {
	UpdateScreenPos();

	UpdateChildsScreenPos();

	SendCommonEvent( cUIEvent::EventOnPosChange );
}

void cUIControl::OnSizeChange() {
	SendCommonEvent( cUIEvent::EventOnSizeChange );
}

void cUIControl::BackgroundDraw() {
	cPrimitives P;
	P.SetColor( mBackground.Color() );

	if ( 4 == mBackground.Colors().size() ) {
		P.DrawRectangle( (eeFloat)mScreenPos.x, (eeFloat)mScreenPos.y, (eeFloat)mSize.Width(), (eeFloat)mSize.Height(), mBackground.Colors()[0], mBackground.Colors()[1], mBackground.Colors()[2], mBackground.Colors()[3], 0.f, 1.f, EE_DRAW_FILL, mBackground.Blend(), 1.0f, mBackground.Corners() );
	} else {
		P.DrawRectangle( (eeFloat)mScreenPos.x, (eeFloat)mScreenPos.y, (eeFloat)mSize.Width(), (eeFloat)mSize.Height(), 0.f, 1.f, EE_DRAW_FILL, mBackground.Blend(), 1.0f, mBackground.Corners() );
	}
}

void cUIControl::BorderDraw() {
	cPrimitives P;
	P.SetColor( mBorder.Color() );

	if ( IsClipped() )
		P.DrawRectangle( (eeFloat)mScreenPos.x + 0.1f, (eeFloat)mScreenPos.y + 0.1f, (eeFloat)mSize.Width() - 0.1f, (eeFloat)mSize.Height() - 0.1f, 0.f, 1.f, EE_DRAW_LINE, mBlend, (eeFloat)mBorder.Width(), mBackground.Corners() );
	else
		P.DrawRectangle( (eeFloat)mScreenPos.x, (eeFloat)mScreenPos.y, (eeFloat)mSize.Width(), (eeFloat)mSize.Height(), 0.f, 1.f, EE_DRAW_LINE, mBlend, (eeFloat)mBorder.Width(), mBackground.Corners() );
}

const Uint32& cUIControl::ControlFlags() const {
	return mControlFlags;
}

void cUIControl::CheckClose() {
	cUIControl * ChildLoop = mChild;

	while( NULL != ChildLoop ) {
		if ( ChildLoop->ControlFlags() & UI_CTRL_FLAG_CLOSE ) {
			eeDelete( ChildLoop );

			ChildLoop = mChild;

			continue;
		}

		ChildLoop->CheckClose();

		ChildLoop = ChildLoop->NextGet();
	}
}

void cUIControl::ClipTo() {
	if ( !IsClipped() && NULL != Parent() ) {

		cUIControl * parent = Parent();

		while ( NULL != parent ) {
			if ( parent->IsClipped() ) {
				Parent()->ClipMe();
				parent = NULL;
			} else {
				parent = Parent()->Parent();
			}
		}
	}
}

void cUIControl::DrawChilds() {
	cUIControl * ChildLoop = mChild;

	while ( NULL != ChildLoop ) {
		if ( ChildLoop->Visible() ) {
			ChildLoop->InternalDraw();
		}

		ChildLoop = ChildLoop->NextGet();
	}
}

void cUIControl::InternalDraw() {
	if ( mVisible ) {
		ClipTo();

		MatrixSet();

		ClipMe();

		Draw();

		DrawChilds();

		ClipDisable();

		MatrixUnset();
	}
}

void cUIControl::ClipMe() {
	if ( IsClipped() ) {
		if ( mFlags & UI_BORDER )
			cUIManager::instance()->ClipEnable( mScreenPos.x, mScreenPos.y, mSize.Width(), mSize.Height() + 1 );
		else
			cUIManager::instance()->ClipEnable( mScreenPos.x, mScreenPos.y, mSize.Width(), mSize.Height() );
	}
}

void cUIControl::ClipDisable() {
	if ( IsClipped() )
		cUIManager::instance()->ClipDisable();
}

void cUIControl::MatrixSet() {
}

void cUIControl::MatrixUnset() {
}

void cUIControl::ChildDeleteAll() {
	while( NULL != mChild )
		eeDelete( mChild );
}

void cUIControl::ChildAdd( cUIControl * ChildCtrl ) {
	if ( NULL == mChild )
		mChild = ChildCtrl;
	else {
		cUIControl * ChildLoop = mChild;

		while ( NULL != ChildLoop->NextGet() )
			ChildLoop = ChildLoop->NextGet();

		ChildLoop->mNext = ChildCtrl;
	}
}

void cUIControl::ChildAddAt( cUIControl * ChildCtrl, Uint32 Pos ) {
	cUIControl * ChildLoop = mChild;

	if( Pos == 0 ) {
		if( mChild == NULL) {
			mChild = ChildCtrl;
			ChildCtrl->mNext = NULL;
		} else {
			ChildCtrl->mNext = mChild;
			mChild = ChildCtrl;
		}
	} else {
		Uint32 i = 0;

		while ( NULL != ChildLoop->mNext && i < Pos ) {
			ChildLoop = ChildLoop->mNext;
			i++;
		}

		cUIControl * ChildTmp = ChildLoop->mNext;
		ChildLoop->mNext = ChildCtrl;
		ChildCtrl->mNext = ChildTmp;
	}
}

void cUIControl::ChildRemove( cUIControl * ChildCtrl ) {
	if ( ChildCtrl == mChild )
		mChild = mChild->mNext;
	else {
		cUIControl * ChildLoop = mChild;

		while ( NULL != ChildLoop->mNext ) {
			if ( ChildCtrl == ChildLoop->mNext ) {
				ChildLoop->mNext = ChildLoop->mNext->mNext;
				return;
			}

			ChildLoop = ChildLoop->mNext;
		}
	}
}

bool cUIControl::IsChild( cUIControl * ChildCtrl ) const {
	cUIControl * ChildLoop = mChild;

	while ( NULL != ChildLoop ) {
		if ( ChildCtrl == ChildLoop )
			return true;

		ChildLoop = ChildLoop->mNext;
	}

	return false;
}

Uint32 cUIControl::ChildCount() const {
	cUIControl * ChildLoop = mChild;
	Uint32 Count = 0;

	while( NULL != ChildLoop ) {
		ChildLoop = ChildLoop->mNext;
		Count++;
	}

	return Count;
}

cUIControl * cUIControl::ChildAt( Uint32 Index ) const {
	cUIControl * ChildLoop = mChild;

	while( NULL != ChildLoop && Index) {
		ChildLoop = ChildLoop->mNext;
		Index--;
	}

	return ChildLoop;
}

cUIControl * cUIControl::ChildPrev( cUIControl * Ctrl, bool Loop ) const {
	cUIControl * ChildLoop = NULL;

	if ( NULL != mChild ) {
		if ( mChild != Ctrl ) {
			ChildLoop = mChild;

			while ( NULL != ChildLoop ) {
				if ( ChildLoop->NextGet() == Ctrl )
					break;

				ChildLoop = ChildLoop->NextGet();
			}
		} else if ( Loop ) {
			ChildLoop = mChild;

			while ( NULL != ChildLoop->NextGet() )
				ChildLoop = ChildLoop->NextGet();

		}
	}

	return ChildLoop;
}

cUIControl * cUIControl::ChildNext( cUIControl * Ctrl, bool Loop ) const {
	cUIControl * Return = Ctrl->NextGet();

	if ( NULL == Return && Loop ) {
		Return = mChild;
	}

	return Return;
}

cUIControl * cUIControl::ChildGetFirst() const {
	return mChild;
}

cUIControl * cUIControl::OverFind( const eeVector2i& Point ) {
	cUIControl * pOver = NULL;

	if ( mVisible && mEnabled ) {
		UpdateQuad();

		eeVector2f Localf( (eeFloat)Point.x, (eeFloat)Point.y );

		if ( IntersectQuad2( mQuad, eeQuad2f( Localf, Localf, Localf, Localf ) ) ) {
			cUIControl * ChildLoop = mChild;

			while ( NULL != ChildLoop ) {
				cUIControl * ChildOver = ChildLoop->OverFind( Point );

				if ( NULL != ChildOver )
					pOver = ChildOver;

				ChildLoop = ChildLoop->mNext;
			}

			if ( NULL == pOver )
				pOver = const_cast<cUIControl *>( reinterpret_cast<const cUIControl *>( this ) );
		}
	}

	return pOver;
}

Uint32 cUIControl::IsAnimated() {
	return mControlFlags & UI_CTRL_FLAG_ANIM;
}

Uint32 cUIControl::IsClipped() {
	return mFlags & UI_CLIP_ENABLE;
}

void cUIControl::UpdateQuad() {
	eeVector2i Pos = mPos;
	ControlToScreen( Pos );

	mQuad 	= AABBtoQuad2( eeAABB( (eeFloat)Pos.x, (eeFloat)Pos.y, (eeFloat)Pos.x + mSize.Width(), (eeFloat)Pos.y + mSize.Height() ) );
	mCenter = eeVector2f( (eeFloat)Pos.x + (eeFloat)mSize.Width() * 0.5f, (eeFloat)Pos.y + (eeFloat)mSize.Height() * 0.5f );

	cUIControl * tParent = Parent();

	while ( tParent ) {
		if ( tParent->IsAnimated() ) {
			cUIControlAnim * tP = reinterpret_cast<cUIControlAnim *> ( tParent );

			mQuad.Rotate( tP->Angle(), tP->GetQuadCenter() );
			mQuad.Scale( tP->Scale(), tP->GetQuadCenter() );
		}

		tParent = tParent->Parent();
	};
}

eeFloat cUIControl::Elapsed() {
	return cUIManager::instance()->Elapsed();
}

Uint32 cUIControl::AddEventListener( const Uint32& EventType, const UIEventCallback& Callback ) {
	mNumCallBacks++;

	mEvents[ EventType ][ mNumCallBacks ] = Callback;

	return mNumCallBacks;
}

void cUIControl::RemoveEventListener( const Uint32& CallbackId ) {
	std::map< Uint32, std::map<Uint32, UIEventCallback> >::iterator it;

	for ( it = mEvents.begin(); it != mEvents.end(); ++it ) {
		std::map<Uint32, UIEventCallback> event = it->second;

		if ( event.erase( CallbackId ) )
			break;
	}
}

void cUIControl::SendEvent( const cUIEvent * Event ) {
	if ( 0 != mEvents.count( Event->EventType() ) ) {
		std::map<Uint32, UIEventCallback>			event = mEvents[ Event->EventType() ];
		std::map<Uint32, UIEventCallback>::iterator it;

		if ( event.begin() != event.end() ) {
			for ( it = event.begin(); it != event.end(); ++it )
				it->second( Event );
		}
	}
}

cUIBackground * cUIControl::Background() {
	return &mBackground;
}

cUIBorder * cUIControl::Border() {
	return &mBorder;
}

void cUIControl::SetThemeByName( const std::string& Theme ) {
	SetTheme( cUIThemeManager::instance()->GetByName( Theme ) );
}

void cUIControl::SetTheme( cUITheme * Theme ) {
	SetTheme( Theme, "control" );
}

void cUIControl::SetTheme( cUITheme * Theme, const std::string& ControlName ) {
	if ( NULL != Theme ) {
		if ( mSkinForcedName.size() && NULL != mSkin && mSkin->Theme() == Theme ) {
			mSkin = Theme->GetByName( Theme->Abbr() + "_" + mSkinForcedName );
		} else {
			mSkin = Theme->GetByName( Theme->Abbr() + "_" + ControlName );
		}
	}
}

void cUIControl::ForceThemeSkin( cUITheme * Theme, const std::string& ControlName ) {
	mSkinForcedName = ControlName;

	SetTheme( Theme, ControlName );
}

void cUIControl::SetSkinState( const Uint32& State ) {
	if ( NULL != mSkin )
		mSkin->SetState( State );
}

void cUIControl::SetPrevSkinState() {
	if ( NULL != mSkin )
		mSkin->SetPrevState();
}

void cUIControl::SetThemeToChilds( cUITheme * Theme ) {
	cUIControl * ChildLoop = mChild;

	while ( NULL != ChildLoop ) {
		ChildLoop->SetTheme( Theme );
		ChildLoop->SetThemeToChilds( Theme );

		ChildLoop = ChildLoop->mNext;
	}
}

void cUIControl::UpdateChildsScreenPos() {
	cUIControl * ChildLoop = mChild;

	while ( NULL != ChildLoop ) {
		ChildLoop->UpdateScreenPos();
		ChildLoop->UpdateChildsScreenPos();

		ChildLoop = ChildLoop->mNext;
	}
}

void cUIControl::UpdateScreenPos() {
	eeVector2i Pos( mPos );

	ControlToScreen( Pos );

	mScreenPos = Pos;
}

cUISkin * cUIControl::GetSkin() {
	return mSkin;
}

}}
