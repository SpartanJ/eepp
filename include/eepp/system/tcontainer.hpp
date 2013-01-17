#ifndef EE_SYSTEMTCONTAINER_HPP
#define EE_SYSTEMTCONTAINER_HPP

#include <eepp/system/base.hpp>
#include <list>

namespace EE { namespace System {

/** @brief A simple resource container template, to keep track of the resources loaded. */
template <class T>
class tContainer {
	public:
		tContainer();

		virtual ~tContainer();

		/** @brief Add to the list the resource. */
		T * Add( T * Resource );

		/** @brief Remove from the list the resource. */
		bool Remove( T * Resource );

		/** @returns The number of resources added to the container. */
		Uint32 Count();
	protected:
		std::list<T*> mResources;
};

template <class T>
tContainer<T>::tContainer()
{
}

template <class T>
tContainer<T>::~tContainer()
{
}

template <class T>
T * tContainer<T>::Add( T * Resource ) {
	if ( NULL != Resource ) {
		mResources.push_back( Resource );

		return Resource;
	}

	return NULL;
}

template <class T>
bool tContainer<T>::Remove( T * Resource ) {
	if ( NULL != Resource ) {
		mResources.remove( Resource );

		return true;
	}

	return false;
}

template <class T>
Uint32 tContainer<T>::Count() {
	return mResources.size();
}

}}

#endif


