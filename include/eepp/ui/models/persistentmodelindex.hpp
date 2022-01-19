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

	PersistentHandle( ModelIndex const& index ) : m_index( index ) {}

	ModelIndex m_index;
};

class PersistentModelIndex {
  public:
	PersistentModelIndex() {}
	PersistentModelIndex( ModelIndex const& );
	PersistentModelIndex( PersistentModelIndex const& ) = default;
	PersistentModelIndex( PersistentModelIndex&& ) = default;

	PersistentModelIndex& operator=( PersistentModelIndex const& ) = default;
	PersistentModelIndex& operator=( PersistentModelIndex&& ) = default;

	bool isValid() const { return hasValidHandle() && m_handle.lock()->m_index.isValid(); }
	bool hasValidHandle() const { return !m_handle.expired(); }

	int row() const;
	int column() const;
	PersistentModelIndex parent() const;
	PersistentModelIndex siblingAtColumn( int column ) const;
	Variant data( ModelRole = ModelRole::Display ) const;

	void* internalData() const {
		if ( hasValidHandle() )
			return m_handle.lock()->m_index.internalData();
		else
			return nullptr;
	}

	operator ModelIndex() const;
	bool operator==( PersistentModelIndex const& ) const;
	bool operator!=( PersistentModelIndex const& ) const;
	bool operator==( ModelIndex const& ) const;
	bool operator!=( ModelIndex const& ) const;

  private:
	std::weak_ptr<PersistentHandle> m_handle;
};

}}} // namespace EE::UI::Models

#endif // EE_UI_MODEL_PERSISTENTMODELINDEX_HPP
