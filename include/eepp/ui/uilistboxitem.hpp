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

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void SetTheme( UITheme * Theme );

		virtual void Update();

		bool Selected() const;

		void Unselect();

		void Select();
	protected:
		friend class UIItemContainer<UIListBox>;

		virtual void OnStateChange();

		virtual Uint32 OnMouseClick( const Vector2i& Pos, const Uint32 Flags );

		virtual Uint32 OnMouseExit( const Vector2i& Pos, const Uint32 Flags );
};

}}

#endif

