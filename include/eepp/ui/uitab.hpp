#ifndef EE_UICUITAB_HPP
#define EE_UICUITAB_HPP

#include <eepp/ui/uiselectbutton.hpp>

namespace EE { namespace UI {

class UITabWidget;

class EE_API UITab : public UISelectButton {
	public:
		UITab( UISelectButton::CreateParams& Params, UIControl * ctrlOwned );

		UIControl * ctrlOwned() const;

		virtual ~UITab();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void setTheme( UITheme * Theme );

		virtual const String& getText();

		virtual void setText( const String& text );

		virtual void update();
	protected:
		UIControl *	mCtrlOwned;

		virtual Uint32 onMouseClick( const Vector2i &position, const Uint32 flags );

		virtual void onStateChange();

		void autoSize();

		UITabWidget * getTabWidget();
};

}}

#endif
