#ifndef EE_UI_TOOLS_UISETTINGSPANEL_HPP
#define EE_UI_TOOLS_UISETTINGSPANEL_HPP

#include <eepp/core/string.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <functional>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace pugi {
class xml_node;
}

namespace EE::UI {
class UICheckBox;
}

namespace EE::UI::Tools {

struct SettingView;

struct EE_API SettingDescriptor {
	std::string id;
	std::string category;
	String name;
	String description;
	String group;
};

struct EE_API BoolPointerSetting {
	bool* value{ nullptr };
	std::function<void( bool )> apply;
};

struct EE_API BoolSetting {
	std::function<bool()> get;
	std::function<void( bool )> set;
};

struct EE_API ChoiceSetting {
	std::vector<String> choices;
	std::vector<String> descriptions;
	std::function<size_t()> get;
	std::function<void( size_t )> set;
};

struct EE_API EditableChoiceSetting {
	std::vector<String> choices;
	std::function<String()> get;
	std::function<bool( const String& )> set;
};

struct EE_API IntegerSetting {
	int min{ 0 };
	int max{ 0 };
	std::function<int()> get;
	std::function<void( int )> set;
};

struct EE_API TextSetting {
	std::function<std::string()> get;
	std::function<bool( const std::string& )> set;
	bool commitOnFocusLoss{ false };
	bool password{ false };
};

struct EE_API FloatSetting {
	double min{ 0 };
	double max{ 0 };
	double step{ 0 };
	std::function<double()> get;
	std::function<void( double )> set;
};

struct EE_API ActionSetting {
	String buttonText;
	std::function<void()> action;
};

using SettingValue =
	std::variant<BoolPointerSetting, BoolSetting, ChoiceSetting, EditableChoiceSetting,
				 IntegerSetting, TextSetting, FloatSetting, ActionSetting>;

struct EE_API SettingDefinition {
	SettingDescriptor descriptor;
	SettingValue value;
	bool enabled{ true };
};

struct EE_API SettingsCategory {
	std::string id;
	String parent;
	String name;
};

struct EE_API SettingsGroup {
	std::string category;
	String name;
	size_t beforeSetting{ 0 };
};

class EE_API SettingsModel {
  public:
	void clear();

	bool addCategory( SettingsCategory category );

	bool addGroup( SettingsGroup group );

	bool addSetting( SettingDefinition setting );

	bool hasCategory( const std::string& id ) const;

	const std::vector<SettingsCategory>& categories() const { return mCategories; }

	const std::vector<SettingsGroup>& groups() const { return mGroups; }

	std::vector<SettingDefinition>& settings() { return mSettings; }

	const std::vector<SettingDefinition>& settings() const { return mSettings; }

  private:
	std::vector<SettingsCategory> mCategories;
	std::vector<SettingsGroup> mGroups;
	std::vector<SettingDefinition> mSettings;
};

class EE_API UISettingsPanel : public UILinearLayout {
  public:
	static UISettingsPanel* New( UIWidget* parent );

	virtual ~UISettingsPanel();

	SettingsModel& getModel();

	const SettingsModel& getModel() const;

	bool addCategory( std::string id, String parent, String name );

	bool addGroup( std::string category, String name );

	bool addBool( SettingDescriptor descriptor, bool* value,
				  std::function<void( bool )> apply = {} );

	bool addBool( SettingDescriptor descriptor, std::function<bool()> get,
				  std::function<void( bool )> set );

	bool addChoice( SettingDescriptor descriptor, std::vector<String> choices,
					std::function<size_t()> get, std::function<void( size_t )> set,
					std::vector<String> choiceDescriptions = {} );

	bool addEditableChoice( SettingDescriptor descriptor, std::vector<String> choices,
							std::function<String()> get, std::function<bool( const String& )> set );

	bool addInteger( SettingDescriptor descriptor, int min, int max, std::function<int()> get,
					 std::function<void( int )> set );

	bool addText( SettingDescriptor descriptor, std::function<std::string()> get,
				  std::function<bool( const std::string& )> set, bool commitOnFocusLoss = false );

	bool addFloat( SettingDescriptor descriptor, double min, double max, double step,
				   std::function<double()> get, std::function<void( double )> set );

	bool addAction( SettingDescriptor descriptor, String buttonText, std::function<void()> action );

	void build();

	void selectCategory( const std::string& category );

	void setCategoryEnabled( const std::string& category, bool enabled,
							 const std::string& excludedSetting = {} );

	void refreshTextSetting( const std::string& id );

	void setSearchResultsText( String text );

	void setFilter( String filter );

	void focusSearch();

	void focusCategories();

	bool isBuilt() const;

  protected:
	struct Impl;
	std::unique_ptr<Impl> mImpl;

	explicit UISettingsPanel( UIWidget* parent );

	void selectCategory( Impl& panel, const std::string& category );

	void addCategory( Impl& panel, const std::string& id, const String& parent,
					  const String& name );

	void addSubcategoryHeading( Impl& panel, const std::string& category, const String& name );

	void setupCategories( Impl& panel );

	UIWidget* createRow( Impl& panel, SettingDefinition& setting, SettingView& view,
						 pugi::xml_node layout );

	UICheckBox* createBoolControl( Impl& panel, SettingDefinition& setting, SettingView& view );

	void materializeCategory( Impl& panel, const std::string& category );

	void materializeVisibleSettings( Impl& panel, const String& query );

	void setCategoryEnabled( Impl& panel, const std::string& category, bool enabled,
							 const std::string& excludedSetting = {} );

	void refreshTextSetting( Impl& panel, const std::string& id );

	void filter( Impl& panel );
};

} // namespace EE::UI::Tools

#endif
