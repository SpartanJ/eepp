#ifndef EE_WINDOWCINPUT_H
#define EE_WINDOWCINPUT_H

#include <eepp/graphics/view.hpp>
#include <eepp/window/base.hpp>
#include <eepp/window/inputevent.hpp>
#include <eepp/window/inputfinger.hpp>
#include <eepp/window/inputhelper.hpp>
#include <eepp/window/joystickmanager.hpp>
#include <eepp/window/window.hpp>

using namespace EE::Graphics;

namespace EE { namespace Window {

/** @brief The basic input class. For mouse and keyboard. */
class EE_API Input {
  public:
	typedef std::function<void( InputEvent* )> InputCallback;

	virtual ~Input();

	/** Update the Input */
	virtual void update() = 0;

	/** If timeout is zero waits indefinitely for the next available event otherwise waits until the
	 * specified timeout for the next available event.
	 */
	virtual void waitEvent( const Time& timeout = Time::Zero ) = 0;

	/** @return If the mouse and keyboard are grabed. */
	virtual bool grabInput() = 0;

	/** Grab or Ungrab the mouse and keyboard. */
	virtual void grabInput( const bool& Grab ) = 0;

	/** Inject the mouse position given */
	virtual void injectMousePos( const Uint16& x, const Uint16& y ) = 0;

	/** Gets the current mouse position relative to the focus window */
	virtual Vector2i queryMousePos() = 0;

	/** Capturing enables your app to obtain mouse events globally, instead of just within your
	 * window. Not all video targets support this function. When capturing is enabled, the current
	 * window will get all mouse events, but unlike relative mode, no change is made to the cursor
	 * and it is not restrained to your window. */
	virtual void captureMouse( const bool& capture ) = 0;

	/** @return If mouse is captured. */
	virtual bool isMouseCaptured() const = 0;

	/** @return Get the key name. */
	virtual std::string getKeyName( const Keycode& keycode ) const = 0;

	/** @return The corresponding keycode from a name. */
	virtual Keycode getKeyFromName( const std::string& keycode ) const = 0;

	/** @return Get the scancode name.  */
	virtual std::string getScancodeName( const Scancode& scancode ) const = 0;

	/** @return The corresponding scancode from a name. */
	virtual Scancode getScancodeFromName( const std::string& scancode ) const = 0;

	/** @return The key from the scancode. */
	virtual Keycode getKeyFromScancode( const Scancode& scancode ) const = 0;

	/** @return The scancode from a key. */
	virtual Scancode getScancodeFromKey( const Keycode& scancode ) const = 0;

	/** @return If keyboard key was released. */
	bool isKeyUp( const Keycode& Key );

	/** @return If keyboard key is pressed. */
	bool isKeyDown( const Keycode& Key );

	/** @return If scancode was released. */
	bool isScancodeUp( const Scancode& scancode );

	/** @return If scancode it's pressed. */
	bool isScancodeDown( const Scancode& scancode );

	/** Inject the key state of a key as key up or released. */
	void injectKeyUp( const Keycode& Key );

	/** Inject the scancode state of a scancode as pressed. */
	void injectScancodeDown( const Scancode& scancode );

	/** Inject the key state of a key as released. */
	void injectScancodeUp( const Scancode& scancode );

	/** Inject the mouse position given */
	void injectMousePos( const Vector2i& Pos );

	/** Inject the mouse button as pressed */
	void injectButtonPress( const Uint32& Button );

	/** Inject the mouse button as released */
	void injectButtonRelease( const Uint32& Button );

	/** @return If the Control Key is pressed */
	bool isControlPressed() const;

	/** @return If the Shift Key is pressed */
	bool isShiftPressed() const;

	/** @return If any Alt Key is pressed */
	bool isAltPressed() const;

	/** @return If the left Alt Key is pressed */
	bool isLeftAltPressed() const;

	/** @return If the left Alt Key is pressed */
	bool isAltGrPressed() const;

	/** @return If the Meta Key is pressed */
	bool isMetaPressed() const;

	/** @return If mouse left button it's pressed */
	bool isMouseLeftPressed() const;

	/** @return If mouse right button it's pressed */
	bool isMouseRightPressed() const;

	/** @return If mouse middle button it's pressed */
	bool isMouseMiddlePressed() const;

	/** @return If mouse left click was clicked */
	bool mouseLeftClicked() const;

	/** @return If mouse right click was clicked */
	bool mouseRightClicked() const;

	/** @return If mouse middle button (scroll button) was clicked. */
	bool mouseMiddleClicked() const;

	/** @return If mouse left click was double clicked */
	bool mouseLeftDoubleClicked() const;

