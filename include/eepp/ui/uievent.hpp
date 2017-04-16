#ifndef EE_UICUIEVENT_HPP
#define EE_UICUIEVENT_HPP

#include <eepp/ui/base.hpp>

namespace EE { namespace UI {

class UIControl;

class EE_API UIEvent {
	public:
		enum Event {
			EventKeyDown = 0,
			EventKeyUp,
			EventMouseMove,
			EventMouseDown,
			EventMouseClick,
			EventMouseDoubleClick,
			EventMouseUp,
			EventMouseEnter,
			EventMouseExit,
			EventOnFocus,
			EventOnFocusLoss,
			EventOnVisibleChange,
			EventOnEnabledChange,
			EventOnPosChange,
			EventOnSizeChange,
			EventOnAngleChange,
			EventOnScaleChange,
			EventOnAlphaChange,
			EventOnTextChanged,
			EventOnFontChanged,
			EventOnPressEnter,
			EventOnValueChange,
			EventOnWidgetFocusLoss,
			EventOnItemClicked,
			EventOnHideByClick,
			EventOnItemKeyDown,
			EventOnItemKeyUp,
			EventOnItemSelected,
			EventOnCursorPosChange,
			EventOnParentSizeChange,
			EventOnWindowClose,
			EventOnWindowCloseClick,
			EventOnWindowMaximizeClick,
			EventOnWindowMinimizeClick,
			EventOpenFile,
			EventSaveFile,
			EventOnControlClear,
			EventMsgBoxConfirmClick,
			EventOnTabSelected,
			EventOnClose, // Warning: Only some controls will report this event.
			EventUser,
			EventForceDWord = eeINDEX_NOT_FOUND
		};

		UIEvent( UIControl * control, const Uint32& eventType = EventForceDWord );

		~UIEvent();

		UIControl * getControl() const;

		const Uint32& getEventType() const;
	protected:
		UIControl	* 	mCtrl;
		Uint32 			mEventType;
};

}}

#endif
