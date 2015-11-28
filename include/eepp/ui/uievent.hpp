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
			EventOnComplexControlFocusLoss,
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

		UIEvent( UIControl * Ctrl, const Uint32& EventType = EventForceDWord );

		~UIEvent();

		UIControl * Ctrl() const;

		const Uint32& EventType() const;
	protected:
		UIControl	* 	mCtrl;
		Uint32 			mEventType;
};

}}

#endif
