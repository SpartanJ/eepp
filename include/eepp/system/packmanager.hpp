#ifndef EE_SYSTEMCPACKMANAGER_HPP
#define EE_SYSTEMCPACKMANAGER_HPP

#include <eepp/system/singleton.hpp>
#include <eepp/system/container.hpp>
#include <eepp/system/pack.hpp>

namespace EE { namespace System {

/** @brief The Pack Manager keep track of the instanciated Packs.
	It's used to find files from any open pack.
*/
class EE_API PackManager : public Container<Pack> {
	SINGLETON_DECLARE_HEADERS(PackManager)

	public:
		virtual ~PackManager();

		/** @brief Searchs for the filepath in the packs, if the file is found it will return the pack that belongs to.
		**	@return The pack where the file exists. If the file is not found, returns NULL.
		**	@param path The file path to search. */
		Pack * Exists( std::string& path );

		/** @brief Search for a pack by its path.
		**	@return The pack instance if found, otherwise returns NULL. */
		Pack * GetPackByPath( std::string path );

		/** @returns If the packs opened are being used as a fallback in case of a file wasn't found in the file system path. */
		const bool& FallbackToPacks() const;

		/**	@brief Sets if the files that failed to be loaded from the file system should try to be loaded from the currently open packs.
		**	For example if you try to load a texture from the file system a fails it will search the same path in the opened packs, and load it from there.
		**			cTextureFactory::instance()->Load( "mytexture.png" );
		**			If the file is not in the file system, it will be searched in the opened packs, and loaded if is found.
		**			In case that the process path is appended to the path... like Sys::GetProcessPath() + "mytexture.png", the process path will be removed from the file path.
		*/
		void FallbackToPacks( const bool& fallback );
	protected:
		bool	mFallback;

		PackManager();
};

}}

#endif
