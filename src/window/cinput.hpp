#ifndef EE_WINDOWCINPUT_H
#define EE_WINDOWCINPUT_H

#include "base.hpp"

namespace EE { namespace Window {

class cEngine;
class cView;

#define EE_BUTTON(X)		( 1 << ( ( X ) - 1 ) )
#define EE_BUTTON_LEFT		(1)
#define EE_BUTTON_MIDDLE	(2)
#define EE_BUTTON_RIGHT		(3)
#define EE_BUTTON_WHEELUP	(4)
#define EE_BUTTON_WHEELDOWN	(5)
#define EE_BUTTON_X1		(6)
#define EE_BUTTON_X2		(7)
#define EE_BUTTON_LMASK		(EE_BUTTON(EE_BUTTON_LEFT))
#define EE_BUTTON_MMASK		(EE_BUTTON(EE_BUTTON_MIDDLE))
#define EE_BUTTON_RMASK		(EE_BUTTON(EE_BUTTON_RIGHT))
#define EE_BUTTON_X1MASK	(EE_BUTTON(EE_BUTTON_X1))
#define EE_BUTTON_X2MASK	(EE_BUTTON(EE_BUTTON_X2))
#define EE_BUTTON_WUMASK	(EE_BUTTON(EE_BUTTON_WHEELUP))
#define EE_BUTTON_WDMASK	(EE_BUTTON(EE_BUTTON_WHEELDOWN))

#define EE_BUTTONS_LRM		( EE_BUTTON(EE_BUTTON_LEFT) | EE_BUTTON(EE_BUTTON_MIDDLE) | EE_BUTTON(EE_BUTTON_RIGHT) )
#define EE_BUTTONS_WUWD		( EE_BUTTON(EE_BUTTON_WHEELUP) | EE_BUTTON(EE_BUTTON_WHEELDOWN) )
#define EE_BUTTONS_ALL 		(0xFFffFFff)

#define EE_KEYS_SPACE 		(336/8)

/** EE_Event is just an SDL_Event */
typedef SDL_Event EE_Event;

int EE_API convertKeyCharacter(EE_Event* event);

/** @enum EE_KEY Enum of keyboard keys. */
enum EE_KEY {
	KEY_UNKNOWN= SDLK_UNKNOWN,
	KEY_FIRST= SDLK_FIRST,
	KEY_BACKSPACE= SDLK_BACKSPACE,
	KEY_TAB= SDLK_TAB,
	KEY_CLEAR= SDLK_CLEAR,
	KEY_RETURN= SDLK_RETURN,
	KEY_PAUSE= SDLK_PAUSE,
	KEY_ESCAPE= SDLK_ESCAPE,
	KEY_SPACE= SDLK_SPACE,
	KEY_EXCLAIM= SDLK_EXCLAIM,
	KEY_QUOTEDBL= SDLK_QUOTEDBL,
	KEY_HASH= SDLK_HASH,
	KEY_DOLLAR= SDLK_DOLLAR,
	KEY_AMPERSAND= SDLK_AMPERSAND,
	KEY_QUOTE= SDLK_QUOTE,
	KEY_LEFTPAREN= SDLK_LEFTPAREN,
	KEY_RIGHTPAREN= SDLK_RIGHTPAREN,
	KEY_ASTERISK= SDLK_ASTERISK,
	KEY_PLUS= SDLK_PLUS,
	KEY_COMMA= SDLK_COMMA,
	KEY_MINUS= SDLK_MINUS,
	KEY_PERIOD= SDLK_PERIOD,
	KEY_SLASH= SDLK_SLASH,
	KEY_0= SDLK_0,
	KEY_1= SDLK_1,
	KEY_2= SDLK_2,
	KEY_3= SDLK_3,
	KEY_4= SDLK_4,
	KEY_5= SDLK_5,
	KEY_6= SDLK_6,
	KEY_7= SDLK_7,
	KEY_8= SDLK_8,
	KEY_9= SDLK_9,
	KEY_COLON= SDLK_COLON,
	KEY_SEMICOLON= SDLK_SEMICOLON,
	KEY_LESS= SDLK_LESS,
	KEY_EQUALS= SDLK_EQUALS,
	KEY_GREATER= SDLK_GREATER,
	KEY_QUESTION= SDLK_QUESTION,
	KEY_AT= SDLK_AT,


