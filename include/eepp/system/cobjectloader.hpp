#ifndef EE_SYSTEMCOBJECTLOADER
#define EE_SYSTEMCOBJECTLOADER

#include <eepp/system/base.hpp>
#include <eepp/system/cthread.hpp>
#include <list>

namespace EE { namespace System {

/** @brief Base class that defines resources to be loaded in synchronous or asynchronous mode. */
class EE_API cObjectLoader : protected cThread {
	public:
		typedef cb::Callback1<void, cObjectLoader *> ObjLoadCallback;

		/** @enum Definition of the Object Loaders implemented by the engine. */
		enum ObjLoaderType {
			TextureLoader	= 1,
			SoundLoader		= 2,
			FontTTFLoader	= 3,
			FontTexLoader	= 4,
			UserObjLoader
		};

		/** Creates an instance of an object loader of the type defined */
		cObjectLoader( Uint32 ObjType );

		virtual ~cObjectLoader();

		/** @brief Starts loading the resource */
		void 			Load();

		/** @brief Starts loading the resource.
		**	@param Cb A callback that is called when the resources is loaded.
		*/
		void 			Load( ObjLoadCallback Cb );

		/** @brief Force to unload the resource from memory in case that it was already loaded. */
		virtual void	Unload() = 0;

		/** @brief Update the state of the current loading resource.
		**	This is needed by some loaders to know if the resource was loaded.
		**	The loaders that need to load a texture to the GPU, will upload the data to the GPU only from the OpenGL context thread ( the main application thread )
		*/
		virtual void 	Update();

		/** @brief Starts the loading. */
		void 			Launch();

		/** @returns If the resource was loaded. */
		virtual bool	IsLoaded();

		/** @returns If the resource is loading. */
		virtual bool	IsLoading();

		/** @returns If the loader is asynchronous */
		bool			Threaded() const;

		/** @brief Sets if the loader is asynchronous.
		**	This must be called before the load starts.
		*/
		void			Threaded( const bool& threaded );

		/** @return The object loader type */
		const Uint32&	Type() const;
	protected:
		Uint32			mObjType;	// Texture Loader Object Type
		bool			mLoaded;
		bool			mLoading;
		bool			mThreaded;

		std::list<ObjLoadCallback>	mLoadCbs;

		virtual void 	Start();

		virtual void	SetLoaded();

		virtual void	Reset();
	private:
		virtual void 	Run();
};

}}

#endif
