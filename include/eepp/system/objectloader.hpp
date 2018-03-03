#ifndef EE_SYSTEMCOBJECTLOADER
#define EE_SYSTEMCOBJECTLOADER

#include <eepp/core.hpp>
#include <eepp/system/thread.hpp>
#include <list>

namespace EE { namespace System {

/** @brief Base class that defines resources to be loaded in synchronous or asynchronous mode. */
class EE_API ObjectLoader : protected Thread {
	public:
		typedef cb::Callback1<void, ObjectLoader *> ObjLoadCallback;

		/** @brief LoaderType Definition of the Object Loaders implemented by the engine. */
		enum ObjLoaderType {
			TextureLoader		= 1,
			SoundLoader			= 2,
			FontLoader			= 3,
			UserObjLoaderType
		};

		/** Creates an instance of an object loader of the type defined */
		ObjectLoader( Uint32 ObjType );

		virtual ~ObjectLoader();

		/** @brief Starts loading the resource */
		void 			load();

		/** @brief Starts loading the resource.
		**	@param Cb A callback that is called when the resource finished loading. */
		void 			load( ObjLoadCallback Cb );

		/** @brief Force to unload the resource from memory in case that it was already loaded. */
		virtual void	unload() = 0;

		/** @brief Update the state of the current loading resource.
		**	This is needed by some loaders to know if the resource was loaded.
		**	The loaders that need to load a texture to the GPU, will upload the data to the GPU only from the OpenGL context thread ( the main application thread )
		*/
		virtual void 	update();

		/** @brief Starts the loading. */
		void 			launch();

		/** @returns If the resource was loaded. */
		virtual bool	isLoaded();

		/** @returns If the resource is loading. */
		virtual bool	isLoading();

		/** @returns If the loader is asynchronous */
		bool			isThreaded() const;

		/** @brief Sets if the loader is asynchronous.
		**	This must be called before the load starts. */
		void			setThreaded( const bool& setThreaded );

		/** @return The object loader type */
		const Uint32&	type() const;
	protected:
		Uint32			mObjType;	// Texture Loader Object Type
		bool			mLoaded;
		bool			mLoading;
		bool			mThreaded;

		std::list<ObjLoadCallback>	mLoadCbs;

		virtual void 	start();

		virtual void	setLoaded();

		virtual void	reset();
	private:
		virtual void 	run();
};

}}

#endif
