#include "chathistory.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/uiscenenode.hpp>

namespace ecode {

std::vector<ChatHistory> ChatHistory::getHistory( const std::string& historyFolder ) {
	auto files = FileSystem::filesInfoGetInPath( historyFolder, false, false, false, true );
	std::vector<ChatHistory> history;
	history.reserve( files.size() );
	for ( auto& file : files ) {
		auto filename( file.getFileName() );
		auto firstSpace = filename.find_first_of( " " );
		if ( firstSpace == std::string::npos )
			continue;
		auto secondSpace = filename.find_first_of( " ", firstSpace + 1 );
		if ( secondSpace == std::string::npos )
			continue;
		auto uuid = UUID::fromString( filename.substr( 0, firstSpace ) );
		if ( !uuid )
			continue;
		auto summary = filename.substr( secondSpace + 1 );
		size_t finalPartSize = 5; // std::strlen( ".json" )

		if ( !String::endsWith( summary, ".json" ) || summary.size() < finalPartSize )
			continue;

		bool locked = false;
		if ( String::endsWith( summary, ".locked.json" ) ) {
			finalPartSize = 12;
			locked = true;
		}

		summary = summary.substr( 0, summary.size() - finalPartSize );
		history.emplace_back( std::move( *uuid ), std::move( summary ), std::move( file ), locked );
	}

	std::sort( history.begin(), history.end(), []( const ChatHistory& a, const ChatHistory& b ) {
		return a.file.getModificationTime() > b.file.getModificationTime();
	} );

	return history;
}

ChatHistoryModel::ChatHistoryModel( std::vector<ChatHistory>&& history, UISceneNode* uiSceneNode ) :
	mHistory( std::move( history ) ), mUISceneNode( uiSceneNode ) {
	mCurHistory.reserve( mHistory.size() );
	for ( const auto& item : mHistory )
		mCurHistory.emplace_back( &item );
}

void ChatHistoryModel::setFilter( const std::string& filter ) {
	mCurFilter = filter;
	mCurHistory.clear();
	for ( const auto& item : mHistory ) {
		if ( filter.empty() || String::icontains( item.summary, filter ) )
			mCurHistory.emplace_back( &item );
	}
	invalidate();
}

size_t ChatHistoryModel::rowCount( const ModelIndex& ) const {
	return mCurHistory.size();
}

size_t ChatHistoryModel::columnCount( const ModelIndex& ) const {
	return 5;
}

std::string ChatHistoryModel::columnName( const size_t& column ) const {
	switch ( column ) {
		case Columns::Id:
			return mUISceneNode->i18n( "id", "Id" );
		case Columns::Summary:
			return mUISceneNode->i18n( "summary", "Summary" );
		case Columns::DateTime:
			return mUISceneNode->i18n( "datetime", "Date" );
		case Columns::Path:
			return mUISceneNode->i18n( "path", "Path" );
		case Columns::Delete:
			return mUISceneNode->i18n( "delete", "Delete" );
	}
	return "";
}

Variant ChatHistoryModel::data( const ModelIndex& index, ModelRole role ) const {
	static const char* EMPTY = "";
	auto& item = *mCurHistory[index.row()];
	if ( role == ModelRole::Display ) {
		switch ( index.column() ) {
			case Columns::Id:
				return Variant( item.uuid.toString() );
			case Columns::Summary:
				return Variant( item.summary.c_str() );
			case Columns::DateTime:
				return Variant( Sys::epochToString( item.file.getModificationTime() ) );
			case Columns::Path:
				return Variant( item.file.getFilepath().c_str() );
		}
	} else if ( role == ModelRole::Icon ) {
		if ( index.column() == Columns::Delete ) {
			static UIIcon* eraseIcon = mUISceneNode->findIcon( "chrome-close" );
			return Variant( eraseIcon );
		} else if ( index.column() == Columns::Summary && item.locked ) {
			static UIIcon* lockIcon = mUISceneNode->findIcon( "file-lock-fill" );
			return Variant( lockIcon );
		}
	}
	return Variant( EMPTY );
}

void ChatHistoryModel::remove( const ModelIndex& index ) {
	if ( !index.isValid() )
		return;

	auto& item = *mCurHistory[index.row()];

	FileSystem::fileRemove( item.file.getFilepath() );

	mHistory.erase(
		std::remove_if( mHistory.begin(), mHistory.end(), [&item]( const ChatHistory& history ) {
			return item.uuid == history.uuid;
		} ) );

	setFilter( mCurFilter );
}

} // namespace ecode
