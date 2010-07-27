#ifndef EE_SYSTEMCOBJECTLOADER
#define EE_SYSTEMCOBJECTLOADER

#include "base.hpp"
#include "cthread.hpp"

namespace EE { namespace System {

class cObjectLoader : cThread {
	public:
		typedef boost::function1<void, cObjectLoader *> ObjLoadCallback;

		enum ObjLoaderType {
			TextureLoader = 1,
			UserObjLoader
		};

		cObjectLoader( Uint32 ObjType );

		~cObjectLoader();

		void 			Load( ObjLoadCallback Cb = NULL );

		virtual void 	Update();

		void 			Launch();

		virtual bool	IsLoaded();

		virtual bool	IsLoading();

		bool			Threaded() const;

		void			Threaded( const bool& threaded );
	protected:
		Uint32			mObjType;	// Texture Loader Object Type
		bool			mLoaded;
		bool			mLoading;
		bool			mThreaded;

		std::list<ObjLoadCallback>	mLoadCbs;

		virtual void 	Start();

		virtual void	SetLoaded();
	private:
		virtual void 	Run();
};

}}

#endif


