#ifndef EE_SYSTEMCOBJECTLOADER
#define EE_SYSTEMCOBJECTLOADER

#include "base.hpp"
#include "cthread.hpp"

namespace EE { namespace System {

class cObjectLoader : cThread {
	public:
		cObjectLoader();

		~cObjectLoader();

		void 			Load();

		virtual void 	Update();

		void 			Launch();

		virtual bool	IsLoaded();

		bool			Threaded() const;

		void			Threaded( const bool& threaded );
	protected:
		Uint32			mObjType;	// Texture Loader Object Type
		bool			mLoaded;
		bool			mThreaded;

		virtual void 	Start();
	private:
		virtual void 	Run();
};

}}

#endif