	/*
	Skip uppercase letters
	*/
	KEY_LEFTBRACKET= SDLK_LEFTBRACKET,
	KEY_BACKSLASH= SDLK_BACKSLASH,
	KEY_RIGHTBRACKET= SDLK_RIGHTBRACKET,
	KEY_CARET= SDLK_CARET,
	KEY_UNDERSCORE= SDLK_UNDERSCORE,
	KEY_BACKQUOTE= SDLK_BACKQUOTE, ///< GRAVE
	KEY_A= SDLK_a,
	KEY_B= SDLK_b,
	KEY_C= SDLK_c,
	KEY_D= SDLK_d,
	KEY_E= SDLK_e,
	KEY_F= SDLK_f,
	KEY_G= SDLK_g,
	KEY_H= SDLK_h,
	KEY_I= SDLK_i,
	KEY_J= SDLK_j,
	KEY_K= SDLK_k,
	KEY_L= SDLK_l,
	KEY_M= SDLK_m,
	KEY_N= SDLK_n,
	KEY_O= SDLK_o,
	KEY_P= SDLK_p,
	KEY_Q= SDLK_q,
	KEY_R= SDLK_r,
	KEY_S= SDLK_s,
	KEY_T= SDLK_t,
	KEY_U= SDLK_u,
	KEY_V= SDLK_v,
	KEY_W= SDLK_w,
	KEY_X= SDLK_x,
	KEY_Y= SDLK_y,
	KEY_Z= SDLK_z,
	KEY_DELETE= SDLK_DELETE,
	/* End of ASCII mapped keysyms */

	/* Numeric keypad */
	KEY_KP0= SDLK_KP0,
	KEY_KP1= SDLK_KP1,
	KEY_KP2= SDLK_KP2,
	KEY_KP3= SDLK_KP3,
	KEY_KP4= SDLK_KP4,
	KEY_KP5= SDLK_KP5,
	KEY_KP6= SDLK_KP6,
	KEY_KP7= SDLK_KP7,
	KEY_KP8= SDLK_KP8,
	KEY_KP9= SDLK_KP9,
	KEY_KP_PEROID= SDLK_KP_PERIOD,
	KEY_KP_DIVIDE= SDLK_KP_DIVIDE,
	KEY_KP_MULTIPLY= SDLK_KP_MULTIPLY,
	KEY_KP_MINUS= SDLK_KP_MINUS,
	KEY_KP_PLUS= SDLK_KP_PLUS,
	KEY_KP_ENTER= SDLK_KP_ENTER,
	KEY_KP_EQUALS= SDLK_KP_EQUALS,

	/* Arrows + Home/End pad */
	KEY_UP= SDLK_UP,
	KEY_DOWN= SDLK_DOWN,
	KEY_RIGHT= SDLK_RIGHT,
	KEY_LEFT= SDLK_LEFT,
	KEY_INSERT= SDLK_INSERT,
	KEY_HOME= SDLK_HOME,
	KEY_END= SDLK_END,
	KEY_PAGEUP= SDLK_PAGEUP,
	KEY_PAGEDOWN= SDLK_PAGEDOWN,

	/* Function keys */
	KEY_F1= SDLK_F1,
	KEY_F2= SDLK_F2,
	KEY_F3= SDLK_F3,
	KEY_F4= SDLK_F4,
	KEY_F5= SDLK_F5,
	KEY_F6= SDLK_F6,
	KEY_F7= SDLK_F7,
	KEY_F8= SDLK_F8,
	KEY_F9= SDLK_F9,
	KEY_F10= SDLK_F10,
	KEY_F11= SDLK_F11,
	KEY_F12= SDLK_F12,
	KEY_F13= SDLK_F13,
	KEY_F14= SDLK_F14,
	KEY_F15= SDLK_F15,

	/* Key state modifier keys */
	KEY_NUMLOCK= SDLK_NUMLOCK,
	KEY_CAPSLOCK= SDLK_CAPSLOCK,
	KEY_SCROLLOCK= SDLK_SCROLLOCK,
	KEY_RSHIFT= SDLK_RSHIFT,
	KEY_LSHIFT= SDLK_LSHIFT,
	KEY_RCTRL= SDLK_RCTRL,
	KEY_LCTRL= SDLK_LCTRL,
	KEY_RALT= SDLK_RALT,
	KEY_LALT= SDLK_LALT,
	KEY_RMETA= SDLK_RMETA,
	KEY_LMETA= SDLK_LMETA,
	KEY_LSUPER= SDLK_LSUPER, ///< Left "Windows" key
	KEY_RSUPER= SDLK_RSUPER, ///< Right "Windows" key

