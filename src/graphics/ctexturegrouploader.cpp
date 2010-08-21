#include "ctexturegrouploader.hpp"
#include "cshapegroup.hpp"
#include "cshapegroupmanager.hpp"

namespace EE { namespace Graphics {

cTextureGroupLoader::cTextureGroupLoader() :
	mThreaded(false),
	mLoaded(false),
	mAppPath( AppPath() )
{
}

cTextureGroupLoader::cTextureGroupLoader( const std::string& TextureGroupPath, const bool& Threaded ) :
	mTextureGroupPath( TextureGroupPath ),
	mThreaded( Threaded ),
	mLoaded(false),
	mAppPath( AppPath() )
{
	Load();
}

cTextureGroupLoader::~cTextureGroupLoader()
{
}

void cTextureGroupLoader::Update() {
	mRL.Update();

	if ( mRL.IsLoaded() && !mLoaded )
		CreateShapes();
}

void cTextureGroupLoader::Load( const std::string& TextureGroupPath ) {
	mRL.Threaded( mThreaded );

	if ( TextureGroupPath.size() )
		mTextureGroupPath = TextureGroupPath;

	sTextureGroupHdr TexGrHdr;

	std::fstream fs ( mTextureGroupPath.c_str() , std::ios::in | std::ios::binary );

	if ( fs.is_open() ) {
		fs.read( reinterpret_cast<char*> (&TexGrHdr), sizeof(sTextureGroupHdr) );

		if ( TexGrHdr.Magic == ( ( 'E' << 0 ) | ( 'E' << 8 ) | ( 'T' << 16 ) | ( 'G' << 24 ) ) ) {
			for ( Uint32 i = 0; i < TexGrHdr.TextureCount; i++ ) {
				sTextureHdr tTextureHdr;
				sTempTexGroup tTexGroup;

				fs.read( reinterpret_cast<char*> (&tTextureHdr), sizeof(sTextureHdr) );

				tTexGroup.Texture = tTextureHdr;
				tTexGroup.Shapes.resize( tTextureHdr.ShapeCount );

				std::string name( &tTextureHdr.Name[0] );
				std::string path( FileRemoveFileName( mTextureGroupPath ) + name );

				mRL.Add( new cTextureLoader( path ) );

				fs.read( reinterpret_cast<char*> (&tTexGroup.Shapes[0]), sizeof(sShapeHdr) * tTextureHdr.ShapeCount );

				mTempGroups.push_back( tTexGroup );
			}
		}

		mRL.Load();

		if ( !mThreaded )
			CreateShapes();
	}
}

void cTextureGroupLoader::CreateShapes() {
	cShapeGroup * tSG = NULL;

	for ( Uint32 z = 0; z < mTempGroups.size(); z++ ) {
		sTempTexGroup * tTexGroup 	= &mTempGroups[z];
		sTextureHdr * tTexHdr 		= &tTexGroup->Texture;

		std::string name( &tTexHdr->Name[0] );
		std::string path( FileRemoveFileName( mTextureGroupPath ) + name );

		Int32 pos = StrStartsWith( mAppPath, path );

		if ( -1 != pos && (Uint32)(pos + 1) < path.size() )
			path = path.substr( pos + 1 );

		cTexture * tTex 			= cTextureFactory::instance()->GetByName( path );

		// Create the Shape Group with the name of the real texture, not the Childs ( example load 1.png and not 1_ch1.png )
		if ( 0 == z ) {
			tSG = new cShapeGroup( name );
			cShapeGroupManager::instance()->Add( tSG );
		}

		if ( NULL != tTex ) {
			for ( Int32 i = 0; i < tTexHdr->ShapeCount; i++ ) {
				sShapeHdr * tSh = &tTexGroup->Shapes[i];

				std::string ShapeName( &tSh->Name[0] );
				eeRecti tRect( tSh->X, tSh->Y, tSh->X + tSh->Width, tSh->Y + tSh->Height );

				cShape * tShape = new cShape( tTex->TexId(), tRect, tSh->DestWidth, tSh->DestHeight, tSh->OffsetX, tSh->OffsetY, ShapeName );

				//if ( tSh->Flags & HDR_SHAPE_FLAG_FLIPED )
					// Should rotate the shape, but.. shape rotation is not stored.

				tSG->Add( tShape );
			}
		} else {
			/** @TODO: Error Report */
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

}}

