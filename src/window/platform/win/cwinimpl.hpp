#ifndef EE_WINDOWCWINIMPL_HPP
#define EE_WINDOWCWINIMPL_HPP

#include "../../base.hpp"
#include "../../cplatformimpl.hpp"

#if EE_PLATFORM == EE_PLATFORM_WIN

namespace EE { namespace Window { namespace Platform {

using namespace EE::Window;

class cWinImpl : public cPlatformImpl {
	public:
		cWinImpl( cWindow * window, eeWindowHandler handler );

		~cWinImpl();

		void MinimizeWindow();

		void MaximizeWindow();

		void HideWindow();

		void RaiseWindow();

		void ShowWindow();

		void MoveWindow( int left, int top );

		void SetContext( eeWindowContex Context );

		eeVector2i Position();
	protected:
		eeWindowHandler mHandler;
};

}}}

#endif

#endif
