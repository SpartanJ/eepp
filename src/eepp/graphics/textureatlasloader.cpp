#include <eepp/graphics/textureatlasloader.hpp>
#include <eepp/graphics/textureatlas.hpp>
#include <eepp/graphics/textureatlasmanager.hpp>
#include <eepp/graphics/texturepacker.hpp>
#include <eepp/graphics/textureatlas.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/iostreammemory.hpp>
#include <eepp/graphics/packerhelper.hpp>
#include <eepp/helper/SOIL2/src/SOIL2/stb_image.h>

namespace EE { namespace Graphics {

using namespace Private;

TextureAtlasLoader::TextureAtlasLoader() :
	mThreaded(false),
	mLoaded(false),
	mPack(NULL),
	mSkipResourceLoad(false),
	mIsLoading(false),
	mTextureAtlas(NULL)
{
}

TextureAtlasLoader::TextureAtlasLoader( const std::string& TextureAtlasPath, const bool& Threaded, GLLoadCallback LoadCallback ) :
	mTextureAtlasPath( TextureAtlasPath ),
	mThreaded( Threaded ),
	mLoaded(false),
	mPack(NULL),
	mSkipResourceLoad(false),
	mIsLoading(false),
	mTextureAtlas(NULL),
	mLoadCallback( LoadCallback )
{
	load();
}

TextureAtlasLoader::TextureAtlasLoader( const Uint8* Data, const Uint32& DataSize, const std::string& TextureAtlasName, const bool& Threaded, GLLoadCallback LoadCallback ) :
	mTextureAtlasPath( TextureAtlasName ),
	mThreaded( Threaded ),
	mLoaded(false),
	mPack(NULL),
	mSkipResourceLoad(false),
	mIsLoading(false),
	mTextureAtlas(NULL),
	mLoadCallback( LoadCallback )
{
	loadFromMemory( Data, DataSize, TextureAtlasName );
}

TextureAtlasLoader::TextureAtlasLoader( Pack * Pack, const std::string& FilePackPath, const bool& Threaded, GLLoadCallback LoadCallback ) :
	mTextureAtlasPath( FilePackPath ),
	mThreaded( Threaded ),
	mLoaded(false),
	mPack(NULL),
	mSkipResourceLoad(false),
	mIsLoading(false),
	mTextureAtlas(NULL),
	mLoadCallback( LoadCallback )
{
	loadFromPack( Pack, FilePackPath );
}

TextureAtlasLoader::TextureAtlasLoader( IOStream& IOS, const bool& Threaded, GLLoadCallback LoadCallback ) :
	mThreaded( Threaded ),
	mLoaded(false),
	mPack(NULL),
	mSkipResourceLoad(false),
	mIsLoading(false),
	mTextureAtlas(NULL),
	mLoadCallback( LoadCallback )
{
	loadFromStream( IOS );
}

TextureAtlasLoader::~TextureAtlasLoader()
{
}

void TextureAtlasLoader::setLoadCallback( GLLoadCallback LoadCallback ) {
	mLoadCallback = LoadCallback;
}

void TextureAtlasLoader::update() {
	mRL.update();

	if ( mRL.isLoaded() && !mLoaded )
		createSubTextures();
}

void TextureAtlasLoader::loadFromStream( IOStream& IOS ) {
	mRL.setThreaded( mThreaded );

	if ( IOS.isOpen() ) {
		IOS.read( (char*)&mTexGrHdr, sizeof(sTextureAtlasHdr) );

		if ( mTexGrHdr.Magic == EE_TEXTURE_ATLAS_MAGIC ) {
			for ( Uint32 i = 0; i < mTexGrHdr.TextureCount; i++ ) {
				sTextureHdr tTextureHdr;
				sTempTexAtlas tTexAtlas;

				IOS.read( (char*)&tTextureHdr, sizeof(sTextureHdr) );

				tTexAtlas.Texture = tTextureHdr;
				tTexAtlas.SubTextures.resize( tTextureHdr.SubTextureCount );

				std::string name( &tTextureHdr.Name[0] );
				std::string path( FileSystem::fileRemoveFileName( mTextureAtlasPath ) + name );

				//! Checks if the texture is already loaded
				Texture * tTex = TextureFactory::instance()->getByName( path );

				if ( !mSkipResourceLoad && NULL == tTex ) {
					if ( NULL != mPack ) {
						mRL.add( eeNew( TextureLoader, ( mPack, path ) ) );
					} else {
						mRL.add( eeNew( TextureLoader, ( path ) ) );
					}
				}

				IOS.read( (char*)&tTexAtlas.SubTextures[0], sizeof(sSubTextureHdr) * tTextureHdr.SubTextureCount );

				mTempAtlass.push_back( tTexAtlas );
			}
		}

		if ( !mSkipResourceLoad || ( !mSkipResourceLoad && 0 == mRL.getCount() ) ) {
			mIsLoading = true;
			mRL.load();

			if ( !mThreaded || ( !mSkipResourceLoad && 0 == mRL.getCount() ) )
				createSubTextures();
		}
	}
}

void TextureAtlasLoader::load( const std::string& TextureAtlasPath ) {
	if ( TextureAtlasPath.size() )
		mTextureAtlasPath = TextureAtlasPath;

	if ( FileSystem::fileExists( mTextureAtlasPath ) ) {
		IOStreamFile IOS( mTextureAtlasPath, std::ios::in | std::ios::binary );

		loadFromStream( IOS );
	} else if ( PackManager::instance()->isFallbackToPacksActive() ) {
		std::string tgPath( mTextureAtlasPath );

		Pack * tPack = PackManager::instance()->exists( tgPath );

		if ( NULL != tPack ) {
			loadFromPack( tPack, tgPath );
		}
	}
}

void TextureAtlasLoader::loadFromPack( Pack * Pack, const std::string& FilePackPath ) {
	if ( NULL != Pack && Pack->isOpen() && -1 != Pack->exists( FilePackPath ) ) {
		mPack = Pack;

		SafeDataPointer PData;

		Pack->extractFileToMemory( FilePackPath, PData );

		loadFromMemory( reinterpret_cast<const Uint8*> ( PData.Data ), PData.DataSize, FilePackPath );
	}
}

void TextureAtlasLoader::loadFromMemory( const Uint8* Data, const Uint32& DataSize, const std::string& TextureAtlasName ) {
	if ( TextureAtlasName.size() )
		mTextureAtlasPath = TextureAtlasName;

	IOStreamMemory IOS( (const char*)Data, DataSize );

	loadFromStream( IOS );
}

TextureAtlas * TextureAtlasLoader::getTextureAtlas() const {
	return mTextureAtlas;
}

void TextureAtlasLoader::createSubTextures() {
	mIsLoading = false;
	bool IsAlreadyLoaded = false;
	
	for ( Uint32 z = 0; z < mTempAtlass.size(); z++ ) {
		sTempTexAtlas * tTexAtlas 	= &mTempAtlass[z];
		sTextureHdr * tTexHdr 		= &tTexAtlas->Texture;

		std::string name( &tTexHdr->Name[0] );
		std::string path( FileSystem::fileRemoveFileName( mTextureAtlasPath ) + name );

		FileSystem::filePathRemoveProcessPath( path );

		Texture * tTex 			= TextureFactory::instance()->getByName( path );

		if ( NULL != tTex )
			mTexuresLoaded.push_back( tTex );

		// Create the Texture Atlas with the name of the real texture, not the Childs ( example load 1.png and not 1_ch1.png )
		if ( 0 == z ) {
			if ( mTexGrHdr.Flags & HDR_TEXTURE_ATLAS_REMOVE_EXTENSION )
				name = FileSystem::fileRemoveExtension( name );

			std::string etapath = FileSystem::fileRemoveExtension( path ) + EE_TEXTURE_ATLAS_EXTENSION;

			TextureAtlas * tTextureAtlas = TextureAtlasManager::instance()->getByName( name );

			if ( NULL != tTextureAtlas && tTextureAtlas->getPath() == etapath ) {
				mTextureAtlas = tTextureAtlas;

				IsAlreadyLoaded = true;
			} else {
				mTextureAtlas = eeNew( TextureAtlas, ( name ) );

				mTextureAtlas->setPath( etapath );

				TextureAtlasManager::instance()->add( mTextureAtlas );
			}
		}

		if ( NULL != tTex ) {
			if ( !IsAlreadyLoaded ) {
				for ( Int32 i = 0; i < tTexHdr->SubTextureCount; i++ ) {
					sSubTextureHdr * tSh = &tTexAtlas->SubTextures[i];

					std::string SubTextureName( &tSh->Name[0] );

					if ( mTexGrHdr.Flags & HDR_TEXTURE_ATLAS_REMOVE_EXTENSION )
						SubTextureName = FileSystem::fileRemoveExtension( SubTextureName );

					Recti tRect( tSh->X, tSh->Y, tSh->X + tSh->Width, tSh->Y + tSh->Height );

					SubTexture * tSubTexture = eeNew( SubTexture, ( tTex->getId(), tRect, Sizef( (Float)tSh->DestWidth, (Float)tSh->DestHeight ), Vector2i( tSh->OffsetX, tSh->OffsetY ), SubTextureName ) );

					tSubTexture->setPixelDensity( PixelDensity::toFloat( tSh->PixelDensity ) );
					//if ( tSh->Flags & HDR_SUBTEXTURE_FLAG_FLIPED )
						// Should rotate the sub texture, but.. sub texture rotation is not stored.

					mTextureAtlas->add( tSubTexture );
				}
			}
		} else {
			eePRINTL( "TextureAtlasLoader::CreateSubTextures: Failed to find texture atlas texture, it seems that is not loaded for some reason. Couldn't find: %s", path.c_str() );

			eeASSERT( NULL != tTex );

			return;
		}
	}

	if ( NULL != mTextureAtlas && mTexuresLoaded.size() ) {
		mTextureAtlas->setTextures( mTexuresLoaded );
	}

	mLoaded = true;

	if ( mLoadCallback.IsSet() ) {
		mLoadCallback( this );
	}
}

bool TextureAtlasLoader::isThreaded() const {
	return mThreaded;
}

void TextureAtlasLoader::setThreaded( const bool& threaded ) {
	mThreaded = threaded;
}

const bool& TextureAtlasLoader::isLoaded() const {
	return mLoaded;
}

const bool& TextureAtlasLoader::isLoading() const {
	return mIsLoading;
}

Texture * TextureAtlasLoader::getTexture( const Uint32& texnum ) const {
	eeASSERT( texnum < mTexuresLoaded.size() );
	return mTexuresLoaded[ texnum ];
}

Uint32 TextureAtlasLoader::getTexturesLoadedCount() {
	return mTexuresLoaded.size();
}

bool TextureAtlasLoader::updateTextureAtlas() {
	if ( NULL == mTextureAtlas || !mTextureAtlasPath.size() )
		return false;

	//! Update the data of the texture atlas
	for ( Uint32 z = 0; z < mTempAtlass.size(); z++ ) {
		sTempTexAtlas * tTexAtlas 	= &mTempAtlass[z];
		sTextureHdr * tTexHdr 		= &tTexAtlas->Texture;

		for ( Int32 i = 0; i < tTexHdr->SubTextureCount; i++ ) {
			sSubTextureHdr * tSh = &tTexAtlas->SubTextures[i];
			SubTexture * tSubTexture = mTextureAtlas->getById( tSh->ResourceID );

			if ( NULL != tSubTexture ) {
				tSh->OffsetX = tSubTexture->getOffset().x;
				tSh->OffsetY = tSubTexture->getOffset().x;
				tSh->DestWidth = (Int32)tSubTexture->getDestSize().x;
				tSh->DestHeight = (Int32)tSubTexture->getDestSize().x;
			}
		}
	}

	IOStreamFile fs( mTextureAtlasPath, std::ios::out | std::ios::binary );

	if ( fs.isOpen() ) {
		fs.write( reinterpret_cast<char*> (&mTexGrHdr), sizeof(sTextureAtlasHdr) );

		for ( Uint32 z = 0; z < mTempAtlass.size(); z++ ) {
			sTempTexAtlas * tTexAtlas 	= &mTempAtlass[z];
			sTextureHdr * tTexHdr 		= &tTexAtlas->Texture;

			fs.write( reinterpret_cast<char*> ( tTexHdr ), sizeof(sTextureHdr) );

			fs.write( reinterpret_cast<char*> ( &tTexAtlas->SubTextures[0] ), sizeof(sSubTextureHdr) * (std::streamsize)tTexAtlas->SubTextures.size() );
		}

		return true;
	}

	return false;
}

static bool IsImage( std::string path ) {
	if ( FileSystem::fileSize( path ) ) {
		std::string File	= path.substr( path.find_last_of("/\\") + 1 );
		std::string Ext		= File.substr( File.find_last_of(".") + 1 );
		String::toLowerInPlace( Ext );

		if ( Ext == "png" ||
			 Ext == "tga" ||
			 Ext == "bmp" ||
			 Ext == "jpg" ||
			 Ext == "gif" ||
			 Ext == "jpeg" ||
			 Ext == "dds" ||
			 Ext == "psd" ||
			 Ext == "hdr" ||
			 Ext == "pic" ||
			 Ext == "pvr" ||
			 Ext == "pkm"
		) {
			return true;
		} else {
			return Image::isImage( path );
		}
	}

	return false;
}

bool TextureAtlasLoader::updateTextureAtlas( std::string TextureAtlasPath, std::string ImagesPath ) {
	if ( !TextureAtlasPath.size() || !ImagesPath.size() || !FileSystem::fileExists( TextureAtlasPath ) || !FileSystem::isDirectory( ImagesPath ) )
		return false;

	mSkipResourceLoad = true;
	load( TextureAtlasPath );
	mSkipResourceLoad = false;

	if ( !mTempAtlass.size() )
		return false;

	Int32 x, y, c;

	Int32 NeedUpdate = 0;

	FileSystem::dirPathAddSlashAtEnd( ImagesPath );

	Uint32 z;

	Uint32 totalSubTextures = 0;
	for ( z = 0; z < mTempAtlass.size(); z++ )
		totalSubTextures += mTempAtlass[z].Texture.SubTextureCount;

	Uint32 totalImages = 0;
	std::vector<std::string> PathFiles = FileSystem::filesGetInPath( ImagesPath );

	for ( z = 0; z < PathFiles.size(); z++ ) {
		std::string realpath( ImagesPath + PathFiles[z] );

		// Avoids reading file headers for known extensions
		if ( IsImage( realpath ) )
			totalImages++;
	}

	if ( totalSubTextures != totalImages ) {
		NeedUpdate = 2;
	} else {
		for ( z = 0; z < mTempAtlass.size(); z++ ) {
			sTempTexAtlas * tTexAtlas 	= &mTempAtlass[z];
			sTextureHdr * tTexHdr 		= &tTexAtlas->Texture;

			if ( 2 != NeedUpdate ) {
				for ( Int32 i = 0; i < tTexHdr->SubTextureCount; i++ ) {
					sSubTextureHdr * tSh = &tTexAtlas->SubTextures[i];

					std::string path( ImagesPath + tSh->Name );

					if ( FileSystem::fileSize( path ) ) {
						if ( tSh->Date != FileSystem::fileGetModificationDate( path ) ) {
							if ( stbi_info( path.c_str(), &x, &y, &c ) ) {
								if ( 	( !( tSh->Flags & HDR_SUBTEXTURE_FLAG_FLIPED ) && tSh->Width == x && tSh->Height == y ) || // If size or channels changed, the image need update
										( ( tSh->Flags & HDR_SUBTEXTURE_FLAG_FLIPED ) && tSh->Width == y && tSh->Height == x ) ||
										tSh->Channels != c
								)
								{
									NeedUpdate = 1;	// Only update the image with the newest one
								} else {
									NeedUpdate = 2; // The image change it, recreate all
									break;
								}
							} else {
								NeedUpdate = 2; // Something is wrong on the image
								break;
							}
						}
					} else {
						NeedUpdate = 2; // Need recreation of the whole texture atlas, some image where deleted.
						break;
					}
				}
			} else {
				break;
			}
		}
	}

	if ( NeedUpdate ) {
		std::string tapath( FileSystem::fileRemoveExtension( TextureAtlasPath ) + "." + Image::saveTypeToExtension( mTexGrHdr.Format ) );

		if ( 2 == NeedUpdate ) {
			TexturePacker tp( mTexGrHdr.Width, mTexGrHdr.Height, PD_MDPI, 0 != ( mTexGrHdr.Flags & HDR_TEXTURE_ATLAS_POW_OF_TWO ), mTexGrHdr.PixelBorder, mTexGrHdr.Flags & HDR_TEXTURE_ATLAS_ALLOW_FLIPPING );

			tp.addTexturesPath( ImagesPath );

			tp.packTextures();

			tp.save( tapath, (EE_SAVE_TYPE)mTexGrHdr.Format );
		} else if ( 1 == NeedUpdate ) {
			std::string etapath = FileSystem::fileRemoveExtension( tapath ) + EE_TEXTURE_ATLAS_EXTENSION;

			IOStreamFile fs( etapath , std::ios::out | std::ios::binary );

			if ( !fs.isOpen() )
				return false;

			fs.write( reinterpret_cast<const char*> (&mTexGrHdr), sizeof(sTextureAtlasHdr) );

			for ( Uint32 z = 0; z < mTempAtlass.size(); z++ ) {
				if ( z != 0 ) {
					tapath = FileSystem::fileRemoveExtension( TextureAtlasPath ) + "_ch" + String::toStr( z ) + "." + Image::saveTypeToExtension( mTexGrHdr.Format );
				}

				unsigned char * imgPtr = stbi_load( tapath.c_str(), &x, &y, &c, 0 );

				if ( NULL != imgPtr ) {
					Image Img( imgPtr, x, y, c );
					Img.avoidFreeImage( true );

					sTempTexAtlas * tTexAtlas 	= &mTempAtlass[z];
					sTextureHdr * tTexHdr 		= &tTexAtlas->Texture;

					fs.write( reinterpret_cast<const char*> (tTexHdr), sizeof(sTextureHdr) );

					for ( Int32 i = 0; i < tTexHdr->SubTextureCount; i++ ) {
						sSubTextureHdr * tSh = &tTexAtlas->SubTextures[i];

						std::string imgcopypath( ImagesPath + tSh->Name );

						Uint32 ModifDate = FileSystem::fileGetModificationDate( imgcopypath );

						if ( tSh->Date != ModifDate ) {
							tSh->Date = ModifDate;	// Update the sub texture hdr

							unsigned char * imgCopyPtr = stbi_load( imgcopypath.c_str(), &x, &y, &c, 0 );

							if ( NULL != imgCopyPtr ) {
								Image ImgCopy( imgCopyPtr, x, y, c );
								ImgCopy.avoidFreeImage( true );

								Img.copyImage( &ImgCopy, tSh->X, tSh->Y );	// Update the image into the texture atlas

								if ( imgCopyPtr )
									free( imgCopyPtr );
							} else
								break;
						}
					}

					fs.write( reinterpret_cast<const char*> (&tTexAtlas->SubTextures[0]), sizeof(sSubTextureHdr) * tTexHdr->SubTextureCount );

					Img.saveToFile( tapath, (EE_SAVE_TYPE)mTexGrHdr.Format );

					if ( imgPtr )
						free( imgPtr );
				}
				else
					return false; // fatal error
			}
		}
	}

	return true;
}

}}
