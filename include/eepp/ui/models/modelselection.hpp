#ifndef EE_UI_MODEL_MODELSELECTION_HPP
#define EE_UI_MODEL_MODELSELECTION_HPP

#include <algorithm>
#include <eepp/system/lock.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/ui/models/modelindex.hpp>
#include <functional>

namespace EE { namespace UI { namespace Abstract {
class UIAbstractView;
}}} // namespace EE::UI::Abstract

using namespace EE::UI::Abstract;

namespace EE { namespace UI { namespace Models {

class EE_API ModelSelection {
  public:
	ModelSelection( UIAbstractView* view ) : mView( view ) {}

	int size() const {
		Lock l( mMutex );
		return mIndexes.size();
	}
	bool isEmpty() const {
		Lock l( mMutex );
		return mIndexes.empty();
	}
	bool contains( const ModelIndex& index ) const {
		Lock l( mMutex );
		return std::find( mIndexes.begin(), mIndexes.end(), index ) != mIndexes.end();
	}
	bool containsRow( int row ) const {
		Lock l( mMutex );
		for ( auto& index : mIndexes ) {
			if ( index.row() == row )
				return true;
		}
		return false;
	}

	void set( const ModelIndex& );
	void set( const std::vector<ModelIndex>& indexes, bool notify = true );
	void add( const ModelIndex& );
	void toggle( const ModelIndex& );
	bool remove( const ModelIndex& );
	void clear( bool notify = true );

	template <typename Callback> void forEachIndex( Callback callback ) {
		for ( auto& index : indexes() )
			callback( index );
	}

	template <typename Callback> void forEachIndex( Callback callback ) const {
		for ( auto& index : indexes() )
			callback( index );
	}

	std::vector<ModelIndex> indexes() const {
		Lock l( mMutex );
		std::vector<ModelIndex> indexes;
		indexes.reserve( mIndexes.size() );
		for ( auto& index : mIndexes )
			indexes.push_back( index );
		return indexes;
	}

	ModelIndex first() const {
		Lock l( mMutex );
		if ( mIndexes.empty() )
			return {};
		return *mIndexes.begin();
	}

	void removeAllMatching( std::function<bool( ModelIndex const& )> filter );

	template <typename Function> void changeFromModel( Function f ) {
		{
			mDisableNotify = true;
			mNotifyPending = false;
			f( *this );
			mDisableNotify = false;
		}
		if ( mNotifyPending )
			notifySelectionChanged();
	}

  protected:
	UIAbstractView* mView;
	std::vector<ModelIndex> mIndexes;
	bool mDisableNotify{ false };
	bool mNotifyPending{ false };
	void notifySelectionChanged();

	mutable Mutex mMutex;
};

}}} // namespace EE::UI::Models

#endif // EE_UI_MODEL_MODELSELECTION_HPP
