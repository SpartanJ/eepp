#ifndef EE_GAMINGCUIGOTYPENEW_HPP
#define EE_GAMINGCUIGOTYPENEW_HPP

#include "base.hpp"
#include "../../ui/cuiwindow.hpp"

using namespace EE::UI;

namespace EE { namespace Gaming { namespace MapEditor {

class EE_API cUIGOTypeNew {
	public:
		cUIGOTypeNew( cb::Callback2<void, std::string, Uint32> Cb );

		virtual ~cUIGOTypeNew();
	protected:
		cUITheme *			mUITheme;
		cUIWindow *			mUIWindow;
		cUITextInput *		mUIInput;
		cb::Callback2<void, std::string, Uint32>	mCb;

		void WindowClose( const cUIEvent * Event );

		void CancelClick( const cUIEvent * Event );

		void OKClick( const cUIEvent * Event );
};

}}}

#endif
