#ifndef ECODE_SETTINGSPANEL_HPP
#define ECODE_SETTINGSPANEL_HPP

#include <eepp/ee.hpp>
#include <eepp/scene/mainthreadlifetime.hpp>
#include <memory>
#include <vector>

namespace ecode {

class App;
class SettingsDocument;

class SettingsPanel {
  public:
	enum class Scope : Uint8 { User, Project };

	explicit SettingsPanel( App* app );

	void show( Scope scope, const std::string& category = {} );

  protected:
	struct PanelState {
		UIWindow* window{ nullptr };
		UISettingsPanel* panel{ nullptr };
		EventConnectionList connections;
		std::vector<std::shared_ptr<SettingsDocument>> documents;

		void reset();
	};

	App* mApp{ nullptr };
	MainThreadLifetime<SettingsPanel> mLifetime;
	PanelState mUser;
	PanelState mProject;

	PanelState& state( Scope scope );

	void create( Scope scope );

	void selectCategory( PanelState& state, const std::string& category );

	void addUserSettings( PanelState& state );

	void addPluginSettings( PanelState& state );

	void addProjectSettings( PanelState& state );

	void addCategory( PanelState& state, std::string id, String parent, String name );

	void addSubcategoryHeading( PanelState& state, std::string category, String name );

	void addBool( PanelState& state, SettingDescriptor binding, bool* value,
				  std::function<void( bool )> apply = {} );

	void addBool( PanelState& state, SettingDescriptor binding, std::function<bool()> get,
				  std::function<void( bool )> set );

	void addChoice( PanelState& state, SettingDescriptor binding,
					const std::vector<String>& choices, std::function<size_t()> get,
					std::function<void( size_t )> set,
					std::vector<String> choiceDescriptions = {} );

	void addEditableChoice( PanelState& state, SettingDescriptor binding,
							const std::vector<String>& choices, std::function<String()> get,
							std::function<bool( const String& )> set );

	void addInteger( PanelState& state, SettingDescriptor binding, int min, int max,
					 std::function<int()> get, std::function<void( int )> set );

	void addText( PanelState& state, SettingDescriptor binding, std::function<std::string()> get,
				  std::function<bool( const std::string& )> set, bool commitOnFocusLoss = false );

	void addFloat( PanelState& state, SettingDescriptor binding, double min, double max,
				   double step, std::function<double()> get, std::function<void( double )> set );

	void addAction( PanelState& state, SettingDescriptor binding, const String& buttonText,
					std::function<void()> action );

	void refreshTextSetting( PanelState& state, const std::string& id );

	void setCategoryEnabled( PanelState& state, const std::string& category, bool enabled,
							 const std::string& excludedSetting = {} );
};

} // namespace ecode

#endif
