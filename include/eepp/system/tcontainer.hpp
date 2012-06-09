#ifndef EE_SYSTEMTCONTAINER_HPP
#define EE_SYSTEMTCONTAINER_HPP

#include <eepp/system/base.hpp>

namespace EE { namespace System {

template <class T>
class tContainer {
	public:
		tContainer();

		virtual ~tContainer();

		T * Add( T * Resource );

		bool Remove( T * Resource );

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