	/* Miscellaneous function keys */
	KEY_HELP= SDLK_HELP,
	KEY_PRINT= SDLK_PRINT,
	KEY_SYSREQ= SDLK_SYSREQ,
	KEY_BREAK= SDLK_BREAK,
	KEY_MENU= SDLK_MENU,
	KEY_MODE= SDLK_MODE,		///< "Alt Gr" key
	KEY_COMPOSE= SDLK_COMPOSE,	///< Multi-key compose key
	KEY_POWER= SDLK_POWER,		///< Power Macintosh power key
	KEY_EURO= SDLK_EURO,		///< Some european keyboards
	KEY_UNDO= SDLK_UNDO,		///< Atari keyboard has Undo

	/* International keyboard syms */
	KEY_WORLD_0=	SDLK_WORLD_0,
	KEY_WORLD_1=	SDLK_WORLD_1,
	KEY_WORLD_2=	SDLK_WORLD_2,
	KEY_WORLD_3=	SDLK_WORLD_3,
	KEY_WORLD_4=	SDLK_WORLD_4,
	KEY_WORLD_5=	SDLK_WORLD_5,
	KEY_WORLD_6=	SDLK_WORLD_6,
	KEY_WORLD_7=	SDLK_WORLD_7,
	KEY_WORLD_8=	SDLK_WORLD_8,
	KEY_WORLD_9=	SDLK_WORLD_9,
	KEY_WORLD_10=	SDLK_WORLD_10,
	KEY_WORLD_11=	SDLK_WORLD_11,
	KEY_WORLD_12=	SDLK_WORLD_12,
	KEY_WORLD_13=	SDLK_WORLD_13,
	KEY_WORLD_14=	SDLK_WORLD_14,
	KEY_WORLD_15=	SDLK_WORLD_15,
	KEY_WORLD_16=	SDLK_WORLD_16,
	KEY_WORLD_17=	SDLK_WORLD_17,
	KEY_WORLD_18=	SDLK_WORLD_18,
	KEY_WORLD_19=	SDLK_WORLD_19,
	KEY_WORLD_20=	SDLK_WORLD_20,
	KEY_WORLD_21=	SDLK_WORLD_21,
	KEY_WORLD_22=	SDLK_WORLD_22,
	KEY_WORLD_23=	SDLK_WORLD_23,
	KEY_WORLD_24=	SDLK_WORLD_24,
	KEY_WORLD_25=	SDLK_WORLD_25,
	KEY_WORLD_26=	SDLK_WORLD_26,
	KEY_WORLD_27=	SDLK_WORLD_27,
	KEY_WORLD_28=	SDLK_WORLD_28,
	KEY_WORLD_29=	SDLK_WORLD_29,
	KEY_WORLD_30=	SDLK_WORLD_30,
	KEY_WORLD_31=	SDLK_WORLD_31,
	KEY_WORLD_32=	SDLK_WORLD_32,
	KEY_WORLD_33=	SDLK_WORLD_33,
	KEY_WORLD_34=	SDLK_WORLD_34,
	KEY_WORLD_35=	SDLK_WORLD_35,
	KEY_WORLD_36=	SDLK_WORLD_36,
	KEY_WORLD_37=	SDLK_WORLD_37,
	KEY_WORLD_38=	SDLK_WORLD_38,
	KEY_WORLD_39=	SDLK_WORLD_39,
	KEY_WORLD_40=	SDLK_WORLD_40,
	KEY_WORLD_41=	SDLK_WORLD_41,
	KEY_WORLD_42=	SDLK_WORLD_42,
	KEY_WORLD_43=	SDLK_WORLD_43,
	KEY_WORLD_44=	SDLK_WORLD_44,
	KEY_WORLD_45=	SDLK_WORLD_45,
	KEY_WORLD_46=	SDLK_WORLD_46,
	KEY_WORLD_47=	SDLK_WORLD_47,
	KEY_WORLD_48=	SDLK_WORLD_48,
	KEY_WORLD_49=	SDLK_WORLD_49,
	KEY_WORLD_50=	SDLK_WORLD_50,
	KEY_WORLD_51=	SDLK_WORLD_51,
	KEY_WORLD_52=	SDLK_WORLD_52,
	KEY_WORLD_53=	SDLK_WORLD_53,
	KEY_WORLD_54=	SDLK_WORLD_54,
	KEY_WORLD_55=	SDLK_WORLD_55,
	KEY_WORLD_56=	SDLK_WORLD_56,
	KEY_WORLD_57=	SDLK_WORLD_57,
	KEY_WORLD_58=	SDLK_WORLD_58,
	KEY_WORLD_59=	SDLK_WORLD_59,
	KEY_WORLD_60=	SDLK_WORLD_60,
	KEY_WORLD_61=	SDLK_WORLD_61,
	KEY_WORLD_62=	SDLK_WORLD_62,
	KEY_WORLD_63=	SDLK_WORLD_63,
	KEY_WORLD_64=	SDLK_WORLD_64,
	KEY_WORLD_65=	SDLK_WORLD_65,
	KEY_WORLD_66=	SDLK_WORLD_66,
	KEY_WORLD_67=	SDLK_WORLD_67,
	KEY_WORLD_68=	SDLK_WORLD_68,
	KEY_WORLD_69=	SDLK_WORLD_69,
	KEY_WORLD_70=	SDLK_WORLD_70,
	KEY_WORLD_71=	SDLK_WORLD_71,
	KEY_WORLD_72=	SDLK_WORLD_72,
	KEY_WORLD_73=	SDLK_WORLD_73,
	KEY_WORLD_74=	SDLK_WORLD_74,
	KEY_WORLD_75=	SDLK_WORLD_75,
	KEY_WORLD_76=	SDLK_WORLD_76,
	KEY_WORLD_77=	SDLK_WORLD_77,
	KEY_WORLD_78=	SDLK_WORLD_78,
	KEY_WORLD_79=	SDLK_WORLD_79,
	KEY_WORLD_80=	SDLK_WORLD_80,
	KEY_WORLD_81=	SDLK_WORLD_81,
	KEY_WORLD_82=	SDLK_WORLD_82,
	KEY_WORLD_83=	SDLK_WORLD_83,
	KEY_WORLD_84=	SDLK_WORLD_84,
	KEY_WORLD_85=	SDLK_WORLD_85,
	KEY_WORLD_86=	SDLK_WORLD_86,
	KEY_WORLD_87=	SDLK_WORLD_87,
	KEY_WORLD_88=	SDLK_WORLD_88,
	KEY_WORLD_89=	SDLK_WORLD_89,
	KEY_WORLD_90=	SDLK_WORLD_90,
	KEY_WORLD_91=	SDLK_WORLD_91,
	KEY_WORLD_92=	SDLK_WORLD_92,
	KEY_WORLD_93=	SDLK_WORLD_93,
	KEY_WORLD_94=	SDLK_WORLD_94,
	KEY_WORLD_95=	SDLK_WORLD_95
};

/** @brief The basic input class. For mouse and keyboard. */
class EE_API cInput : public tSingleton<cInput> {
	friend class tSingleton<cInput>;
	public:
		typedef cb::Callback1<void, EE_Event*> InputCallback;
		typedef cb::Callback0<void> VideoResizeCallback;

