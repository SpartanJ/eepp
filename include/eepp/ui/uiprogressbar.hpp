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

		UIProgressBar();

		virtual ~UIProgressBar();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void setTheme( UITheme * Theme );

		virtual void setProgress( Float Val );

		const Float& getProgress() const;

		virtual void setTotalSteps( const Float& Steps );

		const Float& getTotalSteps() const;

		virtual void draw();

		void setMovementSpeed( const Vector2f& Speed );

		const Vector2f& getMovementSpeed() const;

		void setVerticalExpand( const bool& verticalExpand );

		const bool& getVerticalExpand() const;

		void setFillerPadding( const Rectf& margin );

		const Rectf& getFillerPadding() const;

		void setDisplayPercent( const bool& displayPercent );

		const bool& getDisplayPercent() const;
		
		UITextBox * getTextBox() const;
		
	protected:
		bool				mVerticalExpand;
		Vector2f			mSpeed;
		Rectf				mFillerPadding;
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

