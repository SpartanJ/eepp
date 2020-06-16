#ifndef EE_SYSTEMTRESOURCEMANAGER_HPP
#define EE_SYSTEMTRESOURCEMANAGER_HPP

#include <eepp/core/string.hpp>
#include <string>
#include <unordered_map>

namespace EE { namespace System {

/** @brief A simple resource manager. It keeps a list of the resources, and free the instances of
 * the resources when the manager is closed. Resources must implement getId() and getName()
 * properties getId() is the string hash of getName().
 */
template <class T> class ResourceManager {
  public:
	ResourceManager();

	/** @brief The destructor will call destroy() and destroy all the resources added to the manager
	 */
	virtual ~ResourceManager();

	/** @brief Add the resource to the resource manager
	**	@param resource The resource to be managed by the manager */
	virtual T* add( T* resource );

	/** @brief Removes the resource from the manager
	**	@param resource The resource to remove
	**	@param remove Indicates if the resource must be destroyed after being removed from the
	*manager */
	bool remove( T* resource, bool remove = true );

	/** @brief Removes the resource by its id
	**	@see remove */
	bool removeById( const String::HashType& id, bool remove = true );

	/** @brief Removes the resource by its name
	**	@see remove */
	bool removeByName( const std::string& name, bool remove = true );

	/** @returns A resource by its name. If not found returns NULL. */
	T* getByName( const std::string& name );

	/** @returns A resource by its id. If not found returns NULL. */
	T* getById( const String::HashType& id );

	/** @returns The number of resources added */
	Uint32 getCount();

	/** @returns The number of resources that where added with the indicated name. */
	Uint32 getCount( const std::string& name );

	/** @returns The number of resources that where added with the indicated id. */
	Uint32 getCount( const String::HashType& id );

	/** @returns If the resource name exists in the resources list. */
	bool exists( const std::string& name );

	/** @returns If the resource id exists in the resources list. */
	bool existsId( const String::HashType& id );

	/** @brief Destroy all the resources added ( delete the instances of the resources ) */
	void destroy();

	/** @brief Prints all the resources names added to the manager. */
	void printNames();

	/** @returns A reference to the resources list of the manager. */
	std::unordered_map<String::HashType, T*>& getResources();

	/** @brief Indicates if the resource manager is destroy the resources. */
	const bool& isDestroying() const;

  protected:
	std::unordered_map<String::HashType, T*> mResources;
	bool mIsDestroying;
};

template <class T> ResourceManager<T>::ResourceManager() : mIsDestroying( false ) {}

template <class T> const bool& ResourceManager<T>::isDestroying() const {
	return mIsDestroying;
}

template <class T> ResourceManager<T>::~ResourceManager() {
	destroy();
}

template <class T> void ResourceManager<T>::destroy() {
	mIsDestroying = true;

	for ( auto& it : mResources ) {
		T* res = it.second;
		eeSAFE_DELETE( res );
	}

	mResources.clear();

	mIsDestroying = false;
}

template <class T> std::unordered_map<String::HashType, T*>& ResourceManager<T>::getResources() {
	return mResources;
}

template <class T> T* ResourceManager<T>::add( T* resource ) {
	if ( NULL != resource ) {
		if ( !existsId( resource->getId() ) ) {
			mResources[resource->getId()] = resource;

			return resource;
		} else {
			std::string realName( resource->getName() );
			Uint32 c = 1;

			while ( existsId( resource->getId() ) ) {
				c++;
				resource->setName( realName + String::toString( c ) );
			}

			return add( resource );
		}

		mResources[resource->getId()] = resource;
		return resource;
	}
	return NULL;
}

template <class T> bool ResourceManager<T>::remove( T* resource, bool remove ) {
	if ( NULL != resource ) {
		mResources.erase( resource->getId() );

		if ( remove )
			eeSAFE_DELETE( resource );

		return true;
	}

	return false;
}

template <class T> bool ResourceManager<T>::removeById( const String::HashType& id, bool _remove ) {
	return remove( getById( id ), _remove );
}

template <class T> bool ResourceManager<T>::removeByName( const std::string& name, bool _remove ) {
	return remove( getByName( name ), _remove );
}

template <class T> bool ResourceManager<T>::exists( const std::string& name ) {
	return existsId( String::hash( name ) );
}

template <class T> bool ResourceManager<T>::existsId( const String::HashType& id ) {
	return mResources.find( id ) != mResources.end();
}

template <class T> T* ResourceManager<T>::getByName( const std::string& name ) {
	return getById( String::hash( name ) );
}

template <class T> T* ResourceManager<T>::getById( const String::HashType& id ) {
	auto it = mResources.find( id );
	return it != mResources.end() ? it->second : nullptr;
}

template <class T> void ResourceManager<T>::printNames() {
	for ( auto& it : mResources ) {
		eePRINT( "'%s'\n", it.second->getName().c_str() );
	}
}

template <class T> Uint32 ResourceManager<T>::getCount() {
	return (Uint32)mResources.size();
}

template <class T> Uint32 ResourceManager<T>::getCount( const String::HashType& id ) {
	return existsId( id ) ? 1 : 0;
}

template <class T> Uint32 ResourceManager<T>::getCount( const std::string& name ) {
	return getCount( String::hash( name ) );
}

/** @brief A simple resource manager. It keeps a list of the resources, and free the instances of
 * the resources when the manager is closed. Resources must implement getId() and getName()
 * properties getId() is the string hash of getName(). Allows repeated keys.
 */
template <class T> class ResourceManagerMulti {
  public:
	/** @param UniqueId Indicates if the resources id must be unique */
	ResourceManagerMulti();

	/** @brief The destructor will call destroy() and destroy all the resources added to the manager
	 */
	virtual ~ResourceManagerMulti();

	/** @brief Add the resource to the resource manager
	**	@param resource The resource to be managed by the manager */
	virtual T* add( T* resource );

	/** @brief Removes the resource from the manager
	**	@param resource The resource to remove
	**	@param remove Indicates if the resource must be destroyed after being removed from the
	*manager */
	bool remove( T* resource, bool remove = true );

	/** @brief Removes the resource by its id
	**	@see remove */
	bool removeById( const String::HashType& id, bool remove = true );

	/** @brief Removes the resource by its name
	**	@see remove */
	bool removeByName( const std::string& name, bool remove = true );

	/** @returns A resource by its name. If not found returns NULL. */
	T* getByName( const std::string& name );

	/** @returns A resource by its id. If not found returns NULL. */
	T* getById( const String::HashType& id );

	/** @returns The number of resources added */
	Uint32 getCount();

	/** @returns The number of resources that where added with the indicated name. */
	Uint32 getCount( const std::string& name );

	/** @returns The number of resources that where added with the indicated id. */
	Uint32 getCount( const String::HashType& id );

	/** @returns If the resource name exists in the resources list. */
	bool exists( const std::string& name );

	/** @returns If the resource id exists in the resources list. */
	bool existsId( const String::HashType& id );

	/** @brief Destroy all the resources added ( delete the instances of the resources ) */
	void destroy();

	/** @brief Prints all the resources names added to the manager. */
	void printNames();

	/** @returns A reference to the resources list of the manager. */
	std::unordered_multimap<String::HashType, T*>& getResources();

	/** @brief Indicates if the resource manager is destroy the resources. */
	const bool& isDestroying() const;

  protected:
	std::unordered_multimap<String::HashType, T*> mResources;
	bool mIsDestroying;
};

template <class T> ResourceManagerMulti<T>::ResourceManagerMulti() : mIsDestroying( false ) {}

template <class T> const bool& ResourceManagerMulti<T>::isDestroying() const {
	return mIsDestroying;
}

template <class T> ResourceManagerMulti<T>::~ResourceManagerMulti() {
	destroy();
}

template <class T> void ResourceManagerMulti<T>::destroy() {
	mIsDestroying = true;

	for ( auto& it : mResources ) {
		T* res = it.second;
		eeSAFE_DELETE( res );
	}

	mResources.clear();

	mIsDestroying = false;
}

template <class T>
std::unordered_multimap<String::HashType, T*>& ResourceManagerMulti<T>::getResources() {
	return mResources;
}

template <class T> T* ResourceManagerMulti<T>::add( T* resource ) {
	if ( NULL != resource ) {
		mResources.insert( std::pair<String::HashType, T*>( resource->getId(), resource ) );
		return resource;
	}
	return NULL;
}

template <class T> bool ResourceManagerMulti<T>::remove( T* resource, bool remove ) {
	if ( NULL != resource ) {
		mResources.erase( resource->getId() );

		if ( remove )
			eeSAFE_DELETE( resource );

		return true;
	}

	return false;
}

template <class T>
bool ResourceManagerMulti<T>::removeById( const String::HashType& id, bool _remove ) {
	return remove( getById( id ), _remove );
}

template <class T>
bool ResourceManagerMulti<T>::removeByName( const std::string& name, bool _remove ) {
	return remove( getByName( name ), _remove );
}

template <class T> bool ResourceManagerMulti<T>::exists( const std::string& name ) {
	return existsId( String::hash( name ) );
}

template <class T> bool ResourceManagerMulti<T>::existsId( const String::HashType& id ) {
	return mResources.find( id ) != mResources.end();
}

template <class T> T* ResourceManagerMulti<T>::getByName( const std::string& name ) {
	return getById( String::hash( name ) );
}

template <class T> T* ResourceManagerMulti<T>::getById( const String::HashType& id ) {
	auto it = mResources.find( id );
	return it != mResources.end() ? it->second : nullptr;
}

template <class T> void ResourceManagerMulti<T>::printNames() {
	for ( auto& it : mResources ) {
		eePRINT( "'%s'\n", it.second->getName().c_str() );
	}
}

template <class T> Uint32 ResourceManagerMulti<T>::getCount() {
	return (Uint32)mResources.size();
}

template <class T> Uint32 ResourceManagerMulti<T>::getCount( const String::HashType& id ) {
	return mResources.count( id );
}

template <class T> Uint32 ResourceManagerMulti<T>::getCount( const std::string& name ) {
	return getCount( String::hash( name ) );
}

}} // namespace EE::System

#endif
