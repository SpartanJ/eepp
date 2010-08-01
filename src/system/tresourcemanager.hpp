#ifndef EE_SYSTEMTRESOURCEMANAGER_HPP
#define EE_SYSTEMTRESOURCEMANAGER_HPP

#include "base.hpp"

namespace EE { namespace System {

template <class T>
class tResourceManager {
	public:
		tResourceManager( bool UniqueId = true );

		virtual ~tResourceManager();

		T * Add( T * Resource );

		bool Remove( T * Resource, bool Delete = true );

		bool RemoveById( const Uint32& Id, bool Delete = true );

		bool RemoveByName( const std::string& Name, bool Delete = true );

		T * GetByName( const std::string& Name );

		T * GetById( const Uint32& Id );

		Uint32 Count();

		Uint32 Count( const std::string& Name );

		Uint32 Count( const Uint32& Id );

		void Reload();

		Uint32 Exists( const std::string& Name );

		Uint32 Exists( const Uint32& Id );

		void Destroy();
	protected:
		std::list<T*> mResources;
		bool mUniqueId;
};

template <class T>
tResourceManager<T>::tResourceManager( bool UniqueId ) :
	mUniqueId( UniqueId )
{
}

template <class T>
tResourceManager<T>::~tResourceManager() {
	Destroy();
}

template <class T>
void tResourceManager<T>::Destroy() {
	typename std::list<T*>::iterator it;

	for ( it = mResources.begin() ; it != mResources.end(); it++ )
		eeSAFE_DELETE( (*it) );
}

template <class T>
T * tResourceManager<T>::Add( T * Resource ) {
	if ( NULL != Resource ) {
		if ( mUniqueId ) {
			Uint32 c = Count( Resource->Id() );

			if ( 0 == c ) {
				mResources.push_back( Resource );

				return Resource;
			} else {
				Resource->Name( Resource->Name() + intToStr( c + 1 ) );

				return Add( Resource );
			}
		} else {
			mResources.push_back( Resource );

			return Resource;
		}
	}

	return NULL;
}

template <class T>
bool tResourceManager<T>::Remove( T * Resource, bool Delete ) {
	if ( NULL != Resource ) {
		mResources.remove( Resource );

		if ( Delete )
			eeSAFE_DELETE( Resource );

		return true;
	}

	return false;
}

template <class T>
bool tResourceManager<T>::RemoveById( const Uint32& Id, bool Delete ) {
	return Remove( GetById( Id ), Delete );
}

template <class T>
bool tResourceManager<T>::RemoveByName( const std::string& Name, bool Delete ) {
	return Remove( GetByName( Name ), Delete );
}

template <class T>
Uint32 tResourceManager<T>::Exists( const std::string& Name ) {
	return Count( Name );
}

template <class T>
Uint32 tResourceManager<T>::Exists( const Uint32& Id ) {
	return Count( Id );
}

template <class T>
T * tResourceManager<T>::GetByName( const std::string& Name ) {
	return GetById( MakeHash( Name ) );
}

template <class T>
T * tResourceManager<T>::GetById( const Uint32& id ) {
	typename std::list<T*>::reverse_iterator it;

	T * sp = NULL;

	for ( it = mResources.rbegin(); it != mResources.rend(); it++ ) {
		sp = (*it);

		if ( id == sp->Id() )
			return sp;
	}

	return NULL;
}

template <class T>
Uint32 tResourceManager<T>::Count() {
	return mResources.size();
}

template <class T>
Uint32 tResourceManager<T>::Count( const Uint32& Id ) {
	typename std::list<T*>::iterator it;
	Uint32 Count = 0;

	for ( it = mResources.begin() ; it != mResources.end(); it++ )
		if ( (*it)->Id() == Id )
			Count++;

	return Count;
}

template <class T>
Uint32 tResourceManager<T>::Count( const std::string& Name ) {
	return Count( MakeHash( Name ) );
}

}}

#endif

