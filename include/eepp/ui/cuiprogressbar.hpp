#ifndef EE_UICPROGRESSBAR_HPP
#define EE_UICPROGRESSBAR_HPP

#include <eepp/ui/cuicontrolanim.hpp>
#include <eepp/ui/cuitextbox.hpp>
#include <eepp/graphics/scrollparallax.hpp>

namespace EE { namespace UI {

class EE_API cUIProgressBar : public cUIComplexControl {
	public:
		class CreateParams : public cUITextBox::CreateParams {
			public:
				inline CreateParams() :
					cUITextBox::CreateParams(),
					DisplayPercent( false ),
					VerticalExpand( false ),
					MovementSpeed( 64.f, 0.f )
				{
				}

				inline ~CreateParams() {}

				bool DisplayPercent;
				bool VerticalExpand;
				Vector2f MovementSpeed;
				Rectf FillerMargin;
		};

		cUIProgressBar( const cUIProgressBar::CreateParams& Params );

		virtual ~cUIProgressBar();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void SetTheme( cUITheme * Theme );

		virtual void Progress( Float Val );

		const Float& Progress() const;

		virtual void TotalSteps( const Float& Steps );

		const Float& TotalSteps() const;

		virtual void Draw();

		void MovementSpeed( const Vector2f& Speed );

		const Vector2f& MovementSpeed() const;

		void VerticalExpand( const bool& VerticalExpand );

		const bool& VerticalExpand() const;

		void FillerMargin( const Rectf& margin );

		const Rectf& FillerMargin() const;

		void DisplayPercent( const bool& DisplayPercent );

		const bool& DisplayPercent() const;
		
		cUITextBox * TextBox() const;
		
	protected:
		bool				mVerticalExpand;
		Vector2f			mSpeed;
		Rectf 			mFillerMargin;
		bool				mDisplayPercent;

		Float				mProgress;
		Float				mTotalSteps;

		ScrollParallax *	mParallax;

		cUITextBox * 		mTextBox;

		virtual Uint32 OnValueChange();

		virtual void OnSizeChange();
		
		void UpdateTextBox();
		
		virtual void OnAlphaChange();
};

}}

#endif

