#ifndef EE_GAMINGCMAPPROPERTIES_HPP
#define EE_GAMINGCMAPPROPERTIES_HPP

#include "base.hpp"
#include "../cmap.hpp"
#include "../../ui/cuiwindow.hpp"
#include "../../ui/cuigenericgrid.hpp"
#include "../../ui/cuitextinput.hpp"

using namespace EE::UI;

namespace EE { namespace Gaming { namespace MapEditor {

class cMapProperties {
	public:
		cMapProperties( cMap * Map );

		virtual ~cMapProperties();

	protected:
		cUITheme *			mUITheme;
		cUIWindow *			mUIWindow;
		cUIGenericGrid *	mGenGrid;
		cMap *				mMap;

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
