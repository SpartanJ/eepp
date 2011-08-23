#ifndef EE_UICUISEPARATOR
#define EE_UICUISEPARATOR

#include "cuicontrolanim.hpp"

namespace EE { namespace UI {

class cUISeparator : public cUIControlAnim {
	public:
		cUISeparator( cUIControlAnim::CreateParams Params );
		
		~cUISeparator();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void SetTheme( cUITheme * Theme );
	protected:
};

}}

#endif

