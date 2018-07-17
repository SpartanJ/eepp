#ifndef EE_SYSTEMCRESOURCELOADER
#define EE_SYSTEMCRESOURCELOADER

#include <eepp/system/objectloader.hpp>

namespace EE { namespace System {

#define THREADS_AUTO (eeINDEX_NOT_FOUND)

/** @brief A simple resource loader that can load a batch of resources synchronously or asynchronously */
class EE_API ResourceLoader {
	public:
		typedef std::function<void( ResourceLoader * )> ResLoadCallback;

		/** @param MaxThreads Set the maximun simultaneous threads to load resources, THREADS_AUTO will use the cpu number of cores. */
		ResourceLoader( const Uint32& MaxThreads = THREADS_AUTO );

		virtual ~ResourceLoader();

		/** @brief Adds a resource to load.
		**	Must be called before the loading starts.
		**	Once an object loader is added to the resource loader, the instance of that object will be managed and released by the loader.
		**	@param Object The instance object loader to load
		*/
		void			add( ObjectLoader * Object );

		/** @brief Starts loading the resources.
		**	@param Cb A callback that is called when the resources finished loading. */
		void 			load( ResLoadCallback Cb );

		/** @brief Starts loading the resources. */
		void 			load();

		/** @brief Unload all the resources already loaded. */
		void			unload();

		/** @brief Update must be called from the thread that started the loading to update the state of the resource loader. */
		virtual void 	update();

		/** @returns If the resources were loaded. */
		virtual bool	isLoaded();

		/** @returns If the resources are still loading. */
		virtual bool	isLoading();

		/** @returns If the resource loader is asynchronous */
		bool			isThreaded() const;

		/** @brief Sets if the resource loader is asynchronous.
		**	This must be called before the load starts. */
		void			setThreaded( const bool& setThreaded );

		/** @brief Clears the resources added to load that werent loaded, and delete the instances of the loaders.
		**	@param ClearObjectsLoaded Sets if the objects loader that were already loaded must be also deleted ( it will not unload the loaded resources, but the instance of the object loader ). */
		bool			clear( const bool& ClearObjectsLoaded = true );

		/** @return The aproximate percent of progress ( between 0 and 100 ) */
		Float			getProgress();

		/** @returns The number of resources added to load. */
		Uint32			getCount() const;
	protected:
		bool			mLoaded;
		bool			mLoading;
		bool			mThreaded;
		Uint32			mThreads;

		std::list<ResLoadCallback>	mLoadCbs;
		std::list<ObjectLoader *>	mObjs;
		std::list<ObjectLoader *>	mObjsLoaded;

		void			setThreads();

		virtual void	setLoaded();
};

}}

#endif
