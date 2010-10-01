#ifndef EE_SYSTEMCOBJECTLOADER
#define EE_SYSTEMCOBJECTLOADER

#include "base.hpp"
#include "cthread.hpp"

namespace EE { namespace System {

class EE_API cObjectLoader : cThread {
	public:
		typedef cb::Callback1<void, cObjectLoader *> ObjLoadCallback;

		enum ObjLoaderType {
			TextureLoader = 1,
			SoundLoader = 2,
			FontTTFLoader = 3,
			FontTexLoader = 4,
			UserObjLoader
		};

		cObjectLoader( Uint32 ObjType );

		~cObjectLoader();

		void 			Load();

		void 			Load( ObjLoadCallback Cb );

		virtual void 	Update();

		void 			Launch();

		virtual bool	IsLoaded();

		virtual bool	IsLoading();

		bool			Threaded() const;

		void			Threaded( const bool& threaded );

		const Uint32&	Type() const;
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
