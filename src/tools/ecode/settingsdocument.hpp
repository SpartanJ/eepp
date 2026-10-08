#ifndef ECODE_SETTINGSDOCUMENT_HPP
#define ECODE_SETTINGSDOCUMENT_HPP

#include <memory>
#include <nlohmann/json.hpp>
#include <string>

namespace ecode {

class SettingsDocument {
  public:
	static std::shared_ptr<SettingsDocument> load( std::string path, std::string& error );

	template <typename T> T get( const std::string& pointer, T fallback ) const {
		try {
			const auto path = nlohmann::json::json_pointer( pointer );
			if ( mData.contains( path ) )
				return mData.at( path ).get<T>();
		} catch ( const nlohmann::json::exception& ) {
		}
		return fallback;
	}

	template <typename T> bool set( const std::string& pointer, T value ) {
		try {
			const auto path = nlohmann::json::json_pointer( pointer );
			if ( mData.contains( path ) && mData.at( path ) == value )
				return false;
			mData[path] = std::move( value );
			mDirty = true;
			return true;
		} catch ( const nlohmann::json::exception& ) {
			return false;
		}
	}

	nlohmann::json getValue( const std::string& pointer, nlohmann::json fallback ) const;

	bool setValue( const std::string& pointer, nlohmann::json value );

	bool contains( const std::string& pointer ) const;

	bool save();

	void discard();

	bool isDirty() const { return mDirty; }

	const std::string& path() const { return mPath; }

  private:
	SettingsDocument( std::string path, nlohmann::json data );

	std::string mPath;
	nlohmann::json mOriginal;
	nlohmann::json mData;
	bool mDirty{ false };
};

} // namespace ecode

#endif
