#ifndef EE_GRAPHICSCTTFFONTLOADER
#define EE_GRAPHICSCTTFFONTLOADER

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/font.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/system/objectloader.hpp>

namespace EE { namespace System {
class IOStream;
class Pack;
}}

namespace EE { namespace Graphics {

/** @brief The TTF Font loader loads a true type font to memory in synchronous or asynchronous mode.
@see ObjectLoader
*/
class EE_API FontTrueTypeLoader : public ObjectLoader {
	public:
		/** Load a True Type Font from path
		* @param FontName The font name
		* @param Filepath The TTF file path
		*/
		FontTrueTypeLoader( const std::string& FontName, const std::string& Filepath );

		/** Load a True Type Font from a Pack
		* @param FontName The font name
		* @param Pack Pointer to the pack instance
		* @param FilePackPath The path of the file inside the pack
		*/
		FontTrueTypeLoader( const std::string& FontName, Pack * Pack, const std::string& FilePackPath );

		/** Loads a True Type Font from memory
		* @param FontName The font name
		* @param TTFData The pointer to the data
		* @param TTFDataSize The size of the data
		*/
		FontTrueTypeLoader( const std::string& FontName, Uint8* TTFData, const unsigned int& TTFDataSize );
		
		/** Loads a True Type Font from a IO Steam
		* @param FontName The font name
		* @oaram stream The IO Stream
		*/
		FontTrueTypeLoader( const std::string& FontName, IOStream& stream );

		virtual ~FontTrueTypeLoader();

		/** This must be called for the asynchronous mode to update the texture data to the GPU, the call must be done from the same thread that the GL Context was created ( the main thread ).
		**	The TTF Font creates texture from the data obtained from the true type file.
		** @see ObjectLoader::Update */
		void 				update();

		/** Releases the Font instance and the texture loaded ( if was already loaded ), it will destroy the font texture from memory */
		void				unload();

		/** @return The font name. */
		const std::string&	getId() const;

		/** @return The font instance if already exists, otherwise returns NULL. */
		Graphics::Font *	getFont() const;
	protected:
		enum TTF_LOAD_TYPE
		{
			TTF_LT_PATH	= 1,
			TTF_LT_MEM	= 2,
			TTF_LT_PACK	= 3,
			TTF_LT_STREAM = 4
		};

		Uint32				mLoadType; 	// From memory, from path, from pack

		FontTrueType *		mFont;

		std::string			mFontName;
		std::string			mFilepath;
		Pack *				mPack;
		Uint8 *				mData;
		unsigned int		mDataSize;
		IOStream *			mIOStream;

		void 				start();

		void				reset();
	private:
		bool				mFontLoaded;

		void 				loadFromFile();
		void				loadFromMemory();
		void				loadFromPack();
		void				loadFromStream();
		void 				create();
};

}}

#endif


