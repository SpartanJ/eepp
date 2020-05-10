#ifndef EE_SYSTEMTCONTAINER_HPP
#define EE_SYSTEMTCONTAINER_HPP

#include <eepp/config.hpp>
#include <unordered_set>

namespace EE { namespace System {

/** @brief A simple resource container template, to keep track of the resources loaded. */
template <class T> class Container {
  public:
	Container();

	virtual ~Container();

	/** @brief Add to the list the resource. */
	T* add( T* resource );

	/** @brief Remove from the list the resource. */
	bool remove( T* resource );

	/** @returns The number of resources added to the container. */
	Uint32 count();

  protected:
	std::unordered_set<T*> mResources;
};

template <class T> Container<T>::Container() {}

template <class T> Container<T>::~Container() {}

template <class T> T* Container<T>::add( T* resource ) {
	if ( NULL != resource ) {
		mResources.insert( resource );

		return resource;
	}

	return NULL;
}

template <class T> bool Container<T>::remove( T* resource ) {
	if ( NULL != resource ) {
		mResources.erase( resource );

		return true;
	}

	return false;
}

template <class T> Uint32 Container<T>::count() {
	return mResources.size();
}

}} // namespace EE::System

#endif
