#ifndef EE_WINDOWCJOYSTICKMANAGER_HPP
#define EE_WINDOWCJOYSTICKMANAGER_HPP

#include <eepp/window/base.hpp>
#include <eepp/window/joystick.hpp>

namespace EE { namespace Window {

/** @brief A Joystick Manager class */
class EE_API JoystickManager {
  public:
	typedef std::function<void()> OpenCb;

	JoystickManager();

	virtual ~JoystickManager();

	/** @return The number of joysticks attached to the system */
	Uint32 getCount();

	/** Update the states of all joysticks */
	virtual void update() = 0;

	/** @return The joystick instante of the joystick index */
	Joystick* getJoystick( const Uint32& index );

	/** Rescan all joysticks to look for new joystick connected.
	 * This could be slow on some backends, and unnecessary on others.
	 */
	virtual void rescan();

	/** Close all the joysticks */
	virtual void close();

	/** Open all the joysticks */
	virtual void open( OpenCb openCb = nullptr );

  protected:
	friend class Joystick;

	bool mInit;
	OpenCb mOpenCb;

	Joystick* mJoysticks[MAX_JOYSTICKS];

	Uint32 mCount;

	virtual void create( const Uint32& index ) = 0;
};

}} // namespace EE::Window

#endif
