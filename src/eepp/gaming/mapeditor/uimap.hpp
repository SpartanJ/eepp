#ifndef EE_GAMINGCUIMAP_HPP
#define EE_GAMINGCUIMAP_HPP

#include <eepp/gaming/base.hpp>
#include <eepp/ui/uicomplexcontrol.hpp>
#include <eepp/ui/uitextbox.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/gaming/tilemap.hpp>
#include <eepp/gaming/maplightmanager.hpp>
#include <eepp/gaming/maplight.hpp>
#include <eepp/graphics/primitives.hpp>

using namespace EE::UI;

namespace EE { namespace Gaming {

class GameObjectObject;

namespace Private {

class EE_API UIMap : public UIComplexControl {
	public:
		enum EDITING_OBJ_MODE {
			SELECT_OBJECTS,
			EDIT_POLYGONS,
			INSERT_OBJECT,
			INSERT_POLYGON,
			INSERT_POLYLINE
		};

		typedef cb::Callback1<void, MapLight *> LightSelectCb;
		typedef cb::Callback1<void, MapLight *> LightRadiusChangeCb;
		typedef cb::Callback2<void, Uint32, Polygon2f> ObjAddCb;
		typedef cb::Callback2<UIMessageBox*, const String&, const String&> AlertCb;
		typedef cb::Callback0<void> OnMapLoadCb;
		typedef cb::Callback0<void> UpdateScrollCb;

		UIMap( const UIComplexControl::CreateParams& Params, UITheme * Theme, TileMap * Map = NULL );

		virtual ~UIMap();

		virtual void draw();

		virtual void update();

		TileMap * Map() const;

		void EditingLights( const bool& editing );

		bool EditingLights();

		void EditingObjects( const bool& editing );

		void EditingDisabled();

		bool EditingObjects();

		MapLight * GetSelectedLight();

		MapLight * GetAddLight();

		void AddLight( MapLight * Light );

		void SetLightSelectCb( LightSelectCb Cb );

		void SetLightRadiusChangeCb( LightRadiusChangeCb Cb );

		void SetAddObjectCallback( ObjAddCb Cb );

		void SetAlertCb( AlertCb Cb );

		void SetUpdateScrollCb( UpdateScrollCb Cb );

		void ClearLights();

		void ClampToTile( const bool& clamp );

		const bool& ClampToTile() const;

		void EditingObjMode( EDITING_OBJ_MODE mode );

		void CurLayer( MapLayer * layer );

		void CreateObjPopUpMenu();

		void SetTileBox( UITextBox * tilebox );

		void ReplaceMap( TileMap * newMap );
	protected:		
		enum EDITING_MODE {
			EDITING_LIGHT = 1,
			EDITING_OBJECT
		};

		TileMap *				mMap;
		MapLayer *			mCurLayer;
		Uint32				mEditingMode;
		Primitives			mP;

		Uint32				mEditingObjMode;

		MapLight *			mAddLight;
		MapLight *			mSelLight;

		LightSelectCb		mLightSelCb;
		LightRadiusChangeCb	mLightRadiusChangeCb;
		ObjAddCb			mAddObjectCallback;

		bool				mClampToTile;

		bool				mObjRECTEditing;
		Rectf				mObjRECT;

		bool				mObjPolyEditing;
		Polygon2f			mObjPoly;

		bool				mObjDragging;

		GameObjectObject *	mSelObj;
		Vector2f			mObjDragDist;

		AlertCb				mAlertCb;
		UITheme *			mTheme;

		Uint32				mSelPointIndex;
		Rectf				mSelPointRect;
		bool				mSelPoint;

		UITextBox *		mTileBox;
		Vector2i			mLastMouseTilePos;

		UpdateScrollCb		mUpdateScrollCb;

		virtual Uint32 onMessage( const UIMessage * Msg );

		virtual Uint32 onMouseMove( const Vector2i& position, const Uint32 flags );

		virtual void onSizeChange();

		virtual void updateScreenPos();

		virtual void onAlphaChange();

		virtual Uint32 OnDrag( const Vector2i& position );

		void ObjItemClick( const UIEvent * Event );

		void MapDraw();

		void TryToSelectLight();

		void ManageObject( Uint32 flags );

		Vector2f GetMouseMapPos();

		void SelectPolyObj();

		void SelectPolyPoint();

		void SetPointRect( Vector2f p );

		void DragPoly( Uint32 flags, Uint32 PFlags );
};

}}}

#endif
