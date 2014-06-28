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

		const EE_LAYER_TYPE& Type() const;

		UITextInput * UILayerName() const;

		const String& Name() const;

		MapLayer * Layer() const;
	protected:
		UITheme *			mTheme;
		UIMap *			mUIMap;
		EE_LAYER_TYPE		mType;
		NewLayerCb			mNewLayerCb;
		UIWindow *			mUIWindow;
		UITextInput *		mUILayerName;
		MapLayer *			mLayer;

		void WindowClose( const UIEvent * Event );

		void CancelClick( const UIEvent * Event );

		void OKClick( const UIEvent * Event );

		void OnKeyUp( const UIEvent * Event );
};

}}}

#endif
