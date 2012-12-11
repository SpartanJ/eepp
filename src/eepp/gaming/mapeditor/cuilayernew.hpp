#ifndef EE_GAMINGCUILAYERNEW_HPP
#define EE_GAMINGCUILAYERNEW_HPP

#include <eepp/gaming/base.hpp>
#include <eepp/ui/cuiwindow.hpp>
#include <eepp/gaming/mapeditor/cuimap.hpp>

using namespace EE::UI;

namespace EE { namespace Gaming { namespace MapEditor {

class EE_API cUILayerNew {
	public:
		typedef cb::Callback1<void, cUILayerNew*> NewLayerCb;

		cUILayerNew( cUIMap * Map, EE_LAYER_TYPE Type, NewLayerCb newLayerCb = NewLayerCb() );

		virtual ~cUILayerNew();

		const EE_LAYER_TYPE& Type() const;

		cUITextInput * UILayerName() const;

		const String& Name() const;

		cLayer * Layer() const;
	protected:
		cUITheme *			mTheme;
		cUIMap *			mUIMap;
		EE_LAYER_TYPE		mType;
		NewLayerCb			mNewLayerCb;
		cUIWindow *			mUIWindow;
		cUITextInput *		mUILayerName;
		cLayer *			mLayer;

		void WindowClose( const cUIEvent * Event );

		void CancelClick( const cUIEvent * Event );

		void OKClick( const cUIEvent * Event );

		void OnKeyUp( const cUIEvent * Event );
};

}}}

#endif
