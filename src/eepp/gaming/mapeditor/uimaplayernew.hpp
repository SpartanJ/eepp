#ifndef EE_GAMINGCUILAYERNEW_HPP
#define EE_GAMINGCUILAYERNEW_HPP

#include <eepp/gaming/base.hpp>
#include <eepp/ui/cuiwindow.hpp>
#include <eepp/gaming/mapeditor/uimap.hpp>

using namespace EE::UI;

namespace EE { namespace Gaming { namespace Private {

class EE_API UIMapLayerNew {
	public:
		typedef cb::Callback1<void, UIMapLayerNew*> NewLayerCb;

		UIMapLayerNew( UIMap * Map, EE_LAYER_TYPE Type, NewLayerCb newLayerCb = NewLayerCb() );

		virtual ~UIMapLayerNew();

		const EE_LAYER_TYPE& Type() const;

		cUITextInput * UILayerName() const;

		const String& Name() const;

		MapLayer * Layer() const;
	protected:
		cUITheme *			mTheme;
		UIMap *			mUIMap;
		EE_LAYER_TYPE		mType;
		NewLayerCb			mNewLayerCb;
		cUIWindow *			mUIWindow;
		cUITextInput *		mUILayerName;
		MapLayer *			mLayer;

		void WindowClose( const cUIEvent * Event );

		void CancelClick( const cUIEvent * Event );

		void OKClick( const cUIEvent * Event );

		void OnKeyUp( const cUIEvent * Event );
};

}}}

#endif
