#ifndef ECODE_SETTINGSPAGE_HPP
#define ECODE_SETTINGSPAGE_HPP

#include "settingsdocument.hpp"
#include "settingsmodel.hpp"

namespace ecode {

class SettingsPage {
  public:
	static bool parseNonNegativeSettingsTime( const std::string& text, Time& time );

	SettingsPage( SettingsModel& model, std::shared_ptr<SettingsDocument> document,
				  std::string category, std::string idPrefix );

	bool addGroup( String name );

	bool addBool( std::string id, std::string jsonPointer, String name, String description,
				  bool defaultValue, String group = {} );

	bool addChoice( std::string id, std::string jsonPointer, String name, String description,
					std::vector<String> choices, std::vector<nlohmann::json> values,
					nlohmann::json defaultValue, std::vector<String> choiceDescriptions = {},
					String group = {} );

	bool addEditableChoice( std::string id, std::string jsonPointer, String name,
							String description, std::vector<String> choices,
							std::string defaultValue,
							std::function<bool( const std::string& )> validate = {},
							String group = {} );

	bool addInteger( std::string id, std::string jsonPointer, String name, String description,
					 int min, int max, int defaultValue, String group = {} );

	bool addText( std::string id, std::string jsonPointer, String name, String description,
				  std::string defaultValue, std::function<bool( const std::string& )> validate = {},
				  bool commitOnFocusLoss = false, String group = {} );

	bool addSecret( std::string id, std::string jsonPointer, String name, String description,
					std::string defaultValue = {}, String group = {},
					std::string legacyJsonPointer = {} );

	bool addJsonObject( std::string id, std::string jsonPointer, String name, String description,
						nlohmann::json defaultValue = nlohmann::json::object(), String group = {} );

	bool addStringList( std::string id, std::string jsonPointer, String name, String description,
						std::vector<std::string> defaultValue = {}, String group = {} );

	bool addFloat( std::string id, std::string jsonPointer, String name, String description,
				   double min, double max, double step, double defaultValue, String group = {} );

	bool addAction( std::string id, String name, String description, String buttonText,
					std::function<void()> action, String group = {} );

  private:
	SettingsModel& mModel;
	std::shared_ptr<SettingsDocument> mDocument;
	std::string mCategory;
	std::string mIdPrefix;
};

} // namespace ecode

#endif
