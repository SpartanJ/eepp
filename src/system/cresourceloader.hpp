#ifndef EE_SYSTEMCRESOURCELOADER
#define EE_SYSTEMCRESOURCELOADER

#include "base.hpp"
#include "cobjectloader.hpp"

namespace EE { namespace System {

#define THREADS_AUTO (0xFFFFFFFF)

class EE_API cResourceLoader {
	public:
		typedef cb::Callback1<void, cResourceLoader *> ResLoadCallback;

		/** @param MaxThreads Set the maximun simultaneous threads to load resources, if not value is seted it will use Num Cores - 1 or 1 thread if single core. */
		cResourceLoader( const Uint32& MaxThreads = THREADS_AUTO );

		virtual ~cResourceLoader();

		void			Add( cObjectLoader * Object );

		void 			Load( ResLoadCallback Cb );

		void 			Load();

		virtual void 	Update();

		virtual bool	IsLoaded();

		virtual bool	IsLoading();

		bool			Threaded() const;

		void			Threaded( const bool& threaded );

		bool			Clear( const bool& ClearObjectsLoaded = true );

		eeFloat			Progress();
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
