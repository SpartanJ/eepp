#include <eepp/ui/uicontrolanim.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/primitives.hpp>

namespace EE { namespace UI {

UIControlAnim * UIControlAnim::New() {
	return eeNew( UIControlAnim, () );
}

UIControlAnim::UIControlAnim() :
	UIDragableControl(),
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

void UIControlAnim::drawSkin() {
	if ( NULL != mSkinState ) {
		if ( mFlags & UI_SKIN_KEEP_SIZE_ON_DRAW ) {
			Sizei rSize = PixelDensity::dpToPxI( mSkinState->getSkin()->getSize( mSkinState->getState() ) );
			Sizei diff = ( mRealSize - rSize ) / 2;

			mSkinState->draw( mScreenPosf.x + diff.x, mScreenPosf.y + diff.y, (Float)rSize.getWidth(), (Float)rSize.getHeight(), (Uint32)mAlpha );
		} else {
			mSkinState->draw( mScreenPosf.x, mScreenPosf.y, (Float)mRealSize.getWidth(), (Float)mRealSize.getHeight(), (Uint32)mAlpha );
		}
	}
}

void UIControlAnim::draw() {
	if ( mVisible && 0.f != mAlpha ) {
		drawBackground();

		drawBorder();

		drawSkin();

		drawHighlightFocus();

		drawOverControl();

		drawDebugData();

		drawBox();
	}
}

const Float& UIControlAnim::getRotation() const {
	return mAngle;
}

const OriginPoint& UIControlAnim::getRotationOriginPoint() const {
	return mRotationOriginPoint;
}

void UIControlAnim::setRotationOriginPoint( const OriginPoint & center ) {
	mRotationOriginPoint = PixelDensity::dpToPx( center );
	updateOriginPoint();
}

Vector2f UIControlAnim::getRotationCenter() {
	switch ( mRotationOriginPoint.OriginType ) {
		case OriginPoint::OriginCenter: return mCenter;
		case OriginPoint::OriginTopLeft: return mScreenPosf;
		case OriginPoint::OriginCustom: default: return mScreenPosf + mRotationOriginPoint;
	}
}

void UIControlAnim::setRotation( const Float& angle ) {
	mAngle = angle;

	if ( mAngle != 0.f ) {
		mControlFlags |= UI_CTRL_FLAG_ROTATED;
	} else {
		if ( mControlFlags & UI_CTRL_FLAG_ROTATED )
			mControlFlags &= ~UI_CTRL_FLAG_ROTATED;
	}

	onAngleChange();
}

void UIControlAnim::setRotation( const Float& angle , const OriginPoint & center ) {
	mRotationOriginPoint = center;
	updateOriginPoint();
	this->setRotation( angle );
}

const Vector2f& UIControlAnim::getScale() const {
	return mScale;
}

void UIControlAnim::setScale( const Vector2f & scale ) {
	mScale = scale;

	if ( mScale != 1.f ) {
		mControlFlags |= UI_CTRL_FLAG_SCALED;
	} else {
		if ( mControlFlags & UI_CTRL_FLAG_SCALED )
			mControlFlags &= ~UI_CTRL_FLAG_SCALED;
	}

	onScaleChange();
}

const OriginPoint& UIControlAnim::getScaleOriginPoint() const {
	return mScaleOriginPoint;
}

void UIControlAnim::setScaleOriginPoint( const OriginPoint & center ) {
	mScaleOriginPoint = PixelDensity::dpToPx( center );
	updateOriginPoint();
}

Vector2f UIControlAnim::getScaleCenter() {
	switch ( mScaleOriginPoint.OriginType ) {
		case OriginPoint::OriginCenter: return mCenter;
		case OriginPoint::OriginTopLeft: return mScreenPosf;
		case OriginPoint::OriginCustom: default: return mScreenPosf + mScaleOriginPoint;
	}
}

void UIControlAnim::setScale( const Vector2f& scale, const OriginPoint& center ) {
	mScaleOriginPoint = center;
	updateOriginPoint();
	this->setScale( scale );
}

void UIControlAnim::setScale( const Float& scale, const OriginPoint& center ) {
	this->setScale( Vector2f( scale, scale ), center );
}

const Float& UIControlAnim::getAlpha() const {
	return mAlpha;
}

void UIControlAnim::setAlpha( const Float& alpha ) {
	mAlpha = alpha;
	onAlphaChange();
}

void UIControlAnim::setChildsAlpha( const Float &alpha ) {
	UIControlAnim * AnimChild;
	UIControl * CurChild = mChild;

	while ( NULL != CurChild ) {
		if ( CurChild->isAnimated() ) {
			AnimChild = reinterpret_cast<UIControlAnim*> ( CurChild );

			AnimChild->setAlpha( alpha );
			AnimChild->setChildsAlpha( alpha );
		}

		CurChild = CurChild->getNextControl();
	}
}

void UIControlAnim::matrixSet() {
	if ( mScale != 1.f || mAngle != 0.f ) {
		GlobalBatchRenderer::instance()->draw();

		GLi->pushMatrix();

		Vector2f scaleCenter = this->getScaleCenter();
		GLi->translatef( scaleCenter.x , scaleCenter.y, 0.f );
		GLi->scalef( mScale.x, mScale.y, 1.0f );
		GLi->translatef( -scaleCenter.x, -scaleCenter.y, 0.f );

		Vector2f rotationCenter = this->getRotationCenter();
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
	UIDragableControl::update();

	if ( NULL != mMoveAnim && mMoveAnim->isEnabled() ) {
		mMoveAnim->update( getElapsed() );
		setPosition( (int)mMoveAnim->getPos().x, (int)mMoveAnim->getPos().y );

		if ( mMoveAnim->ended() )
			eeSAFE_DELETE( mMoveAnim );
	}

	if ( NULL != mAlphaAnim && mAlphaAnim->isEnabled() ) {
		mAlphaAnim->update( getElapsed() );
		setAlpha( mAlphaAnim->getRealPos() );

		if ( mAlphaAnim->ended() ) {
			if ( ( mControlFlags & UI_CTRL_FLAG_CLOSE_FO )  )
				close();

			if ( ( mControlFlags & UI_CTRL_FLAG_DISABLE_FADE_OUT ) ) {
				mControlFlags &= ~UI_CTRL_FLAG_DISABLE_FADE_OUT;

				setVisible( false );
			}

			eeSAFE_DELETE( mAlphaAnim );
		}
	}

	if ( NULL != mScaleAnim && mScaleAnim->isEnabled() ) {
		mScaleAnim->update( getElapsed() );
		setScale( mScaleAnim->getPos() );

		if ( mScaleAnim->ended() )
			eeSAFE_DELETE( mScaleAnim );
	}

	if ( NULL != mAngleAnim && mAngleAnim->isEnabled() ) {
		mAngleAnim->update( getElapsed() );
		setRotation( mAngleAnim->getRealPos() );

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

Interpolation1d * UIControlAnim::startAlphaAnim( const Float& From, const Float& To, const Time& TotalTime, const bool& AlphaChilds, const Ease::Interpolation& Type, Interpolation1d::OnPathEndCallback PathEndCallback ) {
	if ( NULL == mAlphaAnim )
		mAlphaAnim = eeNew( Interpolation1d, () );

	mAlphaAnim->clearWaypoints();
	mAlphaAnim->addWaypoint( From );
	mAlphaAnim->addWaypoint( To );
	mAlphaAnim->setTotalTime( TotalTime );
	mAlphaAnim->start( PathEndCallback );
	mAlphaAnim->setType( Type );

	setAlpha( From );

	if ( AlphaChilds ) {
		UIControlAnim * AnimChild;
		UIControl * CurChild = mChild;

		while ( NULL != CurChild ) {
			if ( CurChild->isAnimated() ) {
				AnimChild = reinterpret_cast<UIControlAnim*> ( CurChild );

				AnimChild->startAlphaAnim( From, To, TotalTime, AlphaChilds );
			}

			CurChild = CurChild->getNextControl();
		}
	}

	return mAlphaAnim;
}

Interpolation2d * UIControlAnim::startScaleAnim( const Vector2f& From, const Vector2f& To, const Time& TotalTime, const Ease::Interpolation& Type, Interpolation1d::OnPathEndCallback PathEndCallback ) {
	if ( NULL == mScaleAnim )
		mScaleAnim = eeNew( Interpolation2d, () );

	mScaleAnim->clearWaypoints();
	mScaleAnim->addWaypoint( From );
	mScaleAnim->addWaypoint( To );
	mScaleAnim->setTotalTime( TotalTime );
	mScaleAnim->start( PathEndCallback );
	mScaleAnim->setType( Type );

	setScale( From );

	return mScaleAnim;
}

Interpolation2d * UIControlAnim::startScaleAnim( const Float& From, const Float& To, const Time& TotalTime, const Ease::Interpolation& Type, Interpolation1d::OnPathEndCallback PathEndCallback ) {
	return startScaleAnim( Vector2f( From, From ), Vector2f( To, To ), TotalTime, Type, PathEndCallback );
}

Interpolation2d * UIControlAnim::startTranslation( const Vector2i& From, const Vector2i& To, const Time& TotalTime, const Ease::Interpolation& Type, Interpolation2d::OnPathEndCallback PathEndCallback ) {
	if ( NULL == mMoveAnim )
		mMoveAnim = eeNew( Interpolation2d, () );

	mMoveAnim->clearWaypoints();
	mMoveAnim->addWaypoint( Vector2f( (Float)From.x, (Float)From.y ) );
	mMoveAnim->addWaypoint( Vector2f( (Float)To.x, (Float)To.y ) );
	mMoveAnim->setTotalTime( TotalTime );
	mMoveAnim->start( PathEndCallback );
	mMoveAnim->setType( Type );

	setPosition( From );

	return mMoveAnim;
}

Interpolation1d * UIControlAnim::startRotation( const Float& From, const Float& To, const Time& TotalTime, const Ease::Interpolation& Type, Interpolation1d::OnPathEndCallback PathEndCallback ) {
	if ( NULL == mAngleAnim )
		mAngleAnim = eeNew( Interpolation1d, () );

	mAngleAnim->clearWaypoints();
	mAngleAnim->addWaypoint( From );
	mAngleAnim->addWaypoint( To );
	mAngleAnim->setTotalTime( TotalTime );
	mAngleAnim->start( PathEndCallback );
	mAngleAnim->setType( Type );

	setRotation( From );

	return mAngleAnim;
}

Interpolation1d * UIControlAnim::createFadeIn( const Time& Time, const bool& AlphaChilds, const Ease::Interpolation& Type ) {
	return startAlphaAnim( mAlpha, 255.f, Time, AlphaChilds, Type );
}

Interpolation1d * UIControlAnim::createFadeOut( const Time& Time, const bool& AlphaChilds, const Ease::Interpolation& Type ) {
	return startAlphaAnim( 255.f, mAlpha, Time, AlphaChilds, Type );
}

Interpolation1d * UIControlAnim::closeFadeOut( const Time& Time, const bool& AlphaChilds, const Ease::Interpolation& Type ) {
	startAlphaAnim	( mAlpha, 0.f, Time, AlphaChilds, Type );
	mControlFlags |= UI_CTRL_FLAG_CLOSE_FO;
	return mAlphaAnim;
}

Interpolation1d * UIControlAnim::disableFadeOut( const Time& Time, const bool& AlphaChilds, const Ease::Interpolation& Type ) {
	setEnabled( false );

	startAlphaAnim	( mAlpha, 0.f, Time, AlphaChilds, Type );

	mControlFlags |= UI_CTRL_FLAG_DISABLE_FADE_OUT;

	return mAlphaAnim;
}

void UIControlAnim::drawBackground() {
	if ( mFlags & UI_FILL_BACKGROUND ) {
		mBackground->draw( getRectf(), mAlpha );
	}
}

void UIControlAnim::drawBorder() {
	if ( mFlags & UI_BORDER ) {
		mBorder->draw( getRectf(), mAlpha, mBackground->getCorners(), ( mFlags & UI_CLIP_ENABLE ) != 0 );
	}
}

Color UIControlAnim::getColor( const Color& Col ) {
	return Color( Col.r, Col.g, Col.b, static_cast<Uint8>( (Float)Col.a * ( mAlpha / 255.f ) ) );
}

void UIControlAnim::updateQuad() {
	mPoly		= Polygon2f( Rectf( mScreenPosf.x, mScreenPosf.y, mScreenPosf.x + mRealSize.getWidth(), mScreenPosf.y + mRealSize.getHeight() ) );

	mPoly.rotate( mAngle, getRotationCenter() );
	mPoly.scale( mScale, getScaleCenter() );

	UIControl * tParent = getParent();

	while ( tParent ) {
		if ( tParent->isAnimated() ) {
			UIControlAnim * tP = reinterpret_cast<UIControlAnim *> ( tParent );

			mPoly.rotate( tP->getRotation(), tP->getRotationCenter() );
			mPoly.scale( tP->getScale(), tP->getScaleCenter() );
		}

		tParent = tParent->getParent();
	};
}

void UIControlAnim::onSizeChange() {
	UIDragableControl::onSizeChange();

	updateOriginPoint();
}

void UIControlAnim::updateOriginPoint() {
	switch ( mRotationOriginPoint.OriginType ) {
		case OriginPoint::OriginCenter:
			mRotationOriginPoint.x = mRealSize.x * 0.5f;
			mRotationOriginPoint.y = mRealSize.y * 0.5f;
			break;
		case OriginPoint::OriginTopLeft:
			mRotationOriginPoint.x = mRotationOriginPoint.y = 0;
			break;
		default: {}
	}

	switch ( mScaleOriginPoint.OriginType ) {
		case OriginPoint::OriginCenter:
			mScaleOriginPoint.x = mRealSize.x * 0.5f;
			mScaleOriginPoint.y = mRealSize.y * 0.5f;
			break;
		case OriginPoint::OriginTopLeft:
			mScaleOriginPoint.x = mScaleOriginPoint.y = 0;
			break;
		default: {}
	}
}

Interpolation1d * UIControlAnim::getRotationInterpolation() {
	if ( NULL == mAngleAnim )
		mAngleAnim = eeNew( Interpolation1d, () );

	return mAngleAnim;
}

Interpolation2d * UIControlAnim::getScaleInterpolation() {
	if ( NULL == mScaleAnim )
		mScaleAnim = eeNew( Interpolation2d, () );

	return mScaleAnim;
}

Interpolation1d * UIControlAnim::getAlphaInterpolation() {
	if ( NULL == mAlphaAnim )
		mAlphaAnim = eeNew( Interpolation1d, () );

	return mAlphaAnim;
}

Interpolation2d * UIControlAnim::getTranslationInterpolation() {
	if ( NULL == mMoveAnim )
		mMoveAnim = eeNew( Interpolation2d, () );

	return mMoveAnim;
}

void UIControlAnim::onAngleChange() {
	sendCommonEvent( UIEvent::OnAngleChange );
	invalidateDraw();
}

void UIControlAnim::onScaleChange() {
	sendCommonEvent( UIEvent::OnScaleChange );
	invalidateDraw();
}

void UIControlAnim::onAlphaChange() {
	sendCommonEvent( UIEvent::OnAlphaChange );
	invalidateDraw();
}

}}

