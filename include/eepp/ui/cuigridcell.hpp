#ifndef EE_UICUIGRIDCELL_HPP
#define EE_UICUIGRIDCELL_HPP

#include <eepp/ui/cuicomplexcontrol.hpp>
#include <eepp/ui/tuiitemcontainer.hpp>

namespace EE { namespace UI {

class cUIGenericGrid;

class EE_API cUIGridCell : public cUIComplexControl {
	public:
		cUIGridCell( cUIGridCell::CreateParams& Params );

		virtual ~cUIGridCell();

		virtual void SetTheme( cUITheme * Theme );

		void Cell( const Uint32& CollumnIndex, cUIControl * Ctrl );

		cUIControl * Cell( const Uint32& CollumnIndex ) const;

		virtual void Update();

		bool Selected() const;

		void Unselect();

		void Select();

		virtual Uint32 OnMessage( const cUIMessage * Msg );

		void AutoSize();
	protected:
		friend class tUIItemContainer<cUIGenericGrid>;
		friend class cUIGenericGrid;

		std::vector<cUIControl*> mCells;

		cUIGenericGrid * GridParent() const;

		void FixCell();

		virtual Uint32 OnMouseExit( const eeVector2i& Pos, const Uint32 Flags );

		virtual void OnStateChange();
};

}}

#endif
