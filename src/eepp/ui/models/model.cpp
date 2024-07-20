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

void Model::forEachView( std::function<void( UIAbstractView* )> callback ) {
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

void Model::refreshView() {
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
	mOperationStack.push( { OperationType::Move, Direction::Row, sourceParent, first, last,
							targetParent, targetIndex } );
}

void Model::beginMoveColumns( ModelIndex const& sourceParent, int first, int last,
							  ModelIndex const& targetParent, int targetIndex ) {
	eeASSERT( first >= 0 );
	eeASSERT( first <= last );
	eeASSERT( targetIndex >= 0 );
	mOperationStack.push( { OperationType::Move, Direction::Column, sourceParent, first, last,
							targetParent, targetIndex } );
}

bool Model::beginDeleteRows( ModelIndex const& parent, int first, int last ) {
	if ( first >= 0 && first <= last && (size_t)last < rowCount( parent ) ) {
		saveDeletedIndices<true>( parent, first, last );
		mOperationStack.push( { OperationType::Delete, Direction::Row, parent, first, last } );
		return true;
	}
	return false;
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
	std::vector<const ModelIndex*> toShift;

	for ( auto& entry : mPersistentHandles ) {
		if ( entry.first.parent() == operation.sourceParent ) {
			if ( isRow && entry.first.row() >= operation.first ) {
				toShift.emplace_back( &entry.first );
			} else if ( !isRow && entry.first.column() >= operation.first ) {
				toShift.emplace_back( &entry.first );
			}
		}
	}

	int offset = operation.last - operation.first + 1;

	for ( auto currentIndex : toShift ) {
		int newRow = isRow ? currentIndex->row() + offset : currentIndex->row();
		int newColumn = isRow ? currentIndex->column() : currentIndex->column() + offset;
		auto newIndex = createIndex( newRow, newColumn, currentIndex->internalData() );

		auto it = mPersistentHandles.find( *currentIndex );
		auto handle = std::move( it->second );

		handle->mIndex = newIndex;

		mPersistentHandles.erase( it );
		mPersistentHandles[std::move( newIndex )] = std::move( handle );
	}
}

void Model::handleDelete( Operation const& operation ) {
	bool isRow = operation.direction == Direction::Row;
	std::vector<ModelIndex> deletedIndices = mDeletedIndicesStack.top();
	mDeletedIndicesStack.pop();
	std::vector<const ModelIndex*> toShift;

	// Get rid of all persistent handles which have been marked for death
	for ( auto& deletedIndex : deletedIndices ) {
		mPersistentHandles.erase( deletedIndex );
	}

	for ( auto& entry : mPersistentHandles ) {
		if ( entry.first.parent() == operation.sourceParent ) {
			if ( isRow ) {
				if ( entry.first.row() > operation.last ) {
					toShift.emplace_back( &entry.first );
				}
			} else {
				if ( entry.first.column() > operation.last ) {
					toShift.emplace_back( &entry.first );
				}
			}
		}
	}

	int offset = operation.last - operation.first + 1;

	for ( auto currentIndex : toShift ) {
		int newRow = isRow ? currentIndex->row() - offset : currentIndex->row();
		int newColumn = isRow ? currentIndex->column() : currentIndex->column() - offset;
		auto newIndex = createIndex( newRow, newColumn, currentIndex->internalData() );

		auto it = mPersistentHandles.find( *currentIndex );
		auto handle = std::move( it->second );

		handle->mIndex = newIndex;

		mPersistentHandles.erase( it );
		mPersistentHandles[std::move( newIndex )] = std::move( handle );
	}
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

Uint32 Model::subsribeModelStyler( const ModelStyler& styler ) {
	mStylers[++mLastStylerId] = styler;
	return mLastStylerId;
}

void Model::unsubsribeModelStyler( Uint32 id ) {
	mStylers.erase( id );
}

void Model::handleMove( Operation const& operation ) {
	bool isRow = operation.direction == Direction::Row;
	bool moveWithin = operation.sourceParent == operation.targetParent;
	bool movingDown = operation.target > operation.first;

	if ( moveWithin && operation.first == operation.target )
		return;

	if ( isRow ) {
		eeASSERT( operation.target <= (int)rowCount( operation.targetParent ) );
		eeASSERT( operation.last < (int)rowCount( operation.sourceParent ) );
	} else {
		eeASSERT( operation.target <= (int)columnCount( operation.targetParent ) );
		eeASSERT( operation.last < (int)columnCount( operation.sourceParent ) );
	}

	// NOTE: to_shift_down is used as a generic "to shift" when move_within is true.
	std::vector<const ModelIndex*> toMove;		// Items to be moved between the source and target
	std::vector<const ModelIndex*> toShiftDown; // Items to be shifted down after a move-to
	std::vector<const ModelIndex*> toShiftUp;	// Items to be shifted up after a move-from

	int count = operation.last - operation.first + 1;
	// [start, end)
	int workAreaStart = std::min( operation.first, operation.target );
	int work_area_end = std::max( operation.last + 1, operation.target + count );

	for ( auto& entry : mPersistentHandles ) {
		int dimension = isRow ? entry.first.row() : entry.first.column();

		if ( moveWithin ) {
			if ( entry.first.parent() == operation.sourceParent ) {
				if ( dimension >= operation.first && dimension <= operation.last ) {
					toMove.emplace_back( &entry.first );
				} else if ( movingDown && dimension > operation.last &&
							dimension < work_area_end ) {
					toShiftDown.emplace_back( &entry.first );
				} else if ( !movingDown && dimension >= workAreaStart &&
							dimension < operation.first ) {
					toShiftDown.emplace_back( &entry.first );
				}
			}
		} else {
			if ( entry.first.parent() == operation.sourceParent ) {
				if ( dimension >= operation.first && dimension <= operation.last ) {
					toMove.emplace_back( &entry.first );
				} else if ( dimension > operation.last ) {
					toShiftUp.emplace_back( &entry.first );
				}
			} else if ( entry.first.parent() == operation.targetParent ) {
				if ( dimension >= operation.target ) {
					toShiftDown.emplace_back( &entry.first );
				}
			}
		}
	}

	auto replaceHandle = [&]( ModelIndex const& currentIndex, int newDimension, bool relative ) {
		int newRow = isRow ? ( relative ? currentIndex.row() + newDimension : newDimension )
						   : currentIndex.row();
		int newColumn = !isRow ? ( relative ? currentIndex.column() + newDimension : newDimension )
							   : currentIndex.column();
		auto newIndex = index( newRow, newColumn, operation.targetParent );

		auto it = mPersistentHandles.find( currentIndex );
		auto handle = std::move( it->second );

		handle->mIndex = newIndex;

		mPersistentHandles.erase( it );
		mPersistentHandles[std::move( newIndex )] = std::move( handle );
	};

	for ( auto currentIndex : toMove ) {
		int dimension = isRow ? currentIndex->row() : currentIndex->column();
		int targetOffset = dimension - operation.first;
		int newDimension = operation.target + targetOffset;

		replaceHandle( *currentIndex, newDimension, false );
	}

	if ( moveWithin ) {
		for ( auto currentIndex : toShiftDown ) {
			int dimension = isRow ? currentIndex->row() : currentIndex->column();
			int targetOffset =
				movingDown ? dimension - ( operation.last + 1 ) : dimension - workAreaStart + count;
			int newDimension = workAreaStart + targetOffset;

			replaceHandle( *currentIndex, newDimension, false );
		}
	} else {
		for ( auto currentIndex : toShiftDown ) {
			replaceHandle( *currentIndex, count, true );
		}

		for ( auto currentIndex : toShiftUp ) {
			replaceHandle( *currentIndex, count, true );
		}
	}
}

}}} // namespace EE::UI::Models
