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
	/** PersistentModelIndex constructions that registered this handle and have not released it.
	 * The model keeps the handle while any registration remains. */
	Uint32 mRegistrations{ 0 };
};

class EE_API PersistentModelIndex {
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

	/** Gives back the registration made when this index was constructed from a ModelIndex, and
	 * detaches it. The model drops the handle, and stops updating it on every row operation,
	 * once all its registrations are released. Indexes that never release keep the handle
	 * alive: call this at most once per construction, never on a copy. */
	void release();

  private:
	std::weak_ptr<PersistentHandle> mHandle;
};

}}} // namespace EE::UI::Models

#endif // EE_UI_MODEL_PERSISTENTMODELINDEX_HPP
