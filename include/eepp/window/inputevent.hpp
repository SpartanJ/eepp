#ifndef EE_WINDOWINPUTEVENT_HPP
#define EE_WINDOWINPUTEVENT_HPP

namespace EE { namespace Window {

#define EE_TEXT_INPUT_SIZE (32)
#define EE_APPMOUSEFOCUS	0x01
#define EE_APPINPUTFOCUS	0x02
#define EE_APPACTIVE		0x04

class InputEvent {
	public:
		inline InputEvent() : Type( NoEvent ) {}

		inline InputEvent( Uint32 type ) : Type( type ) {}

		struct KeySym {
			Uint32 sym;			/** virtual keysym */
			Uint32 mod;			/** current key modifiers */
			Uint16 unicode;		/** translated character */
		};

		/** Application visibility event structure */
		struct ActiveEvent {
			Uint8 gain;		/** Whether given states were gained or lost (1/0) */
			Uint8 state;	/** A mask of the focus states */
		};

		/** Keyboard event structure */
		struct KeyboardEvent {
			Uint8 which;		/** The keyboard device index */
			Uint8 state;
			KeySym keysym;
		};

		/** Mouse motion event structure */
		struct MouseMotionEvent {
			Uint8 which;	/** The mouse device index */
			Uint8 state;	/** The current button state */
			Int16 x, y;		/** The X/Y coordinates of the mouse */
			Int16 xrel;		/** The relative motion in the X direction */
			Int16 yrel;		/** The relative motion in the Y direction */
		};

		/** Mouse button event structure */
		struct MouseButtonEvent {
			Uint8 which;	/** The mouse device index */
			Uint8 button;	/** The mouse button index */
			Uint8 state;	/** EE_PRESSED or EE_RELEASED */
			Int16 x, y;		/** The X/Y coordinates of the mouse at press time */
		};

		/** Touch finger motion/finger event structure */
		struct FingerEvent
		{
			Uint32 timestamp;
			Int64 touchId;		/** The touch device id */
			Int64 fingerId;		/** The finger id */
			float x;			/** The x coordinate of the touch. Normalized in the range 0...1 */
			float y;			/** The y coordinate of the touch. Normalized in the range 0...1 */
			float dx;			/** Change in x coordinate during this motion event. Normalized in the range 0...1 */
			float dy;			/** Change in y coordinate during this motion event. Normalized in the range 0...1 */
			float pressure;		/** The pressure of the touch. Normalized in the range 0...1 */
		};

		/** Keyboard text editing event */
		struct TextInputEvent {
			Uint32 timestamp;
			Uint32 text;

			String Text() {
				return String( text );
			}
		};

		/** Joystick axis motion event structure */
		struct JoyAxisEvent {
			Uint8 which;	/** The joystick device index */
			Uint8 axis;		/** The joystick axis index */
			Int16 value;	/** The axis value (range: -32768 to 32767) */
		};

		/** Joystick trackball motion event structure */
		struct JoyBallEvent {
			Uint8 which;	/** The joystick device index */
			Uint8 ball;		/** The joystick trackball index */
			Int16 xrel;		/** The relative motion in the X direction */
			Int16 yrel;		/** The relative motion in the Y direction */
		};

		/** Joystick hat position change event structure */
		struct JoyHatEvent {
			Uint8 which;	/** The joystick device index */
			Uint8 hat;		/** The joystick hat index */
			Uint8 value;	/** The hat position value:
					 *   EE_HAT_LEFTUP   EE_HAT_UP       EE_HAT_RIGHTUP
					 *   EE_HAT_LEFT     EE_HAT_CENTERED EE_HAT_RIGHT
					 *   EE_HAT_LEFTDOWN EE_HAT_DOWN     EE_HAT_RIGHTDOWN
					 *  Note that zero means the POV is centered.
					 */
		};

		/** Joystick button event structure */
		struct JoyButtonEvent {
			Uint8 which;	/** The joystick device index */
			Uint8 button;	/** The joystick button index */
			Uint8 state;	/** EE_PRESSED or EE_RELEASED */
		};

		/** The "window resized" event */
		struct ResizeEvent {
			int w;		/** New width */
			int h;		/** New height */
		};

		/** The "screen redraw" event */
		struct ExposeEvent {
			Uint8 type;
		};

		/** The "quit requested" event */
		struct QuitEvent {
			Uint8 type;
		};

		/** A user-defined event type */
		struct UserEvent {
			Uint8 type;		/** EE_USEREVENT through EE_NUMEVENTS-1 */
			int code;		/** User defined event code */
			void *data1;	/** User defined data pointer */
			void *data2;	/** User defined data pointer */
		};

		struct SysWMmsg;
		typedef struct SysWMmsg SysWMmsg;
		struct SysWMEvent {
			SysWMmsg * msg;
		};

		enum EventType {
			NoEvent = 0,
			Active,
			KeyDown,
			KeyUp,
			TextInput,
			MouseMotion,
			MouseButtonDown,
			MouseButtonUp,
			JoyAxisMotion,
			JoyBallMotion,
			JoyHatMotion,
			JoyButtonDown,
			JoyButtonUp,
			FingerMotion,
			FingerDown,
			FingerUp,
			Quit,
			SysWM,
			VideoResize,
			VideoExpose,
			EventUser,
			EventCount = 32
		};

		/** Event Type */
		Uint32 Type;

		/** General event structure */
		union {
			ActiveEvent			active;
			KeyboardEvent		key;
			TextInputEvent		text;
			MouseMotionEvent	motion;
			MouseButtonEvent	button;
			FingerEvent			finger;
			JoyAxisEvent		jaxis;
			JoyBallEvent		jball;
			JoyHatEvent			jhat;
			JoyButtonEvent		jbutton;
			ResizeEvent			resize;
			ExposeEvent			expose;
			QuitEvent			quit;
			UserEvent			user;
			SysWMEvent			syswm;
		};
};

}}

#endif
