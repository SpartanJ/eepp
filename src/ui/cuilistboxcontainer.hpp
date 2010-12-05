#ifndef EE_UICUILISTBOXCONTAINER_HPP
#define EE_UICUILISTBOXCONTAINER_HPP

#include "cuicontrol.hpp"

namespace EE { namespace UI {

class cUIListBoxContainer : public cUIControl {
	public:
		cUIListBoxContainer( cUIControl::CreateParams& Params );

		~cUIListBoxContainer();

		void Update();

		void DrawChilds();
	protected:
		cUIControl * OverFind( const eeVector2f& Point );
};

}}

#endif
