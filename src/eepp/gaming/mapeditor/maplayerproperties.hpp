#ifndef EE_GAMINGCLAYERPROPERTIES_HPP
#define EE_GAMINGCLAYERPROPERTIES_HPP

#include <eepp/gaming/base.hpp>
#include <eepp/gaming/maphelper.hpp>
#include <eepp/gaming/maplayer.hpp>
#include <eepp/ui/cuiwindow.hpp>
#include <eepp/ui/cuigenericgrid.hpp>
#include <eepp/ui/cuitextinput.hpp>

using namespace EE::UI;

namespace EE { namespace Gaming { namespace Private {

class MapEditor;

class EE_API MapLayerProperties {
	public:
		typedef cb::Callback0<void> RefreshLayerListCb;

		MapLayerProperties( MapLayer * Map, RefreshLayerListCb Cb = RefreshLayerListCb() );

		virtual ~MapLayerProperties();

	protected:
		cUITheme *			mUITheme;
		cUIWindow *			mUIWindow;
		cUIGenericGrid *	mGenGrid;
		MapLayer *			mLayer;
		cUITextInput *		mUIInput;
		RefreshLayerListCb	mRefreshCb;

		void WindowClose( const cUIEvent * Event );

		void CancelClick( const cUIEvent * Event );

		void OKClick( const cUIEvent * Event );

		void AddCellClick( const cUIEvent * Event );

		void RemoveCellClick( const cUIEvent * Event );

		void CreateGridElems();

		void SaveProperties();

		void LoadProperties();

		cUIGridCell * CreateCell();
};

}}}

#endif
