#ifndef EE_GAMINGCTILEMAP_HPP
#define EE_GAMINGCTILEMAP_HPP

#include <eepp/gaming/base.hpp>

#include <eepp/gaming/cgameobject.hpp>
#include <eepp/gaming/cgameobjectobject.hpp>
#include <eepp/gaming/clight.hpp>
#include <eepp/gaming/clightmanager.hpp>
#include <eepp/gaming/clayer.hpp>

#include <eepp/window/cinput.hpp>
#include <eepp/window/cengine.hpp>
#include <eepp/window/cwindow.hpp>
using namespace EE::Window;

#include <eepp/graphics/ctexture.hpp>
using namespace EE::Graphics;

namespace EE { namespace Gaming {

namespace MapEditor { class cUIMapNew; }

#define EE_MAP_LAYER_UNKNOWN eeINDEX_NOT_FOUND
#define EE_MAP_MAGIC ( ( 'E' << 0 ) | ( 'E' << 8 ) | ( 'M' << 16 ) | ( 'P' << 24 ) )

class EE_API cMap {
	public:
		typedef std::map<std::string, std::string>	PropertiesMap;
		typedef std::list<std::string>				GOTypesList;		//! Special object types used in this map
		typedef cb::Callback4< cGameObject *, const Uint32&, const Uint32&, cLayer *, const Uint32&>		CreateGOCb;
		typedef cb::Callback0<void>	MapDrawCb;
		typedef cb::Callback0<void> MapUpdateCb;

		cMap();

		virtual ~cMap();

		virtual void Create( eeSize Size, Uint32 MaxLayers, eeSize TileSize, Uint32 Flags = 0, eeSize viewSize = eeSize( 800, 600 ), Window::cWindow * Window = NULL );

		virtual cLayer * AddLayer( Uint32 Type, Uint32 flags, std::string name );

		virtual cLayer * GetLayer( Uint32 index );

		virtual Uint32 GetLayerIndex( cLayer * Layer );

		virtual cLayer * GetLayerByHash( Uint32 hash );

		virtual cLayer * GetLayer( const std::string& name );

		virtual bool Load( const std::string& path );

		virtual bool LoadFromStream( cIOStream& IOS );

		virtual bool LoadFromPack( cPack * Pack, const std::string& FilePackPath );

		virtual bool LoadFromMemory( const char * Data, const Uint32& DataSize );

		virtual void Save( const std::string& path );

		virtual void SaveToStream( cIOStream& IOS );

		virtual void Draw();

		virtual void Update();

		const eeSize& TileSize() const;

		const eeSize& Size() const;

		const Uint32& LayerCount() const;

		const Uint32& MaxLayers() const;

		const eeSize& ViewSize() const;

		void ViewSize( const eeSize& viewSize );

		const eeVector2f& Offset() const;

		const eeVector2f& OffsetFixed() const;

		void Offset( const eeVector2f& offset );

		const eeVector2i& Position() const;

		void Position( const eeVector2i& position );

		void Move( const eeVector2f& offset );

		void Move( const eeFloat& offsetx, const eeFloat& offsety );

		const eeVector2i& StartTile() const;

		const eeVector2i& EndTile() const;

		const Uint32& Flags() const;

		bool ClampBorders() const;

		void ClampBorders( const bool& clamp );

		bool ClipedArea() const;

		void ClipedArea( const bool& clip );

		void DrawGrid( const bool& draw );

		Uint32 DrawGrid() const;

		void ShowBlocked( const bool& show );

		Uint32 ShowBlocked() const;

		void DrawBackground( const bool& draw );

		Uint32 DrawBackground() const;

		Uint32 DrawTileOver() const;

		void DrawTileOver( const bool& draw );

		bool LightsEnabled();

		void LightsEnabled( const bool& enabled );

		bool LightsByVertex();

		void Reset();

		bool MoveLayerUp( cLayer * Layer );

		bool MoveLayerDown( cLayer * Layer );

		bool RemoveLayer( cLayer * Layer );

		const eeVector2i& GetMouseTilePos() const;

		eeVector2i GetMouseTilePosCoords();

		eeVector2i GetTileCoords( const eeVector2i& TilePos );

		const eeVector2i& GetRealMouseTilePos() const;

		const eeVector2i& GetMouseMapPos() const;

		eeVector2f GetMouseMapPosf() const;

		const eeSize& TotalSize() const;

		void AddProperty( std::string Text, std::string Value );

		void EditProperty( std::string Text, std::string Value );

		void RemoveProperty( std::string Text );

