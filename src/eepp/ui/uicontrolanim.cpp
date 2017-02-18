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

	updateOriginPoint();
}

UIControlAnim::~UIControlAnim() {
	eeSAFE_DELETE( mAlphaAnim );
	eeSAFE_DELETE( mAngleAnim );
	eeSAFE_DELETE( mScaleAnim );
	eeSAFE_DELETE( mMoveAnim );
}

Uint32 UIControlAnim::getType() const {
	return UI_TYPE_CONTROL_ANIM;
}

bool UIControlAnim::isType( const Uint32& type ) const {
	return UIControlAnim::getType() == type ? true : UIControl::isType( type );
}

void UIControlAnim::draw() {
	if ( mVisible && 0.f != mAlpha ) {
		if ( mFlags & UI_FILL_BACKGROUND )
			backgroundDraw();

		if ( mFlags & UI_BORDER )
			borderDraw();

		if ( NULL != mSkinState )
			mSkinState->draw( mScreenPosf.x, mScreenPosf.y, (Float)mSize.getWidth(), (Float)mSize.getHeight(), (Uint32)mAlpha );

		if ( UIManager::instance()->highlightFocus() && UIManager::instance()->focusControl() == this ) {
			Primitives P;
			P.setFillMode( DRAW_LINE );
			P.setBlendMode( blend() );
			P.setColor( UIManager::instance()->highlightFocusColor() );
			P.drawRectangle( getRectf() );
		}

		if ( UIManager::instance()->highlightOver() && UIManager::instance()->overControl() == this ) {
			Primitives P;
			P.setFillMode( DRAW_LINE );
			P.setBlendMode( blend() );
			P.setColor( UIManager::instance()->highlightOverColor() );
			P.drawRectangle( getRectf() );
		}
	}
}

const Float& UIControlAnim::angle() const {
	return mAngle;
}

const OriginPoint& UIControlAnim::rotationOriginPoint() const {
	return mRotationOriginPoint;
}

void UIControlAnim::rotationOriginPoint( const OriginPoint & center ) {
	mRotationOriginPoint = center;
	updateOriginPoint();
}

Vector2f UIControlAnim::rotationCenter() {
	switch ( mRotationOriginPoint.OriginType ) {
		case OriginPoint::OriginCenter: return mCenter;
		case OriginPoint::OriginTopLeft: return mScreenPosf;
		case OriginPoint::OriginCustom: default: return mScreenPosf + mRotationOriginPoint;
	}
}

void UIControlAnim::angle( const Float& angle ) {
	mAngle = angle;
	onAngleChange();
}

void UIControlAnim::angle( const Float& angle , const OriginPoint & center ) {
	mRotationOriginPoint = center;
	updateOriginPoint();
	this->angle( angle );
}

const Vector2f& UIControlAnim::scale() const {
	return mScale;
}

void UIControlAnim::scale( const Vector2f & scale ) {
	mScale = scale;
	onScaleChange();
}

const OriginPoint& UIControlAnim::scaleOriginPoint() const {
	return mScaleOriginPoint;
}

void UIControlAnim::scaleOriginPoint( const OriginPoint & center ) {
	mScaleOriginPoint = center;
	updateOriginPoint();
}

Vector2f UIControlAnim::scaleCenter() {
	switch ( mScaleOriginPoint.OriginType ) {
		case OriginPoint::OriginCenter: return mCenter;
		case OriginPoint::OriginTopLeft: return mScreenPosf;
		case OriginPoint::OriginCustom: default: return mScreenPosf + mScaleOriginPoint;
	}
}

void UIControlAnim::scale( const Vector2f& scale, const OriginPoint& center ) {
	mScaleOriginPoint = center;
	updateOriginPoint();
	this->scale( scale );
}

void UIControlAnim::scale( const Float& scale, const OriginPoint& center ) {
	this->scale( Vector2f( scale, scale ), center );
}

const Float& UIControlAnim::alpha() const {
	return mAlpha;
}

void UIControlAnim::alpha( const Float& alpha ) {
	mAlpha = alpha;
	onAlphaChange();
}

