#ifndef EE_UI_UIWIDGETCREATOR_HPP
#define EE_UI_UIWIDGETCREATOR_HPP

#include <eepp/core.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class EE_API UIWidgetCreator {
  public:
	typedef std::function<UIWidget*( const std::string& )> CustomWidgetCb;
	typedef std::function<UIWidget*()> RegisterWidgetCb;

	typedef std::unordered_map<std::string, UIWidgetCreator::CustomWidgetCb> WidgetCallbackMap;
	typedef std::unordered_map<std::string, UIWidgetCreator::RegisterWidgetCb>
		RegisteredWidgetCallbackMap;

	static UIWidget* createFromName( const std::string& widgetName );

	static void addCustomWidgetCallback( const std::string& widgetName, const CustomWidgetCb& cb );

	static void removeCustomWidgetCallback( const std::string& widgetName );

	static bool existsCustomWidgetCallback( const std::string& widgetName );

	static void registerWidget( const std::string& widgetName, const RegisterWidgetCb& cb );

	static void unregisterWidget( const std::string& widgetName );

	static bool isWidgetRegistered( const std::string& widgetName );

	static const RegisteredWidgetCallbackMap& getRegisteredWidgets();

	static std::vector<std::string> getWidgetNames();

  protected:
	static RegisteredWidgetCallbackMap registeredWidget;

	static WidgetCallbackMap widgetCallback;

	static void createBaseWidgetList();
};

}} // namespace EE::UI

#endif
