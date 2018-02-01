#ifndef EE_UICUIEVENT_HPP
#define EE_UICUIEVENT_HPP

#include <eepp/ui/base.hpp>

namespace EE { namespace Scene {

class Node;

class EE_API UIEvent {
	public:
		enum Event {
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

		UIEvent( Node * control, const Uint32& eventType = NoEvent );

		~UIEvent();

		Node * getControl() const;

		const Uint32& getEventType() const;
	protected:
		Node	* 	mCtrl;
		Uint32 		mEventType;
};

}}

#endif
