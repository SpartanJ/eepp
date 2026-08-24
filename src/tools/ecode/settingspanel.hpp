#ifndef ECODE_SETTINGSPANEL_HPP
#define ECODE_SETTINGSPANEL_HPP

#include <eepp/ee.hpp>
#include <functional>
#include <string>
#include <vector>

namespace ecode {

class App;

class SettingsPanel {
  public:
	enum class Scope : Uint8 { User, Project };

	explicit SettingsPanel( App* app );

	void show( Scope scope, const std::string& category = {} );

  protected:
	struct SettingBinding {
		std::string id;
		std::string category;
		String name;
		String description;
		String group;
		UIWidget* row{ nullptr };
	};
	struct SubcategoryHeading {
		std::string category;
		String name;
		UITextView* heading{ nullptr };
	};

	struct PanelState {
		UIWindow* window{ nullptr };
		EventConnectionList connections;
		UITextInput* search{ nullptr };
		UITreeView* categories{ nullptr };
		UIScrollView* scroll{ nullptr };
		UILinearLayout* settings{ nullptr };
		UITextView* pageTitle{ nullptr };
		std::shared_ptr<Model> categoryModel;
		UIBindingGroup bindingGroup;
		std::vector<std::pair<std::string, std::vector<std::string>>> categoryItems;
		UnorderedMap<std::string, std::string> categoryIds;
		UnorderedMap<std::string, String> categorySearchText;
		UnorderedMap<std::string, String> categoryTitles;
		UnorderedMap<std::string, UITextView*> categoryHeadings;
		std::vector<SubcategoryHeading> subcategoryHeadings;
		std::vector<SettingBinding> bindings;
		std::string selectedCategory;
		std::string categoryFilter;

		void reset();
	};

	App* mApp{ nullptr };
	PanelState mUser;
	PanelState mProject;

	PanelState& state( Scope scope );
	void create( Scope scope );
	void selectCategory( PanelState& state, const std::string& category );
	void addUserSettings( PanelState& state );
	void addProjectSettings( PanelState& state );
	void addCategory( PanelState& state, const std::string& id, const String& parent,
					  const String& name );
	void addSubcategoryHeading( PanelState& state, const std::string& category,
								const String& name );
	void setupCategories( PanelState& state );
	void addBool( PanelState& state, SettingBinding binding, bool* value,
				  std::function<void( bool )> apply = {} );
	void addBool( PanelState& state, SettingBinding binding, std::function<bool()> get,
				  std::function<void( bool )> set );
	void addChoice( PanelState& state, SettingBinding binding, const std::vector<String>& choices,
					std::function<size_t()> get, std::function<void( size_t )> set,
					std::vector<String> choiceDescriptions = {} );
	void addEditableChoice( PanelState& state, SettingBinding binding,
							const std::vector<String>& choices, std::function<String()> get,
							std::function<bool( const String& )> set );
	void addInteger( PanelState& state, SettingBinding binding, int min, int max,
					 std::function<int()> get, std::function<void( int )> set );
	void addText( PanelState& state, SettingBinding binding, std::function<std::string()> get,
				  std::function<bool( const std::string& )> set, bool commitOnFocusLoss = false );
	void addFloat( PanelState& state, SettingBinding binding, double min, double max, double step,
				   std::function<double()> get, std::function<void( double )> set );
	void addAction( PanelState& state, SettingBinding binding, const String& buttonText,
					std::function<void()> action );
	UIWidget* createRow( PanelState& state, SettingBinding& binding, pugi::xml_node layout );
	UICheckBox* createBoolControl( PanelState& state, SettingBinding& binding );
	void setCategoryEnabled( PanelState& state, const std::string& category, bool enabled,
							 const std::string& excludedSetting = {} );
	void filter( PanelState& state );
};

} // namespace ecode

#endif
