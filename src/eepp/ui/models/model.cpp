#include <eepp/ui/abstract/uiabstractview.hpp>
#include <eepp/ui/models/model.hpp>
#include <eepp/ui/models/persistentmodelindex.hpp>

namespace EE { namespace UI { namespace Models {

void Model::onModelUpdate( unsigned flags ) {
	if ( mOnUpdate )
		mOnUpdate();
	for ( auto& client : mClients )
		client->onModelUpdated( flags );
	forEachView( [&]( UIAbstractView* view ) { view->onModelUpdate( flags ); } );
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
	forEachView( [&]( UIAbstractView* view ) { view->invalidateDraw(); } );
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

void Model::beginMoveRows( ModelIndex const& source_parent, int first, int last,
						   ModelIndex const& target_parent, int target_index ) {
	eeASSERT( first >= 0 );
	eeASSERT( first <= last );
	eeASSERT( target_index >= 0 );
	mOperationStack.push( { OperationType::Move, Direction::Row, source_parent, first, last,
							target_parent, target_index } );
}

void Model::beginMoveColumns( ModelIndex const& source_parent, int first, int last,
							  ModelIndex const& target_parent, int target_index ) {
	eeASSERT( first >= 0 );
	eeASSERT( first <= last );
	eeASSERT( target_index >= 0 );
	mOperationStack.push( { OperationType::Move, Direction::Column, source_parent, first, last,
							target_parent, target_index } );
}

void Model::beginDeleteRows( ModelIndex const& parent, int first, int last ) {
	eeASSERT( first >= 0 );
	eeASSERT( first <= last );
	eeASSERT( (size_t)last < rowCount( parent ) );

	saveDeletedIndices<true>( parent, first, last );
	mOperationStack.push( { OperationType::Delete, Direction::Row, parent, first, last } );
}

void Model::beginDeleteColumns( ModelIndex const& parent, int first, int last ) {
	eeASSERT( first >= 0 );
	eeASSERT( first <= last );
	eeASSERT( (size_t)last < columnCount( parent ) );

	saveDeletedIndices<false>( parent, first, last );
	mOperationStack.push( { OperationType::Delete, Direction::Column, parent, first, last } );
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
	std::vector<ModelIndex> deleted_indices;

	for ( auto& entry : mPersistentHandles ) {
		auto current_index = entry.first;

		// Walk up the persistent handle's parents to see if it is contained
		// within the range that is being deleted.
		while ( current_index.isValid() ) {
			auto current_parent = current_index.parent();

			if ( current_parent == parent ) {
				if ( IsRow ) {
					if ( current_index.row() >= first && current_index.row() <= last )
						deleted_indices.emplace_back( current_index );
				} else {
					if ( current_index.column() >= first && current_index.column() <= last )
						deleted_indices.emplace_back( current_index );
				}
			}

			current_index = current_parent;
		}
	}

	mDeletedIndicesStack.push( std::move( deleted_indices ) );
}

void Model::endInsertRows() {
	auto operation = mOperationStack.top();
	mOperationStack.pop();
	eeASSERT( operation.type == OperationType::Insert );
	eeASSERT( operation.direction == Direction::Row );
	handleInsert( operation );

	for ( auto& client : mClients ) {
		client->modelDidInsertRows( operation.source_parent, operation.first, operation.last );
	}
}

void Model::endInsertColumns() {
	auto operation = mOperationStack.top();
	mOperationStack.pop();
	eeASSERT( operation.type == OperationType::Insert );
	eeASSERT( operation.direction == Direction::Column );
	handleInsert( operation );

	for ( auto& client : mClients )
		client->modelDidInsertColumns( operation.source_parent, operation.first, operation.last );
}

void Model::endMoveRows() {
	auto operation = mOperationStack.top();
	mOperationStack.pop();
	eeASSERT( operation.type == OperationType::Move );
	eeASSERT( operation.direction == Direction::Row );
	handleMove( operation );

	for ( auto& client : mClients )
		client->modelDidMoveRows( operation.source_parent, operation.first, operation.last,
								  operation.target_parent, operation.target );
}

void Model::endMoveColumns() {
	auto operation = mOperationStack.top();
	mOperationStack.pop();
	eeASSERT( operation.type == OperationType::Move );
	eeASSERT( operation.direction == Direction::Column );
	handleMove( operation );

	for ( auto& client : mClients ) {
		client->modeldidMoveColumns( operation.source_parent, operation.first, operation.last,
									 operation.target_parent, operation.target );
	}
}

void Model::endDeleteRows() {
	auto operation = mOperationStack.top();
	mOperationStack.pop();
	eeASSERT( operation.type == OperationType::Delete );
	eeASSERT( operation.direction == Direction::Row );
	handleDelete( operation );

	for ( auto& client : mClients ) {
		client->modelDidDeleteRows( operation.source_parent, operation.first, operation.last );
	}
}

void Model::endDeleteColumns() {
	auto operation = mOperationStack.top();
	mOperationStack.pop();
	eeASSERT( operation.type == OperationType::Delete );
	eeASSERT( operation.direction == Direction::Column );
	handleDelete( operation );

	for ( auto& client : mClients ) {
		client->modelDidDeleteColumns( operation.source_parent, operation.first, operation.last );
	}
}

void Model::handleInsert( Operation const& operation ) {
	bool is_row = operation.direction == Direction::Row;
	std::vector<const ModelIndex*> to_shift;

	for ( auto& entry : mPersistentHandles ) {
		if ( entry.first.parent() == operation.source_parent ) {
			if ( is_row && entry.first.row() >= operation.first ) {
				to_shift.emplace_back( &entry.first );
			} else if ( !is_row && entry.first.column() >= operation.first ) {
				to_shift.emplace_back( &entry.first );
			}
		}
	}

	int offset = operation.last - operation.first + 1;

	for ( auto current_index : to_shift ) {
		int new_row = is_row ? current_index->row() + offset : current_index->row();
		int new_column = is_row ? current_index->column() : current_index->column() + offset;
		auto new_index = createIndex( new_row, new_column, current_index->internalData() );

		auto it = mPersistentHandles.find( *current_index );
		auto handle = std::move( it->second );

		handle->m_index = new_index;

		mPersistentHandles.erase( it );
		mPersistentHandles[std::move( new_index )] = std::move( handle );
	}
}

void Model::handleDelete( Operation const& operation ) {
	bool is_row = operation.direction == Direction::Row;
	std::vector<ModelIndex> deleted_indices = mDeletedIndicesStack.top();
	mDeletedIndicesStack.pop();
	std::vector<const ModelIndex*> to_shift;

	// Get rid of all persistent handles which have been marked for death
	for ( auto& deleted_index : deleted_indices ) {
		mPersistentHandles.erase( deleted_index );
	}

	for ( auto& entry : mPersistentHandles ) {
		if ( entry.first.parent() == operation.source_parent ) {
			if ( is_row ) {
				if ( entry.first.row() > operation.last ) {
					to_shift.emplace_back( &entry.first );
				}
			} else {
				if ( entry.first.column() > operation.last ) {
					to_shift.emplace_back( &entry.first );
				}
			}
		}
	}

	int offset = operation.last - operation.first + 1;

	for ( auto current_index : to_shift ) {
		int new_row = is_row ? current_index->row() - offset : current_index->row();
		int new_column = is_row ? current_index->column() : current_index->column() - offset;
		auto new_index = createIndex( new_row, new_column, current_index->internalData() );

		auto it = mPersistentHandles.find( *current_index );
		auto handle = std::move( it->second );

		handle->m_index = new_index;

		mPersistentHandles.erase( it );
		mPersistentHandles[std::move( new_index )] = std::move( handle );
	}
}

void Model::handleMove( Operation const& operation ) {
	bool is_row = operation.direction == Direction::Row;
	bool move_within = operation.source_parent == operation.target_parent;
	bool moving_down = operation.target > operation.first;

	if ( move_within && operation.first == operation.target )
		return;

	if ( is_row ) {
		eeASSERT( operation.target <= (int)rowCount( operation.target_parent ) );
		eeASSERT( operation.last < (int)rowCount( operation.source_parent ) );
	} else {
		eeASSERT( operation.target <= (int)columnCount( operation.target_parent ) );
		eeASSERT( operation.last < (int)columnCount( operation.source_parent ) );
	}

	// NOTE: to_shift_down is used as a generic "to shift" when move_within is true.
	std::vector<const ModelIndex*> to_move;		  // Items to be moved between the source and target
	std::vector<const ModelIndex*> to_shift_down; // Items to be shifted down after a move-to
	std::vector<const ModelIndex*> to_shift_up;	  // Items to be shifted up after a move-from

	int count = operation.last - operation.first + 1;
	// [start, end)
	int work_area_start = std::min( operation.first, operation.target );
	int work_area_end = std::max( operation.last + 1, operation.target + count );

	for ( auto& entry : mPersistentHandles ) {
		int dimension = is_row ? entry.first.row() : entry.first.column();

		if ( move_within ) {
			if ( entry.first.parent() == operation.source_parent ) {
				if ( dimension >= operation.first && dimension <= operation.last ) {
					to_move.emplace_back( &entry.first );
				} else if ( moving_down && dimension > operation.last &&
							dimension < work_area_end ) {
					to_shift_down.emplace_back( &entry.first );
				} else if ( !moving_down && dimension >= work_area_start &&
							dimension < operation.first ) {
					to_shift_down.emplace_back( &entry.first );
				}
			}
		} else {
			if ( entry.first.parent() == operation.source_parent ) {
				if ( dimension >= operation.first && dimension <= operation.last ) {
					to_move.emplace_back( &entry.first );
				} else if ( dimension > operation.last ) {
					to_shift_up.emplace_back( &entry.first );
				}
			} else if ( entry.first.parent() == operation.target_parent ) {
				if ( dimension >= operation.target ) {
					to_shift_down.emplace_back( &entry.first );
				}
			}
		}
	}

	auto replace_handle = [&]( ModelIndex const& current_index, int new_dimension, bool relative ) {
		int new_row = is_row ? ( relative ? current_index.row() + new_dimension : new_dimension )
							 : current_index.row();
		int new_column = !is_row
							 ? ( relative ? current_index.column() + new_dimension : new_dimension )
							 : current_index.column();
		auto new_index = index( new_row, new_column, operation.target_parent );

		auto it = mPersistentHandles.find( current_index );
		auto handle = std::move( it->second );

		handle->m_index = new_index;

		mPersistentHandles.erase( it );
		mPersistentHandles[std::move( new_index )] = std::move( handle );
	};

	for ( auto current_index : to_move ) {
		int dimension = is_row ? current_index->row() : current_index->column();
		int target_offset = dimension - operation.first;
		int new_dimension = operation.target + target_offset;

		replace_handle( *current_index, new_dimension, false );
	}

	if ( move_within ) {
		for ( auto current_index : to_shift_down ) {
			int dimension = is_row ? current_index->row() : current_index->column();
			int target_offset = moving_down ? dimension - ( operation.last + 1 )
											: dimension - work_area_start + count;
			int new_dimension = work_area_start + target_offset;

			replace_handle( *current_index, new_dimension, false );
		}
	} else {
		for ( auto current_index : to_shift_down ) {
			replace_handle( *current_index, count, true );
		}

		for ( auto current_index : to_shift_up ) {
			replace_handle( *current_index, count, true );
		}
	}
}

}}} // namespace EE::UI::Models
