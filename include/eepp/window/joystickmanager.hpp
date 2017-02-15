#ifndef EE_WINDOWCJOYSTICKMANAGER_HPP
#define EE_WINDOWCJOYSTICKMANAGER_HPP

#include <eepp/window/base.hpp>
#include <eepp/window/joystick.hpp>

namespace EE { namespace Window {

/** @brief A Joystick Manager class */
class EE_API JoystickManager {
	public:
		JoystickManager();

		virtual ~JoystickManager();

		/** @return The number of joysticks attached to the system */
		virtual Uint32 count();

		/** Update the states of all joysticks */
		virtual void 	update() = 0;

		/** @return The joystick instante of the joystick index */
		Joystick * 	getJoystick( const Uint32& index );

		/** Rescan all joysticks to look for new joystick connected.
		* This could be slow on some backends, and unnecessary on others.
		* Is slow in SDL. SFML and SDL2 shouldn't need this.
		*/
		virtual void	rescan();

		/** Close all the joysticks */
		virtual void 	close();

		/** Open all the joysticks */
		virtual void 	open();
	protected:
		friend class Joystick;
		
		bool			mInit;

		Joystick * 	mJoysticks[ MAX_JOYSTICKS ];

		Uint32			mCount;

		virtual void 	create( const Uint32& index ) = 0;
};

}}

#endif
