#ifndef EE_GAMINGCUIMAP_HPP
#define EE_GAMINGCUIMAP_HPP

#include "base.hpp"
#include "../../ui/cuicomplexcontrol.hpp"
#include "../cmap.hpp"

using namespace EE::UI;

namespace EE { namespace Gaming { namespace MapEditor {

class cUIMap : public cUIComplexControl {
	public:
		cUIMap( const cUIComplexControl::CreateParams& Params );

		virtual ~cUIMap();

		virtual void Draw();

		virtual void Update();

		cMap * Map() const;
	protected:
		cMap *		mMap;

		virtual void OnSizeChange();
};

}}}

#endif
