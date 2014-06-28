#ifndef EE_GAMINGCTILEMAP_HPP
#define EE_GAMINGCTILEMAP_HPP

#include <eepp/gaming/base.hpp>

#include <eepp/gaming/gameobject.hpp>
#include <eepp/gaming/gameobjectobject.hpp>
#include <eepp/gaming/maplight.hpp>
#include <eepp/gaming/maplightmanager.hpp>
#include <eepp/gaming/maplayer.hpp>

#include <eepp/window/input.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/window.hpp>
using namespace EE::Window;

#include <eepp/graphics/texture.hpp>
using namespace EE::Graphics;

namespace EE { namespace Gaming {

namespace Private { class UIMapNew; }

#define EE_MAP_LAYER_UNKNOWN eeINDEX_NOT_FOUND
#define EE_MAP_MAGIC ( ( 'E' << 0 ) | ( 'E' << 8 ) | ( 'M' << 16 ) | ( 'P' << 24 ) )

class EE_API TileMap {
	public:
		typedef std::map<std::string, std::string>	PropertiesMap;
		typedef std::list<std::string>				GOTypesList;		//! Special object types used in this map
		typedef cb::Callback4< GameObject *, const Uint32&, const Uint32&, MapLayer *, const Uint32&>		CreateGOCb;
		typedef cb::Callback0<void>	MapDrawCb;
		typedef cb::Callback0<void> MapUpdateCb;

		TileMap();

		virtual ~TileMap();

		virtual void Create( Sizei Size, Uint32 MaxLayers, Sizei TileSize, Uint32 Flags = 0, Sizei viewSize = Sizei( 800, 600 ), EE::Window::Window * Window = NULL );

		virtual MapLayer * AddLayer( Uint32 Type, Uint32 flags, std::string name );

		virtual MapLayer * GetLayer( Uint32 index );

		virtual Uint32 GetLayerIndex( MapLayer * Layer );

		virtual MapLayer * GetLayerByHash( Uint32 hash );

		virtual MapLayer * GetLayer( const std::string& name );

		virtual bool Load( const std::string& path );

		virtual bool LoadFromStream( IOStream& IOS );

		virtual bool LoadFromPack( Pack * Pack, const std::string& FilePackPath );

		virtual bool LoadFromMemory( const char * Data, const Uint32& DataSize );

		virtual void Save( const std::string& path );

		virtual void SaveToStream( IOStream& IOS );

		virtual void Draw();

		virtual void Update();

		const Sizei& TileSize() const;

		const Sizei& Size() const;

		const Uint32& LayerCount() const;

		const Uint32& MaxLayers() const;

		const Sizei& ViewSize() const;

		void ViewSize( const Sizei& viewSize );

		const Vector2f& Offset() const;

		const Vector2f& OffsetFixed() const;

		void Offset( const Vector2f& offset );

		const Vector2i& Position() const;

		void Position( const Vector2i& position );

		void Move( const Vector2f& offset );

		void Move( const Float& offsetx, const Float& offsety );

		const Vector2i& StartTile() const;

		const Vector2i& EndTile() const;

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

		bool MoveLayerUp( MapLayer * Layer );

		bool MoveLayerDown( MapLayer * Layer );

		bool RemoveLayer( MapLayer * Layer );

		const Vector2i& GetMouseTilePos() const;

		Vector2i GetMouseTilePosCoords();

		Vector2i GetTileCoords( const Vector2i& TilePos );

		const Vector2i& GetRealMouseTilePos() const;

		const Vector2i& GetMouseMapPos() const;

		Vector2f GetMouseMapPosf() const;

		const Sizei& TotalSize() const;

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

		void BaseColor( const ColorA& color );

		const ColorA& BaseColor() const;

		const eeAABB& GetViewAreaAABB() const;

		MapLightManager * GetLightManager() const;

		/** Tiles to add or subtract from the real values of StartTile and EndTile ( so it will loop over more/less tiles than the required to render every tile on screen ). */
		void ExtraTiles( const Vector2i& extra );

		const Vector2i& ExtraTiles() const;

		void SetDrawCallback( MapDrawCb Cb );

		void SetUpdateCallback( MapUpdateCb Cb );

		Texture * GetBlankTileTexture();

		bool IsTileBlocked( const Vector2i& TilePos );

		void Data( void * value );

		void * Data() const;

		const bool& IsMouseOver() const;

		GameObject * IsTypeInTilePos( const Uint32& Type, const Vector2i& TilePos );

		const Uint8& BackAlpha() const;

		void BackAlpha( const Uint8& alpha );

		const ColorA& BackColor() const;

		void BackColor( const ColorA& col );

		const Float& Scale() const;

		void Scale( const Float& scale );

		Vector2i GetMaxOffset();

		Uint32 GetNewObjectId();

		GameObjectPolyData& GetPolyObjData( Uint32 Id );

		void ForceHeadersOnLoad( Sizei mapSize, Sizei tileSize, Uint32 numLayers, Uint32 flags );

		void DisableForcedHeaders();

		void GridLinesColor( const ColorA& Col );

		const ColorA& GridLinesColor() const;
	protected:
		friend class EE::Gaming::Private::UIMapNew;

		class cForcedHeaders
		{
			public:
				cForcedHeaders( Sizei mapSize, Sizei tileSize, Uint32 numLayers, Uint32 flags ) :
					MapSize( mapSize ),
					TileSize( tileSize ),
					NumLayers( numLayers ),
					Flags( flags )
				{}

				Sizei MapSize;
				Sizei TileSize;
				Uint32 NumLayers;
				Uint32 Flags;
		};

		typedef std::map<Uint32, GameObjectPolyData> PolyObjMap;

		EE::Window::Window *		mWindow;
		MapLayer**		mLayers;
		Uint32			mFlags;
		Uint32			mMaxLayers;
		Uint32			mLayerCount;
		Sizei			mSize;
		Sizei			mPixelSize;
		Sizei			mTileSize;
		Sizei			mViewSize;
		Vector2f		mOffset;
		Vector2i		mScreenPos;
		Vector2i		mStartTile;
		Vector2i		mEndTile;
		Vector2i		mExtraTiles;
		Vector2i		mMouseOverTile;
		Vector2i		mMouseOverTileFinal;
		Vector2i		mMouseMapPos;
		ColorA		mBaseColor;
		PropertiesMap	mProperties;
		GOTypesList		mObjTypes;
		CreateGOCb		mCreateGOCb;
		Texture *		mTileTex;
		eeAABB			mScreenAABB;
		MapLightManager *	mLightManager;
		MapDrawCb		mDrawCb;
		MapUpdateCb		mUpdateCb;
		void *			mData;
		ColorA		mTileOverColor;
		ColorA		mBackColor;
		ColorA		mGridLinesColor;
		Uint8			mBackAlpha;
		bool			mMouseOver;
		std::string		mPath;
		Float			mScale;
		Vector2f		mOffscale;
		Uint32			mLastObjId;
		PolyObjMap		mPolyObjs;
		cForcedHeaders*	mForcedHeaders;

		virtual GameObject *	CreateGameObject( const Uint32& Type, const Uint32& Flags, MapLayer * Layer, const Uint32& DataId = 0 );

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
