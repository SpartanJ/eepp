#ifndef EE_WINDOWCINPUTNULL_HPP 
#define EE_WINDOWCINPUTNULL_HPP

#include <eepp/window/input.hpp>

namespace EE { namespace Window { namespace Backend { namespace Null {

class EE_API InputNull : public Input {
	public:
		virtual ~InputNull();
		
		void update();

		bool grabInput();

		void grabInput( const bool& Grab );

		void injectMousePos( const Uint16& x, const Uint16& y );
	protected:
		friend class WindowNull;

		InputNull( EE::Window::Window * window );
		
		virtual void init();
};

}}}}

#endif
