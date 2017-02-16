#include <eepp/gaming/mapeditor/uimap.hpp>
#include <eepp/gaming/mapeditor/mapobjectproperties.hpp>
#include <eepp/gaming/gameobjectobject.hpp>
#include <eepp/gaming/mapobjectlayer.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/ui/uipopupmenu.hpp>

namespace EE { namespace Gaming { namespace Private {

UIMap::UIMap( const UIComplexControl::CreateParams& Params, UITheme * Theme, TileMap * Map ) :
	UIComplexControl( Params ),
	mMap( Map ),
	mCurLayer( NULL ),
	mEditingMode( 0 ),
	mEditingObjMode( SELECT_OBJECTS ),
	mAddLight( NULL ),
	mSelLight( NULL ),
	mClampToTile(true),
	mObjRECTEditing(false),
	mObjPolyEditing(false),
	mObjDragging( false ),
	mSelObj( NULL ),
	mTheme( Theme ),
	mSelPointIndex( eeINDEX_NOT_FOUND ),
	mSelPoint( false ),
	mTileBox( NULL )
{
	if ( NULL == Map ) {
		mMap = eeNew( TileMap, () );
	}

	mMap->BackColor( ColorA( 100, 100, 100, 100 ) );
	mMap->GridLinesColor( ColorA( 150, 150, 150, 150 ) );

	mMap->SetDrawCallback( cb::Make0( this, &UIMap::MapDraw ) );

	mDragButton = EE_BUTTON_MMASK;
	dragEnable( true );

	updateScreenPos();
}

UIMap::~UIMap() {
	eeSAFE_DELETE( mMap );
}

Uint32 UIMap::OnDrag( const Vector2i& Pos ) {

	if (	( EDITING_OBJECT == mEditingMode && NULL != mSelObj ) ||
			( EDITING_LIGHT == mEditingMode && NULL != mSelLight ) ) {
		mDragPoint = Pos;
		return 0;
	}

	Vector2i nPos( -( mDragPoint - Pos ) );
	Vector2f nPosf( nPos.x, nPos.y );

	mMap->Move( nPosf );

	mDragPoint = Pos;

	if ( mUpdateScrollCb.IsSet() ) {
		mUpdateScrollCb();
	}

	return 0;
}

void UIMap::ReplaceMap( TileMap * newMap ) {
	eeSAFE_DELETE( mMap );
	mMap = newMap;
	updateScreenPos();
}

TileMap * UIMap::Map() const {
	return mMap;
}

void UIMap::draw() {
	UIComplexControl::draw();

	if ( NULL != mMap ) {
		mMap->Draw();
	}
}

void UIMap::updateScreenPos() {
	UIComplexControl::updateScreenPos();

	if ( NULL != mMap ) {
		mMap->Position( mScreenPos );
	}
}

void UIMap::update() {
	UIComplexControl::update();

	if ( NULL != mMap ) {
		mMap->Update();

		if ( mEnabled && mVisible && isMouseOver() ) {
			Uint32 Flags 			= UIManager::instance()->getInput()->clickTrigger();

			if ( EDITING_LIGHT == mEditingMode ) {
				if ( NULL != mSelLight ) {
					if ( Flags & EE_BUTTONS_WUWD ) {
						if ( Flags & EE_BUTTON_WUMASK ) {
							mSelLight->Radius( mSelLight->Radius() + 10 );
						} else if ( Flags & EE_BUTTON_WDMASK ) {
							mSelLight->Radius( mSelLight->Radius() - 10 );
						}

						if ( mLightRadiusChangeCb.IsSet() )
							mLightRadiusChangeCb( mSelLight );
					} else if ( Flags & EE_BUTTON_RMASK ) {
						if ( mSelLight == mAddLight ) {
							mMap->GetLightManager()->RemoveLight( mAddLight );

							eeSAFE_DELETE( mAddLight );

							mSelLight = NULL;
						} else if ( NULL != mSelLight ) {
							if ( mSelLight->GetAABB().contains( mMap->GetMouseMapPosf() ) ) {
								mMap->GetLightManager()->RemoveLight( mSelLight );

								eeSAFE_DELETE( mSelLight );
							}
						}
					} else if ( Flags & EE_BUTTON_LMASK ) {
						if ( mSelLight == mAddLight ) {
							mAddLight = NULL;
						} else {
							TryToSelectLight();
						}
					}

					Flags = UIManager::instance()->getInput()->pressTrigger();

					if ( Flags & EE_BUTTON_MMASK ) {
						mSelLight->Position( mMap->GetMouseMapPosf() );
					}
				} else {
					if ( Flags & EE_BUTTON_LMASK ) {
						TryToSelectLight();
					}
				}
			} else if ( EDITING_OBJECT == mEditingMode ) {
				ManageObject( Flags );
			}
		}
	}
}

Vector2f UIMap::GetMouseMapPos() {
	Vector2f mp( mMap->GetMouseMapPosf() );

	if ( mClampToTile ) {
		Vector2i mpc( mMap->GetTileCoords( mMap->GetMouseTilePos() + 1 ) );
		mp = Vector2f( mpc.x, mpc.y );
	}

	return mp;
}

void UIMap::SelectPolyObj() {
	if ( NULL != mCurLayer && mCurLayer->Type() == MAP_LAYER_OBJECT ) {
		MapObjectLayer * tLayer = reinterpret_cast<MapObjectLayer*>( mCurLayer );

		GameObject * tObj = tLayer->GetObjectOver( mMap->GetMouseMapPos(), MapObjectLayer::SEARCH_POLY );

		if ( NULL != tObj ) {
			if ( NULL != mSelObj ) {
				mSelObj->Selected( false );
			}

			mSelObj = reinterpret_cast<GameObjectObject*>( tObj );
			mSelObj->Selected( true );
		} else {
			if ( NULL != mSelObj ) {
				mSelObj->Selected( false );
				mSelObj = NULL;
			}
		}
	} else {
		if ( mAlertCb.IsSet() ) {
			mAlertCb( "No layer found", "An Object Layer must be selected first." )->setFocus();
		}
	}
}

void UIMap::SelectPolyPoint() {
	if ( NULL != mCurLayer && mCurLayer->Type() == MAP_LAYER_OBJECT && NULL != mSelObj ) {
		if ( mSelObj->PointInside( mMap->GetMouseMapPosf() ) ) {
			mSelPointIndex = mSelObj->GetPolygon().closestPoint( mMap->GetMouseMapPosf() );
			SetPointRect( mSelObj->GetPolygon().getAt( mSelPointIndex ) );
		}
	}
}

void UIMap::DragPoly( Uint32 Flags, Uint32 PFlags ) {
	if ( ( PFlags & EE_BUTTON_MMASK ) && NULL != mSelObj ) {
		if ( mSelObj->PointInside( mMap->GetMouseMapPosf() ) ) {
			if ( !mObjDragging ) {
				mObjDragging = true;
				mObjDragDist = GetMouseMapPos() - mSelObj->Pos();
			}
		}

		mSelObj->Pos( GetMouseMapPos() - mObjDragDist );

		if ( EDIT_POLYGONS == mEditingObjMode ) {
			SelectPolyPoint();
		}
	} else if ( Flags & EE_BUTTON_MMASK ) {
		if ( mObjDragging ) {
			mObjDragging = false;
			mObjDragDist = Vector2f(0,0);
		}
	}
}

void UIMap::ManageObject( Uint32 Flags ) {
	Uint32 PFlags	= UIManager::instance()->getInput()->pressTrigger();
	Uint32 LPFlags	= UIManager::instance()->getInput()->lastPressTrigger();

	switch ( mEditingObjMode )
	{
		case INSERT_OBJECT:
		{
			if ( PFlags & EE_BUTTON_LMASK ) {
				Vector2f mp( GetMouseMapPos() );

				if ( !mObjRECTEditing ) {
					mObjRECTEditing = true;
					mObjRECT		= Rectf( mp, Sizef(0,0) );
				} else {
					if ( mObjRECT.pos().x < mp.x && mObjRECT.pos().y < mp.y ) {
						mObjRECT		= Rectf( mObjRECT.pos(), Sizef( mp - mObjRECT.pos() ) );
					}
				}
			}

			if ( Flags & EE_BUTTON_LMASK ){
				if ( mObjRECTEditing ) {
					mAddObjectCallback( GAMEOBJECT_TYPE_OBJECT, Polygon2f( mObjRECT ) );
					mObjRECTEditing = false;
				}
			}

			break;
		}
		case INSERT_POLYLINE:
		case INSERT_POLYGON:
		{
			if ( Flags & EE_BUTTON_LMASK ) {
				mObjPoly.pushBack( GetMouseMapPos() );
			} else if ( Flags & EE_BUTTON_RMASK ) {
				mAddObjectCallback( ( INSERT_POLYGON == mEditingObjMode ) ? GAMEOBJECT_TYPE_POLYGON : GAMEOBJECT_TYPE_POLYLINE, mObjPoly );

				mObjPoly.clear();
			}

			break;
		}
		case SELECT_OBJECTS:
		{
			if ( ( Flags & EE_BUTTON_LMASK ) ) {
				SelectPolyObj();
			} else {
				DragPoly( Flags, PFlags );
			}

			break;
		}
		case EDIT_POLYGONS:
		{
			if ( ( Flags & EE_BUTTON_LMASK ) ) {
				if ( !mSelPoint ) {
					SelectPolyObj();
					SelectPolyPoint();
				} else {
					mSelPoint = false;
				}
			} else if ( !( LPFlags & EE_BUTTON_LMASK  ) && ( PFlags & EE_BUTTON_LMASK ) ) {
				if ( NULL != mSelObj && eeINDEX_NOT_FOUND != mSelPointIndex && mSelPointRect.contains( mMap->GetMouseMapPosf() ) ) {
					mSelPoint = true;
				}
			} else if ( ( PFlags & EE_BUTTON_LMASK ) ) {
				if ( mSelPoint && NULL != mSelObj && eeINDEX_NOT_FOUND != mSelPointIndex ) {
					mSelObj->SetPolygonPoint( mSelPointIndex, GetMouseMapPos() );
					SetPointRect( GetMouseMapPos() );
				}
			} else {
				DragPoly( Flags, PFlags );
			}

			break;
		}
		default:
		{
		}
	}
}

void UIMap::SetPointRect( Vector2f p ) {
	mSelPointRect = Rectf( Vector2f( p.x - 10, p.y - 10 ), Sizef( 20, 20 ) );
}

void UIMap::TryToSelectLight() {
	MapLight * tLight = mSelLight;
	mSelLight = mMap->GetLightManager()->GetLightOver( mMap->GetMouseMapPosf(), mSelLight );

	if ( NULL != mSelLight && mSelLight != tLight ) {
		if ( mLightSelCb.IsSet() )
			mLightSelCb( mSelLight );
	}
}

void UIMap::onSizeChange() {
	if ( NULL != mMap ) {
		mMap->Position( mScreenPos );
		mMap->ViewSize( mSize );
	}

	UIComplexControl::onSizeChange();
}

Uint32 UIMap::onMouseMove( const Vector2i& Pos, const Uint32 Flags ) {
	if ( NULL != mMap ) {
		if ( EDITING_LIGHT == mEditingMode && NULL != mAddLight ) {
			mAddLight->Position( mMap->GetMouseMapPosf() );
		}
	}

	if ( NULL != mTileBox ) {
		Vector2i mp( mMap->GetMouseTilePos() );

		if ( mLastMouseTilePos != mp ) {
			mLastMouseTilePos = mp;
			mTileBox->text( String::toStr( mp.x ) + "," + String::toStr( mp.y ) );
		}
	}

	return UIComplexControl::onMouseMove( Pos, Flags );
}

void UIMap::AddLight( MapLight * Light ) {
	if ( NULL != mMap->GetLightManager() ) {
		if ( NULL != mAddLight ) {
			mMap->GetLightManager()->RemoveLight( mAddLight );

			eeSAFE_DELETE( mAddLight );
		}

		mAddLight = Light;

		mSelLight = Light;

		mMap->GetLightManager()->AddLight( Light );

		if ( mLightSelCb.IsSet() )
			mLightSelCb( mSelLight );
	}
}

void UIMap::MapDraw() {
	if ( EDITING_LIGHT == mEditingMode ) {
		if ( NULL != mSelLight ) {
			mP.setColor( ColorA( 255, 0, 0, (Uint8)mAlpha ) );

			Vector2f Pos( mSelLight->GetAABB().Left, mSelLight->GetAABB().Top );
			eeAABB AB( mSelLight->GetAABB() );

			mP.fillMode( DRAW_LINE );
			mP.drawRectangle( Rectf( Pos, AB.size() ) );
		}
	} else if ( EDITING_OBJECT == mEditingMode ) {
		switch ( mEditingObjMode ) {
			case INSERT_OBJECT:
			{
				if ( mObjRECTEditing ) {
					mP.fillMode( DRAW_FILL );
					mP.setColor( ColorA( 100, 100, 100, 20 ) );
					mP.drawRectangle( mObjRECT );

					mP.fillMode( DRAW_LINE );
					mP.setColor( ColorA( 255, 0, 0, 200 ) );
					mP.drawRectangle( mObjRECT );
				}

				break;
			}
			case INSERT_POLYGON:
			{
				mP.fillMode( DRAW_FILL );
				mP.setColor( ColorA( 50, 50, 50, 50 ) );
				mP.drawPolygon( mObjPoly );

				mP.fillMode( DRAW_LINE );
				mP.setColor( ColorA( 255, 0, 0, 200 ) );
				mP.drawPolygon( mObjPoly );

				Polygon2f polyN( mObjPoly );
				polyN.pushBack( GetMouseMapPos() );

				mP.fillMode( DRAW_FILL );
				mP.setColor( ColorA( 100, 100, 100, 100 ) );
				mP.drawPolygon( polyN );

				mP.fillMode( DRAW_LINE );
				mP.setColor( ColorA( 255, 255, 0, 200 ) );
				mP.drawPolygon( polyN );

				break;
			}
			case INSERT_POLYLINE:
			{
				mP.fillMode( DRAW_LINE );
				mP.setColor( ColorA( 255, 0, 0, 200 ) );
				mP.drawPolygon( mObjPoly );

				Polygon2f polyN( mObjPoly );
				polyN.pushBack( GetMouseMapPos() );

				mP.fillMode( DRAW_LINE );
				mP.setColor( ColorA( 255, 255, 0, 200 ) );
				mP.drawPolygon( polyN );

				break;
			}
			case EDIT_POLYGONS:
			{
				if ( NULL != mSelObj && eeINDEX_NOT_FOUND != mSelPointIndex ) {
					mP.setColor( ColorA( 255, 255, 100, 100 ) );

					mP.fillMode( DRAW_FILL );
					mP.drawRectangle( mSelPointRect );

					mP.fillMode( DRAW_LINE );
					mP.drawRectangle( mSelPointRect );
				}

				break;
			}
		}
	}
}

void UIMap::EditingLights( const bool& editing ) {
	mEditingMode = ( editing ) ? EDITING_LIGHT : 0;

	if ( editing && NULL != mMap->GetLightManager() && NULL != mAddLight ) {
		mMap->GetLightManager()->RemoveLight( mAddLight );

		mAddLight = NULL;
	}
}

bool UIMap::EditingLights() {
	return EDITING_LIGHT == mEditingMode;
}

void UIMap::EditingObjects( const bool& editing ) {
	mEditingMode = ( editing ) ? EDITING_OBJECT : 0;
}

bool UIMap::EditingObjects() {
	return EDITING_OBJECT == mEditingMode;
}

void UIMap::EditingDisabled() {
	mEditingMode = 0;
}

MapLight * UIMap::GetSelectedLight() {
	return mSelLight;
}

MapLight * UIMap::GetAddLight() {
	return mAddLight;
}

void UIMap::SetLightSelectCb( LightSelectCb Cb ) {
	mLightSelCb = Cb;
}

void UIMap::SetLightRadiusChangeCb( LightRadiusChangeCb Cb ) {
	mLightRadiusChangeCb = Cb;
}

void UIMap::SetAddObjectCallback( ObjAddCb Cb ) {
	mAddObjectCallback = Cb;
}

void UIMap::ClearLights() {
	mSelLight = NULL;
	mAddLight = NULL;
}

void UIMap::onAlphaChange() {
	UIComplexControl::onAlphaChange();

	if ( NULL != mMap ) {
		mMap->BackAlpha( (Uint8)mAlpha );
	}
}

void UIMap::ClampToTile( const bool& clamp ) {
	mClampToTile = clamp;
}

const bool& UIMap::ClampToTile() const {
	return mClampToTile;
}

void UIMap::EditingObjMode( EDITING_OBJ_MODE mode ) {
	mObjPoly.clear();
	mSelPointIndex = eeINDEX_NOT_FOUND;

	mEditingObjMode = mode;
}

void UIMap::CurLayer( MapLayer * layer ) {
	mCurLayer = layer;
}

void UIMap::SetAlertCb( AlertCb Cb ) {
	mAlertCb = Cb;
}

void UIMap::SetUpdateScrollCb( UpdateScrollCb Cb ) {
	mUpdateScrollCb = Cb;
}

Uint32 UIMap::onMessage( const UIMessage * Msg ) {
	if ( Msg->getMsg() == UIMessage::MsgClick && Msg->getSender() == this && ( Msg->getFlags() & EE_BUTTON_RMASK ) ) {
		if ( SELECT_OBJECTS == mEditingObjMode && NULL != mSelObj && mSelObj->PointInside( mMap->GetMouseMapPosf() ) ) {
			CreateObjPopUpMenu();
		}
	}

	return 0;
}

void UIMap::ObjItemClick( const UIEvent * Event ) {
	if ( !Event->getControl()->isType( UI_TYPE_MENUITEM ) )
		return;

	if ( NULL != mSelObj && NULL != mCurLayer && mCurLayer->Type() == MAP_LAYER_OBJECT && mSelObj->Layer() == mCurLayer ) {
		const String& txt = reinterpret_cast<UIMenuItem*> ( Event->getControl() )->text();

		MapObjectLayer * tLayer = reinterpret_cast<MapObjectLayer*>( mCurLayer );

		if ( "Duplicate Object" == txt ) {
			tLayer->AddGameObject( mSelObj->Copy() );
		} else if ( "Remove Object" == txt ) {
			tLayer->RemoveGameObject( mSelObj );

			mSelObj->Selected( false );
			mSelObj = NULL;
		} else if ( "Object Properties..." == txt ) {
			eeNew( MapObjectProperties, ( mSelObj ) );
		}
	}
}

void UIMap::CreateObjPopUpMenu() {
	UIPopUpMenu * Menu = mTheme->createPopUpMenu();

	Menu->Add( "Duplicate Object" );
	Menu->Add( "Remove Object" );
	Menu->AddSeparator();
	Menu->Add( "Object Properties..." );
	Menu->addEventListener( UIEvent::EventOnItemClicked, cb::Make1( this, &UIMap::ObjItemClick ) );

	if ( Menu->show() ) {
		Vector2i Pos = UIManager::instance()->getInput()->getMousePos();
		UIMenu::FixMenuPos( Pos , Menu );
		Menu->position( Pos );
	}
}

void UIMap::SetTileBox( UITextBox * tilebox ) {
	mTileBox = tilebox;
}

}}}
