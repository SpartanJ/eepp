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

		bool stateExists( const Uint32& state );

		void addAttribute( int state, NodeAttribute attribute );
	protected:
		typedef std::map<std::string, NodeAttribute> AttributesMap;

		std::map<int, AttributesMap> mStates;
};

}}

#endif
