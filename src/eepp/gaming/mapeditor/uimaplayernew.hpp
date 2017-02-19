#ifndef EE_GAMINGCUILAYERNEW_HPP
#define EE_GAMINGCUILAYERNEW_HPP

#include <eepp/gaming/base.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/gaming/mapeditor/uimap.hpp>

using namespace EE::UI;

namespace EE { namespace Gaming { namespace Private {

class EE_API UIMapLayerNew {
	public:
		typedef cb::Callback1<void, UIMapLayerNew*> NewLayerCb;

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

		void onWindowClose( const UIEvent * Event );

		void onCancelClick( const UIEvent * Event );

		void onOKClick( const UIEvent * Event );

		void onOnKeyUp( const UIEvent * Event );
};

}}}

#endif
