#ifndef EE_UICUICONTROLANIM_H
#define EE_UICUICONTROLANIM_H

#include <eepp/ui/base.hpp>
#include <eepp/ui/uicontrol.hpp>
#include <eepp/ui/uidragable.hpp>

namespace EE { namespace UI {

class EE_API UIControlAnim : public UIDragable {
	public:
		UIControlAnim( const CreateParams& Params );

		virtual ~UIControlAnim();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void Update();

		const Float& Angle() const;

		void Angle( const Float& angle );

		const Vector2f& Scale() const;

		void Scale( const Vector2f& scale );

		void Scale( const Float& scale );

		const Float& Alpha() const;

		virtual void Alpha( const Float& alpha );

		virtual void AlphaChilds( const Float& alpha );

		bool Animating();

		Interpolation * StartAlphaAnim( const Float& From, const Float& To, const Time& TotalTime, const bool& AlphaChilds = true, const Ease::Interpolation& Type = Ease::Linear, Interpolation::OnPathEndCallback PathEndCallback = Interpolation::OnPathEndCallback() );

		Waypoints * StartScaleAnim( const Vector2f& From, const Vector2f& To, const Time& TotalTime, const Ease::Interpolation& Type = Ease::Linear, Interpolation::OnPathEndCallback PathEndCallback = Interpolation::OnPathEndCallback() );

		Waypoints * StartScaleAnim( const Float& From, const Float& To, const Time& TotalTime, const Ease::Interpolation& Type = Ease::Linear, Interpolation::OnPathEndCallback PathEndCallback = Interpolation::OnPathEndCallback() );

		Waypoints * StartMovement( const Vector2i& From, const Vector2i& To, const Time& TotalTime, const Ease::Interpolation& Type = Ease::Linear, Waypoints::OnPathEndCallback PathEndCallback = Interpolation::OnPathEndCallback() );

		Interpolation * StartRotation( const Float& From, const Float& To, const Time& TotalTime, const Ease::Interpolation& Type = Ease::Linear, Interpolation::OnPathEndCallback PathEndCallback = Interpolation::OnPathEndCallback() );

		Interpolation * CreateFadeIn( const Time& Time, const bool& AlphaChilds = true, const Ease::Interpolation& Type = Ease::Linear );

		Interpolation * CreateFadeOut( const Time& Time, const bool& AlphaChilds = true, const Ease::Interpolation& Type = Ease::Linear );

		Interpolation * CloseFadeOut( const Time& Time, const bool& AlphaChilds = true, const Ease::Interpolation& Type = Ease::Linear );

		Interpolation * DisableFadeOut( const Time & Time, const bool& AlphaChilds = true, const Ease::Interpolation& Type = Ease::Linear );

		Interpolation * RotationInterpolation();

		Waypoints * ScaleInterpolation();

		Interpolation * AlphaInterpolation();

		Waypoints * MovementInterpolation();

		virtual void Draw();

		bool FadingOut();
	protected:
		friend class UIManager;

		Float				mAngle;
		Vector2f 			mScale;
		Float				mAlpha;

		Interpolation * 	mAngleAnim;
		Waypoints *			mScaleAnim;
		Interpolation * 	mAlphaAnim;
		Waypoints * 		mMoveAnim;

		virtual void BackgroundDraw();

		virtual void BorderDraw();

		ColorA GetColor( const ColorA& Col );

		virtual void UpdateQuad();

		virtual void OnAngleChange();

		virtual void OnScaleChange();

		virtual void OnAlphaChange();

		virtual void MatrixSet();

		virtual void MatrixUnset();
};

}}

#endif
