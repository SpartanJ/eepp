#ifndef EE_UICUICONTROLANIM_H
#define EE_UICUICONTROLANIM_H

#include <eepp/ui/base.hpp>
#include <eepp/ui/uicontrol.hpp>
#include <eepp/ui/uidragable.hpp>

namespace EE { namespace UI {

class EE_API UIControlAnim : public UIDragable {
	public:
		static UIControlAnim * New();

		UIControlAnim( const CreateParams& Params );

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

		Interpolation * startAlphaAnim( const Float& From, const Float& To, const Time& TotalTime, const bool& alphaChilds = true, const Ease::Interpolation& getType = Ease::Linear, Interpolation::OnPathEndCallback PathEndCallback = Interpolation::OnPathEndCallback() );

		Waypoints * startScaleAnim( const Vector2f& From, const Vector2f& To, const Time& TotalTime, const Ease::Interpolation& getType = Ease::Linear, Interpolation::OnPathEndCallback PathEndCallback = Interpolation::OnPathEndCallback() );

		Waypoints * startScaleAnim( const Float& From, const Float& To, const Time& TotalTime, const Ease::Interpolation& getType = Ease::Linear, Interpolation::OnPathEndCallback PathEndCallback = Interpolation::OnPathEndCallback() );

		Waypoints * startMovement( const Vector2i& From, const Vector2i& To, const Time& TotalTime, const Ease::Interpolation& getType = Ease::Linear, Waypoints::OnPathEndCallback PathEndCallback = Interpolation::OnPathEndCallback() );

		Interpolation * startRotation( const Float& From, const Float& To, const Time& TotalTime, const Ease::Interpolation& getType = Ease::Linear, Interpolation::OnPathEndCallback PathEndCallback = Interpolation::OnPathEndCallback() );

		Interpolation * createFadeIn( const Time& Time, const bool& alphaChilds = true, const Ease::Interpolation& getType = Ease::Linear );

		Interpolation * createFadeOut( const Time& Time, const bool& alphaChilds = true, const Ease::Interpolation& getType = Ease::Linear );

		Interpolation * closeFadeOut( const Time& Time, const bool& alphaChilds = true, const Ease::Interpolation& getType = Ease::Linear );

		Interpolation * disableFadeOut( const Time & Time, const bool& alphaChilds = true, const Ease::Interpolation& getType = Ease::Linear );

		Interpolation * getRotationInterpolation();

		Waypoints * getScaleInterpolation();

		Interpolation * getAlphaInterpolation();

		Waypoints * getMovementInterpolation();

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

		Interpolation * 	mAngleAnim;
		Waypoints *			mScaleAnim;
		Interpolation * 	mAlphaAnim;
		Waypoints * 		mMoveAnim;

		virtual void backgroundDraw();

		virtual void borderDraw();

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