void UIControlAnim::alphaChilds( const Float &alpha ) {
	UIControlAnim * AnimChild;
	UIControl * CurChild = mChild;

	while ( NULL != CurChild ) {
		if ( CurChild->isAnimated() ) {
			AnimChild = reinterpret_cast<UIControlAnim*> ( CurChild );

			AnimChild->alpha( alpha );
			AnimChild->alphaChilds( alpha );
		}

		CurChild = CurChild->nextGet();
	}
}

void UIControlAnim::matrixSet() {
	if ( mScale != 1.f || mAngle != 0.f ) {
		GlobalBatchRenderer::instance()->draw();

		GLi->pushMatrix();

		Vector2f scaleCenter = this->scaleCenter();
		GLi->translatef( scaleCenter.x , scaleCenter.y, 0.f );
		GLi->scalef( mScale.x, mScale.y, 1.0f );
		GLi->translatef( -scaleCenter.x, -scaleCenter.y, 0.f );

		Vector2f rotationCenter = this->rotationCenter();
		GLi->translatef( rotationCenter.x , rotationCenter.y, 0.f );
		GLi->rotatef( mAngle, 0.0f, 0.0f, 1.0f );
		GLi->translatef( -rotationCenter.x, -rotationCenter.y, 0.f );
	}
}

void UIControlAnim::matrixUnset() {
	if ( mScale != 1.f || mAngle != 0.f ) {
		GlobalBatchRenderer::instance()->draw();

		GLi->popMatrix();
	}
}

void UIControlAnim::update() {
	UIDragable::update();

	if ( NULL != mMoveAnim && mMoveAnim->isEnabled() ) {
		mMoveAnim->update( elapsed() );
		position( (int)mMoveAnim->getPos().x, (int)mMoveAnim->getPos().y );

		if ( mMoveAnim->ended() )
			eeSAFE_DELETE( mMoveAnim );
	}

	if ( NULL != mAlphaAnim && mAlphaAnim->isEnabled() ) {
		mAlphaAnim->update( elapsed() );
		alpha( mAlphaAnim->getRealPos() );

		if ( mAlphaAnim->ended() ) {
			if ( ( mControlFlags & UI_CTRL_FLAG_CLOSE_FO )  )
				close();

			if ( ( mControlFlags & UI_CTRL_FLAG_DISABLE_FADE_OUT ) ) {
				mControlFlags &= ~UI_CTRL_FLAG_DISABLE_FADE_OUT;

				visible( false );
			}

			eeSAFE_DELETE( mAlphaAnim );
		}
	}

	if ( NULL != mScaleAnim && mScaleAnim->isEnabled() ) {
		mScaleAnim->update( elapsed() );
		scale( mScaleAnim->getPos() );

		if ( mScaleAnim->ended() )
			eeSAFE_DELETE( mScaleAnim );
	}

	if ( NULL != mAngleAnim && mAngleAnim->isEnabled() ) {
		mAngleAnim->update( elapsed() );
		angle( mAngleAnim->getRealPos() );

		if ( mAngleAnim->ended() )
			eeSAFE_DELETE( mAngleAnim );
	}
}

bool UIControlAnim::isFadingOut() {
	return 0 != ( mControlFlags & UI_CTRL_FLAG_DISABLE_FADE_OUT );
}

bool UIControlAnim::isAnimating() {
	return ( NULL != mAlphaAnim && mAlphaAnim->isEnabled() ) || ( NULL != mAngleAnim && mAngleAnim->isEnabled() ) || ( NULL != mScaleAnim && mScaleAnim->isEnabled() ) || ( NULL != mMoveAnim && mMoveAnim->isEnabled() );
}

Interpolation * UIControlAnim::startAlphaAnim( const Float& From, const Float& To, const Time& TotalTime, const bool& AlphaChilds, const Ease::Interpolation& Type, Interpolation::OnPathEndCallback PathEndCallback ) {
	if ( NULL == mAlphaAnim )
		mAlphaAnim = eeNew( Interpolation, () );

	mAlphaAnim->clearWaypoints();
	mAlphaAnim->addWaypoint( From );
	mAlphaAnim->addWaypoint( To );
	mAlphaAnim->setTotalTime( TotalTime );
	mAlphaAnim->start( PathEndCallback );
	mAlphaAnim->setType( Type );

	alpha( From );

	if ( AlphaChilds ) {
		UIControlAnim * AnimChild;
		UIControl * CurChild = mChild;

		while ( NULL != CurChild ) {
			if ( CurChild->isAnimated() ) {
				AnimChild = reinterpret_cast<UIControlAnim*> ( CurChild );

				AnimChild->startAlphaAnim( From, To, TotalTime, AlphaChilds );
			}

			CurChild = CurChild->nextGet();
		}
	}

	return mAlphaAnim;
}

