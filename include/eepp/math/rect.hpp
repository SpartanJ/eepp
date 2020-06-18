#ifndef EE_MATHCRECT_H
#define EE_MATHCRECT_H

#include <eepp/math/size.hpp>
#include <eepp/math/vector2.hpp>

namespace EE { namespace Math {

template <typename T> class tRECT {
  public:
	T Left, Right, Top, Bottom;

	tRECT( T left, T top, T right, T bottom );

	tRECT( const Vector2<T>& pos, const tSize<T>& size );

	tRECT();

	tRECT<T> copy() const;

	bool intersect( const tRECT<T>& rect );

	bool contains( const tRECT<T>& rect );

	bool contains( const Vector2<T>& Vect );

	void expand( const tRECT<T>& rect );

	void shrink( const tRECT<T>& rect );

	void expand( const Vector2<T>& Vect );

	T area() const;

	T mergedArea( const tRECT<T>& rect ) const;

	bool intersectsSegment( const Vector2<T>& a, const Vector2<T>& b ) const;

	/** Determine if a RECT and a Circle are intersecting
	 * @param pos Circle position
	 * @param radius Circle Radius
	 * @return True if are intersecting
	 */
	bool intersectCircle( Vector2<T> pos, const T& radius ) const;

	/** Determine if a RECT ( representing a circle ) is intersecting another RECT ( also
	 * representing a circle ) */
	bool intersectCircles( const tRECT<T>& b ) const;

	Vector2<T> clampVector( const Vector2<T>& Vect ) const;

	Vector2<T> wrapVector( const Vector2<T>& Vect ) const;

	Vector2<T> getPosition() const;

	Vector2<T> getCenter() const;

	tSize<T> getSize() const;

	T getWidth() const;

	T getHeight() const;

	void scale( T scale, const Vector2<T>& center );

	void scale( T scale );

	void scale( Vector2<T> scale, const Vector2<T>& center );

	void scale( Vector2<T> scale );

	tRECT<Float> asFloat() const;

	tRECT<int> asInt() const;
};

template <typename T> tRECT<Float> tRECT<T>::asFloat() const {
	return tRECT<Float>( Left, Top, Right, Bottom );
}

template <typename T> tRECT<int> tRECT<T>::asInt() const {
	return tRECT<int>( Left, Top, Right, Bottom );
}

template <typename T> bool operator==( const tRECT<T>& R1, const tRECT<T>& R2 ) {
	return ( R1.Left == R2.Left ) && ( R1.Right == R2.Right ) && ( R1.Top == R2.Top ) &&
		   ( R1.Bottom == R2.Bottom );
}

template <typename T> bool operator!=( const tRECT<T>& R1, const tRECT<T>& R2 ) {
	return ( R1.Left != R2.Left ) || ( R1.Right != R2.Right ) || ( R1.Top != R2.Top ) ||
		   ( R1.Bottom != R2.Bottom );
}

template <typename T> tRECT<T> operator+( const tRECT<T>& R, T X ) {
	return tRECT<T>( R.Left + X, R.Top + X, R.Right + X, R.Bottom + X );
}

template <typename T> tRECT<T>& operator+=( tRECT<T>& R, T X ) {
	R.Left += X;
	R.Top += X;
	R.Right += X;
	R.Bottom += X;
	return R;
}

template <typename T> tRECT<T> operator-( const tRECT<T>& R, T X ) {
	return tRECT<T>( R.Left - X, R.Top - X, R.Right - X, R.Bottom - X );
}

template <typename T> tRECT<T>& operator-=( tRECT<T>& R, T X ) {
	R.Left -= X;
	R.Top -= X;
	R.Right -= X;
	R.Bottom -= X;
	return R;
}

template <typename T> tRECT<T> operator+( const tRECT<T>& R, tRECT<T> X ) {
	return tRECT<T>( R.Left + X.Left, R.Top + X.Top, R.Right + X.Right, R.Bottom + X.Bottom );
}

template <typename T> tRECT<T>& operator+=( tRECT<T>& R, tRECT<T> X ) {
	R.Left += X.Left;
	R.Top += X.Top;
	R.Right += X.Right;
	R.Bottom += X.Bottom;
	return R;
}

template <typename T> tRECT<T> operator-( const tRECT<T>& R, tRECT<T> X ) {
	return tRECT<T>( R.Left - X.Left, R.Top - X.Top, R.Right - X.Right, R.Bottom - X.Bottom );
}

template <typename T> tRECT<T>& operator-=( tRECT<T>& R, tRECT<T> X ) {
	R.Left -= X.Left;
	R.Top -= X.Top;
	R.Right -= X.Right;
	R.Bottom -= X.Bottom;
	return R;
}

template <typename T, typename Y> tRECT<T> operator*( const tRECT<T>& R, Y X ) {
	return tRECT<T>( ( T )( (Y)R.Left * X ), ( T )( (Y)R.Top * X ), ( T )( (Y)R.Right * X ),
					 ( T )( (Y)R.Bottom * X ) );
}

template <typename T, typename Y> tRECT<T>& operator*=( tRECT<T>& R, Y X ) {
	R.Left = ( T )( (Y)R.Left * X );
	R.Top = ( T )( (Y)R.Top * X );
	R.Right = ( T )( (Y)R.Right * X );
	R.Bottom = ( T )( (Y)R.Bottom * X );
	return R;
}

template <typename T, typename Y> tRECT<T>& operator*( tRECT<T>& R, Y X ) {
	R.Left = ( T )( (Y)R.Left * X );
	R.Top = ( T )( (Y)R.Top * X );
	R.Right = ( T )( (Y)R.Right * X );
	R.Bottom = ( T )( (Y)R.Bottom * X );
	return R;
}

template <typename T, typename Y> tRECT<T> operator/( const tRECT<T>& R, Y X ) {
	return tRECT<T>( ( T )( (Y)R.Left / X ), ( T )( (Y)R.Top / X ), ( T )( (Y)R.Right / X ),
					 ( T )( (Y)R.Bottom / X ) );
}

template <typename T, typename Y> tRECT<T>& operator/=( tRECT<T>& R, Y X ) {
	R.Left = ( T )( (Y)R.Left / X );
	R.Top = ( T )( (Y)R.Top / X );
	R.Right = ( T )( (Y)R.Right / X );
	R.Bottom = ( T )( (Y)R.Bottom / X );
	return R;
}

template <typename T, typename Y> tRECT<T>& operator/( tRECT<T>& R, Y X ) {
	R.Left = ( T )( (Y)R.Left / X );
	R.Top = ( T )( (Y)R.Top / X );
	R.Right = ( T )( (Y)R.Right / X );
	R.Bottom = ( T )( (Y)R.Bottom / X );
	return R;
}

template <typename T> Vector2<T> operator-( Vector2<T> X, const tRECT<T>& R ) {
	return Vector2<T>( X.x - R.Left - R.Right, X.y - R.Top - R.Bottom );
}

template <typename T> Vector2<T> operator+( Vector2<T> X, const tRECT<T>& R ) {
	return Vector2<T>( X.x + R.Left + R.Right, X.y + R.Top + R.Bottom );
}

template <typename T>
tRECT<T>::tRECT( T left, T top, T right, T bottom ) :
	Left( left ), Right( right ), Top( top ), Bottom( bottom ) {}

template <typename T> tRECT<T> tRECT<T>::copy() const {
	return tRECT<T>( Left, Top, Right, Bottom );
}

template <typename T>
tRECT<T>::tRECT( const Vector2<T>& Pos, const tSize<T>& Size ) :
	Left( Pos.x ),
	Right( Pos.x + Size.getWidth() ),
	Top( Pos.y ),
	Bottom( Pos.y + Size.getHeight() ) {}

template <typename T> tRECT<T>::tRECT() : Left( 0 ), Right( 0 ), Top( 0 ), Bottom( 0 ) {}

template <typename T> bool tRECT<T>::contains( const tRECT<T>& rect ) {
	return ( Left <= rect.Left && Right >= rect.Right && Top <= rect.Top && Bottom >= rect.Bottom );
}

template <typename T> bool tRECT<T>::intersect( const tRECT<T>& rect ) {
	return !( Left > rect.Right || Right < rect.Left || Top > rect.Bottom || Bottom < rect.Top );
}

template <typename T> bool tRECT<T>::contains( const Vector2<T>& Vect ) {
	return ( Left <= Vect.x && Right >= Vect.x && Top <= Vect.y && Bottom >= Vect.y );
}

template <typename T> Vector2<T> tRECT<T>::getPosition() const {
	return Vector2<T>( Left, Top );
}

template <typename T> Vector2<T> tRECT<T>::getCenter() const {
	return Vector2<T>( Left + ( ( Right - Left ) * 0.5 ), Top + ( ( Bottom - Top ) * 0.5 ) );
}

template <typename T> tSize<T> tRECT<T>::getSize() const {
	return tSize<T>( eeabs( Right - Left ), eeabs( Bottom - Top ) );
}

template <typename T> T tRECT<T>::getWidth() const {
	return eeabs( Right - Left );
}

template <typename T> T tRECT<T>::getHeight() const {
	return eeabs( Bottom - Top );
}

template <typename T> void tRECT<T>::expand( const tRECT<T>& rect ) {
	Left = eemin( Left, rect.Left );
	Bottom = eemax( Bottom, rect.Bottom );
	Right = eemax( Right, rect.Right );
	Top = eemin( Top, rect.Top );
}

template <typename T> void tRECT<T>::shrink( const tRECT<T>& rect ) {
	Left = eemax( Left, rect.Left );
	Top = eemax( Top, rect.Top );
	Right = eemax( Left, eemin( Right, rect.Right ) );
	Bottom = eemax( Top, eemin( Bottom, rect.Bottom ) );
}

template <typename T> void tRECT<T>::expand( const Vector2<T>& Vect ) {
	Left = eemin( Left, Vect.x );
	Bottom = eemax( Bottom, Vect.y );
	Right = eemax( Right, Vect.x );
	Top = eemin( Top, Vect.y );
}

template <typename T> T tRECT<T>::area() const {
	return ( Right - Left ) * ( Bottom - Top );
}

template <typename T> T tRECT<T>::mergedArea( const tRECT<T>& rect ) const {
	return ( eemax( Right, rect.Right ) - eemin( Left, rect.Left ) ) *
		   ( eemin( Bottom, rect.Bottom ) - eemax( Top, rect.Top ) );
}

template <typename T>
bool tRECT<T>::intersectsSegment( const Vector2<T>& a, const Vector2<T>& b ) const {
	tRECT<T> seg_bb =
		tRECT<T>( eemin( a.x, b.x ), eemin( a.y, b.y ), eemax( a.x, b.x ), eemax( a.y, b.y ) );

	if ( Intersects( seg_bb ) ) {
		Vector2<T> axis( b.y - a.y, a.x - b.x );
		Vector2<T> offset( ( a.x + b.x - Right - Left ), ( a.y + b.y - Bottom - Top ) );
		Vector2<T> extents( Right - Left, Bottom - Top );

		return ( eeabs( axis.dot( offset ) ) <
				 eeabs( axis.x * extents.x ) + eeabs( axis.y * extents.y ) );
	}

	return false;
}

template <typename T> bool tRECT<T>::intersectCircle( Vector2<T> pos, const T& radius ) const {
	Vector2<T> tPos( pos );

	if ( tPos.x < Left )
		tPos.x = Left;
	if ( tPos.x > Right )
		tPos.x = Right;
	if ( tPos.y < Top )
		tPos.y = Top;
	if ( tPos.y > Bottom )
		tPos.y = Bottom;

	if ( pos.distance( tPos ) < radius )
		return true;

	return false;
}

template <typename T> bool tRECT<T>::intersectCircles( const tRECT<T>& b ) const {
	Float ra = ( Float )( Right - Left ) * 0.5f;
	Float rb = ( Float )( b.Right - b.Left ) * 0.5f;
	Float dist = ra + rb;
	Float dx = ( b.Left + rb ) - ( Left + ra );
	Float dy = ( b.Top + rb ) - ( Top + ra );
	Float res = ( dx * dx ) + ( dy * dy );

	if ( res <= ( dist * dist ) )
		return true;
	return false;
}

template <typename T> Vector2<T> tRECT<T>::clampVector( const Vector2<T>& Vect ) const {
	T x = eemin( eemax( Left, Vect.x ), Right );
	T y = eemin( eemax( Top, Vect.y ), Bottom );

	return Vector2<T>( x, y );
}

template <typename T> Vector2<T> tRECT<T>::wrapVector( const Vector2<T>& Vect ) const {
	T ix = eeabs( Right - Left );
	T modx = eemod( Vect.x - Left, ix );
	T x = ( modx > 0 ) ? modx : modx + ix;

	T iy = eeabs( Top - Top );
	T mody = eemod( Vect.y - Top, iy );
	T y = ( mody > 0 ) ? mody : mody + iy;

	return Vector2<T>( x + Left, y + Top );
}

template <typename T> void tRECT<T>::scale( Vector2<T> scale, const Vector2<T>& center ) {
	if ( scale != 1.0f ) {
		Left = center.x + ( Left - center.x ) * scale.x;
		Top = center.y + ( Top - center.y ) * scale.y;
		Right = center.x + ( Right - center.x ) * scale.x;
		Bottom = center.y + ( Bottom - center.y ) * scale.y;
	}
}

template <typename T> void tRECT<T>::scale( T scale, const Vector2<T>& center ) {
	scale( Vector2f( scale, scale ), center );
}

template <typename T> void tRECT<T>::scale( T scale ) {
	scale( scale, getCenter() );
}

template <typename T> void tRECT<T>::scale( Vector2<T> scale ) {
	scale( scale, getCenter() );
}

typedef tRECT<unsigned int> Rectu;
typedef tRECT<Float> Rectf;
typedef tRECT<int> Rect;

}} // namespace EE::Math

#endif