		/** Update the Input */
		void Update();

		/** @return If keyboard key was released */
		bool IsKeyUp(const EE_KEY& Key);

		/** @return If keyboard key it's pressed */
		bool IsKeyDown(const EE_KEY& Key);

		/** Inject the key state of a key as KEY UP or RELEASE */
		void InjectKeyUp(const EE_KEY& Key);

		/** Inject the key state of a key as KEY DOWN or PRESSED */
		void InjectKeyDown(const EE_KEY& Key);

		/** Inject the mouse position given */
		void InjectMousePos( const Uint16& x, const Uint16& y );

		/** Inject the mouse position given */
		void InjectMousePos( const eeVector2i& Pos );

		/** Inject the mouse button as pressed */
		void InjectButtonPress( const Uint32& Button );

		/** Inject the mouse button as released */
		void InjectButtonRelease( const Uint32& Button );

		/** @return If the Control Key is pressed */
		bool ControlPressed() const;

		/** @return If the Shift Key is pressed */
		bool ShiftPressed() const;

		/** @return If the Alt Key is pressed */
		bool AltPressed() const;

		/** @return If the Meta Key is pressed */
		bool MetaPressed() const;

		/** @return If mouse left button it's pressed */
		bool MouseLeftPressed() const;

		/** @return If mouse right button it's pressed */
		bool MouseRightPressed() const;

