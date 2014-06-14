#include <eepp/graphics/ctextureatlasloader.hpp>
#include <eepp/graphics/ctextureatlas.hpp>
#include <eepp/graphics/ctextureatlasmanager.hpp>
#include <eepp/graphics/ctexturepacker.hpp>
#include <eepp/graphics/ctextureatlas.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/iostreammemory.hpp>
#include <eepp/graphics/packerhelper.hpp>
#include <eepp/helper/SOIL2/src/SOIL2/stb_image.h>

namespace EE { namespace Graphics {

using namespace Private;

cTextureAtlasLoader::cTextureAtlasLoader() :
	mThreaded(false),
	mLoaded(false),
	mPack(NULL),
	mSkipResourceLoad(false),
	mIsLoading(false),
	mTextureAtlas(NULL)
{
}

cTextureAtlasLoader::cTextureAtlasLoader( const std::string& TextureAtlasPath, const bool& Threaded, GLLoadCallback LoadCallback ) :
	mTextureAtlasPath( TextureAtlasPath ),
	mThreaded( Threaded ),
	mLoaded(false),
	mPack(NULL),
	mSkipResourceLoad(false),
	mIsLoading(false),
	mTextureAtlas(NULL),
	mLoadCallback( LoadCallback )
{
	Load();
}

cTextureAtlasLoader::cTextureAtlasLoader( const Uint8* Data, const Uint32& DataSize, const std::string& TextureAtlasName, const bool& Threaded, GLLoadCallback LoadCallback ) :
	mTextureAtlasPath( TextureAtlasName ),
	mThreaded( Threaded ),
	mLoaded(false),
	mPack(NULL),
	mSkipResourceLoad(false),
	mIsLoading(false),
	mTextureAtlas(NULL),
	mLoadCallback( LoadCallback )
{
	LoadFromMemory( Data, DataSize, TextureAtlasName );
}

cTextureAtlasLoader::cTextureAtlasLoader( Pack * Pack, const std::string& FilePackPath, const bool& Threaded, GLLoadCallback LoadCallback ) :
	mTextureAtlasPath( FilePackPath ),
	mThreaded( Threaded ),
	mLoaded(false),
	mPack(NULL),
	mSkipResourceLoad(false),
	mIsLoading(false),
	mTextureAtlas(NULL),
	mLoadCallback( LoadCallback )
{
	LoadFromPack( Pack, FilePackPath );
}

cTextureAtlasLoader::cTextureAtlasLoader( IOStream& IOS, const bool& Threaded, GLLoadCallback LoadCallback ) :
	mThreaded( Threaded ),
	mLoaded(false),
	mPack(NULL),
	mSkipResourceLoad(false),
	mIsLoading(false),
	mTextureAtlas(NULL),
	mLoadCallback( LoadCallback )
{
	LoadFromStream( IOS );
}

cTextureAtlasLoader::~cTextureAtlasLoader()
{
}

void cTextureAtlasLoader::SetLoadCallback( GLLoadCallback LoadCallback ) {
	mLoadCallback = LoadCallback;
}

void cTextureAtlasLoader::Update() {
	mRL.Update();

	if ( mRL.IsLoaded() && !mLoaded )
		CreateSubTextures();
}

void cTextureAtlasLoader::LoadFromStream( IOStream& IOS ) {
	mRL.Threaded( mThreaded );

	if ( IOS.IsOpen() ) {
		IOS.Read( (char*)&mTexGrHdr, sizeof(sTextureAtlasHdr) );

		if ( mTexGrHdr.Magic == EE_TEXTURE_ATLAS_MAGIC || mTexGrHdr.Magic == EE_TEXTURE_ATLAS_MAGIC_OLD ) {
			for ( Uint32 i = 0; i < mTexGrHdr.TextureCount; i++ ) {
				sTextureHdr tTextureHdr;
				sTempTexAtlas tTexAtlas;

				IOS.Read( (char*)&tTextureHdr, sizeof(sTextureHdr) );

				tTexAtlas.Texture = tTextureHdr;
				tTexAtlas.SubTextures.resize( tTextureHdr.SubTextureCount );

				std::string name( &tTextureHdr.Name[0] );
				std::string path( FileSystem::FileRemoveFileName( mTextureAtlasPath ) + name );

				//! Checks if the texture is already loaded
				cTexture * tTex = cTextureFactory::instance()->GetByName( path );

				if ( !mSkipResourceLoad && NULL == tTex ) {
					if ( NULL != mPack ) {
						mRL.Add( eeNew( cTextureLoader, ( mPack, path ) ) );
					} else {
						mRL.Add( eeNew( cTextureLoader, ( path ) ) );
					}
				}

				IOS.Read( (char*)&tTexAtlas.SubTextures[0], sizeof(sSubTextureHdr) * tTextureHdr.SubTextureCount );

				mTempAtlass.push_back( tTexAtlas );
			}
		}

		if ( !mSkipResourceLoad || ( !mSkipResourceLoad && 0 == mRL.Count() ) ) {
			mIsLoading = true;
			mRL.Load();

			if ( !mThreaded || ( !mSkipResourceLoad && 0 == mRL.Count() ) )
				CreateSubTextures();
		}
	}
}

void cTextureAtlasLoader::Load( const std::string& TextureAtlasPath ) {
	if ( TextureAtlasPath.size() )
		mTextureAtlasPath = TextureAtlasPath;

	if ( FileSystem::FileExists( mTextureAtlasPath ) ) {
		IOStreamFile IOS( mTextureAtlasPath, std::ios::in | std::ios::binary );

		LoadFromStream( IOS );
	} else if ( PackManager::instance()->FallbackToPacks() ) {
		std::string tgPath( mTextureAtlasPath );

		Pack * tPack = PackManager::instance()->Exists( tgPath );

		if ( NULL != tPack ) {
			LoadFromPack( tPack, tgPath );
		}
	}
}

void cTextureAtlasLoader::LoadFromPack( Pack * Pack, const std::string& FilePackPath ) {
	if ( NULL != Pack && Pack->IsOpen() && -1 != Pack->Exists( FilePackPath ) ) {
		mPack = Pack;

		SafeDataPointer PData;

		Pack->ExtractFileToMemory( FilePackPath, PData );

		LoadFromMemory( reinterpret_cast<const Uint8*> ( PData.Data ), PData.DataSize, FilePackPath );
	}
}

void cTextureAtlasLoader::LoadFromMemory( const Uint8* Data, const Uint32& DataSize, const std::string& TextureAtlasName ) {
	if ( TextureAtlasName.size() )
		mTextureAtlasPath = TextureAtlasName;

	IOStreamMemory IOS( (const char*)Data, DataSize );

	LoadFromStream( IOS );
}

cTextureAtlas * cTextureAtlasLoader::GetTextureAtlas() const {
	return mTextureAtlas;
}

void cTextureAtlasLoader::CreateSubTextures() {
	mIsLoading = false;
	bool IsAlreadyLoaded = false;
	
	for ( Uint32 z = 0; z < mTempAtlass.size(); z++ ) {
		sTempTexAtlas * tTexAtlas 	= &mTempAtlass[z];
		sTextureHdr * tTexHdr 		= &tTexAtlas->Texture;

		std::string name( &tTexHdr->Name[0] );
		std::string path( FileSystem::FileRemoveFileName( mTextureAtlasPath ) + name );

		FileSystem::FilePathRemoveProcessPath( path );

		cTexture * tTex 			= cTextureFactory::instance()->GetByName( path );

		if ( NULL != tTex )
			mTexuresLoaded.push_back( tTex );

		// Create the Texture Atlas with the name of the real texture, not the Childs ( example load 1.png and not 1_ch1.png )
		if ( 0 == z ) {
			if ( mTexGrHdr.Flags & HDR_TEXTURE_ATLAS_REMOVE_EXTENSION )
				name = FileSystem::FileRemoveExtension( name );

			std::string etapath = FileSystem::FileRemoveExtension( path ) + EE_TEXTURE_ATLAS_EXTENSION;

			cTextureAtlas * tTextureAtlas = cTextureAtlasManager::instance()->GetByName( name );

			if ( NULL != tTextureAtlas && tTextureAtlas->Path() == etapath ) {
				mTextureAtlas = tTextureAtlas;

				IsAlreadyLoaded = true;
			} else {
				mTextureAtlas = eeNew( cTextureAtlas, ( name ) );

				mTextureAtlas->Path( etapath );

				cTextureAtlasManager::instance()->Add( mTextureAtlas );
			}
		}

		if ( NULL != tTex ) {
			if ( !IsAlreadyLoaded ) {
				for ( Int32 i = 0; i < tTexHdr->SubTextureCount; i++ ) {
					sSubTextureHdr * tSh = &tTexAtlas->SubTextures[i];

					std::string SubTextureName( &tSh->Name[0] );

					if ( mTexGrHdr.Flags & HDR_TEXTURE_ATLAS_REMOVE_EXTENSION )
						SubTextureName = FileSystem::FileRemoveExtension( SubTextureName );

					eeRecti tRect( tSh->X, tSh->Y, tSh->X + tSh->Width, tSh->Y + tSh->Height );

					cSubTexture * tSubTexture = eeNew( cSubTexture, ( tTex->Id(), tRect, eeSizef( (Float)tSh->DestWidth, (Float)tSh->DestHeight ), eeVector2i( tSh->OffsetX, tSh->OffsetY ), SubTextureName ) );

					//if ( tSh->Flags & HDR_SUBTEXTURE_FLAG_FLIPED )
						// Should rotate the sub texture, but.. sub texture rotation is not stored.

					mTextureAtlas->Add( tSubTexture );
				}
			}
		} else {
			eePRINTL( "cTextureAtlasLoader::CreateSubTextures: Failed to find texture atlas texture, it seems that is not loaded for some reason. Couldn't find: %s", path.c_str() );

			eeASSERT( NULL != tTex );

			return;
		}
	}

	if ( NULL != mTextureAtlas && mTexuresLoaded.size() ) {
		mTextureAtlas->SetTextures( mTexuresLoaded );
	}

	mLoaded = true;

	if ( mLoadCallback.IsSet() ) {
		mLoadCallback( this );
	}
}

bool cTextureAtlasLoader::Threaded() const {
	return mThreaded;
}

void cTextureAtlasLoader::Threaded( const bool& threaded ) {
	mThreaded = threaded;
}

const bool& cTextureAtlasLoader::IsLoaded() const {
	return mLoaded;
}

const bool& cTextureAtlasLoader::IsLoading() const {
	return mIsLoading;
}

cTexture * cTextureAtlasLoader::GetTexture( const Uint32& texnum ) const {
	eeASSERT( texnum < mTexuresLoaded.size() );
	return mTexuresLoaded[ texnum ];
}

Uint32 cTextureAtlasLoader::GetTexturesLoadedCount() {
	return mTexuresLoaded.size();
}

bool cTextureAtlasLoader::UpdateTextureAtlas() {
	if ( NULL == mTextureAtlas || !mTextureAtlasPath.size() )
		return false;

	//! Update the data of the texture atlas
	for ( Uint32 z = 0; z < mTempAtlass.size(); z++ ) {
		sTempTexAtlas * tTexAtlas 	= &mTempAtlass[z];
		sTextureHdr * tTexHdr 		= &tTexAtlas->Texture;

		for ( Int32 i = 0; i < tTexHdr->SubTextureCount; i++ ) {
			sSubTextureHdr * tSh = &tTexAtlas->SubTextures[i];
			cSubTexture * tSubTexture = mTextureAtlas->GetById( tSh->ResourceID );

			if ( NULL != tSubTexture ) {
				tSh->OffsetX = tSubTexture->Offset().x;
				tSh->OffsetY = tSubTexture->Offset().x;
				tSh->DestWidth = (Int32)tSubTexture->DestSize().x;
				tSh->DestHeight = (Int32)tSubTexture->DestSize().x;
			}
		}
	}

	IOStreamFile fs( mTextureAtlasPath, std::ios::out | std::ios::binary );

	if ( fs.IsOpen() ) {
		fs.Write( reinterpret_cast<char*> (&mTexGrHdr), sizeof(sTextureAtlasHdr) );

		for ( Uint32 z = 0; z < mTempAtlass.size(); z++ ) {
			sTempTexAtlas * tTexAtlas 	= &mTempAtlass[z];
			sTextureHdr * tTexHdr 		= &tTexAtlas->Texture;

			fs.Write( reinterpret_cast<char*> ( tTexHdr ), sizeof(sTextureHdr) );

			fs.Write( reinterpret_cast<char*> ( &tTexAtlas->SubTextures[0] ), sizeof(sSubTextureHdr) * (std::streamsize)tTexAtlas->SubTextures.size() );
		}

		return true;
	}

	return false;
}

static bool IsImage( std::string path ) {
	if ( FileSystem::FileSize( path ) ) {
		std::string File	= path.substr( path.find_last_of("/\\") + 1 );
		std::string Ext		= File.substr( File.find_last_of(".") + 1 );
		String::ToLower( Ext );

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
			return cImage::IsImage( path );
		}
	}

