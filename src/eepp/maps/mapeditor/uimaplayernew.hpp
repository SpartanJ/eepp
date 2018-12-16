#ifndef EE_GAMINGCUILAYERNEW_HPP
#define EE_GAMINGCUILAYERNEW_HPP

#include <eepp/maps/base.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/maps/mapeditor/uimap.hpp>
#include <eepp/ui/uitextinput.hpp>

using namespace EE::UI;

namespace EE { namespace Maps { namespace Private {

class EE_API UIMapLayerNew {
	public:
		typedef std::function<void( UIMapLayerNew* )> NewLayerCb;

		UIMapLayerNew( UIMap * Map, EE_LAYER_TYPE Type, NewLayerCb newLayerCb = NewLayerCb() );

		virtual ~UIMapLayerNew();

		const EE_LAYER_TYPE& getType() const;

		UITextInput * getUILayerName() const;

		const String& getName() const;

		MapLayer * getLayer() const;
	protected:
		UITheme *			mTheme;
		UIMap *			mUIMap;
		EE_LAYER_TYPE		mType;
		NewLayerCb			mNewLayerCb;
		UIWindow *			mUIWindow;
		UITextInput *		mUILayerName;
		MapLayer *			mLayer;

		void onWindowClose( const Event * Event );

		void onCancelClick( const Event * Event );

		void onOKClick( const Event * Event );

		void onOnKeyUp( const Event * Event );
};

}}}

#endif
