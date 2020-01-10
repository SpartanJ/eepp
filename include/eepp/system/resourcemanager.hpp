#ifndef EE_SYSTEMTRESOURCEMANAGER_HPP
#define EE_SYSTEMTRESOURCEMANAGER_HPP

#include <eepp/core/string.hpp>
#include <list>
#include <string>

namespace EE { namespace System {

/** @brief A simple resource manager. It keeps a list of the resources, and free the instances of
*the resources when the manager is closed.
**	Resources must have Id() and Name() properties. Id() is the string hash of Name(). */
template <class T> class ResourceManager {
  public:
	/** @param UniqueId Indicates if the resources id must be unique */
	ResourceManager( bool UniqueId = true );

	/** @brief The destructor will call Destroy() and destroy all the resources added to the manager
	 */
	virtual ~ResourceManager();

	/** @brief Add the resource to the resource manager
	**	@param Resource The resource to be managed by the manager */
	virtual T* add( T* Resource );

	/** @brief Removes the resource from the manager
	**	@param Resource The resource to remove
	**	@param Delete Indicates if the resource must be destroyed after being removed from the
	*manager */
	bool remove( T* Resource, bool Delete = true );

	/** @brief Removes the resource by its id
	**	@see Remove */
	bool removeById( const Uint32& Id, bool Delete = true );

	/** @brief Removes the resource by its name
	**	@see Remove */
	bool removeByName( const std::string& Name, bool Delete = true );

	/** @returns A resource by its name. If not found returns NULL. */
	T* getByName( const std::string& Name );

	/** @returns A resource by its id. If not found returns NULL. */
	T* getById( const Uint32& Id );

	/** @returns The number of resources added */
	Uint32 getCount();

	/** @returns The number of resources that where added with the indicated name. */
	Uint32 setCount( const std::string& Name );

	/** @returns The number of resources that where added with the indicated id. */
	Uint32 setCount( const Uint32& Id );

	/** @returns If the resource name exists in the resources list. */
	bool exists( const std::string& Name );

	/** @returns If the resource id exists in the resources list. */
	bool existsId( const Uint32& Id );

	/** @brief Destroy all the resources added ( delete the instances of the resources ) */
	void destroy();

	/** @brief Prints all the resources names added to the manager. */
	void printNames();

	/** @returns A reference to the resources list of the manager. */
	std::list<T*>& getResources();

	/** @brief Indicates if the resource manager is destroy the resources. */
	const bool& isDestroying() const;

  protected:
	std::list<T*> mResources;
	bool mUniqueId;
	bool mIsDestroying;
};

template <class T>
ResourceManager<T>::ResourceManager( bool UniqueId ) :
	mUniqueId( UniqueId ), mIsDestroying( false ) {}

template <class T> const bool& ResourceManager<T>::isDestroying() const {
	return mIsDestroying;
}

template <class T> ResourceManager<T>::~ResourceManager() {
	destroy();
}

template <class T> void ResourceManager<T>::destroy() {
	typename std::list<T*>::iterator it;

	mIsDestroying = true;

	for ( it = mResources.begin(); it != mResources.end(); it++ ) {
		eeSAFE_DELETE( ( *it ) );
	}

	mResources.clear();

	mIsDestroying = false;
}

template <class T> std::list<T*>& ResourceManager<T>::getResources() {
	return mResources;
}

template <class T> T* ResourceManager<T>::add( T* Resource ) {
	if ( NULL != Resource ) {
		if ( mUniqueId ) {
			Uint32 c = setCount( Resource->getId() );

			if ( 0 == c ) {
				mResources.push_back( Resource );

				return Resource;
			} else {
				std::string RealName( Resource->getName() );

				while ( setCount( Resource->getId() ) ) {
					c++;
					Resource->setName( RealName + String::toStr( c ) );
				}

				return add( Resource );
			}
		} else {
			mResources.push_back( Resource );

			return Resource;
		}
	}

	return NULL;
}

template <class T> bool ResourceManager<T>::remove( T* Resource, bool Delete ) {
	if ( NULL != Resource ) {
		mResources.remove( Resource );

		if ( Delete )
			eeSAFE_DELETE( Resource );

		return true;
	}

	return false;
}

template <class T> bool ResourceManager<T>::removeById( const Uint32& Id, bool Delete ) {
	return remove( getById( Id ), Delete );
}

template <class T> bool ResourceManager<T>::removeByName( const std::string& Name, bool Delete ) {
	return remove( getByName( Name ), Delete );
}

template <class T> bool ResourceManager<T>::exists( const std::string& Name ) {
	return existsId( String::hash( Name ) );
}

template <class T> bool ResourceManager<T>::existsId( const Uint32& Id ) {
	typename std::list<T*>::iterator it;

	for ( it = mResources.begin(); it != mResources.end(); it++ )
		if ( ( *it )->getId() == Id )
			return true;

	return false;
}

template <class T> T* ResourceManager<T>::getByName( const std::string& Name ) {
	return getById( String::hash( Name ) );
}

template <class T> T* ResourceManager<T>::getById( const Uint32& id ) {
	typename std::list<T*>::reverse_iterator it;

	T* sp = NULL;

	for ( it = mResources.rbegin(); it != mResources.rend(); it++ ) {
		sp = ( *it );

		if ( id == sp->getId() )
			return sp;
	}

	return NULL;
}

template <class T> void ResourceManager<T>::printNames() {
	typename std::list<T*>::reverse_iterator it;

	T* sp = NULL;

	for ( it = mResources.rbegin(); it != mResources.rend(); it++ ) {
		sp = ( *it );

		eePRINT( "'%s'\n", sp->getName().c_str() );
	}
}

template <class T> Uint32 ResourceManager<T>::getCount() {
	return (Uint32)mResources.size();
}

template <class T> Uint32 ResourceManager<T>::setCount( const Uint32& Id ) {
	typename std::list<T*>::iterator it;
	Uint32 Count = 0;

	for ( it = mResources.begin(); it != mResources.end(); it++ )
		if ( ( *it )->getId() == Id )
			Count++;

	return Count;
}

template <class T> Uint32 ResourceManager<T>::setCount( const std::string& Name ) {
	return setCount( String::hash( Name ) );
}

}} // namespace EE::System

#endif
