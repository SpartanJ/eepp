#ifndef EE_GAMINGCUIGOTYPENEW_HPP
#define EE_GAMINGCUIGOTYPENEW_HPP

#include <eepp/maps/base.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uiwindow.hpp>

using namespace EE::UI;

namespace EE { namespace Maps { namespace Private {

class EE_MAPS_API UIGOTypeNew {
  public:
	UIGOTypeNew( std::function<void( std::string, Uint32 )> Cb );

	virtual ~UIGOTypeNew();

  protected:
	UITheme* mUITheme;
	UIWindow* mUIWindow;
	UITextInput* mUIInput;
	std::function<void( std::string, Uint32 )> mCb;

	void onWindowClose( const Event* Event );

	void onCancelClick( const Event* Event );

	void onOKClick( const Event* Event );
};

}}} // namespace EE::Maps::Private

#endif
