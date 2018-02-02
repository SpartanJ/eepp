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

		UINode * getActiveButton() const;

		UINode * getInactiveButton() const;

		Int32 getTextSeparation() const;

		void setTextSeparation(const Int32 & textSeparation);

		virtual void loadFromXmlNode( const pugi::xml_node& node );
	protected:
		UINode *	mActiveButton;
		UINode *	mInactiveButton;
		bool			mActive;
		Uint32			mLastTick;
		Int32			mTextSeparation;

		virtual void onSizeChange();

		void switchState();

		virtual void onAlphaChange();

		virtual Uint32 onKeyDown( const KeyEvent& Event );

		virtual Uint32 onMessage( const NodeMessage * Msg );

		virtual void onThemeLoaded();

		virtual void onAutoSize();

		virtual void onPaddingChange();
};

}}

#endif


