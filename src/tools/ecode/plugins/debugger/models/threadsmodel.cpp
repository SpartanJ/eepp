#include "threadsmodel.hpp"
#include <eepp/ui/uiscenenode.hpp>

namespace ecode {

ThreadsModel::ThreadsModel( const std::vector<DapThread>& threads, UISceneNode* sceneNode ) :
	mThreads( threads ), mSceneNode( sceneNode ) {}

size_t ThreadsModel::rowCount( const ModelIndex& ) const {
	return mThreads.size();
}

size_t ThreadsModel::columnCount( const ModelIndex& ) const {
	return 1;
}

std::string ThreadsModel::columnName( const size_t& colIdx ) const {
	switch ( colIdx ) {
		case Columns::ID:
			return mSceneNode->i18n( "thread_id", "Thread ID" );
	}
	return "";
}

Variant ThreadsModel::data( const ModelIndex& modelIndex, ModelRole role ) const {
	if ( role == ModelRole::Display && modelIndex.column() == Columns::ID ) {
		if ( mThreads[modelIndex.row()].name.empty() )
			return Variant( String::format( "#%d", mThreads[modelIndex.row()].id ) );
		else
			return Variant( String::format( "#%d (%s)", mThreads[modelIndex.row()].id,
											mThreads[modelIndex.row()].name.c_str() ) );
	} else if ( role == ModelRole::Icon && modelIndex.column() == Columns::ID &&
				mThreads[modelIndex.row()].id == mCurrentThreadId ) {
		static UIIcon* circleFilled = mSceneNode->findIcon( "circle-filled" );
		return Variant( circleFilled );
	}
	return {};
}

void ThreadsModel::setThreads( std::vector<DapThread>&& threads ) {
	{
		Lock l( mResourceLock );
		mThreads = std::move( threads );
	}
	invalidate();
}

void ThreadsModel::resetThreads() {

	{
		Lock l( mResourceLock );
		mThreads = {};
	}
	invalidate();
}

const DapThread& ThreadsModel::getThread( size_t index ) const {
	Lock l( mResourceLock );
	eeASSERT( index < mThreads.size() );
	return mThreads[index];
}

ModelIndex ThreadsModel::fromThreadId( int id ) {
	Lock l( mResourceLock );
	for ( size_t i = 0; i < mThreads.size(); i++ ) {
		const DapThread& thread = mThreads[i];
		if ( thread.id == id )
			return index( i );
	}
	return {};
}

void ThreadsModel::setCurrentThreadId( int id ) {
	if ( mCurrentThreadId != id ) {
		mCurrentThreadId = id;
		invalidate( Model::UpdateFlag::DontInvalidateIndexes );
	}
}

} // namespace ecode
