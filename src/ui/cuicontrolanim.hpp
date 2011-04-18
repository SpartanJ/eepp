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

		void StartAlphaAnim( const eeFloat& From, const eeFloat& To, const eeFloat& TotalTime, const bool& AlphaChilds = true, const EE_INTERPOLATION& Type = LINEAR, cInterpolation::OnPathEndCallback PathEndCallback = cInterpolation::OnPathEndCallback() );

		void StartScaleAnim( const eeFloat& From, const eeFloat& To, const eeFloat& TotalTime, const EE_INTERPOLATION& Type = LINEAR, cInterpolation::OnPathEndCallback PathEndCallback = cInterpolation::OnPathEndCallback() );

		void StartMovement( const eeVector2i& From, const eeVector2i& To, const eeFloat& TotalTime, const EE_INTERPOLATION& Type = LINEAR, cWaypoints::OnPathEndCallback PathEndCallback = cInterpolation::OnPathEndCallback() );

		void StartRotation( const eeFloat& From, const eeFloat& To, const eeFloat& TotalTime, const EE_INTERPOLATION& Type = LINEAR, cInterpolation::OnPathEndCallback PathEndCallback = cInterpolation::OnPathEndCallback() );

		void CreateFadeIn( const eeFloat& Time, const bool& AlphaChilds = true, const EE_INTERPOLATION& Type = LINEAR );

		void CreateFadeOut( const eeFloat& Time, const bool& AlphaChilds = true, const EE_INTERPOLATION& Type = LINEAR );

		void CloseFadeOut( const eeFloat& Time, const bool& AlphaChilds = true, const EE_INTERPOLATION& Type = LINEAR );

		void DisableFadeOut( const eeFloat& Time, const bool& AlphaChilds = true, const EE_INTERPOLATION& Type = LINEAR );

		cInterpolation * AngleInterpolation();

		cInterpolation * ScaleInterpolation();

		cInterpolation * AlphaInterpolation();

		cWaypoints * MovementInterpolation();

		virtual void Draw();

		bool FadingOut();
    protected:
    	friend class cUIManager;

		eeFloat 			mAngle;
		eeFloat 			mScale;
		eeFloat 			mAlpha;

		cInterpolation * 	mAngleAnim;
		cInterpolation * 	mScaleAnim;
		cInterpolation * 	mAlphaAnim;
		cWaypoints * 		mMoveAnim;

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
