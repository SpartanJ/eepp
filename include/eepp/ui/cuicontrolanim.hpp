#ifndef EE_UICUICONTROLANIM_H
#define EE_UICUICONTROLANIM_H

#include <eepp/ui/base.hpp>
#include <eepp/ui/cuicontrol.hpp>
#include <eepp/ui/cuidragable.hpp>

namespace EE { namespace UI {

class EE_API cUIControlAnim : public cUIDragable {
	public:
    	cUIControlAnim( const CreateParams& Params );

    	virtual ~cUIControlAnim();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

 		virtual void Update();

 		const eeFloat& Angle() const;

 		void Angle( const eeFloat& angle );

 		const eeFloat& Scale() const;

 		void Scale( const eeFloat& scale );

 		const eeFloat& Alpha() const;

 		virtual void Alpha( const eeFloat& alpha );

		virtual void AlphaChilds( const eeFloat& alpha );

 		bool Animating();

		void StartAlphaAnim( const eeFloat& From, const eeFloat& To, const cTime& TotalTime, const bool& AlphaChilds = true, const Ease::Interpolation& Type = Ease::Linear, cInterpolation::OnPathEndCallback PathEndCallback = cInterpolation::OnPathEndCallback() );

		void StartScaleAnim( const eeFloat& From, const eeFloat& To, const cTime& TotalTime, const Ease::Interpolation& Type = Ease::Linear, cInterpolation::OnPathEndCallback PathEndCallback = cInterpolation::OnPathEndCallback() );

		void StartMovement( const eeVector2i& From, const eeVector2i& To, const cTime& TotalTime, const Ease::Interpolation& Type = Ease::Linear, cWaypoints::OnPathEndCallback PathEndCallback = cInterpolation::OnPathEndCallback() );

		void StartRotation( const eeFloat& From, const eeFloat& To, const cTime& TotalTime, const Ease::Interpolation& Type = Ease::Linear, cInterpolation::OnPathEndCallback PathEndCallback = cInterpolation::OnPathEndCallback() );

		void CreateFadeIn( const cTime& Time, const bool& AlphaChilds = true, const Ease::Interpolation& Type = Ease::Linear );

		void CreateFadeOut( const cTime& Time, const bool& AlphaChilds = true, const Ease::Interpolation& Type = Ease::Linear );

		void CloseFadeOut( const cTime& Time, const bool& AlphaChilds = true, const Ease::Interpolation& Type = Ease::Linear );

		void DisableFadeOut( const cTime & Time, const bool& AlphaChilds = true, const Ease::Interpolation& Type = Ease::Linear );

		cInterpolation * RotationInterpolation();

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
