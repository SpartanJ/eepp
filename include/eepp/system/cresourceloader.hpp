#ifndef EE_SYSTEMCRESOURCELOADER
#define EE_SYSTEMCRESOURCELOADER

#include <eepp/system/base.hpp>
#include <eepp/system/cobjectloader.hpp>

namespace EE { namespace System {

#define THREADS_AUTO (0xFFFFFFFF)

class EE_API cResourceLoader {
	public:
		typedef cb::Callback1<void, cResourceLoader *> ResLoadCallback;

		/** @param MaxThreads Set the maximun simultaneous threads to load resources, THREADS_AUTO will use the cpu number of cores. */
		cResourceLoader( const Uint32& MaxThreads = THREADS_AUTO );

		virtual ~cResourceLoader();

		void			Add( cObjectLoader * Object );

		void 			Load( ResLoadCallback Cb );

		void 			Load();

		void			Unload();

		virtual void 	Update();

		virtual bool	IsLoaded();

		virtual bool	IsLoading();

		bool			Threaded() const;

		void			Threaded( const bool& threaded );

		bool			Clear( const bool& ClearObjectsLoaded = true );

		/** @return The aproximate percent of progress ( between 0 and 100 ) */
		eeFloat			Progress();

		Uint32			Count() const;
	protected:
		bool			mLoaded;
		bool			mLoading;
		bool			mThreaded;
		Uint32			mThreads;

		std::list<ResLoadCallback>	mLoadCbs;
		std::list<cObjectLoader *>	mObjs;
		std::list<cObjectLoader *>	mObjsLoaded;

		void			SetThreads();
		virtual void	SetLoaded();
};

}}

#endif
