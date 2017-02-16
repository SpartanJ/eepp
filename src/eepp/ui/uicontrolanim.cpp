#include <eepp/ui/uicontrolanim.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/graphics/renderer/gl.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/primitives.hpp>

namespace EE { namespace UI {

UIControlAnim::UIControlAnim( const CreateParams& Params ) :
	UIDragable( Params ),
	mAngle(0.f),
	mScale(1.f,1.f),
	mAlpha(255.f),
	mAngleAnim(NULL),
	mScaleAnim(NULL),
	mAlphaAnim(NULL),
	mMoveAnim(NULL)
{
	mControlFlags |= UI_CTRL_FLAG_ANIM;

	UpdateOriginPoint();
}

UIControlAnim::~UIControlAnim() {
	eeSAFE_DELETE( mAlphaAnim );
	eeSAFE_DELETE( mAngleAnim );
	eeSAFE_DELETE( mScaleAnim );
	eeSAFE_DELETE( mMoveAnim );
}

Uint32 UIControlAnim::Type() const {
	return UI_TYPE_CONTROL_ANIM;
}

bool UIControlAnim::IsType( const Uint32& type ) const {
	return UIControlAnim::Type() == type ? true : UIControl::IsType( type );
}

void UIControlAnim::Draw() {
	if ( mVisible && 0.f != mAlpha ) {
		if ( mFlags & UI_FILL_BACKGROUND )
			BackgroundDraw();

		if ( mFlags & UI_BORDER )
			BorderDraw();

		if ( NULL != mSkinState )
			mSkinState->Draw( mScreenPosf.x, mScreenPosf.y, (Float)mSize.width(), (Float)mSize.height(), (Uint32)mAlpha );

		if ( UIManager::instance()->HighlightFocus() && UIManager::instance()->FocusControl() == this ) {
			Primitives P;
			P.fillMode( DRAW_LINE );
			P.blendMode( Blend() );
			P.setColor( UIManager::instance()->HighlightFocusColor() );
			P.drawRectangle( GetRectf() );
		}

		if ( UIManager::instance()->HighlightOver() && UIManager::instance()->OverControl() == this ) {
			Primitives P;
			P.fillMode( DRAW_LINE );
			P.blendMode( Blend() );
			P.setColor( UIManager::instance()->HighlightOverColor() );
			P.drawRectangle( GetRectf() );
		}
	}
}

const Float& UIControlAnim::Angle() const {
	return mAngle;
}

const OriginPoint& UIControlAnim::RotationOriginPoint() const {
	return mRotationOriginPoint;
}

void UIControlAnim::RotationOriginPoint( const OriginPoint & center ) {
	mRotationOriginPoint = center;
	UpdateOriginPoint();
}

Vector2f UIControlAnim::RotationCenter() {
	switch ( mRotationOriginPoint.OriginType ) {
		case OriginPoint::OriginCenter: return mCenter;
		case OriginPoint::OriginTopLeft: return mScreenPosf;
		case OriginPoint::OriginCustom: default: return mScreenPosf + mRotationOriginPoint;
	}
}

void UIControlAnim::Angle( const Float& angle ) {
	mAngle = angle;
	OnAngleChange();
}

void UIControlAnim::Angle( const Float& angle , const OriginPoint & center ) {
	mRotationOriginPoint = center;
	UpdateOriginPoint();
	Angle( angle );
}

const Vector2f& UIControlAnim::Scale() const {
	return mScale;
}

void UIControlAnim::Scale( const Vector2f & scale ) {
	mScale = scale;
	OnScaleChange();
}

const OriginPoint& UIControlAnim::ScaleOriginPoint() const {
	return mScaleOriginPoint;
}

void UIControlAnim::ScaleOriginPoint( const OriginPoint & center ) {
	mScaleOriginPoint = center;
	UpdateOriginPoint();
}

Vector2f UIControlAnim::ScaleCenter() {
	switch ( mScaleOriginPoint.OriginType ) {
		case OriginPoint::OriginCenter: return mCenter;
		case OriginPoint::OriginTopLeft: return mScreenPosf;
		case OriginPoint::OriginCustom: default: return mScreenPosf + mScaleOriginPoint;
	}
}

void UIControlAnim::Scale( const Vector2f& scale, const OriginPoint& center ) {
	mScaleOriginPoint = center;
	UpdateOriginPoint();
	Scale( scale );
}

void UIControlAnim::Scale( const Float& scale, const OriginPoint& center ) {
	Scale( Vector2f( scale, scale ), center );
}

const Float& UIControlAnim::Alpha() const {
	return mAlpha;
}

void UIControlAnim::Alpha( const Float& alpha ) {
	mAlpha = alpha;
	OnAlphaChange();
}

void UIControlAnim::AlphaChilds( const Float &alpha ) {
	UIControlAnim * AnimChild;
	UIControl * CurChild = mChild;

	while ( NULL != CurChild ) {
		if ( CurChild->IsAnimated() ) {
			AnimChild = reinterpret_cast<UIControlAnim*> ( CurChild );

			AnimChild->Alpha( alpha );
			AnimChild->AlphaChilds( alpha );
		}

		CurChild = CurChild->NextGet();
	}
}

void UIControlAnim::MatrixSet() {
	if ( mScale != 1.f || mAngle != 0.f ) {
		GlobalBatchRenderer::instance()->draw();

		GLi->pushMatrix();

		Vector2f scaleCenter = ScaleCenter();
		GLi->translatef( scaleCenter.x , scaleCenter.y, 0.f );
		GLi->scalef( mScale.x, mScale.y, 1.0f );
		GLi->translatef( -scaleCenter.x, -scaleCenter.y, 0.f );

		Vector2f rotationCenter = RotationCenter();
		GLi->translatef( rotationCenter.x , rotationCenter.y, 0.f );
		GLi->rotatef( mAngle, 0.0f, 0.0f, 1.0f );
		GLi->translatef( -rotationCenter.x, -rotationCenter.y, 0.f );
	}
}

void UIControlAnim::MatrixUnset() {
	if ( mScale != 1.f || mAngle != 0.f ) {
		GlobalBatchRenderer::instance()->draw();

		GLi->popMatrix();
	}
}

void UIControlAnim::Update() {
	UIDragable::Update();

	if ( NULL != mMoveAnim && mMoveAnim->enabled() ) {
		mMoveAnim->update( Elapsed() );
		Pos( (int)mMoveAnim->getPos().x, (int)mMoveAnim->getPos().y );

		if ( mMoveAnim->ended() )
			eeSAFE_DELETE( mMoveAnim );
	}

	if ( NULL != mAlphaAnim && mAlphaAnim->enabled() ) {
		mAlphaAnim->update( Elapsed() );
		Alpha( mAlphaAnim->getRealPos() );

		if ( mAlphaAnim->ended() ) {
			if ( ( mControlFlags & UI_CTRL_FLAG_CLOSE_FO )  )
				Close();

			if ( ( mControlFlags & UI_CTRL_FLAG_DISABLE_FADE_OUT ) ) {
				mControlFlags &= ~UI_CTRL_FLAG_DISABLE_FADE_OUT;

				Visible( false );
			}

			eeSAFE_DELETE( mAlphaAnim );
		}
	}

	if ( NULL != mScaleAnim && mScaleAnim->enabled() ) {
		mScaleAnim->update( Elapsed() );
		Scale( mScaleAnim->getPos() );

		if ( mScaleAnim->ended() )
			eeSAFE_DELETE( mScaleAnim );
	}

	if ( NULL != mAngleAnim && mAngleAnim->enabled() ) {
		mAngleAnim->update( Elapsed() );
		Angle( mAngleAnim->getRealPos() );

		if ( mAngleAnim->ended() )
			eeSAFE_DELETE( mAngleAnim );
	}
}

bool UIControlAnim::FadingOut() {
	return 0 != ( mControlFlags & UI_CTRL_FLAG_DISABLE_FADE_OUT );
}

bool UIControlAnim::Animating() {
	return ( NULL != mAlphaAnim && mAlphaAnim->enabled() ) || ( NULL != mAngleAnim && mAngleAnim->enabled() ) || ( NULL != mScaleAnim && mScaleAnim->enabled() ) || ( NULL != mMoveAnim && mMoveAnim->enabled() );
}

Interpolation * UIControlAnim::StartAlphaAnim( const Float& From, const Float& To, const Time& TotalTime, const bool& AlphaChilds, const Ease::Interpolation& Type, Interpolation::OnPathEndCallback PathEndCallback ) {
	if ( NULL == mAlphaAnim )
		mAlphaAnim = eeNew( Interpolation, () );

	mAlphaAnim->clearWaypoints();
	mAlphaAnim->addWaypoint( From );
	mAlphaAnim->addWaypoint( To );
	mAlphaAnim->setTotalTime( TotalTime );
	mAlphaAnim->start( PathEndCallback );
	mAlphaAnim->type( Type );

	Alpha( From );

	if ( AlphaChilds ) {
		UIControlAnim * AnimChild;
		UIControl * CurChild = mChild;

		while ( NULL != CurChild ) {
			if ( CurChild->IsAnimated() ) {
				AnimChild = reinterpret_cast<UIControlAnim*> ( CurChild );

				AnimChild->StartAlphaAnim( From, To, TotalTime, AlphaChilds );
			}

			CurChild = CurChild->NextGet();
		}
	}

	return mAlphaAnim;
}

Waypoints * UIControlAnim::StartScaleAnim( const Vector2f& From, const Vector2f& To, const Time& TotalTime, const Ease::Interpolation& Type, Interpolation::OnPathEndCallback PathEndCallback ) {
	if ( NULL == mScaleAnim )
		mScaleAnim = eeNew( Waypoints, () );

	mScaleAnim->clearWaypoints();
	mScaleAnim->addWaypoint( From );
	mScaleAnim->addWaypoint( To );
	mScaleAnim->setTotalTime( TotalTime );
	mScaleAnim->start( PathEndCallback );
	mScaleAnim->type( Type );

	Scale( From );

	return mScaleAnim;
}

Waypoints * UIControlAnim::StartScaleAnim( const Float& From, const Float& To, const Time& TotalTime, const Ease::Interpolation& Type, Interpolation::OnPathEndCallback PathEndCallback ) {
	return StartScaleAnim( Vector2f( From, From ), Vector2f( To, To ), TotalTime, Type, PathEndCallback );
}

Waypoints * UIControlAnim::StartMovement( const Vector2i& From, const Vector2i& To, const Time& TotalTime, const Ease::Interpolation& Type, Waypoints::OnPathEndCallback PathEndCallback ) {
	if ( NULL == mMoveAnim )
		mMoveAnim = eeNew( Waypoints, () );

	mMoveAnim->clearWaypoints();
	mMoveAnim->addWaypoint( Vector2f( (Float)From.x, (Float)From.y ) );
	mMoveAnim->addWaypoint( Vector2f( (Float)To.x, (Float)To.y ) );
	mMoveAnim->setTotalTime( TotalTime );
	mMoveAnim->start( PathEndCallback );
	mMoveAnim->type( Type );

	Pos( From );

	return mMoveAnim;
}

Interpolation * UIControlAnim::StartRotation( const Float& From, const Float& To, const Time& TotalTime, const Ease::Interpolation& Type, Interpolation::OnPathEndCallback PathEndCallback ) {
	if ( NULL == mAngleAnim )
		mAngleAnim = eeNew( Interpolation, () );

	mAngleAnim->clearWaypoints();
	mAngleAnim->addWaypoint( From );
	mAngleAnim->addWaypoint( To );
	mAngleAnim->setTotalTime( TotalTime );
	mAngleAnim->start( PathEndCallback );
	mAngleAnim->type( Type );

	Angle( From );

	return mAngleAnim;
}

Interpolation * UIControlAnim::CreateFadeIn( const Time& Time, const bool& AlphaChilds, const Ease::Interpolation& Type ) {
	return StartAlphaAnim( mAlpha, 255.f, Time, AlphaChilds, Type );
}

Interpolation * UIControlAnim::CreateFadeOut( const Time& Time, const bool& AlphaChilds, const Ease::Interpolation& Type ) {
	return StartAlphaAnim( 255.f, mAlpha, Time, AlphaChilds, Type );
}

Interpolation * UIControlAnim::CloseFadeOut( const Time& Time, const bool& AlphaChilds, const Ease::Interpolation& Type ) {
	StartAlphaAnim	( mAlpha, 0.f, Time, AlphaChilds, Type );
	mControlFlags |= UI_CTRL_FLAG_CLOSE_FO;
	return mAlphaAnim;
}

Interpolation * UIControlAnim::DisableFadeOut( const Time& Time, const bool& AlphaChilds, const Ease::Interpolation& Type ) {
	Enabled( false );

	StartAlphaAnim	( mAlpha, 0.f, Time, AlphaChilds, Type );

	mControlFlags |= UI_CTRL_FLAG_DISABLE_FADE_OUT;

	return mAlphaAnim;
}

void UIControlAnim::BackgroundDraw() {
	Primitives P;
	Rectf R = GetRectf();
	P.blendMode( mBackground->Blend() );
	P.setColor( GetColor( mBackground->Color() ) );

	if ( 4 == mBackground->Colors().size() ) {
		if ( mBackground->Corners() ) {
			P.drawRoundedRectangle( R, GetColor( mBackground->Colors()[0] ), GetColor( mBackground->Colors()[1] ), GetColor( mBackground->Colors()[2] ), GetColor( mBackground->Colors()[3] ), mBackground->Corners() );
		} else {
			P.drawRectangle( R, GetColor( mBackground->Colors()[0] ), GetColor( mBackground->Colors()[1] ), GetColor( mBackground->Colors()[2] ), GetColor( mBackground->Colors()[3] ) );
		}
	} else {
		if ( mBackground->Corners() ) {
			P.drawRoundedRectangle( R, 0.f, Vector2f::One, mBackground->Corners() );
		} else {
			P.drawRectangle( R );
		}
	}
}

void UIControlAnim::BorderDraw() {
	Primitives P;
	P.fillMode( DRAW_LINE );
	P.blendMode( Blend() );
	P.lineWidth( (Float)mBorder->Width() );
	P.setColor( GetColor( mBorder->Color() ) );

	//! @TODO: Check why was this +0.1f -0.1f?
	if ( mFlags & UI_CLIP_ENABLE ) {
		Rectf R( Vector2f( mScreenPosf.x + 0.1f, mScreenPosf.y + 0.1f ), Sizef( (Float)mSize.width() - 0.1f, (Float)mSize.height() - 0.1f ) );

		if ( mBackground->Corners() ) {
			P.drawRoundedRectangle( GetRectf(), 0.f, Vector2f::One, mBackground->Corners() );
		} else {
			P.drawRectangle( R );
		}
	} else {
		if ( mBackground->Corners() ) {
			P.drawRoundedRectangle( GetRectf(), 0.f, Vector2f::One, mBackground->Corners() );
		} else {
			P.drawRectangle( GetRectf() );
		}
	}
}

ColorA UIControlAnim::GetColor( const ColorA& Col ) {
	return ColorA( Col.r(), Col.g(), Col.b(), static_cast<Uint8>( (Float)Col.a() * ( mAlpha / 255.f ) ) );
}

void UIControlAnim::UpdateQuad() {
	mPoly		= Polygon2f( eeAABB( mScreenPosf.x, mScreenPosf.y, mScreenPosf.x + mSize.width(), mScreenPosf.y + mSize.height() ) );

	mPoly.rotate( mAngle, RotationCenter() );
	mPoly.scale( mScale, ScaleCenter() );

	UIControl * tParent = Parent();

	while ( tParent ) {
		if ( tParent->IsAnimated() ) {
			UIControlAnim * tP = reinterpret_cast<UIControlAnim *> ( tParent );

			mPoly.rotate( tP->Angle(), tP->RotationCenter() );
			mPoly.scale( tP->Scale(), tP->ScaleCenter() );
		}

		tParent = tParent->Parent();
	};
}

void UIControlAnim::OnSizeChange() {
	UpdateOriginPoint();
}

void UIControlAnim::UpdateOriginPoint() {
	switch ( mRotationOriginPoint.OriginType ) {
		case OriginPoint::OriginCenter:
			mRotationOriginPoint.x = mSize.x * 0.5f;
			mRotationOriginPoint.y = mSize.y * 0.5f;
			break;
		case OriginPoint::OriginTopLeft:
			mRotationOriginPoint.x = mRotationOriginPoint.y = 0;
			break;
		default: {}
	}

	switch ( mScaleOriginPoint.OriginType ) {
		case OriginPoint::OriginCenter:
			mScaleOriginPoint.x = mSize.x * 0.5f;
			mScaleOriginPoint.y = mSize.y * 0.5f;
			break;
		case OriginPoint::OriginTopLeft:
			mScaleOriginPoint.x = mScaleOriginPoint.y = 0;
			break;
		default: {}
	}
}

Interpolation * UIControlAnim::RotationInterpolation() {
	if ( NULL == mAngleAnim )
		mAngleAnim = eeNew( Interpolation, () );

	return mAngleAnim;
}

Waypoints * UIControlAnim::ScaleInterpolation() {
	if ( NULL == mScaleAnim )
		mScaleAnim = eeNew( Waypoints, () );

	return mScaleAnim;
}

Interpolation * UIControlAnim::AlphaInterpolation() {
	if ( NULL == mAlphaAnim )
		mAlphaAnim = eeNew( Interpolation, () );

	return mAlphaAnim;
}

Waypoints * UIControlAnim::MovementInterpolation() {
	if ( NULL == mMoveAnim )
		mMoveAnim = eeNew( Waypoints, () );

	return mMoveAnim;
}

void UIControlAnim::OnAngleChange() {
	SendCommonEvent( UIEvent::EventOnAngleChange );
}

void UIControlAnim::OnScaleChange() {
	SendCommonEvent( UIEvent::EventOnScaleChange );
}

void UIControlAnim::OnAlphaChange() {
	SendCommonEvent( UIEvent::EventOnAlphaChange );
}

}}

