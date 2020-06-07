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
		TextInput,
		MouseMove,
		MouseDown,
		MouseClick,
		MouseDoubleClick,
		MouseUp,
		MouseOver,
		MouseLeave,
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
		OnFontStyleChanged,
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
		MsgBoxCancelClick,
		OnTabSelected,
		OnTabClosed,
		OnTabNavigate,
		OnClose, // Warning: Only some controls will report this event.
		OnDragStart,
		OnDragStop,
		OnPaddingChange,
		OnBufferChange,
		OnUpdateScreenPosition,
		OnPageChanged,
		OnMarginChange,
		OnTagChange,
		OnIdChange,
		OnClassChange,
		OnLayoutUpdate,
		OnSelectionChanged,
		UserEvent,
		NoEvent = eeINDEX_NOT_FOUND
	};

	Event( Node* node, const Uint32& eventType = NoEvent );

	~Event();

	Node* getNode() const;

	const Uint32& getType() const;

	const Uint32& getCallbackId() const;

  protected:
	friend class Node;
	Node* mNode;
	Uint32 mEventType;
	Uint32 mCallbackId;
};

}} // namespace EE::Scene

#endif