	return false;
}

bool cTextureAtlasLoader::UpdateTextureAtlas( std::string TextureAtlasPath, std::string ImagesPath ) {
	if ( !TextureAtlasPath.size() || !ImagesPath.size() || !FileSystem::FileExists( TextureAtlasPath ) || !FileSystem::IsDirectory( ImagesPath ) )
		return false;

	mSkipResourceLoad = true;
	Load( TextureAtlasPath );
	mSkipResourceLoad = false;

	if ( !mTempAtlass.size() )
		return false;

	Int32 x, y, c;

	Int32 NeedUpdate = 0;

	FileSystem::DirPathAddSlashAtEnd( ImagesPath );

	Uint32 z;

	Uint32 totalSubTextures = 0;
	for ( z = 0; z < mTempAtlass.size(); z++ )
		totalSubTextures += mTempAtlass[z].Texture.SubTextureCount;

	Uint32 totalImages = 0;
	std::vector<std::string> PathFiles = FileSystem::FilesGetInPath( ImagesPath );

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

					if ( FileSystem::FileSize( path ) ) {
						if ( tSh->Date != FileSystem::FileGetModificationDate( path ) ) {
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
		std::string tapath( FileSystem::FileRemoveExtension( TextureAtlasPath ) + "." + cImage::SaveTypeToExtension( mTexGrHdr.Format ) );

		if ( 2 == NeedUpdate ) {
			cTexturePacker tp( mTexGrHdr.Width, mTexGrHdr.Height, 0 != ( mTexGrHdr.Flags & HDR_TEXTURE_ATLAS_POW_OF_TWO ), mTexGrHdr.PixelBorder, mTexGrHdr.Flags & HDR_TEXTURE_ATLAS_ALLOW_FLIPPING );

			tp.AddTexturesPath( ImagesPath );

			tp.PackTextures();

			tp.Save( tapath, (EE_SAVE_TYPE)mTexGrHdr.Format );
		} else if ( 1 == NeedUpdate ) {
			std::string etapath = FileSystem::FileRemoveExtension( tapath ) + EE_TEXTURE_ATLAS_EXTENSION;

			IOStreamFile fs( etapath , std::ios::out | std::ios::binary );

			if ( !fs.IsOpen() )
				return false;

			fs.Write( reinterpret_cast<const char*> (&mTexGrHdr), sizeof(sTextureAtlasHdr) );

			for ( Uint32 z = 0; z < mTempAtlass.size(); z++ ) {
				if ( z != 0 ) {
					tapath = FileSystem::FileRemoveExtension( TextureAtlasPath ) + "_ch" + String::ToStr( z ) + "." + cImage::SaveTypeToExtension( mTexGrHdr.Format );
				}

				unsigned char * imgPtr = stbi_load( tapath.c_str(), &x, &y, &c, 0 );

				if ( NULL != imgPtr ) {
					cImage Img( imgPtr, x, y, c );
					Img.AvoidFreeImage( true );

					sTempTexAtlas * tTexAtlas 	= &mTempAtlass[z];
					sTextureHdr * tTexHdr 		= &tTexAtlas->Texture;

					fs.Write( reinterpret_cast<const char*> (tTexHdr), sizeof(sTextureHdr) );

					for ( Int32 i = 0; i < tTexHdr->SubTextureCount; i++ ) {
						sSubTextureHdr * tSh = &tTexAtlas->SubTextures[i];

						std::string imgcopypath( ImagesPath + tSh->Name );

						Uint32 ModifDate = FileSystem::FileGetModificationDate( imgcopypath );

						if ( tSh->Date != ModifDate ) {
							tSh->Date = ModifDate;	// Update the sub texture hdr

							unsigned char * imgCopyPtr = stbi_load( imgcopypath.c_str(), &x, &y, &c, 0 );

							if ( NULL != imgCopyPtr ) {
								cImage ImgCopy( imgCopyPtr, x, y, c );
								ImgCopy.AvoidFreeImage( true );

								Img.CopyImage( &ImgCopy, tSh->X, tSh->Y );	// Update the image into the texture atlas

								if ( imgCopyPtr )
									free( imgCopyPtr );
							} else
								break;
						}
					}

					fs.Write( reinterpret_cast<const char*> (&tTexAtlas->SubTextures[0]), sizeof(sSubTextureHdr) * tTexHdr->SubTextureCount );

					Img.SaveToFile( tapath, (EE_SAVE_TYPE)mTexGrHdr.Format );

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
