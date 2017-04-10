#ifndef EE_UI_UIWIDGETCREATOR_HPP
#define EE_UI_UIWIDGETCREATOR_HPP

#include <eepp/core.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class EE_API UIWidgetCreator {
	public:
		typedef cb::Callback1<UIWidget*, std::string> CreateUIWidgetCb;

		static UIWidget * createUIWidgetFromName( std::string name );

		static void addCustomWidgetCallback( std::string widgetName, const CreateUIWidgetCb& cb );

		static void removeCustomWidgetCallback( std::string widgetName );

		static bool existsCustomWidgetCallback( std::string widgetName );
};

}}

#endif 
