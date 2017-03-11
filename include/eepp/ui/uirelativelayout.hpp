#ifndef UI_UIRELATIVELAYOUT_HPP
#define UI_UIRELATIVELAYOUT_HPP

#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class EE_API UIRelativeLayout : public UIWidget {
	public:
		static UIRelativeLayout * New();

		UIRelativeLayout();

		UIRelativeLayout * add( UIWidget * widget );
	protected:
		virtual Uint32 onMessage(const UIMessage * Msg);

		virtual void onSizeChange();

		virtual void onChildCountChange();

		virtual void onParentSizeChange( const Vector2i& SizeChange );

		void fixChilds();

		void fixChildPos( UIWidget * widget );

		void fixChildSize( UIWidget * widget );
};

}}

#endif
