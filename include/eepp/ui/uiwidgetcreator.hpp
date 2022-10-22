#ifndef EE_UI_UIWIDGETCREATOR_HPP
#define EE_UI_UIWIDGETCREATOR_HPP

#include <eepp/core.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class EE_API UIWidgetCreator {
  public:
	typedef std::function<UIWidget*( std::string )> CustomWidgetCb;
	typedef std::function<UIWidget*()> RegisterWidgetCb;

	typedef std::map<std::string, UIWidgetCreator::CustomWidgetCb> WidgetCallbackMap;
	typedef std::map<std::string, UIWidgetCreator::RegisterWidgetCb> RegisteredWidgetCallbackMap;

	static UIWidget* createFromName( std::string widgetName );

	static void addCustomWidgetCallback( std::string widgetName, const CustomWidgetCb& cb );

	static void removeCustomWidgetCallback( std::string widgetName );

	static bool existsCustomWidgetCallback( std::string widgetName );

	static void registerWidget( std::string widgetName, const RegisterWidgetCb& cb );

	static void unregisterWidget( std::string widgetName );

	static bool isWidgetRegistered( std::string widgetName );

	static const RegisteredWidgetCallbackMap& getRegisteredWidgets();

	static std::vector<std::string> getWidgetNames();

  protected:
	static RegisteredWidgetCallbackMap registeredWidget;

	static WidgetCallbackMap widgetCallback;

	static void createBaseWidgetList();
};

}} // namespace EE::UI

#endif
