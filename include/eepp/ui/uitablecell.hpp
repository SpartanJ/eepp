#ifndef EE_UICUIGRIDCELL_HPP
#define EE_UICUIGRIDCELL_HPP

#include <eepp/ui/uiwidget.hpp>
#include <eepp/ui/uiitemcontainer.hpp>

namespace EE { namespace UI {

class UITable;

class EE_API UITableCell : public UIWidget {
	public:
		static UITableCell * New();

		UITableCell();

		virtual ~UITableCell();

		virtual void setTheme( UITheme * Theme );

		void setCell( const Uint32& CollumnIndex, UIControl * Ctrl );

		UIControl * getCell( const Uint32& CollumnIndex ) const;

		virtual void update();

		bool isSelected() const;

		void unselect();

		void select();

		virtual Uint32 onMessage( const UIMessage * Msg );
	protected:
		friend class UIItemContainer<UITable>;
		friend class UITable;

		std::vector<UIControl*> mCells;

		UITable * gridParent() const;

		void fixCell();

		virtual Uint32 onMouseExit( const Vector2i& position, const Uint32 flags );

		virtual void onStateChange();

		virtual void onParentChange();

		virtual void onAutoSize();
};

}}

#endif
