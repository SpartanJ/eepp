#ifndef EE_WINDOWCOSXIMPL_HPP
#define EE_WINDOWCOSXIMPL_HPP

#include "../../base.hpp"
#include "../../cplatformimpl.hpp"

#if EE_PLATFORM == EE_PLATFORM_MACOSX

namespace EE { namespace Window { namespace Platform {

using namespace EE::Window;

class cOSXImpl : public cPlatformImpl {
	public:
		cOSXImpl( cWindow * window );

		~cOSXImpl();

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

#endif
