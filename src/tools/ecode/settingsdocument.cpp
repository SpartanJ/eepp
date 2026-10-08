#include "settingsdocument.hpp"
#include <eepp/system/filesystem.hpp>

using namespace EE::System;

namespace ecode {

SettingsDocument::SettingsDocument( std::string path, nlohmann::json data ) :
	mPath( std::move( path ) ), mOriginal( data ), mData( std::move( data ) ) {}

std::shared_ptr<SettingsDocument> SettingsDocument::load( std::string path, std::string& error ) {
	std::string contents;
	if ( !FileSystem::fileGet( path, contents ) ) {
		error = "Unable to read settings file";
		return {};
	}
	try {
		auto data = nlohmann::json::parse( contents, nullptr, true, true );
		if ( !data.is_object() ) {
			error = "Settings file root must be an object";
			return {};
		}
		return std::shared_ptr<SettingsDocument>(
			new SettingsDocument( std::move( path ), std::move( data ) ) );
	} catch ( const nlohmann::json::exception& exception ) {
		error = exception.what();
		return {};
	}
}

bool SettingsDocument::save() {
	if ( !mDirty )
		return true;
	if ( !FileSystem::fileWrite( mPath, mData.dump( 2 ) ) )
		return false;
	mOriginal = mData;
	mDirty = false;
	return true;
}

nlohmann::json SettingsDocument::getValue( const std::string& pointer,
										   nlohmann::json fallback ) const {
	try {
		const auto path = nlohmann::json::json_pointer( pointer );
		if ( mData.contains( path ) )
			return mData.at( path );
	} catch ( const nlohmann::json::exception& ) {
	}
	return fallback;
}

bool SettingsDocument::setValue( const std::string& pointer, nlohmann::json value ) {
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

bool SettingsDocument::contains( const std::string& pointer ) const {
	try {
		return mData.contains( nlohmann::json::json_pointer( pointer ) );
	} catch ( const nlohmann::json::exception& ) {
		return false;
	}
}

void SettingsDocument::discard() {
	mData = mOriginal;
	mDirty = false;
}

} // namespace ecode
