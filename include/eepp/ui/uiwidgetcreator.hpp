#ifndef EE_UI_UIWIDGETCREATOR_HPP
#define EE_UI_UIWIDGETCREATOR_HPP

#include <eepp/core.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class EE_API UIWidgetCreator {
	public:
		typedef cb::Callback1<UIWidget*, std::string> CustomWidgetCb;

		static UIWidget * createFromName( std::string widgetName );

		static void addCustomWidgetCallback( std::string widgetName, const CustomWidgetCb& cb );

		static void removeCustomWidgetCallback( std::string widgetName );

		static bool existsCustomWidgetCallback( std::string widgetName );
	protected:
		typedef std::map<std::string, UIWidgetCreator::CustomWidgetCb> WidgetCallbackMap;

		static WidgetCallbackMap widgetCallback;
};

}}

#endif 
