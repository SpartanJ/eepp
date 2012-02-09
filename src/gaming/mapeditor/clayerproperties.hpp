#ifndef EE_GAMINGCLAYERPROPERTIES_HPP
#define EE_GAMINGCLAYERPROPERTIES_HPP

#include "base.hpp"
#include "../maphelper.hpp"
#include "../clayer.hpp"
#include "../../ui/cuiwindow.hpp"
#include "../../ui/cuigenericgrid.hpp"
#include "../../ui/cuitextinput.hpp"

using namespace EE::UI;

namespace EE { namespace Gaming { namespace MapEditor {

class EE_API cLayerProperties {
	public:
		cLayerProperties( cLayer * Map );

		virtual ~cLayerProperties();

	protected:
		cUITheme *			mUITheme;
		cUIWindow *			mUIWindow;
		cUIGenericGrid *	mGenGrid;
		cLayer *			mLayer;

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