Waypoints * UIControlAnim::startScaleAnim( const Vector2f& From, const Vector2f& To, const Time& TotalTime, const Ease::Interpolation& Type, Interpolation::OnPathEndCallback PathEndCallback ) {
	if ( NULL == mScaleAnim )
		mScaleAnim = eeNew( Waypoints, () );

	mScaleAnim->clearWaypoints();
	mScaleAnim->addWaypoint( From );
	mScaleAnim->addWaypoint( To );
	mScaleAnim->setTotalTime( TotalTime );
	mScaleAnim->start( PathEndCallback );
	mScaleAnim->setType( Type );

	scale( From );

	return mScaleAnim;
}

Waypoints * UIControlAnim::startScaleAnim( const Float& From, const Float& To, const Time& TotalTime, const Ease::Interpolation& Type, Interpolation::OnPathEndCallback PathEndCallback ) {
	return startScaleAnim( Vector2f( From, From ), Vector2f( To, To ), TotalTime, Type, PathEndCallback );
}

Waypoints * UIControlAnim::startMovement( const Vector2i& From, const Vector2i& To, const Time& TotalTime, const Ease::Interpolation& Type, Waypoints::OnPathEndCallback PathEndCallback ) {
	if ( NULL == mMoveAnim )
		mMoveAnim = eeNew( Waypoints, () );

	mMoveAnim->clearWaypoints();
	mMoveAnim->addWaypoint( Vector2f( (Float)From.x, (Float)From.y ) );
	mMoveAnim->addWaypoint( Vector2f( (Float)To.x, (Float)To.y ) );
	mMoveAnim->setTotalTime( TotalTime );
	mMoveAnim->start( PathEndCallback );
	mMoveAnim->setType( Type );

	position( From );

	return mMoveAnim;
}

Interpolation * UIControlAnim::startRotation( const Float& From, const Float& To, const Time& TotalTime, const Ease::Interpolation& Type, Interpolation::OnPathEndCallback PathEndCallback ) {
	if ( NULL == mAngleAnim )
		mAngleAnim = eeNew( Interpolation, () );

	mAngleAnim->clearWaypoints();
	mAngleAnim->addWaypoint( From );
	mAngleAnim->addWaypoint( To );
	mAngleAnim->setTotalTime( TotalTime );
	mAngleAnim->start( PathEndCallback );
	mAngleAnim->setType( Type );

	angle( From );

	return mAngleAnim;
}

Interpolation * UIControlAnim::createFadeIn( const Time& Time, const bool& AlphaChilds, const Ease::Interpolation& Type ) {
	return startAlphaAnim( mAlpha, 255.f, Time, AlphaChilds, Type );
}

Interpolation * UIControlAnim::createFadeOut( const Time& Time, const bool& AlphaChilds, const Ease::Interpolation& Type ) {
	return startAlphaAnim( 255.f, mAlpha, Time, AlphaChilds, Type );
}

Interpolation * UIControlAnim::closeFadeOut( const Time& Time, const bool& AlphaChilds, const Ease::Interpolation& Type ) {
	startAlphaAnim	( mAlpha, 0.f, Time, AlphaChilds, Type );
	mControlFlags |= UI_CTRL_FLAG_CLOSE_FO;
	return mAlphaAnim;
}

Interpolation * UIControlAnim::disableFadeOut( const Time& Time, const bool& AlphaChilds, const Ease::Interpolation& Type ) {
	enabled( false );

	startAlphaAnim	( mAlpha, 0.f, Time, AlphaChilds, Type );

	mControlFlags |= UI_CTRL_FLAG_DISABLE_FADE_OUT;

	return mAlphaAnim;
}

