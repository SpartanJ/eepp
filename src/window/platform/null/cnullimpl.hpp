#ifndef EE_WINDOWcNullImpl_HPP
#define EE_WINDOWcNullImpl_HPP

#include "../../base.hpp"
#include "../../cplatformimpl.hpp"

namespace EE { namespace Window { namespace Platform {

using namespace EE::Window;

class cNullImpl : public cPlatformImpl {
	public:
		cNullImpl( cWindow * window );

		~cNullImpl();

		void MinimizeWindow();

		void MaximizeWindow();

		void HideWindow();

		void RaiseWindow();

		void ShowWindow();

		void MoveWindow( int left, int top );

		void SetContext( eeWindowContex Context );

		eeVector2i Position();
	protected:
};

}}}

#endif