		/** @return If mouse middle button it's pressed */
		bool MouseMiddlePressed() const;

		/** @return If mouse left click was clicked */
		bool MouseLeftClick() const;

		/** @return If mouse right click was clicked */
		bool MouseRightClick() const;

		/** @return If mouse middle button (scroll button) was clicked. */
		bool MouseMiddleClick() const;

		/** @return If mouse left click was double clicked */
		bool MouseLeftDoubleClick() const;

		/** @return If mouse right click was double clicked */
		bool MouseRightDoubleClick() const;

		/** @return If mouse middle button (scroll button) was double clicked. */
		bool MouseMiddleDoubleClick() const;

		/** @return If mouse wheel up scrolled */
		bool MouseWheelUp() const;

		/** @return If mouse wheel down scrolled */
		bool MouseWheelDown() const;

		/** Push a new input callback.
		* @return The Callback Id
		*/
		Uint32 PushCallback( const InputCallback& cb );

		/** Pop the callback id indicated. */
		void PopCallback( const Uint32& CallbackId );

		/** @return If the mouse and keyboard are grabed. */
		bool GrabInput();

		/** Grab or Ungrab the mouse and keyboard. */
		void GrabInput( const bool& Grab );

		/** @return The Mouse position vector */
		eeVector2i GetMousePos() const;

		/** @return The position vector converted to float */
		eeVector2f GetMousePosf();

		/** @return The mouse position over the current view */
		eeVector2i GetMousePosFromView( const cView& View );

		/** @return The Mouse X axis position */
		Uint16 MouseX() const;

		/** @return The Mouse Y axis position */
		Uint16 MouseY() const;

		/** Sets a callback for the video resize ( usefull on windows for reloading textures loaded from memory ) */
		void SetVideoResizeCallback( const VideoResizeCallback& vrc );

		/** Set the mouse speed ( only affects grabed windows ) */
		void MouseSpeed( const eeFloat& Speed );

		/** @return The Mouse Speed */
		const eeFloat& MouseSpeed() const;

		/** @return The bitflags of the last pressed trigger (before the current state of press trigger) */
		const Uint32& LastPressTrigger() const;

		/** @return The current state as flags of the mouse press trigger */
		const Uint32& PressTrigger() const;

		/** @return The current state as flags of the mouse release trigger */
		const Uint32& ReleaseTrigger() const;

		/** @return The current state as flags of the mouse click trigger */
		const Uint32& ClickTrigger() const;

		/** @return The current state as flags of the mouse double click trigger */
		const Uint32& DoubleClickTrigger() const;

		/** @return The double click interval in milliseconds ( default 500 ms ) */
		const Uint32& DoubleClickInterval() const;

		/** Set the double click interval in milliseconds */
		void DoubleClickInterval( const Uint32& Interval );

		~cInput();

		void CallVideoResize();
	protected:
		cInput();

		cEngine* EE;
		EE_Event mEvent;

		Uint8 mKeysDown	[EE_KEYS_SPACE];
		Uint8 mKeysUp	[EE_KEYS_SPACE];

		std::map<Uint32, InputCallback> mCallbacks;

		Uint32 mPressTrigger;
		Uint32 mReleaseTrigger;
		Uint32 mLastPressTrigger;
		Uint32 mClickTrigger;
		Uint32 mDoubleClickTrigger;
		Uint32 mInputMod;
		Uint32 mDoubleClickInterval; //!< Determine the double click inverval in milliseconds ( default 500 ms )
		Uint32 mLastButtonLeftClicked, mLastButtonRightClicked, mLastButtonMiddleClicked;
		Uint32 mLastButtonLeftClick, mLastButtonRightClick, mLastButtonMiddleClick;

		Uint32 mTClick;
		eeVector2i mMousePos;

		VideoResizeCallback mVRCall;
		Uint32 mNumCallBacks;

		bool mInputGrabed;
		eeFloat mMouseSpeed;

		bool GetKey( Uint8 * Key, Uint8 Pos );
		void PushKey( Uint8 * Key, Uint8 Pos, bool BitWrite );
};

}}

#endif
