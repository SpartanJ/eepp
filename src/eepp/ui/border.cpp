#include <eepp/graphics/vertexbuffer.hpp>
#include <eepp/ui/border.hpp>
#include <eepp/ui/uinode.hpp>

namespace EE { namespace UI {

static void borderAddArc( VertexBuffer* vbo, Vector2f pos, Float radiW, Float radiH,
						  double arcStartAngle, double arcEndAngle, Color color, Float lineW,
						  Float lineH, Vector2f basePos, bool decrease = false,
						  bool addInnerVertex = true ) {
	// TODO: Add segment count parameter to change the arc precision (num of vertex).
	Float angleShift = 1;
	double startAngle = eemin( arcStartAngle, arcEndAngle );
	double endAngle = eemax( arcStartAngle, arcEndAngle );
	const Float innerW = eemax( 0.f, radiW - lineW );
	const Float innerH = eemax( 0.f, radiH - lineH );
	Vector2f startPos = innerW > 0.f || innerH > 0.f ? pos : basePos;
	double lastAngle = startAngle;
	double i;

	if ( decrease ) {
		for ( i = endAngle; i >= startAngle; i -= angleShift ) {
			vbo->addVertex(
				Vector2f( pos.x + radiW * Math::cosAng( i ), pos.y + radiH * Math::sinAng( i ) ) );
			vbo->addColor( color );

			if ( addInnerVertex ) {
				if ( innerW > 0.f || innerH > 0.f ) {
					vbo->addVertex( Vector2f( pos.x + innerW * Math::cosAng( i ),
											  pos.y + innerH * Math::sinAng( i ) ) );
				} else {
					vbo->addVertex( startPos );
				}

				vbo->addColor( color );
			}

			lastAngle = i;
		}

		if ( lastAngle != endAngle ) {
			i = endAngle;
			vbo->addVertex(
				Vector2f( pos.x + radiW * Math::cosAng( i ), pos.y + radiH * Math::sinAng( i ) ) );
			vbo->addColor( color );

			if ( addInnerVertex ) {
				if ( innerW > 0.f || innerH > 0.f ) {
					vbo->addVertex( Vector2f( pos.x + innerW * Math::cosAng( i ),
											  pos.y + innerH * Math::sinAng( i ) ) );
				} else {
					vbo->addVertex( startPos );
				}
				vbo->addColor( color );
			}
		}
	} else {
		for ( i = startAngle; i <= endAngle; i += angleShift ) {
			if ( addInnerVertex ) {
				if ( innerW > 0.f || innerH > 0.f ) {
					vbo->addVertex( Vector2f( pos.x + innerW * Math::cosAng( i ),
											  pos.y + innerH * Math::sinAng( i ) ) );
				} else {
					vbo->addVertex( startPos );
				}

				vbo->addColor( color );
			}

			vbo->addVertex(
				Vector2f( pos.x + radiW * Math::cosAng( i ), pos.y + radiH * Math::sinAng( i ) ) );
			vbo->addColor( color );

			lastAngle = i;
		}

		if ( lastAngle != endAngle ) {
			i = endAngle;

			if ( addInnerVertex ) {
				if ( innerW > 0.f || innerH > 0.f ) {
					vbo->addVertex( Vector2f( pos.x + innerW * Math::cosAng( i ),
											  pos.y + innerH * Math::sinAng( i ) ) );
				} else {
					vbo->addVertex( startPos );
				}

				vbo->addColor( color );
			}

			vbo->addVertex(
				Vector2f( pos.x + radiW * Math::cosAng( i ), pos.y + radiH * Math::sinAng( i ) ) );
			vbo->addColor( color );
		}
	}
}

std::string Borders::fromBorderType( const BorderType& borderType ) {
	switch ( borderType ) {
		case BorderType::Outside:
			return "outside";
		case BorderType::Outline:
			return "outline";
		case BorderType::Inside:
		default:
			return "inside";
	}
}

BorderType Borders::toBorderType( const std::string& borderType ) {
	if ( borderType == "outside" ) {
		return BorderType::Outside;
	} else if ( borderType == "outline" ) {
		return BorderType::Outline;
	}
	return BorderType::Inside;
}

std::string Borders::fromBorderStyle( BorderStyle borderStyle ) {
	switch ( borderStyle ) {
		case BorderStyle::None:
			return "none";
		case BorderStyle::Hidden:
			return "hidden";
		case BorderStyle::Dotted:
			return "dotted";
		case BorderStyle::Dashed:
			return "dashed";
		case BorderStyle::Solid:
		default:
			return "solid";
	}
}

BorderStyle Borders::toBorderStyle( const std::string& borderStyle ) {
	if ( borderStyle == "none" )
		return BorderStyle::None;
	if ( borderStyle == "hidden" )
		return BorderStyle::Hidden;
	if ( borderStyle == "dotted" )
		return BorderStyle::Dotted;
	if ( borderStyle == "dashed" )
		return BorderStyle::Dashed;
	return BorderStyle::Solid;
}

Sizef Borders::radiusFromString( const UINode* node, const std::string& val ) {
	auto split = String::split( val, ' ' );
	Sizef size;
	size.x = node->lengthFromValue( split[0], CSS::PropertyRelativeTarget::LocalBlockRadiusWidth );
	size.y = node->lengthFromValue( split[split.size() > 1 ? 1 : 0],
									CSS::PropertyRelativeTarget::LocalBlockRadiusHeight );
	return size;
}

BorderRadiuses Borders::normalizeRadiuses( const BorderRadiuses& radius, const Sizef& size ) {
	BorderRadiuses usedRadius = radius;
	usedRadius.topLeft.x = eemax( 0.f, usedRadius.topLeft.x );
	usedRadius.topLeft.y = eemax( 0.f, usedRadius.topLeft.y );
	usedRadius.topRight.x = eemax( 0.f, usedRadius.topRight.x );
	usedRadius.topRight.y = eemax( 0.f, usedRadius.topRight.y );
	usedRadius.bottomRight.x = eemax( 0.f, usedRadius.bottomRight.x );
	usedRadius.bottomRight.y = eemax( 0.f, usedRadius.bottomRight.y );
	usedRadius.bottomLeft.x = eemax( 0.f, usedRadius.bottomLeft.x );
	usedRadius.bottomLeft.y = eemax( 0.f, usedRadius.bottomLeft.y );

	Float factor = 1.f;
	auto constrain = [&factor]( Float available, Float sum ) {
		if ( sum > 0.f )
			factor = eemin( factor, available / sum );
	};
	const Float width = eemax( 0.f, size.getWidth() );
	const Float height = eemax( 0.f, size.getHeight() );
	constrain( width, usedRadius.topLeft.x + usedRadius.topRight.x );
	constrain( width, usedRadius.bottomLeft.x + usedRadius.bottomRight.x );
	constrain( height, usedRadius.topLeft.y + usedRadius.bottomLeft.y );
	constrain( height, usedRadius.topRight.y + usedRadius.bottomRight.y );

	if ( factor < 1.f ) {
		usedRadius.topLeft *= factor;
		usedRadius.topRight *= factor;
		usedRadius.bottomRight *= factor;
		usedRadius.bottomLeft *= factor;
	}
	return usedRadius;
}

void Borders::createBorders( VertexBuffer* vbo, const Borders& borders, const Vector2f& pos,
							 const Sizef& size ) {
	vbo->clear();

	int borderTop = 0;
	int borderBottom = 0;
	int borderLeft = 0;
	int borderRight = 0;

	if ( borders.top.width >= 0 ) {
		borderTop = eemin( (int)( size.getHeight() * 0.5f ), (int)borders.top.width );
	}

	if ( borders.bottom.width >= 0 ) {
		borderBottom = eemin( (int)( size.getHeight() * 0.5f ), (int)borders.bottom.width );
	}

	if ( borders.left.width >= 0 ) {
		borderLeft = eemin( (int)( size.getWidth() * 0.5f ), (int)borders.left.width );
	}

	if ( borders.right.width >= 0 ) {
		borderRight = eemin( (int)( size.getWidth() * 0.5f ), (int)borders.right.width );
	}

	bool hasTop = borderTop > 0;
	bool hasRight = borderRight > 0;
	bool hasBottom = borderBottom > 0;
	bool hasLeft = borderLeft > 0;

	if ( !hasTop && !hasRight && !hasBottom && !hasLeft )
		return;

	// CSS scales every radius component by one factor when any pair of curves overlaps.
	const BorderRadiuses usedRadius = normalizeRadiuses( borders.radius, size );
	double tlArcW = usedRadius.topLeft.x;
	double tlArcH = usedRadius.topLeft.y;
	double trArcW = usedRadius.topRight.x;
	double trArcH = usedRadius.topRight.y;
	double brArcW = usedRadius.bottomRight.x;
	double brArcH = usedRadius.bottomRight.y;
	double blArcW = usedRadius.bottomLeft.x;
	double blArcH = usedRadius.bottomLeft.y;

	// Corner positions
	Vector2f tlInner( pos.x + borderLeft, pos.y + borderTop );
	Vector2f tlOuter( pos.x, pos.y );
	Vector2f trInner( pos.x + size.getWidth() - borderRight, pos.y + borderTop );
	Vector2f trOuter( pos.x + size.getWidth(), pos.y );
	Vector2f brInner( pos.x + size.getWidth() - borderRight,
					  pos.y + size.getHeight() - borderBottom );
	Vector2f brOuter( pos.x + size.getWidth(), pos.y + size.getHeight() );
	Vector2f blInner( pos.x + borderLeft, pos.y + size.getHeight() - borderBottom );
	Vector2f blOuter( pos.x, pos.y + size.getHeight() );

	// Helper: compute arc outer vertex at a given angle
	auto arcOuterPos = []( const Vector2f& center, double rW, double rH,
						   double angleDeg ) -> Vector2f {
		return Vector2f( center.x + rW * Math::cosAng( angleDeg ),
						 center.y + rH * Math::sinAng( angleDeg ) );
	};

	// Helper: compute arc inner vertex at a given angle
	auto arcInnerPos = []( const Vector2f& center, double rW, double rH, double angleDeg,
						   double lineW, double lineH, const Vector2f& basePos ) -> Vector2f {
		const double innerW = eemax( 0., rW - lineW );
		const double innerH = eemax( 0., rH - lineH );
		if ( innerW > 0. || innerH > 0. )
			return Vector2f( center.x + innerW * Math::cosAng( angleDeg ),
							 center.y + innerH * Math::sinAng( angleDeg ) );
		return basePos;
	};

	// Pre-compute first inner vertex of each border (used as bridge targets)
	// Top border first inner (top-left corner)
	Vector2f topFirstInner;
	if ( tlArcW > 0 && hasLeft ) {
		Vector2f tlCenter( pos.x + tlArcW, pos.y + tlArcH );
		topFirstInner = arcInnerPos( tlCenter, tlArcW, tlArcH, 225, borderLeft, borderTop,
									 Vector2f( pos.x + borderLeft, pos.y + borderTop ) );
	} else {
		topFirstInner = tlInner;
	}

	// Right border first inner (top-right corner)
	Vector2f rightFirstInner;
	if ( trArcW > 0 && hasTop ) {
		Vector2f trCenter( pos.x + size.getWidth() - trArcW, pos.y + trArcH );
		rightFirstInner =
			arcInnerPos( trCenter, trArcW, trArcH, 315, borderRight, borderTop,
						 Vector2f( pos.x + size.getWidth() - borderRight, pos.y + borderTop ) );
	} else {
		rightFirstInner = trInner;
	}

	// Bottom border first inner (bottom-right corner)
	Vector2f bottomFirstInner;
	if ( brArcW > 0 && hasRight ) {
		Vector2f brCenter( pos.x + size.getWidth() - brArcW, pos.y + size.getHeight() - brArcH );
		bottomFirstInner = arcInnerPos( brCenter, brArcW, brArcH, 45, borderRight, borderBottom,
										Vector2f( pos.x + size.getWidth() - borderRight,
												  pos.y + size.getHeight() - borderBottom ) );
	} else {
		bottomFirstInner = brInner;
	}

	// Left border first inner (bottom-left corner)
	Vector2f leftFirstInner;
	if ( blArcW > 0 && hasBottom ) {
		Vector2f blCenter( pos.x + blArcW, pos.y + size.getHeight() - blArcH );
		leftFirstInner =
			arcInnerPos( blCenter, blArcW, blArcH, 135, borderLeft, borderBottom,
						 Vector2f( pos.x + borderLeft, pos.y + size.getHeight() - borderBottom ) );
	} else {
		leftFirstInner = blInner;
	}

	// Helper: insert degenerate triangle bridge between two disconnected border sections
	auto addBridge = [&]( const Vector2f& fromOuter, const Vector2f& toInner,
						  const Color& bridgeColor ) {
		vbo->addVertex( fromOuter );
		vbo->addColor( bridgeColor );
		vbo->addVertex( toInner );
		vbo->addColor( bridgeColor );
		vbo->addVertex( toInner );
		vbo->addColor( bridgeColor );
	};

	Vector2f lastOuter; // last emitted outer vertex, used as bridge source

	// --- draw top border ---
	if ( hasTop ) {
		double leftW = tlArcW;
		double rightW = trArcW;
		double leftH = tlArcH;
		double rightH = trArcH;

		if ( leftW && hasLeft ) {
			double endAngle = 270;
			double startAngle = 225;

			borderAddArc( vbo, Vector2f( pos.x + leftW, pos.y + leftH ), leftW, leftH, startAngle,
						  endAngle, borders.top.color, borderLeft, borderTop,
						  Vector2f( pos.x + borderLeft, pos.y + borderTop ) );
		} else {
			vbo->addVertex( Vector2f( pos.x + borderLeft, pos.y + borderTop ) );
			vbo->addColor( borders.top.color );
			vbo->addVertex( Vector2f( pos.x, pos.y ) );
			vbo->addColor( borders.top.color );
		}

		if ( rightW && hasRight ) {
			double startAngle = 270;
			double endAngle = 315;
			Vector2f basePos( pos.x + size.getWidth() - borderRight, pos.y + borderTop );
			Vector2f tPos( pos.x + size.getWidth() - rightW, pos.y + rightH );

			if ( rightW > borderRight || rightH > borderTop ) {
				vbo->addVertex( Vector2f(
					tPos.x + eemax( 0., rightW - borderRight ) * Math::cosAng( startAngle ),
					tPos.y + ( rightH - borderTop ) * Math::sinAng( startAngle ) ) );
			} else {
				vbo->addVertex( basePos );
			}

			vbo->addColor( borders.top.color );

			vbo->addVertex( Vector2f( pos.x + size.getWidth() - rightW, pos.y ) );
			vbo->addColor( borders.top.color );

			borderAddArc( vbo, tPos, rightW, rightH, startAngle, endAngle, borders.top.color,
						  borderRight, borderTop, basePos );

			lastOuter = arcOuterPos( tPos, rightW, rightH, endAngle );
		} else {
			vbo->addVertex( Vector2f( pos.x + size.getWidth() - borderRight, pos.y + borderTop ) );
			vbo->addColor( borders.top.color );
			vbo->addVertex( Vector2f( pos.x + size.getWidth(), pos.y ) );
			vbo->addColor( borders.top.color );

			lastOuter = trOuter;
		}

		if ( !hasRight ) {
			if ( hasBottom )
				addBridge( lastOuter, bottomFirstInner, borders.top.color );
			else if ( hasLeft )
				addBridge( lastOuter, leftFirstInner, borders.top.color );
		}
	}

	// --- draw right border ---
	if ( hasRight ) {
		double topW = trArcW;
		double bottomW = brArcW;
		double topH = trArcH;
		double bottomH = brArcH;

		if ( topW && hasTop ) {
			double startAngle = 315;
			double endAngle = 360;
			Vector2f basePos( pos.x + size.getWidth() - borderRight, pos.y + borderTop );

			borderAddArc( vbo, Vector2f( pos.x + size.getWidth() - topW, pos.y + topH ), topW, topH,
						  startAngle, endAngle, borders.right.color, borderRight, borderTop,
						  basePos );
		} else {
			vbo->addVertex( Vector2f( pos.x + size.getWidth() - borderRight, pos.y + borderTop ) );
			vbo->addColor( borders.right.color );
			vbo->addVertex( Vector2f( pos.x + size.getWidth(), pos.y ) );
			vbo->addColor( borders.right.color );
		}

		if ( bottomH && hasBottom ) {
			double startAngle = 0;
			double endAngle = 45;
			Vector2f basePos( pos.x + size.getWidth() - borderRight,
							  pos.y + size.getHeight() - borderBottom );
			Vector2f tPos( pos.x + size.getWidth() - bottomW, pos.y + size.getHeight() - bottomH );

			if ( bottomW > borderRight || bottomH > borderBottom ) {
				vbo->addVertex( Vector2f(
					tPos.x + ( bottomW - borderRight ) * Math::cosAng( startAngle ),
					tPos.y + eemax( 0., bottomH - borderBottom ) * Math::sinAng( startAngle ) ) );
			} else {
				vbo->addVertex( basePos );
			}
			vbo->addColor( borders.right.color );

			vbo->addVertex(
				Vector2f( pos.x + size.getWidth(), pos.y + size.getHeight() - bottomH ) );
			vbo->addColor( borders.right.color );

			borderAddArc( vbo, tPos, bottomW, bottomH, startAngle, endAngle, borders.right.color,
						  borderRight, borderBottom, basePos );

			lastOuter = arcOuterPos( tPos, bottomW, bottomH, endAngle );
		} else {
			vbo->addVertex( Vector2f( pos.x + size.getWidth() - borderRight,
									  pos.y + size.getHeight() - borderBottom ) );
			vbo->addColor( borders.right.color );
			vbo->addVertex( Vector2f( pos.x + size.getWidth(), pos.y + size.getHeight() ) );
			vbo->addColor( borders.right.color );

			lastOuter = brOuter;
		}

		if ( !hasBottom && hasLeft )
			addBridge( lastOuter, leftFirstInner, borders.right.color );
	}

	// --- draw bottom border ---
	if ( hasBottom ) {
		double leftW = blArcW;
		double rightW = brArcW;
		double leftH = blArcH;
		double rightH = brArcH;

		if ( rightW && hasRight ) {
			double startAngle = 45;
			double endAngle = 90;
			Vector2f basePos( pos.x + size.getWidth() - borderRight,
							  pos.y + size.getHeight() - borderBottom );

			borderAddArc(
				vbo,
				Vector2f( pos.x + size.getWidth() - rightW, pos.y + size.getHeight() - rightH ),
				rightW, rightH, startAngle, endAngle, borders.bottom.color, borderRight,
				borderBottom, basePos );
		} else {
			vbo->addVertex( Vector2f( pos.x + size.getWidth() - borderRight,
									  pos.y + size.getHeight() - borderBottom ) );
			vbo->addColor( borders.bottom.color );
			vbo->addVertex( Vector2f( pos.x + size.getWidth(), pos.y + size.getHeight() ) );
			vbo->addColor( borders.bottom.color );
		}

		if ( leftW && hasLeft ) {
			double startAngle = 90;
			double endAngle = 135;
			Vector2f basePos( pos.x + borderLeft, pos.y + size.getHeight() - borderBottom );
			Vector2f tPos( Vector2f( pos.x + leftW, pos.y + size.getHeight() - leftH ) );

			if ( leftW > borderLeft || leftH > borderBottom ) {
				vbo->addVertex(
					Vector2f( tPos.x + eemax( 0., leftW - borderLeft ) * Math::cosAng( startAngle ),
							  tPos.y + ( leftH - borderBottom ) * Math::sinAng( startAngle ) ) );
			} else {
				vbo->addVertex( basePos );
			}

			vbo->addColor( borders.bottom.color );

			vbo->addVertex( Vector2f( tPos.x + leftW * Math::cosAng( startAngle ),
									  tPos.y + leftH * Math::sinAng( startAngle ) ) );
			vbo->addColor( borders.bottom.color );

			borderAddArc( vbo, tPos, leftW, leftH, startAngle, endAngle, borders.bottom.color,
						  borderLeft, borderBottom, basePos );

			lastOuter = arcOuterPos( tPos, leftW, leftH, endAngle );
		} else {
			vbo->addVertex(
				Vector2f( pos.x + borderLeft, pos.y + size.getHeight() - borderBottom ) );
			vbo->addColor( borders.bottom.color );
			vbo->addVertex( Vector2f( pos.x, pos.y + size.getHeight() ) );
			vbo->addColor( borders.bottom.color );

			lastOuter = blOuter;
		}

		// After bottom, only left remains (already checked or skipped).
		// Bottom and left are adjacent, no bridge needed.
	}

	// --- draw left border ---
	if ( hasLeft ) {
		double topW = tlArcW;
		double bottomW = blArcW;
		double topH = tlArcH;
		double bottomH = blArcH;

		if ( bottomW && hasBottom ) {
			double startAngle = 135;
			double endAngle = 180;
			Vector2f basePos( pos.x + borderLeft, pos.y + size.getHeight() - borderBottom );

			borderAddArc( vbo, Vector2f( pos.x + bottomW, pos.y + size.getHeight() - bottomH ),
						  bottomW, bottomH, startAngle, endAngle, borders.left.color, borderLeft,
						  borderBottom, basePos );
		} else {
			vbo->addVertex(
				Vector2f( pos.x + borderLeft, pos.y + size.getHeight() - borderBottom ) );
			vbo->addColor( borders.left.color );
			vbo->addVertex( Vector2f( pos.x, pos.y + size.getHeight() ) );
			vbo->addColor( borders.left.color );
		}

		if ( topW && hasTop ) {
			double startAngle = 180;
			double endAngle = 225;
			Vector2f basePos( pos.x + borderLeft, pos.y + borderTop );
			Vector2f tPos( pos.x + topW, pos.y + topH );

			if ( topW > borderLeft || topH > borderTop ) {
				vbo->addVertex( Vector2f(
					tPos.x + ( topW - borderLeft ) * Math::cosAng( startAngle ),
					tPos.y + eemax( 0., topH - borderTop ) * Math::sinAng( startAngle ) ) );
			} else {
				vbo->addVertex( basePos );
			}

			vbo->addColor( borders.left.color );

			vbo->addVertex( Vector2f( tPos.x + topW * Math::cosAng( startAngle ),
									  tPos.y + topH * Math::sinAng( startAngle ) ) );

			vbo->addColor( borders.left.color );

			borderAddArc( vbo, tPos, topW, topH, startAngle, endAngle, borders.left.color,
						  borderLeft, borderTop, basePos );
		} else {
			vbo->addVertex( Vector2f( pos.x + borderLeft, pos.y + borderTop ) );
			vbo->addColor( borders.left.color );
			vbo->addVertex( Vector2f( pos.x, pos.y ) );
			vbo->addColor( borders.left.color );
		}
	}
}

void Borders::createBackground( VertexBuffer* vbo, const BorderRadiuses& radius,
								const Vector2f& pos, const Sizef& size, const Color& color ) {
	vbo->clear();

	const BorderRadiuses usedRadius = normalizeRadiuses( radius, size );

	double leftH = usedRadius.topLeft.y;
	if ( usedRadius.topLeft.x > 0 ) {
		double leftW = usedRadius.topLeft.x;
		double startAngle = 180;
		double endAngle = 270;
		borderAddArc( vbo, Vector2f( pos.x + leftW, pos.y + leftH ), leftW, leftH, startAngle,
					  endAngle, color, 0, 0, Vector2f::Zero, false, false );
	} else {
		vbo->addVertex( Vector2f( pos.x, pos.y ) );
		vbo->addColor( color );
	}

	double rightW = usedRadius.topRight.x;
	vbo->addVertex( Vector2f( pos.x + size.getWidth() - rightW, pos.y ) );
	vbo->addColor( color );

	if ( usedRadius.topRight.x > 0 ) {
		double rightH = usedRadius.topRight.y;
		double startAngle = 270;
		double endAngle = 360;
		borderAddArc( vbo, Vector2f( pos.x + size.getWidth() - rightW, pos.y + rightH ), rightW,
					  rightH, startAngle, endAngle, color, 0, 0, Vector2f::Zero, false, false );
	}

	double bottomH = usedRadius.bottomRight.y;
	vbo->addVertex( Vector2f( pos.x + size.getWidth(), pos.y + size.getHeight() - bottomH ) );
	vbo->addColor( color );

	if ( usedRadius.bottomRight.x > 0 ) {
		double bottomW = usedRadius.bottomRight.x;
		double startAngle = 0;
		double endAngle = 90;
		borderAddArc(
			vbo, Vector2f( pos.x + size.getWidth() - bottomW, pos.y + size.getHeight() - bottomH ),
			bottomW, bottomH, startAngle, endAngle, color, 0, 0, Vector2f::Zero, false, false );
	}

	double bottomW = usedRadius.bottomLeft.x;
	vbo->addVertex( Vector2f( pos.x + bottomW, pos.y + size.getHeight() ) );
	vbo->addColor( color );

	if ( usedRadius.bottomLeft.x > 0 ) {
		double bottomH = usedRadius.bottomLeft.y;
		double startAngle = 90;
		double endAngle = 180;
		borderAddArc( vbo, Vector2f( pos.x + bottomW, pos.y + size.getHeight() - bottomH ), bottomW,
					  bottomH, startAngle, endAngle, color, 0, 0, Vector2f::Zero, false, false );
	}

	vbo->addVertex( Vector2f( pos.x, pos.y + leftH ) );
	vbo->addColor( color );
}

}} // namespace EE::UI
