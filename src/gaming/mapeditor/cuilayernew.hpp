#ifndef EE_GAMINGCUILAYERNEW_HPP
#define EE_GAMINGCUILAYERNEW_HPP

#include "base.hpp"
#include "../../ui/cuiwindow.hpp"
#include "cuimap.hpp"

using namespace EE::UI;

namespace EE { namespace Gaming { namespace MapEditor {

class cUILayerNew {
	public:
		cUILayerNew( cUIMap * Map, EE_LAYER_TYPE Type, cb::Callback1<void, cUILayerNew*> NewLayerCb = cb::Callback1<void, cUILayerNew*>() );

		virtual ~cUILayerNew();

		const EE_LAYER_TYPE& Type() const;

		cUITextInput * UILayerName() const;

		const String& Name() const;

		cLayer * Layer() const;
	protected:
		cUITheme *			mTheme;
		cUIMap *			mUIMap;
		EE_LAYER_TYPE		mType;
		cb::Callback1<void, cUILayerNew*>	mNewLayerCb;
		cUIWindow *			mUIWindow;
		cUITextInput *		mUILayerName;
		cLayer *			mLayer;

		void WindowClose( const cUIEvent * Event );

		void CancelClick( const cUIEvent * Event );

		void OKClick( const cUIEvent * Event );
};

}}}

#endif
