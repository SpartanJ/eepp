#ifndef EE_WINDOWCJOYSTICKMANAGERNULL_HPP
#define EE_WINDOWCJOYSTICKMANAGERNULL_HPP

#include "../../cjoystickmanager.hpp"

namespace EE { namespace Window { namespace Backend { namespace Null {

class EE_API cJoystickManagerNull : public cJoystickManager {
	public:
		cJoystickManagerNull();
		
		~cJoystickManagerNull();
		
		void Update();
	protected:
		void Close();

		void Open();
		
		void Create( const Uint32& index );
};

}}}}

#endif
