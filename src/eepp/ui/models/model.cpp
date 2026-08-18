#include <eepp/ui/abstract/uiabstractview.hpp>
#include <eepp/ui/models/model.hpp>
#include <eepp/ui/models/persistentmodelindex.hpp>

namespace EE { namespace UI { namespace Models {

void Model::onModelUpdate( unsigned flags ) {
	if ( mOnUpdate )
		mOnUpdate();
	for ( auto& client : mClients )
		client->onModelUpdated( flags );
	forEachView( [flags]( UIAbstractView* view ) { view->onModelUpdate( flags ); } );
}

void Model::forEachView( std::function<void( UIAbstractView* )> callback ) const {
	for ( auto view : mViews )
		callback( view );
}

void Model::unregisterView( UIAbstractView* view ) {
	mViews.erase( view );
}

void Model::registerClient( Model::Client* client ) {
	mClients.insert( client );
}

void Model::unregisterClient( Model::Client* client ) {
	mClients.erase( client );
}

void Model::refreshView() const {
	forEachView( []( UIAbstractView* view ) { view->invalidateDraw(); } );
}

void Model::registerView( UIAbstractView* view ) {
	mViews.insert( view );
}

ModelIndex Model::createIndex( int row, int column, const void* data,
							   const Int64& internalId ) const {
	return ModelIndex( *this, row, column, const_cast<void*>( data ), internalId );
}

void Model::setOnUpdate( const std::function<void()>& onUpdate ) {
	mOnUpdate = onUpdate;
}

void Model::invalidate( unsigned int flags ) {
	onModelUpdate( flags );
}

ModelIndex Model::sibling( int row, int column, const ModelIndex& parent ) const {
	if ( !parent.isValid() )
		return index( row, column, {} );
	int rowCount = this->rowCount( parent );
	if ( row < 0 || row > rowCount )
		return {};
	return index( row, column, parent );
}

bool Model::acceptsDrag( const ModelIndex&, const std::string& ) {
	return false;
}

void Model::beginInsertRows( ModelIndex const& parent, int first, int last ) {
	eeASSERT( first >= 0 );
	eeASSERT( first <= last );
	mOperationStack.push( { OperationType::Insert, Direction::Row, parent, first, last } );
}

void Model::beginInsertColumns( ModelIndex const& parent, int first, int last ) {
	eeASSERT( first >= 0 );
	eeASSERT( first <= last );
	mOperationStack.push( { OperationType::Insert, Direction::Column, parent, first, last } );
}

void Model::beginMoveRows( ModelIndex const& sourceParent, int first, int last,
						   ModelIndex const& targetParent, int targetIndex ) {
	eeASSERT( first >= 0 );
	eeASSERT( first <= last );
	eeASSERT( targetIndex >= 0 );
	Operation operation{ OperationType::Move, Direction::Row, sourceParent, first, last,
						 targetParent,		  targetIndex };
	saveMovedIndices( operation );
	mOperationStack.push( std::move( operation ) );
}

void Model::beginMoveColumns( ModelIndex const& sourceParent, int first, int last,
							  ModelIndex const& targetParent, int targetIndex ) {
	eeASSERT( first >= 0 );
	eeASSERT( first <= last );
	eeASSERT( targetIndex >= 0 );
	Operation operation{ OperationType::Move, Direction::Column, sourceParent, first, last,
						 targetParent,		  targetIndex };
	saveMovedIndices( operation );
	mOperationStack.push( std::move( operation ) );
}

void Model::saveMovedIndices( Operation& operation ) {
	const bool isRow = operation.direction == Direction::Row;
	const bool moveWithin = operation.sourceParent == operation.targetParent;
	const bool movingDown = operation.target > operation.first;
	const int count = operation.last - operation.first + 1;
	const int workAreaStart = std::min( operation.first, operation.target );
	const int workAreaEnd = std::max( operation.last + 1, operation.target + count );

	operation.persistentMoves.reserve( mPersistentHandles.size() );
	for ( const auto& entry : mPersistentHandles ) {
		const ModelIndex& index = entry.first;
		const int dimension = isRow ? index.row() : index.column();
		const ModelIndex parent = index.parent();

		if ( parent == operation.sourceParent && dimension >= operation.first &&
			 dimension <= operation.last ) {
			operation.persistentMoves.push_back(
				{ index, operation.targetParent, operation.target + dimension - operation.first } );
		} else if ( moveWithin && parent == operation.sourceParent ) {
			if ( movingDown && dimension > operation.last && dimension < workAreaEnd ) {
				operation.persistentMoves.push_back(
					{ index, operation.sourceParent,
					  workAreaStart + dimension - operation.last - 1 } );
			} else if ( !movingDown && dimension >= workAreaStart && dimension < operation.first ) {
				operation.persistentMoves.push_back(
					{ index, operation.sourceParent, dimension + count } );
			}
		} else if ( !moveWithin && parent == operation.sourceParent &&
					dimension > operation.last ) {
			operation.persistentMoves.push_back(
				{ index, operation.sourceParent, dimension - count } );
		} else if ( !moveWithin && parent == operation.targetParent &&
					dimension >= operation.target ) {
			operation.persistentMoves.push_back(
				{ index, operation.targetParent, dimension + count } );
		}
	}
}

bool Model::beginDeleteRows( ModelIndex const& parent, int first, int last ) {
	if ( first >= 0 && first <= last && (size_t)last < rowCount( parent ) ) {
		for ( int row = first; row <= last; ++row )
			notifyIndexDeleted( index( row, 0, parent ).internalData() );
		saveDeletedIndices<true>( parent, first, last );
		mOperationStack.push( { OperationType::Delete, Direction::Row, parent, first, last } );
		return true;
	}
	return false;
}

void Model::notifyIndexDeleted( const void* internalData ) const {
	forEachView(
		[internalData]( UIAbstractView* view ) { view->onModelIndexDeleted( internalData ); } );
}

bool Model::beginDeleteColumns( ModelIndex const& parent, int first, int last ) {
	if ( first >= 0 && first <= last && (size_t)last < columnCount( parent ) ) {
		saveDeletedIndices<false>( parent, first, last );
		mOperationStack.push( { OperationType::Delete, Direction::Column, parent, first, last } );
		return true;
	}
	return false;
}

std::weak_ptr<PersistentHandle> Model::registerPersistentIndex( ModelIndex const& index ) {
	if ( !index.isValid() )
		return {};

	auto it = mPersistentHandles.find( index );
	// Easy modo: we already have a handle for this model index.
	if ( it != mPersistentHandles.end() ) {
		return it->second;
	}

	// Hard modo: create a new persistent handle.
	auto handle = std::make_shared<PersistentHandle>( index );
	std::weak_ptr<PersistentHandle> weak_handle = handle;
	mPersistentHandles[index] = std::move( handle );

	return weak_handle;
}

template <bool IsRow>
void Model::saveDeletedIndices( ModelIndex const& parent, int first, int last ) {
	std::vector<ModelIndex> deletedIndices;

	for ( auto& entry : mPersistentHandles ) {
		auto currentIndex = entry.first;

		// Walk up the persistent handle's parents to see if it is contained
		// within the range that is being deleted.
		while ( currentIndex.isValid() ) {
			auto currentParent = currentIndex.parent();

			if ( currentParent == parent ) {
				if ( IsRow ) {
					if ( currentIndex.row() >= first && currentIndex.row() <= last )
						deletedIndices.emplace_back( currentIndex );
				} else {
					if ( currentIndex.column() >= first && currentIndex.column() <= last )
						deletedIndices.emplace_back( currentIndex );
				}
			}

			currentIndex = currentParent;
		}
	}

	mDeletedIndicesStack.push( std::move( deletedIndices ) );
}

void Model::endInsertRows() {
	auto operation = mOperationStack.top();
	mOperationStack.pop();
	eeASSERT( operation.type == OperationType::Insert );
	eeASSERT( operation.direction == Direction::Row );
	handleInsert( operation );

	for ( auto& client : mClients ) {
		client->modelDidInsertRows( operation.sourceParent, operation.first, operation.last );
	}
}

void Model::endInsertColumns() {
	auto operation = mOperationStack.top();
	mOperationStack.pop();
	eeASSERT( operation.type == OperationType::Insert );
	eeASSERT( operation.direction == Direction::Column );
	handleInsert( operation );

	for ( auto& client : mClients )
		client->modelDidInsertColumns( operation.sourceParent, operation.first, operation.last );
}

void Model::endMoveRows() {
	auto operation = mOperationStack.top();
	mOperationStack.pop();
	eeASSERT( operation.type == OperationType::Move );
	eeASSERT( operation.direction == Direction::Row );
	handleMove( operation );

	for ( auto& client : mClients )
		client->modelDidMoveRows( operation.sourceParent, operation.first, operation.last,
								  operation.targetParent, operation.target );
}

void Model::endMoveColumns() {
	auto operation = mOperationStack.top();
	mOperationStack.pop();
	eeASSERT( operation.type == OperationType::Move );
	eeASSERT( operation.direction == Direction::Column );
	handleMove( operation );

	for ( auto& client : mClients ) {
		client->modeldidMoveColumns( operation.sourceParent, operation.first, operation.last,
									 operation.targetParent, operation.target );
	}
}

void Model::endDeleteRows() {
	auto operation = mOperationStack.top();
	mOperationStack.pop();
	eeASSERT( operation.type == OperationType::Delete );
	eeASSERT( operation.direction == Direction::Row );
	handleDelete( operation );

	for ( auto& client : mClients ) {
		client->modelDidDeleteRows( operation.sourceParent, operation.first, operation.last );
	}
}

void Model::endDeleteColumns() {
	auto operation = mOperationStack.top();
	mOperationStack.pop();
	eeASSERT( operation.type == OperationType::Delete );
	eeASSERT( operation.direction == Direction::Column );
	handleDelete( operation );

	for ( auto& client : mClients ) {
		client->modelDidDeleteColumns( operation.sourceParent, operation.first, operation.last );
	}
}

void Model::handleInsert( Operation const& operation ) {
	bool isRow = operation.direction == Direction::Row;
	std::vector<std::pair<ModelIndex, ModelIndex>> indexChanges;
	int offset = operation.last - operation.first + 1;

	for ( auto& entry : mPersistentHandles ) {
		if ( entry.first.parent() == operation.sourceParent ) {
			bool shifts = isRow ? entry.first.row() >= operation.first
								: entry.first.column() >= operation.first;
			if ( !shifts )
				continue;
			int newRow = isRow ? entry.first.row() + offset : entry.first.row();
			int newColumn = isRow ? entry.first.column() : entry.first.column() + offset;
			indexChanges.emplace_back( entry.first,
									   createIndex( newRow, newColumn, entry.first.internalData(),
												entry.first.internalId() ) );
		}
	}

	applyPersistentIndexChanges( indexChanges );
}

void Model::handleDelete( Operation const& operation ) {
	bool isRow = operation.direction == Direction::Row;
	std::vector<ModelIndex> deletedIndices = mDeletedIndicesStack.top();
	mDeletedIndicesStack.pop();
	std::vector<std::pair<ModelIndex, ModelIndex>> indexChanges;

	// Get rid of all persistent handles which have been marked for death
	for ( auto& deletedIndex : deletedIndices ) {
		mPersistentHandles.erase( deletedIndex );
	}

	int offset = operation.last - operation.first + 1;
	for ( auto& entry : mPersistentHandles ) {
		if ( entry.first.parent() == operation.sourceParent ) {
			bool shifts =
				isRow ? entry.first.row() > operation.last : entry.first.column() > operation.last;
			if ( !shifts )
				continue;
			int newRow = isRow ? entry.first.row() - offset : entry.first.row();
			int newColumn = isRow ? entry.first.column() : entry.first.column() - offset;
			indexChanges.emplace_back( entry.first,
									   createIndex( newRow, newColumn, entry.first.internalData(),
												entry.first.internalId() ) );
		}
	}

	applyPersistentIndexChanges( indexChanges );
}

void Model::applyPersistentIndexChanges(
	const std::vector<std::pair<ModelIndex, ModelIndex>>& indexChanges ) {
	std::vector<std::pair<ModelIndex, std::shared_ptr<PersistentHandle>>> changedHandles;
	changedHandles.reserve( indexChanges.size() );
	for ( const auto& change : indexChanges ) {
		auto it = mPersistentHandles.find( change.first );
		if ( it == mPersistentHandles.end() )
			continue;
		it->second->mIndex = change.second;
		changedHandles.emplace_back( change.second, std::move( it->second ) );
		mPersistentHandles.erase( it );
	}
	for ( auto& changedHandle : changedHandles )
		mPersistentHandles.emplace( std::move( changedHandle.first ),
									std::move( changedHandle.second ) );
}

Variant Model::stylizeModel( const ModelIndex& index, const void* data ) const {
	for ( const auto& styler : mStylers ) {
		auto ret = styler.second( index, data );
		if ( ret.isValid() )
			return ret;
	}
	return {};
}

Mutex& Model::resourceMutex() {
	return mResourceLock;
}

void Model::acquireResourceMutex() {
	mResourceLock.lock();
}

void Model::releaseResourceMutex() {
	mResourceLock.unlock();
}

Uint32 Model::subscribeModelStyler( const ModelStyler& styler ) {
	mStylers[++mLastStylerId] = styler;
	return mLastStylerId;
}

void Model::unsubscribeModelStyler( Uint32 id ) {
	mStylers.erase( id );
}

void Model::handleMove( Operation const& operation ) {
	bool isRow = operation.direction == Direction::Row;
	bool moveWithin = operation.sourceParent == operation.targetParent;

	if ( moveWithin && operation.first == operation.target )
		return;

	std::vector<std::pair<ModelIndex, ModelIndex>> indexChanges;
	indexChanges.reserve( operation.persistentMoves.size() );
	for ( const auto& move : operation.persistentMoves ) {
		int newRow = isRow ? move.targetDimension : move.index.row();
		int newColumn = isRow ? move.index.column() : move.targetDimension;
		indexChanges.emplace_back( move.index, index( newRow, newColumn, move.targetParent ) );
	}
	applyPersistentIndexChanges( indexChanges );
}

}}} // namespace EE::UI::Models
