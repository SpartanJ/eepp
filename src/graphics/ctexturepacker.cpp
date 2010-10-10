#include "ctexturepacker.hpp"
#include "cimage.hpp"
#include "../helper/SOIL/SOIL.h"

namespace EE { namespace Graphics {

cTexturePacker::cTexturePacker( const Uint32& MaxWidth, const Uint32& MaxHeight, const bool& ForcePowOfTwo, const Uint32& PixelBorder, const bool& AllowFlipping ) :
	mLongestEdge(0),
	mTotalArea(0),
	mFreeList(NULL),
	mWidth(0),
	mHeight(0),
	mPacked(false),
	mAllowFlipping(false),
	mChild(NULL),
	mStrategy(PackBig),
	mCount(0),
	mParent(NULL),
	mPlacedCount(0),
	mForcePowOfTwo(true),
	mPixelBorder(0)
{
	SetOptions( MaxWidth, MaxHeight, ForcePowOfTwo, PixelBorder, AllowFlipping );
}

cTexturePacker::cTexturePacker() :
	mLongestEdge(0),
	mTotalArea(0),
	mFreeList(NULL),
	mWidth(0),
	mHeight(0),
	mPacked(false),
	mAllowFlipping(false),
	mChild(NULL),
	mStrategy(PackBig),
	mCount(0),
	mParent(NULL),
	mPlacedCount(0),
	mForcePowOfTwo(true),
	mPixelBorder(0)
{
}

cTexturePacker::~cTexturePacker()
{
	Close();
}

void cTexturePacker::Close() {
	mLongestEdge 	= 0;
	mTotalArea 		= 0;
	mTextures.clear();

	if ( NULL != mFreeList ) {
		cTexturePackerNode * next = mFreeList;
		cTexturePackerNode * kill = NULL;

		while ( NULL != next ) {
			kill = next;

			next = next->GetNext();

			eeSAFE_DELETE( kill );
		}
	}

	eeSAFE_DELETE( mChild );
}

void cTexturePacker::SetOptions( const Uint32& MaxWidth, const Uint32& MaxHeight, const bool& ForcePowOfTwo, const Uint32& PixelBorder, const bool& AllowFlipping ) {
	if ( !mTextures.size() ) { // only can change the dimensions before adding any texture
		mWidth 	= MaxWidth;
		mHeight = MaxHeight;

		if ( ForcePowOfTwo && !IsPow2( mWidth ) )
			mWidth = NextPowOfTwo( mWidth );

		if ( ForcePowOfTwo && !IsPow2( mHeight ) )
			mHeight = NextPowOfTwo( mHeight );

		mForcePowOfTwo 	= ForcePowOfTwo;
		mAllowFlipping 	= AllowFlipping;
		mPixelBorder	= PixelBorder;
	}
}

void cTexturePacker::NewFree( Int32 x, Int32 y, Int32 width, Int32 height ) {
	cTexturePackerNode * node = eeNew( cTexturePackerNode, ( x, y, width, height ) );
	node->SetNext( mFreeList );
	mFreeList = node;
}

bool cTexturePacker::MergeNodes() {
	cTexturePackerNode *f = mFreeList;

	while ( f ) {
		cTexturePackerNode * prev 	= 0;
		cTexturePackerNode * c 		= mFreeList;

		while ( c ) {
			if ( f != c ) {
				if ( f->Merge( *c ) ) {
					eeASSERT( prev );

					prev->SetNext( c->GetNext() );

					eeSAFE_DELETE( c );

					return true;
				}
			}

			prev 	= c;
			c 		= c->GetNext();
		}

		f = f->GetNext();
	}

	return false;
}

void cTexturePacker::Validate() {
#ifdef EE_DEBUG
	cTexturePackerNode * f = mFreeList;
	while ( f ) {
		cTexturePackerNode * c = mFreeList;

		while ( c ) {
			if ( f != c )
				f->Validate(c);

			c = c->GetNext();
		}

		f = f->GetNext();
	}
#endif
}

cTexturePackerTex * cTexturePacker::GetLonguestEdge() {
	cTexturePackerTex * t = NULL;
	std::list<cTexturePackerTex>::iterator it;

	for ( it = mTextures.begin(); it != mTextures.end(); it++ ) {
		if ( !(*it).Placed() ) {
			t = &(*it);
			break;
		}
	}

	return t;
}

cTexturePackerTex * cTexturePacker::GetShortestEdge() {
	cTexturePackerTex * t = NULL;
	std::list<cTexturePackerTex>::reverse_iterator it;

	for ( it = mTextures.rbegin(); it != mTextures.rend(); it++ ) {
		if ( !(*it).Placed() ) {
			t = &(*it);
			break;
		}
	}

	return t;
}

void cTexturePacker::AddBorderToTextures( const Int32& BorderSize ) {
	cTexturePackerTex * t;

	if ( 0 != BorderSize ) {
		std::list<cTexturePackerTex>::iterator it;

		for ( it = mTextures.begin(); it != mTextures.end(); it++ ) {
			t = &(*it);

			t->Width	( t->Width() 	+ BorderSize );
			t->Height	( t->Height() 	+ BorderSize );
		}

		mLongestEdge += BorderSize;
	}
}

cTexturePackerNode *	cTexturePacker::GetBestFit( cTexturePackerTex * t, cTexturePackerNode ** prevBestFit, Int32 * EdgeCount ) {
	Int32 leastY 							= 0x7FFFFFFF;
	Int32 leastX 							= 0x7FFFFFFF;
	cTexturePackerNode * previousBestFit 	= NULL;
	cTexturePackerNode * bestFit 			= NULL;
	cTexturePackerNode * previous 			= NULL;
	cTexturePackerNode * search 				= mFreeList;
	Int32 edgeCount 						= 0;

	// Walk the singly linked list of free nodes
	// see if it will fit into any currently free space
	while ( search ) {
		Int32 ec;

		// see if the texture will fit into this slot, and if so how many edges does it share.
		if ( search->Fits( t->Width(), t->Height(), ec, mAllowFlipping ) ) {

			if ( ec == 2 ) {
				previousBestFit 	= previous; // record the pointer previous to this one (used to patch the linked list)
				bestFit 			= search; 	// record the best fit.
				edgeCount 			= ec;

				break;
			}

			if ( search->Y() < leastY ) {
				leastY 				= search->Y();
				leastX 				= search->X();
				previousBestFit 	= previous;
				bestFit 			= search;
				edgeCount 			= ec;
			} else if ( search->Y() == leastY && search->X() < leastX ) {
				leastX 				= search->X();
				previousBestFit 	= previous;
				bestFit 			= search;
				edgeCount 			= ec;
			}
		}

		previous 	= search;
		search 		= search->GetNext();
	}

	*EdgeCount 		= edgeCount;
	*prevBestFit	= previousBestFit;

	return bestFit;
}

void cTexturePacker::InsertTexture( cTexturePackerTex * t, cTexturePackerNode * bestFit, Int32 edgeCount, cTexturePackerNode * previousBestFit ) {
	if ( NULL != bestFit ) {
		Validate();

		switch ( edgeCount ) {
			case 0:
			{
				bool flipped 	= false;
				int w 			= t->Width();
				int h 			= t->Height();

				if ( mAllowFlipping ) {
					if ( t->LongestEdge() <= bestFit->Width() ) {
						if ( h > w ) {
							w 		= t->Height();
							h 		= t->Width();
							flipped = true;
						}
					} else {
						eeASSERT( t->LongestEdge() <= bestFit->Height() );

						if ( h < w ) {
							w 		= t->Height();
							h 		= t->Width();
							flipped = true;
						}
					}
				}

				t->Place( bestFit->X(), bestFit->Y(), flipped ); // place it.

				NewFree( bestFit->X(), bestFit->Y() + h, bestFit->Width(), bestFit->Height() - h );

				bestFit->X		( bestFit->X() 		+ w );
				bestFit->Width	( bestFit->Width() 	- w );
				bestFit->Height	( h 					);

				Validate();
			}
			break;
			case 1:
			{
				if ( t->Width() == bestFit->Width() ) {
					t->Place( bestFit->X(), bestFit->Y(), false );

					bestFit->Y		( bestFit->Y() 		+ t->Height() );
					bestFit->Height	( bestFit->Height() - t->Height() );

					Validate();
				} else if ( t->Height() == bestFit->Height() ) {
					t->Place( bestFit->X(), bestFit->Y(), false );

					bestFit->X		( bestFit->X() 		+ t->Width() );
					bestFit->Width	( bestFit->Width() 	- t->Width() );

					Validate();
				} else if ( mAllowFlipping && t->Width() == bestFit->Height() ) {
					t->Place( bestFit->X(), bestFit->Y(), true );

					bestFit->X		( bestFit->X() 		+ t->Height() );
					bestFit->Width	( bestFit->Width() 	- t->Height() );

					Validate();
				} else if ( mAllowFlipping && t->Height() == bestFit->Width() ) {
					t->Place( bestFit->X(), bestFit->Y(), true );

					bestFit->Y		( bestFit->Y() 		+ t->Width() );
					bestFit->Height	( bestFit->Height() - t->Width() );

					Validate();
				}
			}
			break;
			case 2:
			{
				bool flipped = t->Width() != bestFit->Width() || t->Height() != bestFit->Height();

				t->Place( bestFit->X(), bestFit->Y(), flipped );

				if ( previousBestFit )
					previousBestFit->SetNext( bestFit->GetNext() );
				else
					mFreeList = bestFit->GetNext();

				eeSAFE_DELETE( bestFit );

				Validate();
			}
			break;
		}

		while ( MergeNodes() ); // keep merging nodes as much as we can...
	}
}

void cTexturePacker::CreateChild() {
	mChild = eeNew( cTexturePacker, ( mWidth, mHeight, mForcePowOfTwo, mPixelBorder, mAllowFlipping ) );

	std::list<cTexturePackerTex>::iterator it;
	cTexturePackerTex * t = NULL;

	for ( it = mTextures.begin(); it != mTextures.end(); it++ ) {
		t = &(*it);

		if ( !t->Placed() ) {
			mChild->AddTexture( t->Name() );
			mChild->mParent = this;

			t->Disabled( true );

			mCount--;
		}
	}

	mChild->PackTextures();
}

bool cTexturePacker::AddTexturesPath( std::string TexturesPath ) {
	if ( IsDirectory( TexturesPath ) ) {

		DirPathAddSlashAtEnd( TexturesPath );

		std::vector<std::string > files = FilesGetInPath( TexturesPath );
		std::sort( files.begin(), files.end() );

		for ( Uint32 i = 0; i < files.size(); i++ ) {
			std::string path( TexturesPath + files[i] );
			if ( !IsDirectory( path ) )
				AddTexture( path );
		}

		return true;
	}

	return false;
}

bool cTexturePacker::AddTexture( const std::string& TexturePath ) {
	if ( FileExists( TexturePath ) ) {
		cTexturePackerTex TPack( TexturePath );

		if ( TPack.LoadedInfo() ) {
			// Only add the texture if can fit inside the atlas, otherwise it will ignore it
			if ( ( TPack.Width() + mPixelBorder <= mWidth && TPack.Height() + mPixelBorder <= mHeight ) ||
				( mAllowFlipping && ( TPack.Width() + mPixelBorder <= mHeight && TPack.Height() + mPixelBorder <= mWidth ) )
			)
			{
				mTotalArea += TPack.Area();

				// Insert ordered
				std::list<cTexturePackerTex>::iterator it;

				bool Added = false;

				for ( it = mTextures.begin(); it != mTextures.end(); it++ ) {
					if ( (*it).Area() < TPack.Area() ) {
						mTextures.insert( it, TPack );
						Added = true;
						break;
					}
				}

				if ( !Added ) {
					mTextures.push_back( TPack );

					return true;
				}
			}
		}
	}

	return false;
}

Int32 cTexturePacker::PackTextures() { // pack the textures, the return code is the amount of wasted/unused area.
	if ( mWidth <= 0 || mHeight <= 0 )
		return 0;

	cTexturePackerTex * t 	= NULL;

	AddBorderToTextures( (Int32)mPixelBorder );

	NewFree( 0, 0, mWidth, mHeight );

	mCount = (Int32)mTextures.size();

	// We must place each texture
	std::list<cTexturePackerTex>::iterator it;
	for ( it = mTextures.begin(); it != mTextures.end(); it++ ) {
		// For the texture with the longest edge we place it according to this criteria.
		//   (1) If it is a perfect match, we always accept it as it causes the least amount of fragmentation.
		//   (2) A match of one edge with the minimum area left over after the split.
		//   (3) No edges match, so look for the node which leaves the least amount of area left over after the split.

		if ( PackBig == mStrategy )
			t 									= GetLonguestEdge();
		else if ( PackTiny == mStrategy )
			t 									= GetShortestEdge();

		cTexturePackerNode * previousBestFit = NULL;
		Int32 edgeCount 					= 0;
		cTexturePackerNode * bestFit 		= GetBestFit( t, &previousBestFit, &edgeCount );

		if ( NULL == bestFit ) {
			if ( PackBig == mStrategy ) {
				mStrategy = PackTiny;
				eePRINT( "Chaging Strategy to Tiny. %s faults.\n", t->Name().c_str() );
			} else if ( PackTiny == mStrategy ) {
				mStrategy = PackFail;
				eePRINT( "Strategy fail, must create a new image. %s faults.\n", t->Name().c_str() );
			}
		} else {
			InsertTexture( t, bestFit, edgeCount, previousBestFit );
			mCount--;
		}

		if ( PackFail == mStrategy ) {
			eePRINT( "Creating a new image as a child.\n" );
			CreateChild();
			break;
		}
	}

	if ( mCount > 0 ) {
		eePRINT( "Creating a new image as a child. Some textures couldn't get it: %d\n", mCount );
		CreateChild();
	}

	AddBorderToTextures( -( (Int32)mPixelBorder ) );

	mPacked = true;

	for ( it = mTextures.begin(); it != mTextures.end(); it++ ) {
		if ( !(*it).Placed() )
			mTotalArea -= (*it).Area();
	}

	eePRINT( "Total Area Used: %d. This represents the %4.2f percent \n", mTotalArea, ( (eeDouble)mTotalArea / (eeDouble)( mWidth * mHeight ) ) * 100.0 );

	return ( mWidth * mHeight ) - mTotalArea;
}

void cTexturePacker::Save( const std::string& Filepath, const EE_SAVE_TYPE& Format, const bool& SaveExtensions ) {
	if ( !mPacked )
		PackTextures();

	if ( !mTextures.size() )
		return;

	mFilepath = Filepath;
	mSaveExtensions = SaveExtensions;

	cImage Img( (Uint32)mWidth, (Uint32)mHeight, (Uint32)4 );

	Img.FillWithColor( eeColorA(0,0,0,0) );

	cTexturePackerTex * t = NULL;
	int w, h, c;
	std::list<cTexturePackerTex>::iterator it;

	for ( it = mTextures.begin(); it != mTextures.end(); it++ ) {
		t = &(*it);

		if ( t->Placed() ) {
			Uint8 * data = SOIL_load_image( t->Name().c_str(), &w, &h, &c, 0 );

			if ( NULL != data && t->Width() == w && t->Height() == h ) {
				cImage * ImgCopy = eeNew( cImage, ( data, w, h, c ) );

				if ( t->Flipped() )
					Img.Flip();

				Img.CopyImage( ImgCopy, t->X(), t->Y() );

				ImgCopy->AvoidFreeImage( true );

				eeSAFE_DELETE( ImgCopy );

				SOIL_free_image_data( data );

				mPlacedCount++;
			}
		}
	}

	mFormat = Format;

	Img.SaveToFile( Filepath, Format );

	ChildSave( Format );

	SaveShapes();
}

Int32 cTexturePacker::GetChildCount() {
	cTexturePacker * Child 		= mChild;
	Int32 ChildCount 			= 0;

	while ( NULL != Child ) {
		ChildCount++;
		Child = Child->GetChild();
	}

	return ChildCount;
}

void cTexturePacker::SaveShapes() {
	if ( NULL != mParent )
		return;

	sTextureGroupHdr TexGrHdr;

	TexGrHdr.Magic 			= ( ( 'E' << 0 ) | ( 'E' << 8 ) | ( 'T' << 16 ) | ( 'G' << 24 ) );
	TexGrHdr.TextureCount 	= 1 + GetChildCount();
	TexGrHdr.Format			= mFormat;
	TexGrHdr.Width			= mWidth;
	TexGrHdr.Height			= mHeight;
	TexGrHdr.PixelBorder	= mPixelBorder;
	TexGrHdr.Flags			= 0;

	if ( mAllowFlipping )
		TexGrHdr.Flags |= HDR_TEXTURE_GROUP_ALLOW_FLIPPING;

	if ( !mSaveExtensions )
		TexGrHdr.Flags |= HDR_TEXTURE_GROUP_REMOVE_EXTENSION;

	if ( mForcePowOfTwo )
		TexGrHdr.Flags |= HDR_TEXTURE_GROUP_POW_OF_TWO;

	std::vector<sTextureHdr> TexHdr( TexGrHdr.TextureCount );
	//sTextureHdr TexHdr[  ];

	TexHdr[ 0 ] = CreateTextureHdr( this );

	Int32 HdrPos 				= 1;
	cTexturePacker * Child 		= mChild;

	while ( NULL != Child ) {
		TexHdr[ HdrPos ] 	= CreateTextureHdr( Child );
		Child 				= Child->GetChild();
		HdrPos++;
	}

	std::vector<sShapeHdr> tShapesHdr;

	std::string path = FileRemoveExtension( mFilepath ) + ".etg";
	std::fstream fs ( path.c_str() , std::ios::out | std::ios::binary );

	if ( fs.is_open() ) {
		fs.write( reinterpret_cast<const char*> (&TexGrHdr), sizeof(sTextureGroupHdr) );

		fs.write( reinterpret_cast<const char*> (&TexHdr[ 0 ]), sizeof(sTextureHdr) );

		CreateShapesHdr( this, tShapesHdr, &fs );

		if ( tShapesHdr.size() )
			fs.write( reinterpret_cast<const char*> (&tShapesHdr[ 0 ]), sizeof(sShapeHdr) * (std::streamsize)tShapesHdr.size() );

		Int32 HdrPos 				= 1;
		cTexturePacker * Child 		= mChild;

		while ( NULL != Child ) {
			fs.write( reinterpret_cast<const char*> (&TexHdr[ HdrPos ]), sizeof(sTextureHdr) );

			CreateShapesHdr( Child, tShapesHdr, &fs );

			if ( tShapesHdr.size() )
				fs.write( reinterpret_cast<const char*> (&tShapesHdr[ 0 ]), sizeof(sShapeHdr) * (std::streamsize)tShapesHdr.size() );

			Child 				= Child->GetChild();

			HdrPos++;
		}

		fs.close();
	}
}

void cTexturePacker::CreateShapesHdr( cTexturePacker * Packer, std::vector<sShapeHdr>& Shapes, std::fstream * fs ) {
	Shapes.clear();

	sShapeHdr tShapeHdr;

	std::list<cTexturePackerTex> tTextures = *(Packer->GetTexturePackPtr());
	std::list<cTexturePackerTex>::iterator it;
	cTexturePackerTex * tTex;

	for ( it = tTextures.begin(); it != tTextures.end(); it++ ) {
		tTex = &(*it);

		if ( tTex->Placed() ) {
			std::string name = FileNameFromPath( tTex->Name() );

			memset( tShapeHdr.Name, 0, HDR_NAME_SIZE );

			StrCopy( tShapeHdr.Name, name.c_str(), HDR_NAME_SIZE );

			tShapeHdr.ResourceID	= MakeHash( name );
			tShapeHdr.Width 		= tTex->Width();
			tShapeHdr.Height 		= tTex->Height();
			tShapeHdr.Channels		= tTex->Channels();
			tShapeHdr.DestWidth 	= tTex->Width();
			tShapeHdr.DestHeight 	= tTex->Height();
			tShapeHdr.OffsetX		= 0;
			tShapeHdr.OffsetY		= 0;
			tShapeHdr.X				= tTex->X();
			tShapeHdr.Y				= tTex->Y();
			tShapeHdr.Date			= FileGetModificationDate( tTex->Name() );
			tShapeHdr.Flags			= 0;

			if ( tTex->Flipped() )
				tShapeHdr.Flags |= HDR_SHAPE_FLAG_FLIPED;

			fs->write( reinterpret_cast<const char*> (&tShapeHdr), sizeof(sShapeHdr) );
		}
	}
}

sTextureHdr	cTexturePacker::CreateTextureHdr( cTexturePacker * Packer ) {
	sTextureHdr TexHdr;

	std::string name( FileNameFromPath( Packer->GetFilepath() ) );

	memset( TexHdr.Name, 0, HDR_NAME_SIZE );

	StrCopy( TexHdr.Name, name.c_str(), HDR_NAME_SIZE );

	TexHdr.ResourceID 	= MakeHash( name );
	TexHdr.Size			= FileSize( Packer->GetFilepath() );
	TexHdr.ShapeCount 	= Packer->GetPlacedCount();

	return TexHdr;
}

void cTexturePacker::ChildSave( const EE_SAVE_TYPE& Format ) {
	if ( NULL != mChild ) {
		cTexturePacker * Parent 	= mChild->GetParent();
		cTexturePacker * LastParent	= NULL;
		Int32 ParentCount 			= 0;

		// Find the grand parent
		while ( NULL != Parent ) {
			ParentCount++;
			LastParent = Parent;
			Parent 	= Parent->GetParent();
		}

		std::string fFpath	= FileRemoveExtension( LastParent->GetFilepath() );
		std::string fExt	= FileExtension( LastParent->GetFilepath() );
		std::string fName	= fFpath + "_ch" + toStr( ParentCount ) + "." + fExt;

		mChild->Save( fName, Format, mSaveExtensions );
	}
}

cTexturePacker * cTexturePacker::GetChild() const {
	return mChild;
}

cTexturePacker * cTexturePacker::GetParent() const {
	return mParent;
}

std::list<cTexturePackerTex> * cTexturePacker::GetTexturePackPtr() {
	return &mTextures;
}

const std::string&	cTexturePacker::GetFilepath() const {
	return mFilepath;
}

const Int32& cTexturePacker::GetWidth() const {
	return mWidth;
}

const Int32& cTexturePacker::GetHeight() const {
	return mHeight;
}

const Int32& cTexturePacker::GetPlacedCount() const {
	return mPlacedCount;
}

}}
