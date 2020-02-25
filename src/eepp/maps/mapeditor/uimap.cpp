#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/maps/gameobjectobject.hpp>
#include <eepp/maps/mapeditor/mapobjectproperties.hpp>
#include <eepp/maps/mapeditor/uimap.hpp>
#include <eepp/maps/mapobjectlayer.hpp>
#include <eepp/ui/uipopupmenu.hpp>

namespace EE { namespace Maps { namespace Private {

UIMap* UIMap::New( UITheme* Theme, TileMap* Map ) {
	return eeNew( UIMap, ( Theme, Map ) );
}

UIMap::UIMap( UITheme* Theme, TileMap* Map ) :
	UIWindow( SIMPLE_LAYOUT, StyleConfig( UI_WIN_NO_DECORATION | UI_WIN_FRAME_BUFFER ) ),
	mMap( Map ),
	mCurLayer( NULL ),
	mEditingMode( 0 ),
	mEditingObjMode( SELECT_OBJECTS ),
	mAddLight( NULL ),
	mSelLight( NULL ),
	mClampToTile( true ),
	mObjRECTEditing( false ),
	mObjPolyEditing( false ),
	mObjDragging( false ),
	mSelObj( NULL ),
	mTheme( Theme ),
	mSelPointIndex( eeINDEX_NOT_FOUND ),
	mSelPoint( false ),
	mTileBox( NULL ) {
	subscribeScheduledUpdate();

	if ( NULL == Map ) {
		mMap = eeNew( TileMap, () );
	}

	mContainer->setVisible( false )->setEnabled( false );

	mMap->setBackColor( Color( 100, 100, 100, 100 ) );
	mMap->setGridLinesColor( Color( 150, 150, 150, 150 ) );
	mMap->setScale( PixelDensity::getPixelDensity() );
	mMap->setDrawCallback( cb::Make0( this, &UIMap::mapDraw ) );
	mMap->setViewSize( mSize );

	mDragButton = EE_BUTTON_MMASK;
	setDragEnabled( true );

	onUpdateScreenPos();

	addEventListener( Event::OnUpdateScreenPosition, [&]( const Event* ) { onUpdateScreenPos(); } );
}

UIMap::~UIMap() {
	eeSAFE_DELETE( mMap );
}

Uint32 UIMap::onDrag( const Vector2f& Pos, const Uint32& ) {

	if ( ( EDITING_OBJECT == mEditingMode && NULL != mSelObj ) ||
		 ( EDITING_LIGHT == mEditingMode && NULL != mSelLight ) ) {
		mDragPoint = Pos;
		return 0;
	}

	Vector2f nPos( -( mDragPoint - Pos ) );
	Vector2f nPosf( nPos.x, nPos.y );

	mMap->move( nPosf );

	mDragPoint = Pos;

	if ( mUpdateScrollCb ) {
		mUpdateScrollCb();
	}

	return 0;
}

void UIMap::replaceMap( TileMap* newMap ) {
	eeSAFE_DELETE( mMap );
	mMap = newMap;
	updateScreenPos();
}

TileMap* UIMap::Map() const {
	return mMap;
}

void UIMap::draw() {
	UIWindow::draw();

	if ( NULL != mMap ) {
		mMap->draw();
	}
}

void UIMap::onUpdateScreenPos() {
	if ( NULL != mMap ) {
		mMap->setPosition( Vector2i( mScreenPos.x, mScreenPos.y ) );
	}
}

void UIMap::scheduledUpdate( const Time& time ) {
	UIWindow::scheduledUpdate( time );

	if ( NULL != mMap ) {
		invalidate( this );
		invalidateDraw();

		mMap->update();

		if ( mEnabled && mVisible && isMouseOver() ) {
			Uint32 Flags = getEventDispatcher()->getClickTrigger();

			if ( EDITING_LIGHT == mEditingMode ) {
				if ( NULL != mSelLight ) {
					if ( Flags & EE_BUTTONS_WUWD ) {
						if ( Flags & EE_BUTTON_WUMASK ) {
							mSelLight->setRadius( mSelLight->getRadius() + 10 );
						} else if ( Flags & EE_BUTTON_WDMASK ) {
							mSelLight->setRadius( mSelLight->getRadius() - 10 );
						}

						if ( mLightRadiusChangeCb )
							mLightRadiusChangeCb( mSelLight );
					} else if ( Flags & EE_BUTTON_RMASK ) {
						if ( mSelLight == mAddLight ) {
							mMap->getLightManager()->removeLight( mAddLight );

							eeSAFE_DELETE( mAddLight );

							mSelLight = NULL;
						} else if ( NULL != mSelLight ) {
							if ( mSelLight->getAABB().contains( mMap->getMouseMapPosf() ) ) {
								mMap->getLightManager()->removeLight( mSelLight );

								eeSAFE_DELETE( mSelLight );
							}
						}
					} else if ( Flags & EE_BUTTON_LMASK ) {
						if ( mSelLight == mAddLight ) {
							mAddLight = NULL;
						} else {
							tryToSelectLight();
						}
					}

					Flags = getEventDispatcher()->getPressTrigger();

					if ( Flags & EE_BUTTON_MMASK ) {
						mSelLight->setPosition( mMap->getMouseMapPosf() );
					}
				} else {
					if ( Flags & EE_BUTTON_LMASK ) {
						tryToSelectLight();
					}
				}
			} else if ( EDITING_OBJECT == mEditingMode ) {
				manageObject( Flags );
			}
		}
	}
}

Vector2f UIMap::getMouseMapPos() {
	Vector2f mp( mMap->getMouseMapPosf() );

	if ( mClampToTile ) {
		Vector2i mpc( mMap->getTileCoords( mMap->getMouseTilePos() + 1 ) );
		mp = Vector2f( mpc.x, mpc.y );
	}

	return mp;
}

void UIMap::selectPolyObj() {
	if ( NULL != mCurLayer && mCurLayer->getType() == MAP_LAYER_OBJECT ) {
		MapObjectLayer* tLayer = reinterpret_cast<MapObjectLayer*>( mCurLayer );

		GameObject* tObj =
			tLayer->getObjectOver( mMap->getMouseMapPos(), MapObjectLayer::SEARCH_POLY );

		if ( NULL != tObj ) {
			if ( NULL != mSelObj ) {
				mSelObj->setSelected( false );
			}

			mSelObj = reinterpret_cast<GameObjectObject*>( tObj );
			mSelObj->setSelected( true );
		} else {
			if ( NULL != mSelObj ) {
				mSelObj->setSelected( false );
				mSelObj = NULL;
			}
		}
	} else {
		if ( mAlertCb ) {
			mAlertCb( "No layer found", "An Object Layer must be selected first." )->setFocus();
		}
	}
}

void UIMap::selectPolyPoint() {
	if ( NULL != mCurLayer && mCurLayer->getType() == MAP_LAYER_OBJECT && NULL != mSelObj ) {
		if ( mSelObj->pointInside( mMap->getMouseMapPosf() ) ) {
			mSelPointIndex = mSelObj->getPolygon().closestPoint( mMap->getMouseMapPosf() );
			setPointRect( mSelObj->getPolygon().getAt( mSelPointIndex ) );
		}
	}
}

void UIMap::dragPoly( Uint32 Flags, Uint32 PFlags ) {
	if ( ( PFlags & EE_BUTTON_MMASK ) && NULL != mSelObj ) {
		if ( mSelObj->pointInside( mMap->getMouseMapPosf() ) ) {
			if ( !mObjDragging ) {
				mObjDragging = true;
				mObjDragDist = getMouseMapPos() - mSelObj->getPosition();
			}
		}

		mSelObj->setPosition( getMouseMapPos() - mObjDragDist );

		if ( EDIT_POLYGONS == mEditingObjMode ) {
			selectPolyPoint();
		}
	} else if ( Flags & EE_BUTTON_MMASK ) {
		if ( mObjDragging ) {
			mObjDragging = false;
			mObjDragDist = Vector2f( 0, 0 );
		}
	}
}

void UIMap::manageObject( Uint32 Flags ) {
	Uint32 PFlags = getEventDispatcher()->getPressTrigger();
	Uint32 LPFlags = getEventDispatcher()->getLastPressTrigger();

	switch ( mEditingObjMode ) {
		case INSERT_OBJECT: {
			if ( PFlags & EE_BUTTON_LMASK ) {
				Vector2f mp( getMouseMapPos() );

				if ( !mObjRECTEditing ) {
					mObjRECTEditing = true;
					mObjRECT = Rectf( mp, Sizef( 0, 0 ) );
				} else {
					if ( mObjRECT.getPosition().x < mp.x && mObjRECT.getPosition().y < mp.y ) {
						mObjRECT =
							Rectf( mObjRECT.getPosition(), Sizef( mp - mObjRECT.getPosition() ) );
					}
				}
			}

			if ( Flags & EE_BUTTON_LMASK ) {
				if ( mObjRECTEditing ) {
					mAddObjectCallback( GAMEOBJECT_TYPE_OBJECT, Polygon2f( mObjRECT ) );
					mObjRECTEditing = false;
				}
			}

			break;
		}
		case INSERT_POLYLINE:
		case INSERT_POLYGON: {
			if ( Flags & EE_BUTTON_LMASK ) {
				mObjPoly.pushBack( getMouseMapPos() );
			} else if ( Flags & EE_BUTTON_RMASK ) {
				mAddObjectCallback( ( INSERT_POLYGON == mEditingObjMode )
										? GAMEOBJECT_TYPE_POLYGON
										: GAMEOBJECT_TYPE_POLYLINE,
									mObjPoly );

				mObjPoly.clear();
			}

			break;
		}
		case SELECT_OBJECTS: {
			if ( ( Flags & EE_BUTTON_LMASK ) ) {
				selectPolyObj();
			} else {
				dragPoly( Flags, PFlags );
			}

			break;
		}
		case EDIT_POLYGONS: {
			if ( ( Flags & EE_BUTTON_LMASK ) ) {
				if ( !mSelPoint ) {
					selectPolyObj();
					selectPolyPoint();
				} else {
					mSelPoint = false;
				}
			} else if ( !( LPFlags & EE_BUTTON_LMASK ) && ( PFlags & EE_BUTTON_LMASK ) ) {
				if ( NULL != mSelObj && eeINDEX_NOT_FOUND != mSelPointIndex &&
					 mSelPointRect.contains( mMap->getMouseMapPosf() ) ) {
					mSelPoint = true;
				}
			} else if ( ( PFlags & EE_BUTTON_LMASK ) ) {
				if ( mSelPoint && NULL != mSelObj && eeINDEX_NOT_FOUND != mSelPointIndex ) {
					mSelObj->setPolygonPoint( mSelPointIndex, getMouseMapPos() );
					setPointRect( getMouseMapPos() );
				}
			} else {
				dragPoly( Flags, PFlags );
			}

			break;
		}
		default: {
		}
	}
}

void UIMap::setPointRect( Vector2f p ) {
	mSelPointRect = Rectf( Vector2f( p.x - 10, p.y - 10 ), Sizef( 20, 20 ) );
}

void UIMap::tryToSelectLight() {
	MapLight* tLight = mSelLight;
	mSelLight = mMap->getLightManager()->getLightOver( mMap->getMouseMapPosf(), mSelLight );

	if ( NULL != mSelLight && mSelLight != tLight ) {
		if ( mLightSelCb )
			mLightSelCb( mSelLight );
	}
}

void UIMap::onSizeChange() {
	if ( NULL != mMap ) {
		mMap->setPosition( Vector2i( mScreenPos.x, mScreenPos.y ) );
		mMap->setViewSize( mSize );
	}

	UIWindow::onSizeChange();
}

Uint32 UIMap::onMouseMove( const Vector2i& Pos, const Uint32& Flags ) {
	if ( NULL != mMap ) {
		if ( EDITING_LIGHT == mEditingMode && NULL != mAddLight ) {
			mAddLight->setPosition( mMap->getMouseMapPosf() );
		}
	}

	if ( NULL != mTileBox ) {
		Vector2i mp( mMap->getMouseTilePos() );

		if ( mLastMouseTilePos != mp ) {
			mLastMouseTilePos = mp;
			mTileBox->setText( String::toStr( mp.x ) + "," + String::toStr( mp.y ) );
		}
	}

	return UIWindow::onMouseMove( Pos, Flags );
}

void UIMap::addLight( MapLight* Light ) {
	if ( NULL != mMap->getLightManager() ) {
		if ( NULL != mAddLight ) {
			mMap->getLightManager()->removeLight( mAddLight );

			eeSAFE_DELETE( mAddLight );
		}

		mAddLight = Light;

		mSelLight = Light;

		mMap->getLightManager()->addLight( Light );

		if ( mLightSelCb )
			mLightSelCb( mSelLight );
	}
}

void UIMap::mapDraw() {
	if ( EDITING_LIGHT == mEditingMode ) {
		if ( NULL != mSelLight ) {
			mP.setColor( Color( 255, 0, 0, (Uint8)mAlpha ) );

			Vector2f Pos( mSelLight->getAABB().Left, mSelLight->getAABB().Top );
			Rectf AB( mSelLight->getAABB() );

			mP.setFillMode( DRAW_LINE );
			mP.drawRectangle( Rectf( Pos, AB.getSize() ) );
		}
	} else if ( EDITING_OBJECT == mEditingMode ) {
		switch ( mEditingObjMode ) {
			case INSERT_OBJECT: {
				if ( mObjRECTEditing ) {
					mP.setFillMode( DRAW_FILL );
					mP.setColor( Color( 100, 100, 100, 20 ) );
					mP.drawRectangle( mObjRECT );

					mP.setFillMode( DRAW_LINE );
					mP.setColor( Color( 255, 0, 0, 200 ) );
					mP.drawRectangle( mObjRECT );
				}

				break;
			}
			case INSERT_POLYGON: {
				mP.setFillMode( DRAW_FILL );
				mP.setColor( Color( 50, 50, 50, 50 ) );
				mP.drawPolygon( mObjPoly );

				mP.setFillMode( DRAW_LINE );
				mP.setColor( Color( 255, 0, 0, 200 ) );
				mP.drawPolygon( mObjPoly );

				Polygon2f polyN( mObjPoly );
				polyN.pushBack( getMouseMapPos() );

				mP.setFillMode( DRAW_FILL );
				mP.setColor( Color( 100, 100, 100, 100 ) );
				mP.drawPolygon( polyN );

				mP.setFillMode( DRAW_LINE );
				mP.setColor( Color( 255, 255, 0, 200 ) );
				mP.drawPolygon( polyN );

				break;
			}
			case INSERT_POLYLINE: {
				mP.setFillMode( DRAW_LINE );
				mP.setColor( Color( 255, 0, 0, 200 ) );
				mP.drawPolygon( mObjPoly );

				Polygon2f polyN( mObjPoly );
				polyN.pushBack( getMouseMapPos() );

				mP.setFillMode( DRAW_LINE );
				mP.setColor( Color( 255, 255, 0, 200 ) );
				mP.drawPolygon( polyN );

				break;
			}
			case EDIT_POLYGONS: {
				if ( NULL != mSelObj && eeINDEX_NOT_FOUND != mSelPointIndex ) {
					mP.setColor( Color( 255, 255, 100, 100 ) );

					mP.setFillMode( DRAW_FILL );
					mP.drawRectangle( mSelPointRect );

					mP.setFillMode( DRAW_LINE );
					mP.drawRectangle( mSelPointRect );
				}

				break;
			}
		}
	}
}

void UIMap::setEditingLights( const bool& editing ) {
	mEditingMode = ( editing ) ? EDITING_LIGHT : 0;

	if ( editing && NULL != mMap->getLightManager() && NULL != mAddLight ) {
		mMap->getLightManager()->removeLight( mAddLight );

		mAddLight = NULL;
	}
}

bool UIMap::isEditingLights() {
	return EDITING_LIGHT == mEditingMode;
}

void UIMap::setEditingObjects( const bool& editing ) {
	mEditingMode = ( editing ) ? EDITING_OBJECT : 0;
}

bool UIMap::isEditingObjects() {
	return EDITING_OBJECT == mEditingMode;
}

void UIMap::editingDisable() {
	mEditingMode = 0;
}

MapLight* UIMap::getSelectedLight() {
	return mSelLight;
}

MapLight* UIMap::getAddLight() {
	return mAddLight;
}

void UIMap::setLightSelectCb( LightSelectCb Cb ) {
	mLightSelCb = Cb;
}

void UIMap::setLightRadiusChangeCb( LightRadiusChangeCb Cb ) {
	mLightRadiusChangeCb = Cb;
}

void UIMap::setAddObjectCallback( ObjAddCb Cb ) {
	mAddObjectCallback = Cb;
}

void UIMap::clearLights() {
	mSelLight = NULL;
	mAddLight = NULL;
}

void UIMap::onAlphaChange() {
	UIWindow::onAlphaChange();

	if ( NULL != mMap ) {
		mMap->setBackAlpha( (Uint8)mAlpha );
	}
}

void UIMap::setClampToTile( const bool& clamp ) {
	mClampToTile = clamp;
}

const bool& UIMap::getClampToTile() const {
	return mClampToTile;
}

void UIMap::setEditingObjMode( EDITING_OBJ_MODE mode ) {
	mObjPoly.clear();
	mSelPointIndex = eeINDEX_NOT_FOUND;

	mEditingObjMode = mode;
}

void UIMap::setCurLayer( MapLayer* layer ) {
	mCurLayer = layer;
}

void UIMap::setAlertCb( AlertCb Cb ) {
	mAlertCb = Cb;
}

void UIMap::setUpdateScrollCb( UpdateScrollCb Cb ) {
	mUpdateScrollCb = Cb;
}

Uint32 UIMap::onMessage( const NodeMessage* Msg ) {
	if ( Msg->getMsg() == NodeMessage::Click && Msg->getSender() == this &&
		 ( Msg->getFlags() & EE_BUTTON_RMASK ) ) {
		if ( SELECT_OBJECTS == mEditingObjMode && NULL != mSelObj &&
			 mSelObj->pointInside( mMap->getMouseMapPosf() ) ) {
			createObjPopUpMenu();
		}
	}

	return 0;
}

void UIMap::objItemClick( const Event* Event ) {
	if ( !Event->getNode()->isType( UI_TYPE_MENUITEM ) )
		return;

	if ( NULL != mSelObj && NULL != mCurLayer && mCurLayer->getType() == MAP_LAYER_OBJECT &&
		 mSelObj->getLayer() == mCurLayer ) {
		const String& txt = Event->getNode()->asType<UIMenuItem>()->getText();

		MapObjectLayer* tLayer = reinterpret_cast<MapObjectLayer*>( mCurLayer );

		if ( "Duplicate Object" == txt ) {
			tLayer->addGameObject( mSelObj->clone() );
		} else if ( "Remove Object" == txt ) {
			tLayer->removeGameObject( mSelObj );

			mSelObj->setSelected( false );
			mSelObj = NULL;
		} else if ( "Object Properties..." == txt ) {
			eeNew( MapObjectProperties, ( mSelObj ) );
		}
	}
}

void UIMap::createObjPopUpMenu() {
	UIPopUpMenu* Menu = UIPopUpMenu::New();

	Menu->add( "Duplicate Object" );
	Menu->add( "Remove Object" );
	Menu->addSeparator();
	Menu->add( "Object Properties..." );
	Menu->addEventListener( Event::OnItemClicked, cb::Make1( this, &UIMap::objItemClick ) );

	if ( Menu->show() ) {
		Vector2f Pos = getEventDispatcher()->getMousePosf();
		UIMenu::fixMenuPos( Pos, Menu );
		Menu->setPixelsPosition( Pos );
	}
}

void UIMap::setTileBox( UITextView* tilebox ) {
	mTileBox = tilebox;
}

}}} // namespace EE::Maps::Private
