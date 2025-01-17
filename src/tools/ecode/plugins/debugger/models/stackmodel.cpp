#include "stackmodel.hpp"
#include <eepp/ui/uiscenenode.hpp>

namespace ecode {

StackModel::StackModel( StackTraceInfo&& stack, UISceneNode* sceneNode ) :
	mStack( std::move( stack ) ), mSceneNode( sceneNode ) {}

size_t StackModel::rowCount( const ModelIndex& ) const {
	Lock l( mResourceLock );
	return mStack.stackFrames.size();
}

size_t StackModel::columnCount( const ModelIndex& ) const {
	return 5;
}

std::string StackModel::columnName( const size_t& colIdx ) const {
	switch ( colIdx ) {
		case Columns::ID:
			return mSceneNode->i18n( "id", "ID" );
		case Columns::Name:
			return mSceneNode->i18n( "name", "Name" );
		case Columns::SourceName:
			return mSceneNode->i18n( "source_name", "Source Name" );
		case Columns::SourcePath:
			return mSceneNode->i18n( "source_path", "Source Path" );
		case Columns::Line:
			return mSceneNode->i18n( "line", "Line" );
	}
	return "";
}

Variant StackModel::data( const ModelIndex& modelIndex, ModelRole role ) const {
	Lock l( mResourceLock );
	if ( role == ModelRole::Display ) {
		switch ( modelIndex.column() ) {
			case Columns::ID:
				return Variant( String::toString( mStack.stackFrames[modelIndex.row()].id ) );
			case Columns::Name:
				return Variant( mStack.stackFrames[modelIndex.row()].name.c_str() );
			case Columns::SourceName:
				return mStack.stackFrames[modelIndex.row()].source
						   ? Variant( mStack.stackFrames[modelIndex.row()].source->name )
						   : Variant();
			case Columns::SourcePath:
				return mStack.stackFrames[modelIndex.row()].source
						   ? Variant( mStack.stackFrames[modelIndex.row()].source->path )
						   : Variant();
			case Columns::Line:
				return Variant( String::toString( mStack.stackFrames[modelIndex.row()].line ) );
			case Columns::Column:
				return Variant( String::toString( mStack.stackFrames[modelIndex.row()].column ) );
		}
	} else if ( role == ModelRole::Icon && modelIndex.column() == Columns::Name &&
				mCurrentScopeId == mStack.stackFrames[modelIndex.row()].id ) {
		static UIIcon* circleFilled = mSceneNode->findIcon( "circle-filled" );
		return Variant( circleFilled );
	}
	return {};
}

void StackModel::setStack( StackTraceInfo&& stack ) {
	{
		Lock l( mResourceLock );
		mStack = std::move( stack );
	}
	invalidate();
}

void StackModel::resetStack() {

	{
		Lock l( mResourceLock );
		mStack = {};
	}
	invalidate();
}

const StackFrame& StackModel::getStack( size_t index ) const {
	Lock l( mResourceLock );
	eeASSERT( index < mStack.stackFrames.size() );
	return mStack.stackFrames[index];
}

void StackModel::setCurrentScopeId( int scope ) {
	if ( mCurrentScopeId != scope ) {
		mCurrentScopeId = scope;
		invalidate( Model::UpdateFlag::DontInvalidateIndexes );
	}
}

} // namespace ecode
