#ifndef EE_UICPROGRESSBAR_HPP
#define EE_UICPROGRESSBAR_HPP

#include <eepp/ui/uinode.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/graphics/scrollparallax.hpp>

namespace EE { namespace UI {

class EE_API UIProgressBar : public UIWidget {
	public:
		class StyleConfig {
			public:
				bool DisplayPercent = false;
				bool VerticalExpand = true;
				Vector2f MovementSpeed = Vector2f( 64.f, 0 );
				Rectf FillerPadding;
		};

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

		virtual void scheduledUpdate( const Time& time );

		void setMovementSpeed( const Vector2f& Speed );

		const Vector2f& getMovementSpeed() const;

		void setVerticalExpand( const bool& verticalExpand );

		const bool& getVerticalExpand() const;

		void setFillerPadding( const Rectf& padding );

		const Rectf& getFillerPadding() const;

		void setDisplayPercent( const bool& displayPercent );

		const bool& getDisplayPercent() const;
		
		UITextView * getTextBox() const;

		virtual bool setAttribute( const NodeAttribute& attribute, const Uint32& state = UIState::StateFlagNormal );

		const StyleConfig& getStyleConfig() const;
	protected:
		StyleConfig			mStyleConfig;
		Float				mProgress;
		Float				mTotalSteps;
		UITextView * 		mTextBox;
		Vector2f			mOffset;
		UISkin *			mFillerSkin;

		virtual Uint32 onValueChange();

		virtual void onSizeChange();

		virtual void onThemeLoaded();
		
		void updateTextBox();
		
		virtual void onAlphaChange();
};

}}

#endif

