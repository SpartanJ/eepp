#ifndef EE_UICUIDRAGABLE_H
#define EE_UICUIDRAGABLE_H

#include <eepp/ui/cuicontrol.hpp>

namespace EE { namespace UI {

class EE_API cUIDragable : public cUIControl {
	public:
    	cUIDragable( const cUIControl::CreateParams& Params );

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		bool				Dragging() const;
		void				Dragging( const bool& dragging );

		const eeVector2i&	DragPoint() const;
		void				DragPoint( const eeVector2i& Point );

		virtual void		Update();

		bool				DragEnable() const;
		void 				DragEnable( const bool& enable );

		void 				DragButton( const Uint32& Button );
		const Uint32& 		DragButton() const;
	protected:
		virtual ~cUIDragable();

		eeVector2i 	mDragPoint;
		Uint32 		mDragButton;

		virtual Uint32	OnMouseDown( const eeVector2i& Pos, const Uint32 Flags );

		virtual Uint32 	OnMouseUp( const eeVector2i& Pos, const Uint32 Flags );

		virtual Uint32	OnDrag( const eeVector2i& Pos );
};

}}

#endif

