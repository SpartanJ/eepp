#ifndef EE_UI_MODEL_PERSISTENTMODELINDEX_HPP
#define EE_UI_MODEL_PERSISTENTMODELINDEX_HPP

#include <eepp/ui/models/model.hpp>
#include <eepp/ui/models/modelindex.hpp>

namespace EE { namespace UI { namespace Models {

/// A PersistentHandle is an internal data structure used to keep track of the
/// target of multiple PersistentModelIndex instances.
class PersistentHandle {
  public:
	friend class Model;
	friend class PersistentModelIndex;

	PersistentHandle( ModelIndex const& index ) : mIndex( index ) {}

	ModelIndex mIndex;
};

class PersistentModelIndex {
  public:
	PersistentModelIndex() {}
	PersistentModelIndex( ModelIndex const& );
	PersistentModelIndex( PersistentModelIndex const& ) = default;
	PersistentModelIndex( PersistentModelIndex&& ) = default;

	PersistentModelIndex& operator=( PersistentModelIndex const& ) = default;
	PersistentModelIndex& operator=( PersistentModelIndex&& ) = default;

	bool isValid() const { return hasValidHandle() && mHandle.lock()->mIndex.isValid(); }
	bool hasValidHandle() const { return !mHandle.expired(); }

	int row() const;
	int column() const;
	PersistentModelIndex parent() const;
	PersistentModelIndex siblingAtColumn( int column ) const;
	Variant data( ModelRole = ModelRole::Display ) const;

	void* internalData() const {
		if ( hasValidHandle() )
			return mHandle.lock()->mIndex.internalData();
		else
			return nullptr;
	}

	operator ModelIndex() const;
	bool operator==( PersistentModelIndex const& ) const;
	bool operator!=( PersistentModelIndex const& ) const;
	bool operator==( ModelIndex const& ) const;
	bool operator!=( ModelIndex const& ) const;

  private:
	std::weak_ptr<PersistentHandle> mHandle;
};

}}} // namespace EE::UI::Models

#endif // EE_UI_MODEL_PERSISTENTMODELINDEX_HPP
