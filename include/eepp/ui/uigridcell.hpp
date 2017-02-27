#ifndef EE_UICUIGRIDCELL_HPP
#define EE_UICUIGRIDCELL_HPP

#include <eepp/ui/uicomplexcontrol.hpp>
#include <eepp/ui/uiitemcontainer.hpp>

namespace EE { namespace UI {

class UIGenericGrid;

class EE_API UIGridCell : public UIComplexControl {
	public:
		UIGridCell( UIGridCell::CreateParams& Params );

		UIGridCell();

		virtual ~UIGridCell();

		virtual void setTheme( UITheme * Theme );

		void setCell( const Uint32& CollumnIndex, UIControl * Ctrl );

		UIControl * getCell( const Uint32& CollumnIndex ) const;

		virtual void update();

		bool isSelected() const;

		void unselect();

		void select();

		virtual Uint32 onMessage( const UIMessage * Msg );

		void autoSize();
	protected:
		friend class UIItemContainer<UIGenericGrid>;
		friend class UIGenericGrid;

		std::vector<UIControl*> mCells;

		UIGenericGrid * gridParent() const;

		void fixCell();

		virtual Uint32 onMouseExit( const Vector2i& position, const Uint32 flags );

		virtual void onStateChange();

		virtual void onParentChange();
};

}}

#endif
