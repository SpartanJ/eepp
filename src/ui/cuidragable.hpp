#ifndef EE_UICUIDRAGABLE_H
#define EE_UICUIDRAGABLE_H

#include "cuicontrol.hpp"

namespace EE { namespace UI {

class EE_API cUIDragable : public cUIControl {
	public:
    	cUIDragable( const cUIControl::CreateParams& Params );

		bool				Dragging() const;
		void				Dragging( const bool& dragging );

		const eeVector2i&	DragPoint() const;
		void				DragPoint( const eeVector2i& Point );

		const eeVector2i&	DraggingPoint() const;
		void				DraggingPoint( const eeVector2i& Point );

		virtual void		Update();

		const bool&		DragEnable() const;
		void 				DragEnable( const bool& enable );

		void 				DragButton( const Uint32& Button );
		const Uint32& 		DragButton() const;
	protected:
 	   virtual ~cUIDragable();

 	   bool 		mDragEnable;
 	   bool 		mDragging;
 	   eeVector2i 	mDragPoint;
 	   eeVector2i 	mDraggingPoint;
 	   Uint32 		mDragButton;

		virtual Uint32		OnMouseDown( const eeVector2i& Pos, const Uint32 Flags );
		virtual Uint32 	OnMouseUp( const eeVector2i& Pos, const Uint32 Flags );
};

}}

#endif