		void ClearProperties();

		PropertiesMap& GetProperties();

		void AddVirtualObjectType( const std::string& name );

		void RemoveVirtualObjectType( const std::string& name );

		void ClearVirtualObjectTypes();

		GOTypesList& GetVirtualObjectTypes();

		void SetCreateGameObjectCallback( const CreateGOCb& Cb );

		const std::string& Path() const;

		void BaseColor( const eeColorA& color );

		const eeColorA& BaseColor() const;

		const eeAABB& GetViewAreaAABB() const;

		cLightManager * GetLightManager() const;

		/** Tiles to add or subtract from the real values of StartTile and EndTile ( so it will loop over more/less tiles than the required to render every tile on screen ). */
		void ExtraTiles( const eeVector2i& extra );

		const eeVector2i& ExtraTiles() const;

		void SetDrawCallback( MapDrawCb Cb );

		void SetUpdateCallback( MapUpdateCb Cb );

		cTexture * GetBlankTileTexture();

		bool IsTileBlocked( const eeVector2i& TilePos );

		void Data( void * value );

		void * Data() const;

		const bool& IsMouseOver() const;

		cGameObject * IsTypeInTilePos( const Uint32& Type, const eeVector2i& TilePos );

		const Uint8& BackAlpha() const;

		void BackAlpha( const Uint8& alpha );

		const eeColorA& BackColor() const;

		void BackColor( const eeColorA& col );

		const eeFloat& Scale() const;

		void Scale( const eeFloat& scale );

		eeVector2i GetMaxOffset();

		Uint32 GetNewObjectId();

		cGameObjectPolyData& GetPolyObjData( Uint32 Id );

		void ForceHeadersOnLoad( eeSize mapSize, eeSize tileSize, Uint32 numLayers, Uint32 flags );

		void DisableForcedHeaders();

		void GridLinesColor( const eeColorA& Col );

		const eeColorA& GridLinesColor() const;
	protected:
		friend class EE::Gaming::MapEditor::cUIMapNew;

		class cForcedHeaders
		{
			public:
				cForcedHeaders( eeSize mapSize, eeSize tileSize, Uint32 numLayers, Uint32 flags ) :
					MapSize( mapSize ),
					TileSize( tileSize ),
					NumLayers( numLayers ),
					Flags( flags )
				{}

				eeSize MapSize;
				eeSize TileSize;
				Uint32 NumLayers;
				Uint32 Flags;
		};

		typedef std::map<Uint32, cGameObjectPolyData> PolyObjMap;

		Window::cWindow *		mWindow;
		cLayer**		mLayers;
		Uint32			mFlags;
		Uint32			mMaxLayers;
		Uint32			mLayerCount;
		eeSize			mSize;
		eeSize			mPixelSize;
		eeSize			mTileSize;
		eeSize			mViewSize;
		eeVector2f		mOffset;
		eeVector2i		mScreenPos;
		eeVector2i		mStartTile;
		eeVector2i		mEndTile;
		eeVector2i		mExtraTiles;
		eeVector2i		mMouseOverTile;
		eeVector2i		mMouseOverTileFinal;
		eeVector2i		mMouseMapPos;
		eeColorA		mBaseColor;
		PropertiesMap	mProperties;
		GOTypesList		mObjTypes;
		CreateGOCb		mCreateGOCb;
		cTexture *		mTileTex;
		eeAABB			mScreenAABB;
		cLightManager *	mLightManager;
		MapDrawCb		mDrawCb;
		MapUpdateCb		mUpdateCb;
		void *			mData;
		eeColorA		mTileOverColor;
		eeColorA		mBackColor;
		eeColorA		mGridLinesColor;
		Uint8			mBackAlpha;
		bool			mMouseOver;
		std::string		mPath;
		eeFloat			mScale;
		eeVector2f		mOffscale;
		Uint32			mLastObjId;
		PolyObjMap		mPolyObjs;
		cForcedHeaders*	mForcedHeaders;

		virtual cGameObject *	CreateGameObject( const Uint32& Type, const Uint32& Flags, cLayer * Layer, const Uint32& DataId = 0 );

		void			CalcTilesClip();

		void			Clamp();

		void			GetMouseOverTile();

		void			GridDraw();

		void			MouseOverDraw();

		void			DeleteLayers();

		std::vector<std::string> GetTextureAtlases();

		void			CreateEmptyTile();

		void			UpdateScreenAABB();

		void			CreateLightManager();

		virtual void	OnMapLoaded();
};

}}

#endif
