#ifndef EE_UICUIEVENT_HPP
#define EE_UICUIEVENT_HPP

#include <eepp/config.hpp>
#include <string>

namespace EE { namespace UI {
class UITableRow;
}} // namespace EE::UI

namespace EE { namespace Scene {

class Node;
class MouseEvent;
class KeyEvent;
class DropEvent;
class TextEvent;
class TextInputEvent;
class WindowEvent;
class ItemValueEvent;
class RowCreatedEvent;

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
		MouseEnter,
		MouseOver,
		MouseLeave,
		MouseOut,
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
		OnChildCountChanged,
		OnDocumentLoaded,
		OnDocumentChanged,
		OnDocumentClosed,
		OnDocumentSyntaxDefinitionChange,
		OnDocumentDirtyOnFileSysten,
		OnFontStyleChanged,
		OnPressEnter,
		OnValueChange,
		OnWidgetFocusLoss,
		OnItemClicked,
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
		OnClear,
		OnConfirm,
		OnCancel,
		OnTabSelected,
		OnTabAdded,
		OnTabClosed,
		OnTabNavigate,
		OnClose,
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
		OnNodeDropped,
		OnDocumentSave,
		OnDocumentUndoRedo,
		OnModelEvent,
		OnResourceChange,
		OnActiveWidgetChange,
		OnWindowReady,
		OnCreateContextMenu,
		OnDocumentMoved,
		OnTextPasted,
		UserEvent,
		OnMenuShow,
		OnMenuHide,
		OnEditorTabReady,
		OnTitleChange,
		OnWindowAdded,
		OnWindowRemoved,
		OnItemValueChange,
		OnCursorPosChangeInteresting,
		OnCopy,
		OnRowCreated,
		OnScrollChange,
		OnModelChanged,
		NoEvent = eeINDEX_NOT_FOUND
	};

	Event( Node* node, const Uint32& eventType = NoEvent );

	~Event();

	Node* getNode() const;

	const Uint32& getType() const;

	const Uint32& getCallbackId() const;

	const MouseEvent* asMouseEvent() const;

	const KeyEvent* asKeyEvent() const;

	const DropEvent* asDropEvent() const;

	const TextEvent* asTextEvent() const;

	const TextInputEvent* asTextInputEvent() const;

	const WindowEvent* asWindowEvent() const;

	const ItemValueEvent* asItemValueEvent() const;

	const RowCreatedEvent* asRowCreatedEvent() const;

  protected:
	friend class Node;
	Node* mNode;
	Uint32 mEventType;
	Uint32 mCallbackId;
};

class EE_API DropEvent : public Event {
  public:
	DropEvent( Node* node, Node* droppedNode, const Uint32& eventType ) :
		Event( node, eventType ), droppedNode( droppedNode ) {}
	Node* getDroppedNode() const { return droppedNode; }

  protected:
	Node* droppedNode;
};

class EE_API TextEvent : public Event {
  public:
	TextEvent( Node* node, const Uint32& eventType, const std::string& txt ) :
		Event( node, eventType ), text( txt ) {}

	const std::string& getText() const { return text; }

  protected:
	std::string text;
};

class EE_API WindowEvent : public Event {
  public:
	WindowEvent( Node* node, Node* window, const Uint32& eventType ) :
		Event( node, eventType ), window( window ) {}
	Node* getWindow() const { return window; }

  protected:
	Node* window;
};

class EE_API ItemValueEvent : public Event {
  public:
	ItemValueEvent( Node* node, const Uint32& eventType, const Uint32& itemIndex ) :
		Event( node, eventType ), itemIndex( itemIndex ) {}
	const Uint32& getItemIndex() const { return itemIndex; }

  protected:
	Uint32 itemIndex;
};

class EE_API RowCreatedEvent : public Event {
  public:
	RowCreatedEvent( Node* node, const Uint32& eventType, UI::UITableRow* row ) :
		Event( node, eventType ), row( row ) {}

	UI::UITableRow* getRow() const { return row; }

  protected:
	UI::UITableRow* row;
};

}} // namespace EE::Scene

#endif
