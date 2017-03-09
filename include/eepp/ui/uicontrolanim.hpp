#ifndef EE_UICUICONTROLANIM_H
#define EE_UICUICONTROLANIM_H

#include <eepp/ui/base.hpp>
#include <eepp/ui/uicontrol.hpp>
#include <eepp/ui/uidragablecontrol.hpp>

namespace EE { namespace UI {

class EE_API UIControlAnim : public UIDragableControl {
	public:
		static UIControlAnim * New();

		UIControlAnim();

		virtual ~UIControlAnim();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void update();

		const Float& getRotation() const;

		void setRotation( const Float& angle );

		void setRotation( const Float& angle, const OriginPoint& center );

		const OriginPoint& getRotationOriginPoint() const;

		void setRotationOriginPoint( const OriginPoint& center );

		Vector2f getRotationCenter();

		const Vector2f& getScale() const;

		void setScale( const Vector2f& scale );

		void setScale( const Vector2f& scale, const OriginPoint& center );

		void setScale( const Float& scale , const OriginPoint & center = OriginPoint::OriginCenter );

		const OriginPoint& getScaleOriginPoint() const;

		void setScaleOriginPoint( const OriginPoint& center );

		Vector2f getScaleCenter();

		const Float& getAlpha() const;

		virtual void setAlpha( const Float& alpha );

		virtual void setChildsAlpha( const Float& alpha );

		bool isAnimating();

		Interpolation1d * startAlphaAnim( const Float& From, const Float& To, const Time& TotalTime, const bool& alphaChilds = true, const Ease::Interpolation& getType = Ease::Linear, Interpolation1d::OnPathEndCallback PathEndCallback = Interpolation1d::OnPathEndCallback() );

		Interpolation2d * startScaleAnim( const Vector2f& From, const Vector2f& To, const Time& TotalTime, const Ease::Interpolation& getType = Ease::Linear, Interpolation1d::OnPathEndCallback PathEndCallback = Interpolation1d::OnPathEndCallback() );

		Interpolation2d * startScaleAnim( const Float& From, const Float& To, const Time& TotalTime, const Ease::Interpolation& getType = Ease::Linear, Interpolation1d::OnPathEndCallback PathEndCallback = Interpolation1d::OnPathEndCallback() );

		Interpolation2d * startMovement( const Vector2i& From, const Vector2i& To, const Time& TotalTime, const Ease::Interpolation& getType = Ease::Linear, Interpolation2d::OnPathEndCallback PathEndCallback = Interpolation1d::OnPathEndCallback() );

		Interpolation1d * startRotation( const Float& From, const Float& To, const Time& TotalTime, const Ease::Interpolation& getType = Ease::Linear, Interpolation1d::OnPathEndCallback PathEndCallback = Interpolation1d::OnPathEndCallback() );

		Interpolation1d * createFadeIn( const Time& Time, const bool& alphaChilds = true, const Ease::Interpolation& getType = Ease::Linear );

		Interpolation1d * createFadeOut( const Time& Time, const bool& alphaChilds = true, const Ease::Interpolation& getType = Ease::Linear );

		Interpolation1d * closeFadeOut( const Time& Time, const bool& alphaChilds = true, const Ease::Interpolation& getType = Ease::Linear );

		Interpolation1d * disableFadeOut( const Time & Time, const bool& alphaChilds = true, const Ease::Interpolation& getType = Ease::Linear );

		Interpolation1d * getRotationInterpolation();

		Interpolation2d * getScaleInterpolation();

		Interpolation1d * getAlphaInterpolation();

		Interpolation2d * getMovementInterpolation();

		virtual void draw();

		bool isFadingOut();
	protected:
		friend class UIManager;
		friend class UIControl;

		Float				mAngle;
		OriginPoint			mRotationOriginPoint;
		Vector2f 			mScale;
		OriginPoint			mScaleOriginPoint;
		Float				mAlpha;

		Interpolation1d * 	mAngleAnim;
		Interpolation2d *	mScaleAnim;
		Interpolation1d * 	mAlphaAnim;
		Interpolation2d * 	mMoveAnim;

		virtual void drawBackground();

		virtual void drawBorder();

		virtual void drawSkin();

		ColorA getColor( const ColorA& Col );

		virtual void updateQuad();

		virtual void onSizeChange();

		virtual void onAngleChange();

		virtual void onScaleChange();

		virtual void onAlphaChange();

		virtual void matrixSet();

		virtual void matrixUnset();

		void updateOriginPoint();
};

}}

#endif
