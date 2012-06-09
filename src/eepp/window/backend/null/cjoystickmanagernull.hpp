#ifndef EE_WINDOWCJOYSTICKMANAGERNULL_HPP
#define EE_WINDOWCJOYSTICKMANAGERNULL_HPP

#include <eepp/window/cjoystickmanager.hpp>

namespace EE { namespace Window { namespace Backend { namespace Null {

class EE_API cJoystickManagerNull : public cJoystickManager {
	public:
		cJoystickManagerNull();
		
		virtual ~cJoystickManagerNull();
		
		void Update();

		void Close();

		void Open();
	protected:
		void Create( const Uint32& index );
};

}}}}

#endif
