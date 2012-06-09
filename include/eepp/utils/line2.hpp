#ifndef EE_UTILSLINE2_HPP
#define EE_UTILSLINE2_HPP

namespace EE { namespace Utils {

template <typename T>
class Line2 {
	public:
		Line2();
		Line2( const Vector2<T>& v1, const Vector2<T>& v2 );
		Vector2<T> V[2];

		Vector2<T> GetNormal();
};

template <typename T>
Line2<T>::Line2() {
	V[0] = Vector2<T>();
	V[1] = Vector2<T>();
}

template <typename T>
Line2<T>::Line2( const Vector2<T>& v1, const Vector2<T>& v2 ) {
	V[0] = v1;
	V[1] = v2;
}

template <typename T>
Vector2<T> Line2<T>::GetNormal() {
	Vector2<T> tV = Vector2<T>( -(V[1].y - V[0].y) , V[1].x - V[0].x );
	tV.Normalize();
	return tV;
}

typedef Line2<eeFloat> eeLine2f;

}}

#endif
