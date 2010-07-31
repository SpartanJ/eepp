#ifndef EE_SYSTEMTRESOURCEMANAGER_HPP
#define EE_SYSTEMTRESOURCEMANAGER_HPP

#include "base.hpp"

namespace EE { namespace System {

template <class T>
class tResourceManager {
	public:
		tResourceManager();

		virtual ~tResourceManager();

		void Add( T * Resource );

		void Remove( T * Resource );

		T * GetByName( const std::string& Name );

		T * GetById( const Uint32& id );

		Uint32 Count();

		void Reload();

		Uint32 Exists( const std::string& name );

		void Destroy();
	protected:
		std::map<std::string, T*> mResources;
};

template <class T>
tResourceManager<T>::tResourceManager()
{
}

template <class T>
tResourceManager<T>::~tResourceManager() {
	Destroy();
}

template <class T>
void tResourceManager<T>::Destroy() {
	typename std::map<std::string, T*>::iterator it;

	for ( it = mResources.begin() ; it != mResources.end(); it++ )
		eeSAFE_DELETE( it->second );
}

template <class T>
void tResourceManager<T>::Add( T * Resource ) {
	Uint32 c = mResources.count( Resource->Name() );

	if ( 0 == c ) {
		mResources[ Resource->Name() ] = Resource;
	} else {
		Resource->Name( Resource->Name() + intToStr( c + 1 ) );

		Add( Resource );
	}
}

template <class T>
void tResourceManager<T>::Remove( T * Resource ) {
	mResources.erase( Resource->Name() );
}

template <class T>
Uint32 tResourceManager<T>::Exists( const std::string& name ) {
	return mResources.count( name );
}

template <class T>
T * tResourceManager<T>::GetByName( const std::string& Name ) {
	typename std::map<std::string, T*>::iterator it = mResources.find( Name );

	if ( mResources.end() != it ) {
		return it->second;
	}

	return NULL;
}

template <class T>
T * tResourceManager<T>::GetById( const Uint32& id ) {
	typename std::map<std::string, T*>::iterator it;

	for ( it = mResources.begin(); it != mResources.end(); it++ ) {
		T * sp = reinterpret_cast< T* > ( it->second );

		if ( id == sp->Id() )
			return sp;
	}

	return NULL;
}

template <class T>
Uint32 tResourceManager<T>::Count() {
	return mResources.size();
}

}}

#endif