void UIControlAnim::backgroundDraw() {
	Primitives P;
	Rectf R = getRectf();
	P.setBlendMode( mBackground->blend() );
	P.setColor( getColor( mBackground->color() ) );

	if ( 4 == mBackground->colors().size() ) {
		if ( mBackground->corners() ) {
			P.drawRoundedRectangle( R, getColor( mBackground->colors()[0] ), getColor( mBackground->colors()[1] ), getColor( mBackground->colors()[2] ), getColor( mBackground->colors()[3] ), mBackground->corners() );
		} else {
			P.drawRectangle( R, getColor( mBackground->colors()[0] ), getColor( mBackground->colors()[1] ), getColor( mBackground->colors()[2] ), getColor( mBackground->colors()[3] ) );
		}
	} else {
		if ( mBackground->corners() ) {
			P.drawRoundedRectangle( R, 0.f, Vector2f::One, mBackground->corners() );
		} else {
			P.drawRectangle( R );
		}
	}
}

void UIControlAnim::borderDraw() {
	Primitives P;
	P.setFillMode( DRAW_LINE );
	P.setBlendMode( blend() );
	P.setLineWidth( (Float)mBorder->width() );
	P.setColor( getColor( mBorder->color() ) );

	//! @TODO: Check why was this +0.1f -0.1f?
	if ( mFlags & UI_CLIP_ENABLE ) {
		Rectf R( Vector2f( mScreenPosf.x + 0.1f, mScreenPosf.y + 0.1f ), Sizef( (Float)mSize.getWidth() - 0.1f, (Float)mSize.getHeight() - 0.1f ) );

		if ( mBackground->corners() ) {
			P.drawRoundedRectangle( getRectf(), 0.f, Vector2f::One, mBackground->corners() );
		} else {
			P.drawRectangle( R );
		}
	} else {
		if ( mBackground->corners() ) {
			P.drawRoundedRectangle( getRectf(), 0.f, Vector2f::One, mBackground->corners() );
		} else {
			P.drawRectangle( getRectf() );
		}
	}
}

ColorA UIControlAnim::getColor( const ColorA& Col ) {
	return ColorA( Col.r(), Col.g(), Col.b(), static_cast<Uint8>( (Float)Col.a() * ( mAlpha / 255.f ) ) );
}

void UIControlAnim::updateQuad() {
	mPoly		= Polygon2f( eeAABB( mScreenPosf.x, mScreenPosf.y, mScreenPosf.x + mSize.getWidth(), mScreenPosf.y + mSize.getHeight() ) );

	mPoly.rotate( mAngle, rotationCenter() );
	mPoly.scale( mScale, scaleCenter() );

	UIControl * tParent = parent();

	while ( tParent ) {
		if ( tParent->isAnimated() ) {
			UIControlAnim * tP = reinterpret_cast<UIControlAnim *> ( tParent );

			mPoly.rotate( tP->angle(), tP->rotationCenter() );
			mPoly.scale( tP->scale(), tP->scaleCenter() );
		}

		tParent = tParent->parent();
	};
}

void UIControlAnim::onSizeChange() {
	updateOriginPoint();
}

void UIControlAnim::updateOriginPoint() {
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

Interpolation * UIControlAnim::rotationInterpolation() {
	if ( NULL == mAngleAnim )
		mAngleAnim = eeNew( Interpolation, () );

	return mAngleAnim;
}

Waypoints * UIControlAnim::scaleInterpolation() {
	if ( NULL == mScaleAnim )
		mScaleAnim = eeNew( Waypoints, () );

	return mScaleAnim;
}

Interpolation * UIControlAnim::alphaInterpolation() {
	if ( NULL == mAlphaAnim )
		mAlphaAnim = eeNew( Interpolation, () );

	return mAlphaAnim;
}

Waypoints * UIControlAnim::movementInterpolation() {
	if ( NULL == mMoveAnim )
		mMoveAnim = eeNew( Waypoints, () );

	return mMoveAnim;
}

void UIControlAnim::onAngleChange() {
	sendCommonEvent( UIEvent::EventOnAngleChange );
}

void UIControlAnim::onScaleChange() {
	sendCommonEvent( UIEvent::EventOnScaleChange );
}

void UIControlAnim::onAlphaChange() {
	sendCommonEvent( UIEvent::EventOnAlphaChange );
}

}}

