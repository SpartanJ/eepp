#ifndef EE_UICUILISTBOXITEM_HPP
#define EE_UICUILISTBOXITEM_HPP

#include <eepp/ui/uitextbox.hpp>
#include <eepp/ui/uiitemcontainer.hpp>

namespace EE { namespace UI {

class UIListBox;

class EE_API UIListBoxItem : public UITextBox {
	public:
		UIListBoxItem( const UITextBox::CreateParams& Params );

		virtual ~UIListBoxItem();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void setTheme( UITheme * Theme );

		virtual void update();

		bool isSelected() const;

		void unselect();

		void select();
	protected:
		friend class UIItemContainer<UIListBox>;

		virtual void onStateChange();

		virtual Uint32 onMouseClick( const Vector2i& position, const Uint32 flags );

		virtual Uint32 onMouseExit( const Vector2i& position, const Uint32 flags );
};

}}

#endif

