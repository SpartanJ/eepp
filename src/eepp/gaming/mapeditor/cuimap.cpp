#include <eepp/gaming/mapeditor/cuimap.hpp>
#include <eepp/gaming/mapeditor/cobjectproperties.hpp>
#include <eepp/gaming/cgameobjectobject.hpp>
#include <eepp/gaming/cobjectlayer.hpp>
#include <eepp/ui/cuimanager.hpp>
#include <eepp/ui/cuipopupmenu.hpp>

namespace EE { namespace Gaming { namespace MapEditor {

cUIMap::cUIMap( const cUIComplexControl::CreateParams& Params, cUITheme * Theme, cMap * Map ) :
	cUIComplexControl( Params ),
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
		mMap = eeNew( cMap, () );
	}

	mMap->BackColor( ColorA( 100, 100, 100, 100 ) );
	mMap->GridLinesColor( ColorA( 150, 150, 150, 150 ) );

	mMap->SetDrawCallback( cb::Make0( this, &cUIMap::MapDraw ) );

	mDragButton = EE_BUTTON_MMASK;
	DragEnable( true );

	UpdateScreenPos();
}

cUIMap::~cUIMap() {
	eeSAFE_DELETE( mMap );
}

Uint32 cUIMap::OnDrag( const eeVector2i& Pos ) {

	if (	( EDITING_OBJECT == mEditingMode && NULL != mSelObj ) ||
			( EDITING_LIGHT == mEditingMode && NULL != mSelLight ) ) {
		mDragPoint = Pos;
		return 0;
	}

	eeVector2i nPos( -( mDragPoint - Pos ) );
	eeVector2f nPosf( nPos.x, nPos.y );

	mMap->Move( nPosf );

	mDragPoint = Pos;

	if ( mUpdateScrollCb.IsSet() ) {
		mUpdateScrollCb();
	}

	return 0;
}

void cUIMap::ReplaceMap( cMap * newMap ) {
	eeSAFE_DELETE( mMap );
	mMap = newMap;
	UpdateScreenPos();
}

cMap * cUIMap::Map() const {
	return mMap;
}

void cUIMap::Draw() {
	cUIComplexControl::Draw();

	if ( NULL != mMap ) {
		mMap->Draw();
	}
}

void cUIMap::UpdateScreenPos() {
	cUIComplexControl::UpdateScreenPos();

	if ( NULL != mMap ) {
		mMap->Position( mScreenPos );
	}
}

void cUIMap::Update() {
	cUIComplexControl::Update();

	if ( NULL != mMap ) {
		mMap->Update();

		if ( mEnabled && mVisible && IsMouseOver() ) {
			Uint32 Flags 			= cUIManager::instance()->GetInput()->ClickTrigger();

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
							if ( mSelLight->GetAABB().Contains( mMap->GetMouseMapPosf() ) ) {
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

					Flags = cUIManager::instance()->GetInput()->PressTrigger();

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

eeVector2f cUIMap::GetMouseMapPos() {
	eeVector2f mp( mMap->GetMouseMapPosf() );

	if ( mClampToTile ) {
		eeVector2i mpc( mMap->GetTileCoords( mMap->GetMouseTilePos() + 1 ) );
		mp = eeVector2f( mpc.x, mpc.y );
	}

	return mp;
}

void cUIMap::SelectPolyObj() {
	if ( NULL != mCurLayer && mCurLayer->Type() == MAP_LAYER_OBJECT ) {
		cObjectLayer * tLayer = reinterpret_cast<cObjectLayer*>( mCurLayer );

		cGameObject * tObj = tLayer->GetObjectOver( mMap->GetMouseMapPos(), cObjectLayer::SEARCH_POLY );

		if ( NULL != tObj ) {
			if ( NULL != mSelObj ) {
				mSelObj->Selected( false );
			}

			mSelObj = reinterpret_cast<cGameObjectObject*>( tObj );
			mSelObj->Selected( true );
		} else {
			if ( NULL != mSelObj ) {
				mSelObj->Selected( false );
				mSelObj = NULL;
			}
		}
	} else {
		if ( mAlertCb.IsSet() ) {
			mAlertCb( "No layer found", "An Object Layer must be selected first." )->SetFocus();
		}
	}
}

void cUIMap::SelectPolyPoint() {
	if ( NULL != mCurLayer && mCurLayer->Type() == MAP_LAYER_OBJECT && NULL != mSelObj ) {
		if ( mSelObj->PointInside( mMap->GetMouseMapPosf() ) ) {
			mSelPointIndex = mSelObj->GetPolygon().ClosestPoint( mMap->GetMouseMapPosf() );
			SetPointRect( mSelObj->GetPolygon().GetAt( mSelPointIndex ) );
		}
	}
}

void cUIMap::DragPoly( Uint32 Flags, Uint32 PFlags ) {
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
			mObjDragDist = eeVector2f(0,0);
		}
	}
}

void cUIMap::ManageObject( Uint32 Flags ) {
	Uint32 PFlags	= cUIManager::instance()->GetInput()->PressTrigger();
	Uint32 LPFlags	= cUIManager::instance()->GetInput()->LastPressTrigger();

	switch ( mEditingObjMode )
	{
		case INSERT_OBJECT:
		{
			if ( PFlags & EE_BUTTON_LMASK ) {
				eeVector2f mp( GetMouseMapPos() );

				if ( !mObjRECTEditing ) {
					mObjRECTEditing = true;
					mObjRECT		= eeRectf( mp, eeSizef(0,0) );
				} else {
					if ( mObjRECT.Pos().x < mp.x && mObjRECT.Pos().y < mp.y ) {
						mObjRECT		= eeRectf( mObjRECT.Pos(), eeSizef( mp - mObjRECT.Pos() ) );
					}
				}
			}

			if ( Flags & EE_BUTTON_LMASK ){
				if ( mObjRECTEditing ) {
					mAddObjectCallback( GAMEOBJECT_TYPE_OBJECT, eePolygon2f( mObjRECT ) );
					mObjRECTEditing = false;
				}
			}

			break;
		}
		case INSERT_POLYLINE:
		case INSERT_POLYGON:
		{
			if ( Flags & EE_BUTTON_LMASK ) {
				mObjPoly.PushBack( GetMouseMapPos() );
			} else if ( Flags & EE_BUTTON_RMASK ) {
				mAddObjectCallback( ( INSERT_POLYGON == mEditingObjMode ) ? GAMEOBJECT_TYPE_POLYGON : GAMEOBJECT_TYPE_POLYLINE, mObjPoly );

				mObjPoly.Clear();
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
				if ( NULL != mSelObj && eeINDEX_NOT_FOUND != mSelPointIndex && mSelPointRect.Contains( mMap->GetMouseMapPosf() ) ) {
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

void cUIMap::SetPointRect( eeVector2f p ) {
	mSelPointRect = eeRectf( eeVector2f( p.x - 10, p.y - 10 ), eeSizef( 20, 20 ) );
}

void cUIMap::TryToSelectLight() {
	cLight * tLight = mSelLight;
	mSelLight = mMap->GetLightManager()->GetLightOver( mMap->GetMouseMapPosf(), mSelLight );

	if ( NULL != mSelLight && mSelLight != tLight ) {
		if ( mLightSelCb.IsSet() )
			mLightSelCb( mSelLight );
	}
}

void cUIMap::OnSizeChange() {
	if ( NULL != mMap ) {
		mMap->Position( mScreenPos );
		mMap->ViewSize( mSize );
	}

	cUIComplexControl::OnSizeChange();
}

Uint32 cUIMap::OnMouseMove( const eeVector2i& Pos, const Uint32 Flags ) {
	if ( NULL != mMap ) {
		if ( EDITING_LIGHT == mEditingMode && NULL != mAddLight ) {
			mAddLight->Position( mMap->GetMouseMapPosf() );
		}
	}

	if ( NULL != mTileBox ) {
		eeVector2i mp( mMap->GetMouseTilePos() );

		if ( mLastMouseTilePos != mp ) {
			mLastMouseTilePos = mp;
			mTileBox->Text( String::ToStr( mp.x ) + "," + String::ToStr( mp.y ) );
		}
	}

	return cUIComplexControl::OnMouseMove( Pos, Flags );
}

void cUIMap::AddLight( cLight * Light ) {
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

void cUIMap::MapDraw() {
	if ( EDITING_LIGHT == mEditingMode ) {
		if ( NULL != mSelLight ) {
			mP.SetColor( ColorA( 255, 0, 0, (Uint8)mAlpha ) );

			eeVector2f Pos( mSelLight->GetAABB().Left, mSelLight->GetAABB().Top );
			eeAABB AB( mSelLight->GetAABB() );

			mP.FillMode( DRAW_LINE );
			mP.DrawRectangle( eeRectf( Pos, AB.Size() ) );
		}
	} else if ( EDITING_OBJECT == mEditingMode ) {
		switch ( mEditingObjMode ) {
			case INSERT_OBJECT:
			{
				if ( mObjRECTEditing ) {
					mP.FillMode( DRAW_FILL );
					mP.SetColor( ColorA( 100, 100, 100, 20 ) );
					mP.DrawRectangle( mObjRECT );

					mP.FillMode( DRAW_LINE );
					mP.SetColor( ColorA( 255, 0, 0, 200 ) );
					mP.DrawRectangle( mObjRECT );
				}

				break;
			}
			case INSERT_POLYGON:
			{
				mP.FillMode( DRAW_FILL );
				mP.SetColor( ColorA( 50, 50, 50, 50 ) );
				mP.DrawPolygon( mObjPoly );

				mP.FillMode( DRAW_LINE );
				mP.SetColor( ColorA( 255, 0, 0, 200 ) );
				mP.DrawPolygon( mObjPoly );

				eePolygon2f polyN( mObjPoly );
				polyN.PushBack( GetMouseMapPos() );

				mP.FillMode( DRAW_FILL );
				mP.SetColor( ColorA( 100, 100, 100, 100 ) );
				mP.DrawPolygon( polyN );

				mP.FillMode( DRAW_LINE );
				mP.SetColor( ColorA( 255, 255, 0, 200 ) );
				mP.DrawPolygon( polyN );

				break;
			}
			case INSERT_POLYLINE:
			{
				mP.FillMode( DRAW_LINE );
				mP.SetColor( ColorA( 255, 0, 0, 200 ) );
				mP.DrawPolygon( mObjPoly );

				eePolygon2f polyN( mObjPoly );
				polyN.PushBack( GetMouseMapPos() );

				mP.FillMode( DRAW_LINE );
				mP.SetColor( ColorA( 255, 255, 0, 200 ) );
				mP.DrawPolygon( polyN );

				break;
			}
			case EDIT_POLYGONS:
			{
				if ( NULL != mSelObj && eeINDEX_NOT_FOUND != mSelPointIndex ) {
					mP.SetColor( ColorA( 255, 255, 100, 100 ) );

					mP.FillMode( DRAW_FILL );
					mP.DrawRectangle( mSelPointRect );

					mP.FillMode( DRAW_LINE );
					mP.DrawRectangle( mSelPointRect );
				}

				break;
			}
		}
	}
}

void cUIMap::EditingLights( const bool& editing ) {
	mEditingMode = ( editing ) ? EDITING_LIGHT : 0;

	if ( editing && NULL != mMap->GetLightManager() && NULL != mAddLight ) {
		mMap->GetLightManager()->RemoveLight( mAddLight );

		mAddLight = NULL;
	}
}

bool cUIMap::EditingLights() {
	return EDITING_LIGHT == mEditingMode;
}

void cUIMap::EditingObjects( const bool& editing ) {
	mEditingMode = ( editing ) ? EDITING_OBJECT : 0;
}

bool cUIMap::EditingObjects() {
	return EDITING_OBJECT == mEditingMode;
}

void cUIMap::EditingDisabled() {
	mEditingMode = 0;
}

cLight * cUIMap::GetSelectedLight() {
	return mSelLight;
}

cLight * cUIMap::GetAddLight() {
	return mAddLight;
}

void cUIMap::SetLightSelectCb( LightSelectCb Cb ) {
	mLightSelCb = Cb;
}

void cUIMap::SetLightRadiusChangeCb( LightRadiusChangeCb Cb ) {
	mLightRadiusChangeCb = Cb;
}

void cUIMap::SetAddObjectCallback( ObjAddCb Cb ) {
	mAddObjectCallback = Cb;
}

void cUIMap::ClearLights() {
	mSelLight = NULL;
	mAddLight = NULL;
}

void cUIMap::OnAlphaChange() {
	cUIComplexControl::OnAlphaChange();

	if ( NULL != mMap ) {
		mMap->BackAlpha( (Uint8)mAlpha );
	}
}

void cUIMap::ClampToTile( const bool& clamp ) {
	mClampToTile = clamp;
}

const bool& cUIMap::ClampToTile() const {
	return mClampToTile;
}

void cUIMap::EditingObjMode( EDITING_OBJ_MODE mode ) {
	mObjPoly.Clear();
	mSelPointIndex = eeINDEX_NOT_FOUND;

	mEditingObjMode = mode;
}

void cUIMap::CurLayer( cLayer * layer ) {
	mCurLayer = layer;
}

void cUIMap::SetAlertCb( AlertCb Cb ) {
	mAlertCb = Cb;
}

void cUIMap::SetUpdateScrollCb( UpdateScrollCb Cb ) {
	mUpdateScrollCb = Cb;
}

Uint32 cUIMap::OnMessage( const cUIMessage * Msg ) {
	if ( Msg->Msg() == cUIMessage::MsgClick && Msg->Sender() == this && ( Msg->Flags() & EE_BUTTON_RMASK ) ) {
		if ( SELECT_OBJECTS == mEditingObjMode && NULL != mSelObj && mSelObj->PointInside( mMap->GetMouseMapPosf() ) ) {
			CreateObjPopUpMenu();
		}
	}

	return 0;
}

void cUIMap::ObjItemClick( const cUIEvent * Event ) {
	if ( !Event->Ctrl()->IsType( UI_TYPE_MENUITEM ) )
		return;

	if ( NULL != mSelObj && NULL != mCurLayer && mCurLayer->Type() == MAP_LAYER_OBJECT && mSelObj->Layer() == mCurLayer ) {
		const String& txt = reinterpret_cast<cUIMenuItem*> ( Event->Ctrl() )->Text();

		cObjectLayer * tLayer = reinterpret_cast<cObjectLayer*>( mCurLayer );

		if ( "Duplicate Object" == txt ) {
			tLayer->AddGameObject( mSelObj->Copy() );
		} else if ( "Remove Object" == txt ) {
			tLayer->RemoveGameObject( mSelObj );

			mSelObj->Selected( false );
			mSelObj = NULL;
		} else if ( "Object Properties..." == txt ) {
			eeNew( cObjectProperties, ( mSelObj ) );
		}
	}
}

void cUIMap::CreateObjPopUpMenu() {
	cUIPopUpMenu * Menu = mTheme->CreatePopUpMenu();

	Menu->Add( "Duplicate Object" );
	Menu->Add( "Remove Object" );
	Menu->AddSeparator();
	Menu->Add( "Object Properties..." );
	Menu->AddEventListener( cUIEvent::EventOnItemClicked, cb::Make1( this, &cUIMap::ObjItemClick ) );

	if ( Menu->Show() ) {
		eeVector2i Pos = cUIManager::instance()->GetInput()->GetMousePos();
		cUIMenu::FixMenuPos( Pos , Menu );
		Menu->Pos( Pos );
	}
}

void cUIMap::SetTileBox( cUITextBox * tilebox ) {
	mTileBox = tilebox;
}

}}}
