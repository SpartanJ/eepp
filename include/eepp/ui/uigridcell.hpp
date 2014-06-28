#ifndef EE_UICUIGRIDCELL_HPP
#define EE_UICUIGRIDCELL_HPP

#include <eepp/ui/uicomplexcontrol.hpp>
#include <eepp/ui/uiitemcontainer.hpp>

namespace EE { namespace UI {

class UIGenericGrid;

class EE_API UIGridCell : public UIComplexControl {
	public:
		UIGridCell( UIGridCell::CreateParams& Params );

		virtual ~UIGridCell();

		virtual void SetTheme( UITheme * Theme );

		void Cell( const Uint32& CollumnIndex, UIControl * Ctrl );

		UIControl * Cell( const Uint32& CollumnIndex ) const;

		virtual void Update();

		bool Selected() const;

		void Unselect();

		void Select();

		virtual Uint32 OnMessage( const UIMessage * Msg );

		void AutoSize();
	protected:
		friend class UIItemContainer<UIGenericGrid>;
		friend class UIGenericGrid;

		std::vector<UIControl*> mCells;

		UIGenericGrid * GridParent() const;

		void FixCell();

		virtual Uint32 OnMouseExit( const Vector2i& Pos, const Uint32 Flags );

		virtual void OnStateChange();
};

}}

#endif
