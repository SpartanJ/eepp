#ifndef EE_UICUIEVENT_HPP
#define EE_UICUIEVENT_HPP

#include <eepp/config.hpp>

namespace EE { namespace Scene {

class Node;

class EE_API Event {
	public:
		enum EventType {
			KeyDown = 0,
			KeyUp,
			MouseMove,
			MouseDown,
			MouseClick,
			MouseDoubleClick,
			MouseUp,
			MouseEnter,
			MouseExit,
			OnFocus,
			OnFocusLoss,
			OnVisibleChange,
			OnEnabledChange,
			OnPositionChange,
			OnSizeChange,
			OnAngleChange,
			OnScaleChange,
			OnAlphaChange,
			OnTextChanged,
			OnFontChanged,
			OnPressEnter,
			OnValueChange,
			OnWidgetFocusLoss,
			OnItemClicked,
			OnHideByClick,
			OnItemKeyDown,
			OnItemKeyUp,
			OnItemSelected,
			OnCursorPosChange,
			OnParentSizeChange,
			OnWindowClose,
			OnWindowCloseClick,
			OnWindowMaximizeClick,
			OnWindowMinimizeClick,
			OpenFile,
			SaveFile,
			OnControlClear,
			MsgBoxConfirmClick,
			OnTabSelected,
			OnClose, // Warning: Only some controls will report this event.
			OnDragStart,
			OnDragStop,
			UserEvent,
			NoEvent = eeINDEX_NOT_FOUND
		};

		Event( Node * node, const Uint32& eventType = NoEvent );

		~Event();

		Node * getNode() const;

		const Uint32& getType() const;
	protected:
		Node	* 	mNode;
		Uint32 		mEventType;
};

}}

#endif
