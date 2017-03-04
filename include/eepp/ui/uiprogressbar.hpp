#ifndef EE_UICPROGRESSBAR_HPP
#define EE_UICPROGRESSBAR_HPP

#include <eepp/ui/uicontrolanim.hpp>
#include <eepp/ui/uitextbox.hpp>
#include <eepp/graphics/scrollparallax.hpp>

namespace EE { namespace UI {

class EE_API UIProgressBar : public UIComplexControl {
	public:
		static UIProgressBar * New();

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
		ProgressBarStyleConfig mStyleConfig;
		Float				mProgress;
		Float				mTotalSteps;
		ScrollParallax *	mParallax;
		UITextBox * 		mTextBox;

		virtual Uint32 onValueChange();

		virtual void onSizeChange();

		virtual void onThemeLoaded();
		
		void updateTextBox();
		
		virtual void onAlphaChange();
};

}}

#endif

