#ifndef EE_UICUICONTROLANIM_H
#define EE_UICUICONTROLANIM_H

#include "base.hpp"
#include "cuicontrol.hpp"
#include "cuidragable.hpp"

namespace EE { namespace UI {

class EE_API cUIControlAnim : public cUIDragable {
	public:
    	cUIControlAnim( const CreateParams& Params );

    	virtual ~cUIControlAnim();

 		virtual void Update();

 		const eeFloat& Angle() const;

 		void Angle( const eeFloat& angle );

 		const eeFloat& Scale() const;

 		void Scale( const eeFloat& scale );

 		const eeFloat& Alpha() const;

 		virtual void Alpha( const eeFloat& alpha );

 		bool Animating();

		void StartAlphaAnim( const eeFloat& From, const eeFloat& To, const eeFloat& TotalTime, cInterpolation::OnPathEndCallback PathEndCallback = 0 );

		void StartScaleAnim( const eeFloat& From, const eeFloat& To, const eeFloat& TotalTime, cInterpolation::OnPathEndCallback PathEndCallback = 0 );

		void StartMovement( const eeVector2i& From, const eeVector2i& To, const eeFloat& TotalTime, cWaypoints::OnPathEndCallback PathEndCallback = 0 );

		void StartRotation( const eeFloat& From, const eeFloat& To, const eeFloat& TotalTime, cInterpolation::OnPathEndCallback PathEndCallback = 0 );

		void CreateFadeIn( const eeFloat& Time );

		void CreateFadeOut( const eeFloat& Time );

		void CloseFadeOut( const eeFloat& Time );

		const eeQuad2f& GetQuad() const;

		const eeVector2f& GetQuadCenter() const;

		cInterpolation * AngleInterpolation();

		cInterpolation * ScaleInterpolation();

		cInterpolation * AlphaInterpolation();

		cWaypoints * MovementInterpolation();
    protected:
    	friend class cUIManager;

		eeFloat 			mAngle;
		eeFloat 			mScale;
		eeFloat 			mAlpha;

		cInterpolation * 	mAngleAnim;
		cInterpolation * 	mScaleAnim;
		cInterpolation * 	mAlphaAnim;
		cWaypoints * 		mMoveAnim;

		virtual cUIControl * OverFind( const eeVector2i& Point );

		virtual void BackgroundDraw();

		virtual void BorderDraw();

		eeColorA GetColor( const eeColorA& Col );

		virtual void UpdateQuad();

		virtual void OnAngleChange();

		virtual void OnScaleChange();

		virtual void OnAlphaChange();

		virtual void MatrixSet();

		virtual void MatrixUnset();
};

}}

#endif
