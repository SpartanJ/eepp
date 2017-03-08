#ifndef UI_UIRELATIVELAYOUT_HPP
#define UI_UIRELATIVELAYOUT_HPP

#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class UIRelativeLayout : public UIWidget {
	public:
		static UIRelativeLayout * New();

		UIRelativeLayout();


	protected:
		virtual void onSizeChange();

		virtual void onChildCountChange();

		void fixChilds();

		void fixChildPos( UIWidget * widget );

		void fixChildSize( UIWidget * widget );
};

}}

#endif
