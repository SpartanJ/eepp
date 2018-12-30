#ifndef EE_UI_UISTYLE_HPP
#define EE_UI_UISTYLE_HPP

#include <eepp/ui/uistate.hpp>
#include <eepp/scene/nodeattribute.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>

using namespace EE::Scene;

namespace EE { namespace UI {

class UIWidget;

class EE_API UIStyle : public UIState {
	public:
		static UIStyle * New( UIWidget * widget );

		explicit UIStyle( UIWidget * widget );

		virtual ~UIStyle();

		bool stateExists( const Uint32& state );

		void addAttribute( int state, NodeAttribute attribute );

		void load();

		void onStateChange();
	protected:
		typedef std::map<std::string, NodeAttribute> AttributesMap;

		UIWidget * mWidget;
		std::map<int, AttributesMap> mStates;

		void addStyleSheetProperties( int state, const CSS::StyleSheetProperties& properties );
};

}}

#endif
