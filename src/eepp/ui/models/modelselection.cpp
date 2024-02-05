#include <eepp/core.hpp>
#include <eepp/ui/abstract/uiabstractview.hpp>
#include <eepp/ui/models/modelselection.hpp>

namespace EE { namespace UI { namespace Models {

void ModelSelection::removeAllMatching( std::function<bool( const ModelIndex& )> filter ) {
	std::vector<ModelIndex> notMatching;
	for ( auto& index : mIndexes ) {
		if ( !filter( index ) )
			notMatching.emplace_back( index );
	}
	if ( mIndexes.size() != notMatching.size() ) {
		mIndexes = std::move( notMatching );
		mView->notifySelectionChange();
	}
}

void ModelSelection::set( const ModelIndex& index ) {
	eeASSERT( index.isValid() );
	if ( mIndexes.size() == 1 && contains( index ) )
		return;
	mIndexes.clear();
	mIndexes.push_back( index );
	mView->notifySelectionChange();
}

void ModelSelection::set( const std::vector<ModelIndex>& indexes, bool notify ) {
#ifdef EE_DEBUG
	for ( auto& index : indexes )
		eeASSERT( index.isValid() );
#endif
	mIndexes.clear();
	mIndexes = indexes;
	if ( notify )
		mView->notifySelectionChange();
}

void ModelSelection::add( const ModelIndex& index ) {
	eeASSERT( index.isValid() );
	auto contains = std::find( mIndexes.begin(), mIndexes.end(), index );
	if ( contains == mIndexes.end() )
		return;
	mIndexes.push_back( index );
	mView->notifySelectionChange();
}

void ModelSelection::toggle( const ModelIndex& index ) {
	eeASSERT( index.isValid() );
	auto contains = std::find( mIndexes.begin(), mIndexes.end(), index );
	if ( contains != mIndexes.end() )
		mIndexes.erase( contains );
	else
		mIndexes.push_back( index );
	mView->notifySelectionChange();
}

bool ModelSelection::remove( const ModelIndex& index ) {
	eeASSERT( index.isValid() );
	auto contains = std::find( mIndexes.begin(), mIndexes.end(), index );
	if ( contains == mIndexes.end() )
		return false;
	mIndexes.erase( contains );
	mView->notifySelectionChange();
	return true;
}

void ModelSelection::clear( bool notify ) {
	if ( mIndexes.empty() )
		return;
	mIndexes.clear();
	if ( notify )
		mView->notifySelectionChange();
}

void ModelSelection::notifySelectionChanged() {
	if ( !mDisableNotify ) {
		mView->notifySelectionChange();
		mNotifyPending = false;
	} else {
		mNotifyPending = true;
	}
}

}}} // namespace EE::UI::Models
