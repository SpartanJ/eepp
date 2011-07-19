#include "ctexturegrouploader.hpp"
#include "cshapegroup.hpp"
#include "cshapegroupmanager.hpp"
#include "ctexturepacker.hpp"
#include "cshapegroup.hpp"
#include "../system/ciostreamfile.hpp"
#include "../system/ciostreammemory.hpp"

namespace EE { namespace Graphics {

cTextureGroupLoader::cTextureGroupLoader() :
	mThreaded(false),
	mLoaded(false),
	mAppPath( GetProcessPath() ),
	mPack(NULL),
	mSkipResourceLoad(false),
	mIsLoading(false),
	mShapeGroup(NULL)
{
}

cTextureGroupLoader::cTextureGroupLoader( const std::string& TextureGroupPath, const bool& Threaded ) :
	mTextureGroupPath( TextureGroupPath ),
	mThreaded( Threaded ),
	mLoaded(false),
	mAppPath( GetProcessPath() ),
	mPack(NULL),
	mSkipResourceLoad(false),
	mIsLoading(false),
	mShapeGroup(NULL)
{
	Load();
}

cTextureGroupLoader::cTextureGroupLoader( const Uint8* Data, const Uint32& DataSize, const std::string& TextureGroupName, const bool& Threaded ) :
	mTextureGroupPath( TextureGroupName ),
	mThreaded( Threaded ),
	mLoaded(false),
	mAppPath( GetProcessPath() ),
	mPack(NULL),
	mSkipResourceLoad(false),
	mIsLoading(false),
	mShapeGroup(NULL)
{
	LoadFromMemory( Data, DataSize, TextureGroupName );
}

cTextureGroupLoader::cTextureGroupLoader( cPack * Pack, const std::string& FilePackPath, const bool& Threaded ) :
	mTextureGroupPath( FilePackPath ),
	mThreaded( Threaded ),
	mLoaded(false),
	mAppPath( GetProcessPath() ),
	mPack(NULL),
	mSkipResourceLoad(false),
	mIsLoading(false),
	mShapeGroup(NULL)
{
	LoadFromPack( Pack, FilePackPath );
}

cTextureGroupLoader::~cTextureGroupLoader()
{
}

void cTextureGroupLoader::Update() {
	mRL.Update();

	if ( mRL.IsLoaded() && !mLoaded )
		CreateShapes();
}

void cTextureGroupLoader::LoadFromStream( cIOStream& IOS ) {
	mRL.Threaded( mThreaded );

	if ( IOS.IsOpen() ) {
		IOS.Read( (char*)&mTexGrHdr, sizeof(sTextureGroupHdr) );

		if ( mTexGrHdr.Magic == ( ( 'E' << 0 ) | ( 'E' << 8 ) | ( 'T' << 16 ) | ( 'G' << 24 ) ) ) {
			for ( Uint32 i = 0; i < mTexGrHdr.TextureCount; i++ ) {
				sTextureHdr tTextureHdr;
				sTempTexGroup tTexGroup;

				IOS.Read( (char*)&tTextureHdr, sizeof(sTextureHdr) );

				tTexGroup.Texture = tTextureHdr;
				tTexGroup.Shapes.resize( tTextureHdr.ShapeCount );

				std::string name( &tTextureHdr.Name[0] );
				std::string path( FileRemoveFileName( mTextureGroupPath ) + name );

				//! Checks if the texture is already loaded
				cTexture * tTex = cTextureFactory::instance()->GetByName( path );

				if ( !mSkipResourceLoad && NULL == tTex ) {
					if ( NULL != mPack ) {
						mRL.Add( eeNew( cTextureLoader, ( mPack, path ) ) );
					} else {
						mRL.Add( eeNew( cTextureLoader, ( path ) ) );
					}
				}

				IOS.Read( (char*)&tTexGroup.Shapes[0], sizeof(sShapeHdr) * tTextureHdr.ShapeCount );

				mTempGroups.push_back( tTexGroup );
			}
		}

		if ( !mSkipResourceLoad || ( !mSkipResourceLoad && 0 == mRL.Count() ) ) {
			mIsLoading = true;
			mRL.Load();

			if ( !mThreaded || ( !mSkipResourceLoad && 0 == mRL.Count() ) )
				CreateShapes();
		}
	}
}

void cTextureGroupLoader::Load( const std::string& TextureGroupPath ) {
	if ( TextureGroupPath.size() )
		mTextureGroupPath = TextureGroupPath;

	if ( FileExists( mTextureGroupPath ) ) {
		cIOStreamFile IOS( mTextureGroupPath, std::ios::in | std::ios::binary );

		LoadFromStream( IOS );
	}
}

void cTextureGroupLoader::LoadFromPack( cPack * Pack, const std::string& FilePackPath ) {
	if ( NULL != Pack && Pack->IsOpen() && -1 != Pack->Exists( FilePackPath ) ) {
		mPack = Pack;

		SafeDataPointer PData;

		Pack->ExtractFileToMemory( FilePackPath, PData );

		LoadFromMemory( reinterpret_cast<const Uint8*> ( PData.Data ), PData.DataSize, FilePackPath );
	}
}

void cTextureGroupLoader::LoadFromMemory( const Uint8* Data, const Uint32& DataSize, const std::string& TextureGroupName ) {
	if ( TextureGroupName.size() )
		mTextureGroupPath = TextureGroupName;

	cIOStreamMemory IOS( (const char*)Data, DataSize );

	LoadFromStream( IOS );
}

cShapeGroup * cTextureGroupLoader::GetShapeGroup() const {
	return mShapeGroup;
}

void cTextureGroupLoader::CreateShapes() {
	mIsLoading = false;
	bool IsAlreadyLoaded = false;
	
	for ( Uint32 z = 0; z < mTempGroups.size(); z++ ) {
		sTempTexGroup * tTexGroup 	= &mTempGroups[z];
		sTextureHdr * tTexHdr 		= &tTexGroup->Texture;

		std::string name( &tTexHdr->Name[0] );
		std::string path( FileRemoveFileName( mTextureGroupPath ) + name );

		Int32 pos = StrStartsWith( mAppPath, path );

		// Removes the app path part
		if ( -1 != pos && (Uint32)(pos + 1) < path.size() )
			path = path.substr( pos + 1 );

		cTexture * tTex 			= cTextureFactory::instance()->GetByName( path );

		if ( NULL != tTex )
			mTexuresLoaded.push_back( tTex );

		// Create the Shape Group with the name of the real texture, not the Childs ( example load 1.png and not 1_ch1.png )
		if ( 0 == z ) {
			if ( mTexGrHdr.Flags & HDR_TEXTURE_GROUP_REMOVE_EXTENSION )
				name = FileRemoveExtension( name );

			std::string etgpath = FileRemoveExtension( path ) + ".etg";

			cShapeGroup * tShapeGroup = cShapeGroupManager::instance()->GetByName( name );

			if ( NULL != tShapeGroup && tShapeGroup->Path() == etgpath ) {
				mShapeGroup = tShapeGroup;

				IsAlreadyLoaded = true;
			} else {
				mShapeGroup = eeNew( cShapeGroup, ( name ) );

				mShapeGroup->Path( etgpath );

				cShapeGroupManager::instance()->Add( mShapeGroup );
			}
		}

		if ( NULL != tTex ) {
			if ( !IsAlreadyLoaded ) {
				for ( Int32 i = 0; i < tTexHdr->ShapeCount; i++ ) {
					sShapeHdr * tSh = &tTexGroup->Shapes[i];

					std::string ShapeName( &tSh->Name[0] );

					if ( mTexGrHdr.Flags & HDR_TEXTURE_GROUP_REMOVE_EXTENSION )
						ShapeName = FileRemoveExtension( ShapeName );

					eeRecti tRect( tSh->X, tSh->Y, tSh->X + tSh->Width, tSh->Y + tSh->Height );

					cShape * tShape = eeNew( cShape, ( tTex->Id(), tRect, (eeFloat)tSh->DestWidth, (eeFloat)tSh->DestHeight, tSh->OffsetX, tSh->OffsetY, ShapeName ) );

					//if ( tSh->Flags & HDR_SHAPE_FLAG_FLIPED )
						// Should rotate the shape, but.. shape rotation is not stored.

					mShapeGroup->Add( tShape );
				}
			}
		} else {
			cLog::instance()->Write( "cTextureGroupLoader::CreateShapes: Failed to find texture group texture, it seems that is not loaded for some reason. Couldn't find: " + path );

			eeASSERT( NULL != tTex );

			return;
		}
	}

	mLoaded = true;
}

bool cTextureGroupLoader::Threaded() const {
	return mThreaded;
}

void cTextureGroupLoader::Threaded( const bool& threaded ) {
	mThreaded = threaded;
}

const bool& cTextureGroupLoader::IsLoaded() const {
	return mLoaded;
}

const bool& cTextureGroupLoader::IsLoading() const {
	return mIsLoading;
}

cTexture * cTextureGroupLoader::GetTexture( const Uint32& texnum ) const {
	eeASSERT( texnum < mTexuresLoaded.size() );
	return mTexuresLoaded[ texnum ];
}

Uint32 cTextureGroupLoader::GetTexturesLoadedCount() {
	return mTexuresLoaded.size();
}

bool cTextureGroupLoader::UpdateTextureAtlas() {
	if ( NULL == mShapeGroup || !mTextureGroupPath.size() )
		return false;

	//! Update the data of the shape groups
	for ( Uint32 z = 0; z < mTempGroups.size(); z++ ) {
		sTempTexGroup * tTexGroup 	= &mTempGroups[z];
		sTextureHdr * tTexHdr 		= &tTexGroup->Texture;

		for ( Int32 i = 0; i < tTexHdr->ShapeCount; i++ ) {
			sShapeHdr * tSh = &tTexGroup->Shapes[i];
			cShape * tShape = mShapeGroup->GetById( tSh->ResourceID );

			if ( NULL != tShape ) {
				tSh->OffsetX = tShape->OffsetX();
				tSh->OffsetY = tShape->OffsetY();
				tSh->DestWidth = (Int32)tShape->DestWidth();
				tSh->DestHeight = (Int32)tShape->DestHeight();
			}
		}
	}

	std::fstream fs ( mTextureGroupPath.c_str() , std::ios::out | std::ios::binary );

	if ( fs.is_open() ) {
		fs.write( reinterpret_cast<char*> (&mTexGrHdr), sizeof(sTextureGroupHdr) );

		for ( Uint32 z = 0; z < mTempGroups.size(); z++ ) {
			sTempTexGroup * tTexGroup 	= &mTempGroups[z];
			sTextureHdr * tTexHdr 		= &tTexGroup->Texture;

			fs.write( reinterpret_cast<char*> ( tTexHdr ), sizeof(sTextureHdr) );

			fs.write( reinterpret_cast<char*> ( &tTexGroup->Shapes[0] ), sizeof(sShapeHdr) * (std::streamsize)tTexGroup->Shapes.size() );
		}

		fs.close();

		return true;
	}

	return false;
}

bool cTextureGroupLoader::UpdateTextureAtlas( std::string TextureAtlasPath, std::string ImagesPath ) {
	if ( !TextureAtlasPath.size() || !ImagesPath.size() || !FileExists( TextureAtlasPath ) || !IsDirectory( ImagesPath ) )
		return false;

	mSkipResourceLoad = true;
	Load( TextureAtlasPath );
	mSkipResourceLoad = false;

	if ( !mTempGroups.size() )
		return false;

	Int32 x, y, c;

	Int32 NeedUpdate = 0;

	DirPathAddSlashAtEnd( ImagesPath );

	Uint32 z;

	Uint32 totalShapes = 0;
	for ( z = 0; z < mTempGroups.size(); z++ )
		totalShapes += mTempGroups[z].Texture.ShapeCount;

	Uint32 totalImages = 0;
	std::vector<std::string> PathFiles = FilesGetInPath( ImagesPath );

	for ( z = 0; z < PathFiles.size(); z++ ) {
		std::string realpath( ImagesPath + PathFiles[z] );

		if ( stbi_info( realpath.c_str(), &x, &y, &c ) )
			totalImages++;
	}

	if ( totalShapes != totalImages ) {
		NeedUpdate = 2;
	} else {
		for ( z = 0; z < mTempGroups.size(); z++ ) {
			sTempTexGroup * tTexGroup 	= &mTempGroups[z];
			sTextureHdr * tTexHdr 		= &tTexGroup->Texture;

			if ( 2 != NeedUpdate ) {
				for ( Int32 i = 0; i < tTexHdr->ShapeCount; i++ ) {
					sShapeHdr * tSh = &tTexGroup->Shapes[i];

					std::string path( ImagesPath + tSh->Name );

					if ( FileExists( path ) ) {
						if ( stbi_info( path.c_str(), &x, &y, &c ) ) {
							if ( tSh->Date != FileGetModificationDate( path ) ) {
								if ( 	( !( tSh->Flags & HDR_SHAPE_FLAG_FLIPED ) && tSh->Width == x && tSh->Height == y ) || // If size or channels changed, the image need update
										( ( tSh->Flags & HDR_SHAPE_FLAG_FLIPED ) && tSh->Width == y && tSh->Height == x ) ||
										tSh->Channels != c
								)
								{
									NeedUpdate = 1;
								} else {
									NeedUpdate = 2;
									break;
								}
							}
						} else {
							NeedUpdate = 2; // Something is wrong on the image
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
		std::string tapath( FileRemoveExtension( TextureAtlasPath ) + "." + SaveTypeToExtension( mTexGrHdr.Format ) );

		if ( 2 == NeedUpdate ) {
			cTexturePacker tp( mTexGrHdr.Width, mTexGrHdr.Height, 0 != ( mTexGrHdr.Flags & HDR_TEXTURE_GROUP_POW_OF_TWO ), mTexGrHdr.PixelBorder, mTexGrHdr.Flags & HDR_TEXTURE_GROUP_ALLOW_FLIPPING );

			tp.AddTexturesPath( ImagesPath );

			tp.PackTextures();

			tp.Save( tapath, (EE_SAVE_TYPE)mTexGrHdr.Format );
		} else if ( 1 == NeedUpdate ) {
			std::string etgpath = FileRemoveExtension( tapath ) + ".etg";
			std::fstream fs ( etgpath.c_str() , std::ios::out | std::ios::binary );

			if ( !fs.is_open() )
				return false;

			fs.write( reinterpret_cast<const char*> (&mTexGrHdr), sizeof(sTextureGroupHdr) );

			for ( Uint32 z = 0; z < mTempGroups.size(); z++ ) {
				if ( z != 0 ) {
					tapath = FileRemoveExtension( TextureAtlasPath ) + "_ch" + toStr( z ) + "." + SaveTypeToExtension( mTexGrHdr.Format );
				}

				unsigned char * imgPtr = stbi_load( tapath.c_str(), &x, &y, &c, 0 );

				if ( NULL != imgPtr ) {
					cImage Img( imgPtr, x, y, c );
					Img.AvoidFreeImage( true );

					sTempTexGroup * tTexGroup 	= &mTempGroups[z];
					sTextureHdr * tTexHdr 		= &tTexGroup->Texture;

					fs.write( reinterpret_cast<const char*> (tTexHdr), sizeof(sTextureHdr) );

					for ( Int32 i = 0; i < tTexHdr->ShapeCount; i++ ) {
						sShapeHdr * tSh = &tTexGroup->Shapes[i];

						std::string imgcopypath( ImagesPath + tSh->Name );

						Uint32 ModifDate = FileGetModificationDate( imgcopypath );

						if ( tSh->Date != ModifDate ) {
							tSh->Date = ModifDate;	// Update the shape hdr

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

					fs.write( reinterpret_cast<const char*> (&tTexGroup->Shapes[0]), sizeof(sShapeHdr) * tTexHdr->ShapeCount );

					Img.SaveToFile( tapath, (EE_SAVE_TYPE)mTexGrHdr.Format );

					if ( imgPtr )
						free( imgPtr );
				}
				else
					return false; // fatal error
			}

			fs.close();
		}
	}

	return true;
}

std::string	cTextureGroupLoader::AppPath() const {
	return mAppPath;
}

}}
