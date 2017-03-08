#ifndef EE_UICUICHECKBOX_H
#define EE_UICUICHECKBOX_H

#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uipushbutton.hpp>

namespace EE { namespace UI {

class EE_API UICheckBox : public UITextView {
	public:
		static UICheckBox * New();

		UICheckBox();

		virtual ~UICheckBox();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void setTheme( UITheme * Theme );

		const bool& isActive() const;

		void setActive( const bool& active );

		UIControlAnim * getActiveButton() const;

		UIControlAnim * getInactiveButton() const;

		Int32 getTextSeparation() const;

		void setTextSeparation(const Int32 & textSeparation);
	protected:
		UIControlAnim *	mActiveButton;
		UIControlAnim *	mInactiveButton;
		bool			mActive;
		Uint32			mLastTick;
		Int32			mTextSeparation;

		virtual void onSizeChange();

		void switchState();

		virtual void onAlphaChange();

		virtual Uint32 onKeyDown( const UIEventKey& Event );

		virtual Uint32 onMessage( const UIMessage * Msg );

		virtual void onThemeLoaded();

		virtual void onAutoSize();

		virtual void onPaddingChange();
};

}}

#endif


