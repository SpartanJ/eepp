#ifndef EE_UICUILISTBOXITEM_HPP
#define EE_UICUILISTBOXITEM_HPP

#include <eepp/ui/cuitextbox.hpp>
#include <eepp/ui/tuiitemcontainer.hpp>

namespace EE { namespace UI {

class cUIListBox;

class EE_API cUIListBoxItem : public cUITextBox {
	public:
		cUIListBoxItem( const cUITextBox::CreateParams& Params );

		virtual ~cUIListBoxItem();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void SetTheme( cUITheme * Theme );

		virtual void Update();

		bool Selected() const;

		void Unselect();

		void Select();
	protected:
		friend class tUIItemContainer<cUIListBox>;

		virtual void OnStateChange();

		virtual Uint32 OnMouseClick( const eeVector2i& Pos, const Uint32 Flags );

		virtual Uint32 OnMouseExit( const eeVector2i& Pos, const Uint32 Flags );
};

}}

#endif

