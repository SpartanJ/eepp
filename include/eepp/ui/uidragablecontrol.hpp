#ifndef EE_UICUIDRAGABLE_H
#define EE_UICUIDRAGABLE_H

#include <eepp/ui/uicontrol.hpp>

namespace EE { namespace UI {

class EE_API UIDragableControl : public UIControl {
	public:
		static UIDragableControl * New();

		UIDragableControl();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		bool isDragging() const;

		void setDragging( const bool& dragging );

		const Vector2i& getDragPoint() const;

		void setDragPoint( const Vector2i& Point );

		virtual void update();

		bool isDragEnabled() const;

		void setDragEnabled( const bool& enable );

		void setDragButton( const Uint32& Button );

		const Uint32& getDragButton() const;
	protected:
		virtual ~UIDragableControl();

		Vector2i 	mDragPoint;
		Uint32 		mDragButton;

		virtual Uint32 onMouseDown( const Vector2i& position, const Uint32 flags );

		virtual Uint32 onMouseUp( const Vector2i& position, const Uint32 flags );

		virtual Uint32 onDrag( const Vector2i& position );

		virtual Uint32 onDragStart( const Vector2i& position );

		virtual Uint32 onDragStop( const Vector2i& position );
};

}}

#endif

