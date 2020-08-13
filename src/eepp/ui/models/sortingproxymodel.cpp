#include <algorithm>
#include <eepp/ui/abstract/uiabstractview.hpp>
#include <eepp/ui/models/modelselection.hpp>
#include <eepp/ui/models/sortingproxymodel.hpp>
#include <eepp/ui/models/variant.hpp>

using namespace EE::UI::Abstract;

namespace EE { namespace UI { namespace Models {

SortingProxyModel::SortingProxyModel( std::shared_ptr<Model> target ) :
	mTarget( target ), mKeyColumn( -1 ) {
	mTarget->registerClient( this );
	resort();
}

SortingProxyModel::~SortingProxyModel() {
	mTarget->unregisterClient( this );
}

void SortingProxyModel::onModelUpdated( unsigned flags ) {
	resort( flags );
}

Model& SortingProxyModel::target() {
	return *mTarget;
}

const Model& SortingProxyModel::target() const {
	return *mTarget;
}

size_t SortingProxyModel::rowCount( const ModelIndex& index ) const {
	auto targetIndex = mapToTarget( index );
	return target().rowCount( targetIndex );
}

size_t SortingProxyModel::columnCount( const ModelIndex& index ) const {
	auto targetIndex = mapToTarget( index );
	return target().columnCount( targetIndex );
}

ModelIndex SortingProxyModel::mapToTarget( const ModelIndex& index ) const {
	if ( !index.isValid() )
		return {};
	if ( static_cast<size_t>( index.row() ) >= mRowMappings.size() ||
		 static_cast<size_t>( index.column() ) >= columnCount() )
		return {};
	return target().index( mRowMappings[index.row()], index.column() );
}

Model::Role SortingProxyModel::sortRole() const {
	return mSortRole;
}

void SortingProxyModel::setSortRrole( Model::Role role ) {
	mSortRole = role;
}

std::string SortingProxyModel::columnName( const size_t& column ) const {
	return target().columnName( column );
}

Variant SortingProxyModel::data( const ModelIndex& index, Role role ) const {
	auto targetIndex = mapToTarget( index );
	eeASSERT( targetIndex.isValid() );
	return target().data( targetIndex, role );
}

void SortingProxyModel::update() {
	target().update();
}

int SortingProxyModel::keyColumn() const {
	return mKeyColumn;
}

SortOrder SortingProxyModel::sortOrder() const {
	return mSortOrder;
}

void SortingProxyModel::setKeyColumnAndSortOrder( const size_t& column,
												  const SortOrder& sortOrder ) {
	if ( column == (size_t)mKeyColumn && sortOrder == mSortOrder )
		return;
	eeASSERT( column >= 0 && column < columnCount() );
	mKeyColumn = column;
	mSortOrder = sortOrder;
	resort();
}

void SortingProxyModel::resort( unsigned flags ) {
	mSorting = true;
	auto old_row_mappings = mRowMappings;
	int rowCount = target().rowCount();
	mRowMappings.resize( rowCount );
	for ( int i = 0; i < rowCount; ++i )
		mRowMappings[i] = i;
	if ( mKeyColumn == -1 ) {
		onModelUpdate( flags );
		return;
	}
	std::sort( mRowMappings.begin(), mRowMappings.end(), [&]( auto row1, auto row2 ) -> bool {
		Variant data1 = target().data( target().index( row1, mKeyColumn ), mSortRole );
		Variant data2 = target().data( target().index( row2, mKeyColumn ), mSortRole );
		if ( data1 == data2 )
			return 0;
		bool isLessThan;
		if ( !mSortingCaseSensitive && data1.is( Variant::Type::String ) &&
			 data2.is( Variant::Type::String ) ) {
			isLessThan = String::toLower( data1.asString() ) < String::toLower( data2.asString() );
		} else if ( !mSortingCaseSensitive && data1.is( Variant::Type::cstr ) &&
					data2.is( Variant::Type::cstr ) ) {
			isLessThan = String::toLower( std::string( data1.asCStr() ) ) <
						 String::toLower( std::string( data2.asCStr() ) );
		} else {
			isLessThan = data1 < data2;
		}
		return mSortOrder == SortOrder::Ascending ? isLessThan : !isLessThan;
	} );
	forEachView( [&]( UIAbstractView* view ) {
		view->getSelection().changeFromModel( [&]( ModelSelection& selection ) {
			std::vector<ModelIndex> selectedIndexesInTarget;
			selection.forEachIndex( [&]( const ModelIndex& index ) {
				selectedIndexesInTarget.emplace_back(
					target().index( old_row_mappings[index.row()], index.column() ) );
			} );

			selection.clear();
			for ( auto& index : selectedIndexesInTarget ) {
				for ( size_t i = 0; i < mRowMappings.size(); ++i ) {
					if ( mRowMappings[i] == index.row() ) {
						selection.add( this->index( i, index.column() ) );
						continue;
					}
				}
			}
		} );
	} );
	onModelUpdate( flags );
	mSorting = false;
}

void SortingProxyModel::setSortingCaseSensitive( bool b ) {
	mSortingCaseSensitive = b;
}

bool SortingProxyModel::isSortingCaseSensitive() {
	return mSortingCaseSensitive;
}

bool SortingProxyModel::isColumnSortable( const size_t& columnIndex ) const {
	return target().isColumnSortable( columnIndex );
}

}}} // namespace EE::UI::Models
