#include <algorithm>
#include <eepp/ui/abstract/uiabstractview.hpp>
#include <eepp/ui/models/modelselection.hpp>
#include <eepp/ui/models/sortingproxymodel.hpp>
#include <eepp/ui/models/variant.hpp>

using namespace EE::UI::Abstract;

namespace EE { namespace UI { namespace Models {

SortingProxyModel::SortingProxyModel( std::shared_ptr<Model> target ) :
	mSource( target ), mKeyColumn( -1 ) {
	mSource->registerClient( this );
	invalidate();
}

SortingProxyModel::~SortingProxyModel() {
	mSource->unregisterClient( this );
}

void SortingProxyModel::invalidate( unsigned int flags ) {
	if ( flags == UpdateFlag::DontInvalidateIndexes ) {
		sort( mKeyColumn, mSortOrder );
	} else {
		mMappings.clear();

		// FIXME: This is really harsh, but without precise invalidation, not much we can do.
		forEachView( []( UIAbstractView* view ) { view->getSelection().clear( false ); } );
		forEachView( []( UIAbstractView* view ) { view->notifySelectionChange(); } );
	}
	onModelUpdate( flags );
}

void SortingProxyModel::onModelUpdated( unsigned flags ) {
	invalidate( flags );
}

Model& SortingProxyModel::source() {
	return *mSource;
}

const Model& SortingProxyModel::source() const {
	return *mSource;
}

size_t SortingProxyModel::rowCount( const ModelIndex& index ) const {
	auto targetIndex = mapToSource( index );
	return source().rowCount( targetIndex );
}

size_t SortingProxyModel::columnCount( const ModelIndex& index ) const {
	auto targetIndex = mapToSource( index );
	return source().columnCount( targetIndex );
}

ModelIndex SortingProxyModel::mapToSource( const ModelIndex& proxyIndex ) const {
	if ( !proxyIndex.isValid() )
		return {};

	eeASSERT( proxyIndex.model() == this );
	eeASSERT( proxyIndex.internalData() );

	auto& indexMapping = *static_cast<Mapping*>( proxyIndex.internalData() );
	auto it = mMappings.find( indexMapping.sourceParent );
	eeASSERT( it != mMappings.end() );

	auto& mapping = *it->second;
	if ( static_cast<size_t>( proxyIndex.row() ) >= mapping.sourceRows.size() ||
		 proxyIndex.column() >= (Int64)columnCount() )
		return {};
	int sourceRow = mapping.sourceRows[proxyIndex.row()];
	int sourceColumn = proxyIndex.column();
	return source().index( sourceRow, sourceColumn, it->first );
}

SortingProxyModel::InternalMapIterator
SortingProxyModel::buildMapping( const ModelIndex& sourceParent ) {
	auto it = mMappings.find( sourceParent );
	if ( it != mMappings.end() )
		return it;

	auto mapping = std::make_shared<Mapping>();

	mapping->sourceParent = sourceParent;

	int rowCount = source().rowCount( sourceParent );
	mapping->sourceRows.resize( rowCount );
	mapping->proxyRows.resize( rowCount );

	sortMapping( *mapping, mKeyColumn, mSortOrder );

	if ( sourceParent.isValid() ) {
		auto sourceGrandParent = sourceParent.parent();
		buildMapping( sourceGrandParent );
	}
	mMappings.insert( { sourceParent, mapping } );
	return mMappings.find( sourceParent );
}

ModelIndex SortingProxyModel::mapToProxy( const ModelIndex& sourceIndex ) const {
	if ( !sourceIndex.isValid() )
		return {};

	eeASSERT( sourceIndex.model() == mSource.get() );

	auto sourceParent = sourceIndex.parent();
	auto it = const_cast<SortingProxyModel*>( this )->buildMapping( sourceParent );

	auto& mapping = *( it->second );

	if ( sourceIndex.row() >= static_cast<int>( mapping.proxyRows.size() ) ||
		 sourceIndex.column() >= (Int64)columnCount() )
		return {};

	int proxyRow = mapping.proxyRows[sourceIndex.row()];
	int proxyColumn = sourceIndex.column();
	if ( proxyRow < 0 || proxyColumn < 0 )
		return {};
	return createIndex( proxyRow, proxyColumn, &mapping );
}

ModelRole SortingProxyModel::sortRole() const {
	return mSortRole;
}

void SortingProxyModel::setSortRrole( ModelRole role ) {
	mSortRole = role;
}

std::string SortingProxyModel::columnName( const size_t& column ) const {
	return source().columnName( column );
}

Variant SortingProxyModel::data( const ModelIndex& proxyIndex, ModelRole role ) const {
	auto targetIndex = mapToSource( proxyIndex );
	eeASSERT( targetIndex.isValid() );
	return source().data( targetIndex, role );
}

ModelIndex SortingProxyModel::index( int row, int column, const ModelIndex& parent ) const {
	if ( row < 0 || column < 0 )
		return {};

	auto sourceParent = mapToSource( parent );
	const_cast<SortingProxyModel*>( this )->buildMapping( sourceParent );

	auto it = mMappings.find( sourceParent );
	eeASSERT( it != mMappings.end() );
	auto& mapping = *it->second;
	if ( row >= static_cast<int>( mapping.sourceRows.size() ) || column >= (Int64)columnCount() )
		return {};
	return createIndex( row, column, &mapping );
}

ModelIndex SortingProxyModel::parentIndex( const ModelIndex& proxyIndex ) const {
	if ( !proxyIndex.isValid() )
		return {};

	eeASSERT( proxyIndex.model() == this );
	eeASSERT( proxyIndex.internalData() );

	auto& index_mapping = *static_cast<Mapping*>( proxyIndex.internalData() );
	auto it = mMappings.find( index_mapping.sourceParent );
	eeASSERT( it != mMappings.end() );

	return mapToProxy( it->second->sourceParent );
}

void SortingProxyModel::update() {
	source().update();
}

int SortingProxyModel::keyColumn() const {
	return mKeyColumn;
}

size_t SortingProxyModel::treeColumn() const {
	return source().treeColumn();
}

SortOrder SortingProxyModel::sortOrder() const {
	return mSortOrder;
}

void SortingProxyModel::sort( const size_t& column, const SortOrder& sortOrder ) {
	for ( auto& it : mMappings ) {
		auto& mapping = *it.second;
		sortMapping( mapping, column, sortOrder );
	}

	mKeyColumn = column;
	mSortOrder = sortOrder;

	onModelUpdate( UpdateFlag::DontInvalidateIndexes );
}

void SortingProxyModel::setSortingCaseSensitive( bool b ) {
	mSortingCaseSensitive = b;
}

bool SortingProxyModel::isSortingCaseSensitive() {
	return mSortingCaseSensitive;
}

std::shared_ptr<Model> SortingProxyModel::getSource() const {
	return mSource;
}

bool SortingProxyModel::classModelRoleEnabled() {
	return source().classModelRoleEnabled();
}

bool SortingProxyModel::isColumnSortable( const size_t& columnIndex ) const {
	return source().isColumnSortable( columnIndex );
}

bool SortingProxyModel::lessThan( const ModelIndex& index1, const ModelIndex& index2 ) const {
	auto data1 = mSource->data( index1, mSortRole );
	auto data2 = mSource->data( index2, mSortRole );
	if ( data1.isString() && data2.isString() ) {
		if ( data1.is( Variant::Type::StdString ) && data2.is( Variant::Type::StdString ) )
			return String::toLower( data1.asStdString() ) < String::toLower( data2.asStdString() );
		if ( data1.is( Variant::Type::String ) && data2.is( Variant::Type::String ) )
			return String::toLower( data1.asString() ) < String::toLower( data2.asString() );
		if ( data1.is( Variant::Type::cstr ) && data2.is( Variant::Type::cstr ) )
			return String::toLower( std::string( data1.asCStr() ) ) <
				   String::toLower( std::string( data2.asCStr() ) );
	}
	return data1 < data2;
}

void SortingProxyModel::sortMapping( SortingProxyModel::Mapping& mapping, int column,
									 SortOrder sortOrder ) {
	if ( column == -1 ) {
		int rowCount = source().rowCount( mapping.sourceParent );
		for ( int i = 0; i < rowCount; ++i ) {
			mapping.sourceRows[i] = i;
			mapping.proxyRows[i] = i;
		}
		return;
	}

	auto oldSourceRows = mapping.sourceRows;

	int rowCount = source().rowCount( mapping.sourceParent );
	for ( int i = 0; i < rowCount; ++i )
		mapping.sourceRows[i] = i;

	std::stable_sort(
		mapping.sourceRows.begin(), mapping.sourceRows.end(), [&]( auto row1, auto row2 ) -> bool {
			bool isLessThan =
				this->lessThan( mSource->index( row1, column, mapping.sourceParent ),
								mSource->index( row2, column, mapping.sourceParent ) );
			return sortOrder == SortOrder::Ascending ? isLessThan : !isLessThan;
		} );

	for ( int i = 0; i < rowCount; ++i )
		mapping.proxyRows[mapping.sourceRows[i]] = i;

	// FIXME: I really feel like this should be done at the view layer somehow.
	forEachView( [&]( UIAbstractView* view ) {
		// Update the view's selection.
		view->getSelection().changeFromModel( [&]( ModelSelection& selection ) {
			std::vector<ModelIndex> selectedIndexesInSource;
			std::vector<ModelIndex> staleIndexesInSelection;
			selection.forEachIndex( [&]( const ModelIndex& index ) {
				if ( index.parent() == mapping.sourceParent ) {
					staleIndexesInSelection.push_back( index );
					selectedIndexesInSource.push_back( source().index(
						oldSourceRows[index.row()], index.column(), mapping.sourceParent ) );
				}
			} );

			for ( auto& index : staleIndexesInSelection )
				selection.remove( index );

			for ( auto& index : selectedIndexesInSource ) {
				for ( size_t i = 0; i < mapping.sourceRows.size(); ++i ) {
					if ( mapping.sourceRows[i] == index.row() ) {
						auto newSourceIndex =
							this->index( i, index.column(), mapping.sourceParent );
						selection.add( newSourceIndex );
						break;
					}
				}
			}
		} );
	} );
}

}}} // namespace EE::UI::Models
