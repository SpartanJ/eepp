#include <algorithm>
#include <eepp/ui/abstract/uiabstractview.hpp>
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
	struct ViewSelection {
		UIAbstractView* view;
		std::vector<ModelIndex> sourceIndexes;
	};
	auto removeInvalid = []( std::vector<ModelIndex>& indexes ) {
		indexes.erase( std::remove_if( indexes.begin(), indexes.end(),
									   []( const ModelIndex& index ) { return !index.isValid(); } ),
					   indexes.end() );
	};
	std::vector<ViewSelection> selections;
	forEachView( [&]( UIAbstractView* view ) {
		if ( view->getSelection().isEmpty() )
			return;
		auto selected = view->getSelection().indexes();
		if ( selected.empty() )
			return;
		for ( auto& index : selected )
			index = mapToSource( index );
		removeInvalid( selected );
		selections.push_back( { view, std::move( selected ) } );
	} );

	for ( auto& it : mMappings ) {
		auto& mapping = *it.second;
		sortMapping( mapping, column, sortOrder );
	}

	mKeyColumn = column;
	mSortOrder = sortOrder;

	for ( auto& selection : selections ) {
		for ( auto& index : selection.sourceIndexes )
			index = mapToProxy( index );
		removeInvalid( selection.sourceIndexes );
		selection.view->getSelection().set( selection.sourceIndexes, false );
		selection.view->notifySelectionChange();
	}

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
		if ( data1.isStdStringLike() && data2.isStdStringLike() )
			return String::toLower( std::string{ data1.asStdStringView() } ) <
				   String::toLower( std::string{ data2.asStdStringView() } );
		if ( data1.is( Variant::Type::String ) && data2.is( Variant::Type::String ) )
			return String::toLower( data1.asString() ) < String::toLower( data2.asString() );
		if ( data1.is( Variant::Type::StringPtr ) && data2.is( Variant::Type::StringPtr ) )
			return String::toLower( data1.asStringPtr() ) < String::toLower( data2.asStringPtr() );
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

	int rowCount = source().rowCount( mapping.sourceParent );
	for ( int i = 0; i < rowCount; ++i )
		mapping.sourceRows[i] = i;

	std::stable_sort( mapping.sourceRows.begin(), mapping.sourceRows.end(),
					  [&]( auto row1, auto row2 ) -> bool {
						  const auto first = mSource->index( row1, column, mapping.sourceParent );
						  const auto second = mSource->index( row2, column, mapping.sourceParent );
						  return sortOrder == SortOrder::Ascending ? lessThan( first, second )
																   : lessThan( second, first );
					  } );

	for ( int i = 0; i < rowCount; ++i )
		mapping.proxyRows[mapping.sourceRows[i]] = i;
}

}}} // namespace EE::UI::Models
