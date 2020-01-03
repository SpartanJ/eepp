#ifndef EE_SYSTEMCRESOURCELOADER
#define EE_SYSTEMCRESOURCELOADER

#include <eepp/core.hpp>
#include <eepp/system/thread.hpp>
#include <vector>

namespace EE { namespace System {

#define THREADS_AUTO (eeINDEX_NOT_FOUND)

/** @brief A simple resource loader that can load a batch of resources synchronously or asynchronously */
class EE_API ResourceLoader {
	public:
		typedef std::function<void( ResourceLoader * )> ResLoadCallback;
		typedef std::function<void()> ObjectLoaderTask;

		/** @param MaxThreads Set the maximun simultaneous threads to load resources, THREADS_AUTO will use the cpu number of cores. */
		ResourceLoader( const Uint32& MaxThreads = THREADS_AUTO );

		virtual ~ResourceLoader();

		/** @brief Adds a resource to load.
		**	Must be called before the loading starts.
		**	Once an object loader is added to the resource loader, the instance of that object will be managed and released by the loader.
		**	@param objectLoaderTask The function callback of the load process
		*/
		void			add( const ObjectLoaderTask& objectLoaderTask );

		/** @brief Starts loading the resources.
		**	@param callback A callback that is called when the resources finished loading. */
		void 			load( const ResLoadCallback& callback );

		/** @brief Starts loading the resources. */
		void 			load();

		/** @returns If the resources were loaded. */
		virtual bool	isLoaded();

		/** @returns If the resources are still loading. */
		virtual bool	isLoading();

		/** @returns If the resource loader is asynchronous */
		bool			isThreaded() const;

		/** @brief Sets if the resource loader is asynchronous.
		**	This must be called before the load starts. */
		void			setThreaded( const bool& setThreaded );

		/** @brief Clears the resources added to load that werent loaded, and delete the instances of the loaders. */
		bool			clear();

		/** @return The aproximate percent of progress ( between 0 and 100 ) */
		Float			getProgress();

		/** @returns The number of resources added to load. */
		Uint32			getCount() const;
	protected:
		bool			mLoaded;
		bool			mLoading;
		bool			mThreaded;
		Uint32			mThreads;
		Uint32			mTotalLoaded;
		Thread			mThread;

		std::vector<ResLoadCallback>	mLoadCbs;
		std::vector<ObjectLoaderTask>	mTasks;

		void			setThreads();

		virtual void	setLoaded();

		void			taskRunner();

		void			serializedLoad();
};

}}

#endif
