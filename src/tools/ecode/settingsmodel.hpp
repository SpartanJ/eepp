#ifndef ECODE_SETTINGSMODEL_HPP
#define ECODE_SETTINGSMODEL_HPP

#include <algorithm>
#include <eepp/ee.hpp>
#include <functional>
#include <string>
#include <variant>
#include <vector>

namespace ecode {

struct SettingDescriptor {
	std::string id;
	std::string category;
	String name;
	String description;
	String group;
};

struct BoolPointerSetting {
	bool* value{ nullptr };
	std::function<void( bool )> apply;
};

struct BoolSetting {
	std::function<bool()> get;
	std::function<void( bool )> set;
};

struct ChoiceSetting {
	std::vector<String> choices;
	std::vector<String> descriptions;
	std::function<size_t()> get;
	std::function<void( size_t )> set;
};

struct EditableChoiceSetting {
	std::vector<String> choices;
	std::function<String()> get;
	std::function<bool( const String& )> set;
};

struct IntegerSetting {
	int min{ 0 };
	int max{ 0 };
	std::function<int()> get;
	std::function<void( int )> set;
};

struct TextSetting {
	std::function<std::string()> get;
	std::function<bool( const std::string& )> set;
	bool commitOnFocusLoss{ false };
};

struct FloatSetting {
	double min{ 0 };
	double max{ 0 };
	double step{ 0 };
	std::function<double()> get;
	std::function<void( double )> set;
};

struct ActionSetting {
	String buttonText;
	std::function<void()> action;
};

using SettingValue =
	std::variant<BoolPointerSetting, BoolSetting, ChoiceSetting, EditableChoiceSetting,
				 IntegerSetting, TextSetting, FloatSetting, ActionSetting>;

struct SettingDefinition {
	SettingDescriptor descriptor;
	SettingValue value;
	bool enabled{ true };
};

struct SettingsCategory {
	std::string id;
	String parent;
	String name;
};

struct SettingsGroup {
	std::string category;
	String name;
	size_t beforeSetting{ 0 };
};

class SettingsModel {
  public:
	void clear() {
		mCategories.clear();
		mGroups.clear();
		mSettings.clear();
	}

	bool addCategory( SettingsCategory category ) {
		if ( category.id.empty() ||
			 std::any_of( mCategories.begin(), mCategories.end(),
						  [&category]( const auto& item ) { return item.id == category.id; } ) )
			return false;
		mCategories.emplace_back( std::move( category ) );
		return true;
	}

	bool addGroup( SettingsGroup group ) {
		if ( !hasCategory( group.category ) )
			return false;
		mGroups.emplace_back( std::move( group ) );
		return true;
	}

	bool addSetting( SettingDefinition setting ) {
		if ( setting.descriptor.id.empty() || !hasCategory( setting.descriptor.category ) ||
			 std::any_of( mSettings.begin(), mSettings.end(), [&setting]( const auto& item ) {
				 return item.descriptor.id == setting.descriptor.id;
			 } ) )
			return false;
		mSettings.emplace_back( std::move( setting ) );
		return true;
	}

	bool hasCategory( const std::string& id ) const {
		return std::any_of( mCategories.begin(), mCategories.end(),
							[&id]( const auto& item ) { return item.id == id; } );
	}

	const std::vector<SettingsCategory>& categories() const { return mCategories; }
	const std::vector<SettingsGroup>& groups() const { return mGroups; }
	std::vector<SettingDefinition>& settings() { return mSettings; }
	const std::vector<SettingDefinition>& settings() const { return mSettings; }

  private:
	std::vector<SettingsCategory> mCategories;
	std::vector<SettingsGroup> mGroups;
	std::vector<SettingDefinition> mSettings;
};

} // namespace ecode

#endif
