#include "cuicontrolanim.hpp"
#include "cuimanager.hpp"

namespace EE { namespace UI {

cUIControlAnim::cUIControlAnim( const CreateParams& Params ) :
	cUIDragable( Params ),
	mAngle(0.f),
	mScale(1.f),
	mAlpha(255.f),
	mAngleAnim(NULL),
	mScaleAnim(NULL),
	mAlphaAnim(NULL),
	mMoveAnim(NULL)
{
	mType |= UI_TYPE_GET(UI_TYPE_CONTROL_ANIM);
	mControlFlags |= UI_CTRL_FLAG_ANIM;
}

cUIControlAnim::~cUIControlAnim() {
	eeSAFE_DELETE( mAlphaAnim );
	eeSAFE_DELETE( mAngleAnim );
	eeSAFE_DELETE( mScaleAnim );
	eeSAFE_DELETE( mMoveAnim );
}

void cUIControlAnim::Draw() {
	if ( mVisible && 0.f != mAlpha ) {
		if ( mFlags & UI_FILL_BACKGROUND )
			BackgroundDraw();

		if ( mFlags & UI_BORDER )
			BorderDraw();

		if ( NULL != mSkinState )
			mSkinState->Draw( (eeFloat)mScreenPos.x, (eeFloat)mScreenPos.y, (eeFloat)mSize.Width(), (eeFloat)mSize.Height(), (Uint32)mAlpha );
	}
}

const eeFloat& cUIControlAnim::Angle() const {
	return mAngle;
}

void cUIControlAnim::Angle( const eeFloat& angle ) {
	mAngle = angle;
	OnAngleChange();
}

const eeFloat& cUIControlAnim::Scale() const {
	return mScale;
}

void cUIControlAnim::Scale( const eeFloat& scale ) {
	if ( scale >= 0.f ) {
		mScale = scale;
		OnScaleChange();
	}
}

const eeFloat& cUIControlAnim::Alpha() const {
	return mAlpha;
}

void cUIControlAnim::Alpha( const eeFloat& alpha ) {
	mAlpha = alpha;
	OnAlphaChange();
}

void cUIControlAnim::MatrixSet() {
	if ( mScale != 1.f || mAngle != 0.f ) {
		cGlobalBatchRenderer::instance()->Draw();
		glPushMatrix();
		eeVector2f Center( mScreenPos.x + mSize.Width() * 0.5f, mScreenPos.y + mSize.Height() * 0.5f );
		glTranslatef( Center.x , Center.y, 0.f );
		glRotatef( mAngle, 0.0f, 0.0f, 1.0f );
		glScalef( mScale, mScale, 1.0f );
		glTranslatef( -Center.x, -Center.y, 0.f );
	}
}

void cUIControlAnim::MatrixUnset() {
	if ( mScale != 1.f || mAngle != 0.f ) {
		cGlobalBatchRenderer::instance()->Draw();
		glPopMatrix();
	}
}

void cUIControlAnim::Update() {
	cUIDragable::Update();

	if ( NULL != mMoveAnim && mMoveAnim->Enabled() ) {
		mMoveAnim->Update( Elapsed() );
		Pos( (eeInt)mMoveAnim->GetPos().x, (eeInt)mMoveAnim->GetPos().y );

		if ( mMoveAnim->Ended() )
			eeSAFE_DELETE( mMoveAnim );
	}

	if ( NULL != mAlphaAnim && mAlphaAnim->Enabled() ) {
		mAlphaAnim->Update( Elapsed() );
		Alpha( mAlphaAnim->GetRealPos() );

		if ( mAlphaAnim->Ended() ) {
			if ( ( mControlFlags & UI_CTRL_FLAG_CLOSE_FO )  )
				Close();

			if ( ( mControlFlags & UI_CTRL_FLAG_DISABLE_FADE_OUT ) ) {
				mControlFlags &= ~UI_CTRL_FLAG_DISABLE_FADE_OUT;

				Visible( false );
			}

			eeSAFE_DELETE( mAlphaAnim );
		}
	}

	if ( NULL != mScaleAnim && mScaleAnim->Enabled() ) {
		mScaleAnim->Update( Elapsed() );
		Scale( mScaleAnim->GetRealPos() );

		if ( mScaleAnim->Ended() )
			eeSAFE_DELETE( mScaleAnim );
	}

	if ( NULL != mAngleAnim && mAngleAnim->Enabled() ) {
		mAngleAnim->Update( Elapsed() );
		Angle( mAngleAnim->GetRealPos() );

		if ( mAngleAnim->Ended() )
			eeSAFE_DELETE( mAngleAnim );
	}
}

bool cUIControlAnim::Animating() {
	return ( NULL != mAlphaAnim && mAlphaAnim->Enabled() ) || ( NULL != mAngleAnim && mAngleAnim->Enabled() ) || ( NULL != mScaleAnim && mScaleAnim->Enabled() ) || ( NULL != mMoveAnim && mMoveAnim->Enabled() );
}

void cUIControlAnim::StartAlphaAnim( const eeFloat& From, const eeFloat& To, const eeFloat& TotalTime, const bool& AlphaChilds, const EE_INTERPOLATION& Type, cInterpolation::OnPathEndCallback PathEndCallback ) {
	if ( NULL == mAlphaAnim )
		mAlphaAnim = eeNew( cInterpolation, () );

	mAlphaAnim->ClearWaypoints();
	mAlphaAnim->AddWaypoint( From );
	mAlphaAnim->AddWaypoint( To );
	mAlphaAnim->SetTotalTime( TotalTime );
	mAlphaAnim->Start( PathEndCallback );

	Alpha( From );

	if ( AlphaChilds ) {
		cUIControlAnim * AnimChild;
		cUIControl * CurChild = mChild;

		while ( NULL != CurChild ) {
			if ( CurChild->IsAnimated() ) {
				AnimChild = reinterpret_cast<cUIControlAnim*> ( CurChild );

				AnimChild->StartAlphaAnim( From, To, TotalTime, AlphaChilds );
			}

			CurChild = CurChild->NextGet();
		}
	}
}

void cUIControlAnim::StartScaleAnim( const eeFloat& From, const eeFloat& To, const eeFloat& TotalTime, const EE_INTERPOLATION& Type, cInterpolation::OnPathEndCallback PathEndCallback ) {
	if ( NULL == mScaleAnim )
		mScaleAnim = eeNew( cInterpolation, () );

	mScaleAnim->ClearWaypoints();
	mScaleAnim->AddWaypoint( From );
	mScaleAnim->AddWaypoint( To );
	mScaleAnim->SetTotalTime( TotalTime );
	mScaleAnim->Start( PathEndCallback );
	mScaleAnim->Type( Type );

	Scale( From );
}

void cUIControlAnim::StartMovement( const eeVector2i& From, const eeVector2i& To, const eeFloat& TotalTime, const EE_INTERPOLATION& Type, cWaypoints::OnPathEndCallback PathEndCallback ) {
	if ( NULL == mMoveAnim )
		mMoveAnim = eeNew( cWaypoints, () );

	mMoveAnim->ClearWaypoints();
	mMoveAnim->AddWaypoint( eeVector2f( (eeFloat)From.x, (eeFloat)From.y ) );
	mMoveAnim->AddWaypoint( eeVector2f( (eeFloat)To.x, (eeFloat)To.y ) );
	mMoveAnim->SetTotalTime( TotalTime );
	mMoveAnim->Start( PathEndCallback );
	mMoveAnim->Type( Type );

	Pos( From );
}

void cUIControlAnim::StartRotation( const eeFloat& From, const eeFloat& To, const eeFloat& TotalTime, const EE_INTERPOLATION& Type, cInterpolation::OnPathEndCallback PathEndCallback ) {
	if ( NULL == mAngleAnim )
		mAngleAnim = eeNew( cInterpolation, () );

	mAngleAnim->ClearWaypoints();
	mAngleAnim->AddWaypoint( From );
	mAngleAnim->AddWaypoint( To );
	mAngleAnim->SetTotalTime( TotalTime );
	mAngleAnim->Start( PathEndCallback );
	mAngleAnim->Type( Type );

	Angle( From );
}

void cUIControlAnim::CreateFadeIn( const eeFloat& Time, const bool& AlphaChilds, const EE_INTERPOLATION& Type ) {
	StartAlphaAnim( mAlpha, 255.f, Time, AlphaChilds, Type );
}

void cUIControlAnim::CreateFadeOut( const eeFloat& Time, const bool& AlphaChilds, const EE_INTERPOLATION& Type ) {
	StartAlphaAnim( 255.f, mAlpha, Time, AlphaChilds, Type );
}

void cUIControlAnim::CloseFadeOut( const eeFloat& Time, const bool& AlphaChilds, const EE_INTERPOLATION& Type ) {
	StartAlphaAnim	( mAlpha, 0.f, Time, AlphaChilds, Type );
	mControlFlags |= UI_CTRL_FLAG_CLOSE_FO;
}

void cUIControlAnim::DisableFadeOut( const eeFloat& Time, const bool& AlphaChilds, const EE_INTERPOLATION& Type ) {
	Enabled( false );

	StartAlphaAnim	( mAlpha, 0.f, Time, AlphaChilds, Type );

	mControlFlags |= UI_CTRL_FLAG_DISABLE_FADE_OUT;
}

void cUIControlAnim::BackgroundDraw() {
	cPrimitives P;
	P.SetColor( GetColor( mBackground->Color() ) );

	if ( 4 == mBackground->Colors().size() ) {
		P.DrawRectangle( (eeFloat)mScreenPos.x, (eeFloat)mScreenPos.y, (eeFloat)mSize.Width(), (eeFloat)mSize.Height(), GetColor( mBackground->Colors()[0] ), GetColor( mBackground->Colors()[1] ), GetColor( mBackground->Colors()[2] ), GetColor( mBackground->Colors()[3] ), 0.f, 1.f, EE_DRAW_FILL, mBackground->Blend(), 1.0f, mBackground->Corners() );
	} else {
		P.DrawRectangle( (eeFloat)mScreenPos.x, (eeFloat)mScreenPos.y, (eeFloat)mSize.Width(), (eeFloat)mSize.Height(), 0.f, 1.f, EE_DRAW_FILL, mBackground->Blend(), 1.0f, mBackground->Corners() );
	}
}

void cUIControlAnim::BorderDraw() {
	cPrimitives P;
	P.SetColor( GetColor( mBorder->Color() ) );

	if ( mFlags & UI_CLIP_ENABLE )
		P.DrawRectangle( (eeFloat)mScreenPos.x + 0.1f, (eeFloat)mScreenPos.y + 0.1f, (eeFloat)mSize.Width() - 0.1f, (eeFloat)mSize.Height() - 0.1f, 0.f, 1.f, EE_DRAW_LINE, mBlend, (eeFloat)mBorder->Width(), mBackground->Corners() );
	else
		P.DrawRectangle( (eeFloat)mScreenPos.x, (eeFloat)mScreenPos.y, (eeFloat)mSize.Width(), (eeFloat)mSize.Height(), 0.f, 1.f, EE_DRAW_LINE, mBlend, (eeFloat)mBorder->Width(), mBackground->Corners() );
}

eeColorA cUIControlAnim::GetColor( const eeColorA& Col ) {
	return eeColorA( Col.R(), Col.G(), Col.B(), static_cast<Uint8>( (eeFloat)Col.A() * ( mAlpha / 255.f ) ) );
}

void cUIControlAnim::UpdateQuad() {
	mPoly 	= eePolygon2f( eeAABB( (eeFloat)mScreenPos.x, (eeFloat)mScreenPos.y, (eeFloat)mScreenPos.x + mSize.Width(), (eeFloat)mScreenPos.y + mSize.Height() ) );
	mCenter = eeVector2f( (eeFloat)mScreenPos.x + (eeFloat)mSize.Width() * 0.5f, (eeFloat)mScreenPos.y + (eeFloat)mSize.Height() * 0.5f );

	mPoly.Rotate( mAngle, mCenter );
	mPoly.Scale( mScale, mCenter );

	cUIControl * tParent = Parent();

	while ( tParent ) {
		if ( tParent->IsAnimated() ) {
			cUIControlAnim * tP = reinterpret_cast<cUIControlAnim *> ( tParent );

			if ( tP->Angle() != 0.f )
				mPoly.Rotate( tP->Angle(), tP->GetPolygonCenter() );

			if ( tP->Scale() != 1.f )
				mPoly.Scale( tP->Scale(), tP->GetPolygonCenter() );
		}

		tParent = tParent->Parent();
	};
}

cInterpolation * cUIControlAnim::AngleInterpolation() {
	if ( NULL == mAngleAnim )
		mAngleAnim = eeNew( cInterpolation, () );

	return mAngleAnim;
}

cInterpolation * cUIControlAnim::ScaleInterpolation() {
	if ( NULL == mScaleAnim )
		mScaleAnim = eeNew( cInterpolation, () );

	return mScaleAnim;
}

cInterpolation * cUIControlAnim::AlphaInterpolation() {
	if ( NULL == mAlphaAnim )
		mAlphaAnim = eeNew( cInterpolation, () );

	return mAlphaAnim;
}

cWaypoints * cUIControlAnim::MovementInterpolation() {
	if ( NULL == mMoveAnim )
		mMoveAnim = eeNew( cWaypoints, () );

	return mMoveAnim;
}

void cUIControlAnim::OnAngleChange() {
	SendCommonEvent( cUIEvent::EventOnAngleChange );
}

void cUIControlAnim::OnScaleChange() {
	SendCommonEvent( cUIEvent::EventOnScaleChange );
}

void cUIControlAnim::OnAlphaChange() {
	SendCommonEvent( cUIEvent::EventOnAlphaChange );
}

}}