	/** @return If mouse right click was double clicked */
	bool mouseRightDoubleClicked() const;

	/** @return If mouse middle button (scroll button) was double clicked. */
	bool mouseMiddleDoubleClicked() const;

	/** @return If mouse wheel up scrolled */
	bool mouseWheelScrolledUp() const;

	/** @return If mouse wheel down scrolled */
	bool mouseWheelScrolledDown() const;

	/** Push a new input callback.
	 * @return The Callback Id
	 */
	Uint32 pushCallback( const InputCallback& cb );

	/** Pop the callback id indicated. */
	void popCallback( const Uint32& CallbackId );

	/** @return The Mouse position vector */
	Vector2i getMousePos() const;

	/** @return The position vector converted to float */
	Vector2f getMousePosf();

	/** This will change the value of the mouse pos, will not REALLY move the mouse ( for that is
	 * InjectMousePos ). */
	void setMousePos( const Vector2i& Pos );

	/** @return The mouse position over the current view */
	Vector2f getMousePosFromView( const View& View );

	/** Set the mouse speed ( only affects grabed windows ) */
	void setMouseSpeed( const Float& Speed );

	/** @return The Mouse Speed */
	const Float& getMouseSpeed() const;

	/** @return The bitflags of the last pressed trigger (before the current state of press trigger)
	 */
	const Uint32& getLastPressTrigger() const;

	/** @return The current state as flags of the mouse press trigger
		@brief 	Triggers are used mostly for the UI components. They are simple to manage.
				The mouse flags are defined in keycodes.hpp
		For Example The usage is simple, to know if the left mouse click is pressed you need to
	   check against the left mouse flag mask if ( myInput->PressTrigger() & EE_BUTTON_LMASK ) ...
	*/
	const Uint32& getPressTrigger() const;

	/** @return The current state as flags of the mouse release trigger */
	const Uint32& getReleaseTrigger() const;

	/** @return The current state as flags of the mouse click trigger */
	const Uint32& getClickTrigger() const;

	/** @return The current state as flags of the mouse double click trigger */
	const Uint32& getDoubleClickTrigger() const;

	/** @return The double click interval in milliseconds ( default 500 ms ) */
	const Time& getDoubleClickInterval() const;

	/** Set the double click interval in milliseconds */
	void setDoubleClickInterval( const Time& Interval );

	/** Clean the keyboard and mouse states */
	void cleanStates();

	/** Send an input event to the window */
	void sendEvent( InputEvent* Event );

	/** @return The joystick manager */
	JoystickManager* getJoystickManager() const;

	/** @return The maximun number of fingers */
	Uint32 getFingerCount();

	/** @return The input finger from it's index */
	InputFinger* getFingerIndex( const Uint32& Index );

	/** @return The Input Finder from it's id */
	InputFinger* getFinger( const Int64& fingerId );

	/** @return A list of the input finders that are currently down */
	std::vector<InputFinger*> getFingersDown();

	/** @return A list of the input finders that were down in the last update */
	std::vector<InputFinger*> getFingersWasDown();

	/** @return the state of the mod keys. */
	const Uint32& getModState() const;

	/** Process an input event. Called by the input update. */
	void processEvent( InputEvent* Event );

  protected:
	friend class Window;

	Input( EE::Window::Window* window, JoystickManager* joystickmanager );

	virtual void init() = 0;

	EE::Window::Window* mWindow;
	JoystickManager* mJoystickManager;
	Uint8 mScancodeDown[SCANCODES_NUM];
	Uint8 mScancodeUp[SCANCODES_NUM];
	Uint32 mPressTrigger;
	Uint32 mReleaseTrigger;
	Uint32 mLastPressTrigger;
	Uint32 mClickTrigger;
	Uint32 mDoubleClickTrigger;
	Uint32 mInputMod;
	Time mDoubleClickInterval; /// Determine the double click inverval in milliseconds ( default 400
							   /// ms )
	Uint32 mLastButtonLeftClicked;
	Uint32 mLastButtonRightClicked;
	Uint32 mLastButtonMiddleClicked;
	Uint32 mLastButtonLeftClick;
	Uint32 mLastButtonRightClick;
	Uint32 mLastButtonMiddleClick;
	Uint32 mTClick;
	Vector2i mMousePos;
	Uint32 mNumCallBacks;
	Float mMouseSpeed;
	bool mInputGrabed;
	InputFinger mFingers[EE_MAX_FINGERS];

	std::map<Uint32, InputCallback> mCallbacks;

	InputFinger* getFingerId( const Int64& fingerId );

	void resetFingerWasDown();
};

}} // namespace EE::Window

#endif
