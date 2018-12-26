#ifndef EE_UI_UISTYLE_HPP
#define EE_UI_UISTYLE_HPP

#include <eepp/ui/uistate.hpp>
#include <eepp/scene/nodeattribute.hpp>

using namespace EE::Scene;

namespace EE { namespace UI {

class EE_API UIStyle : public UIState {
	public:
		static UIStyle * New();

		explicit UIStyle();

		virtual ~UIStyle();

		bool stateExists( const Uint32& State );
	protected:
		std::map<int, std::map<std::string, NodeAttribute> > states;;
};

}}


#endif
