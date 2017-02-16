#ifndef EE_UICPROGRESSBAR_HPP
#define EE_UICPROGRESSBAR_HPP

#include <eepp/ui/uicontrolanim.hpp>
#include <eepp/ui/uitextbox.hpp>
#include <eepp/graphics/scrollparallax.hpp>

namespace EE { namespace UI {

class EE_API UIProgressBar : public UIComplexControl {
	public:
		class CreateParams : public UITextBox::CreateParams {
			public:
				inline CreateParams() :
					UITextBox::CreateParams(),
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

		UIProgressBar( const UIProgressBar::CreateParams& Params );

		virtual ~UIProgressBar();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void setTheme( UITheme * Theme );

		virtual void progress( Float Val );

		const Float& progress() const;

		virtual void totalSteps( const Float& Steps );

		const Float& totalSteps() const;

		virtual void draw();

		void movementSpeed( const Vector2f& Speed );

		const Vector2f& movementSpeed() const;

		void verticalExpand( const bool& verticalExpand );

		const bool& verticalExpand() const;

		void fillerMargin( const Rectf& margin );

		const Rectf& fillerMargin() const;

		void displayPercent( const bool& displayPercent );

		const bool& displayPercent() const;
		
		UITextBox * getTextBox() const;
		
	protected:
		bool				mVerticalExpand;
		Vector2f			mSpeed;
		Rectf 			mFillerMargin;
		bool				mDisplayPercent;

		Float				mProgress;
		Float				mTotalSteps;

		ScrollParallax *	mParallax;

		UITextBox * 		mTextBox;

		virtual Uint32 onValueChange();

		virtual void onSizeChange();
		
		void updateTextBox();
		
		virtual void onAlphaChange();
};

}}

#endif

