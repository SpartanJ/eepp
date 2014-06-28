#ifndef EE_UICUISEPARATOR
#define EE_UICUISEPARATOR

#include <eepp/ui/uicontrolanim.hpp>

namespace EE { namespace UI {

class EE_API UISeparator : public UIControlAnim {
	public:
		UISeparator( UIControlAnim::CreateParams Params );
		
		virtual ~UISeparator();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void SetTheme( UITheme * Theme );
	protected:
};

}}

#endif

