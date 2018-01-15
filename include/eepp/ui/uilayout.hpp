#ifndef EE_GRAPHICS_UILAYOUT_HPP
#define EE_GRAPHICS_UILAYOUT_HPP

#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class EE_API UILayout : public UIWidget {
	public:
		UILayout();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;
};

}}

#endif
