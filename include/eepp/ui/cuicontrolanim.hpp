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

 		const Float& Angle() const;

 		void Angle( const Float& angle );

		const eeVector2f& Scale() const;

		void Scale( const eeVector2f& scale );

		void Scale( const Float& scale );

 		const Float& Alpha() const;

 		virtual void Alpha( const Float& alpha );

		virtual void AlphaChilds( const Float& alpha );

 		bool Animating();

		void StartAlphaAnim( const Float& From, const Float& To, const cTime& TotalTime, const bool& AlphaChilds = true, const Ease::Interpolation& Type = Ease::Linear, cInterpolation::OnPathEndCallback PathEndCallback = cInterpolation::OnPathEndCallback() );

		void StartScaleAnim( const eeVector2f& From, const eeVector2f& To, const cTime& TotalTime, const Ease::Interpolation& Type = Ease::Linear, cInterpolation::OnPathEndCallback PathEndCallback = cInterpolation::OnPathEndCallback() );

		void StartScaleAnim( const Float& From, const Float& To, const cTime& TotalTime, const Ease::Interpolation& Type = Ease::Linear, cInterpolation::OnPathEndCallback PathEndCallback = cInterpolation::OnPathEndCallback() );

		void StartMovement( const eeVector2i& From, const eeVector2i& To, const cTime& TotalTime, const Ease::Interpolation& Type = Ease::Linear, cWaypoints::OnPathEndCallback PathEndCallback = cInterpolation::OnPathEndCallback() );

		void StartRotation( const Float& From, const Float& To, const cTime& TotalTime, const Ease::Interpolation& Type = Ease::Linear, cInterpolation::OnPathEndCallback PathEndCallback = cInterpolation::OnPathEndCallback() );

		void CreateFadeIn( const cTime& Time, const bool& AlphaChilds = true, const Ease::Interpolation& Type = Ease::Linear );

		void CreateFadeOut( const cTime& Time, const bool& AlphaChilds = true, const Ease::Interpolation& Type = Ease::Linear );

		void CloseFadeOut( const cTime& Time, const bool& AlphaChilds = true, const Ease::Interpolation& Type = Ease::Linear );

		void DisableFadeOut( const cTime & Time, const bool& AlphaChilds = true, const Ease::Interpolation& Type = Ease::Linear );

		cInterpolation * RotationInterpolation();

		cWaypoints * ScaleInterpolation();

		cInterpolation * AlphaInterpolation();

		cWaypoints * MovementInterpolation();

		virtual void Draw();

		bool FadingOut();
    protected:
    	friend class cUIManager;

		Float 			mAngle;
		eeVector2f 			mScale;
		Float 			mAlpha;

		cInterpolation * 	mAngleAnim;
		cWaypoints *		mScaleAnim;
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
