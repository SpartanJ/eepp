#ifndef EE_UTILSTRIANGLE2_HPP
#define EE_UTILSTRIANGLE2_HPP

namespace EE { namespace Utils {

template <typename T>
class Triangle2 {
	public:
		Triangle2();
		Triangle2( const Vector2<T>& v1, const Vector2<T>& v2, const Vector2<T>& v3 );
		Vector2<T> V[3];
};

template <typename T>
Triangle2<T>::Triangle2() {
	V[0] = Vector2<T>();
	V[1] = Vector2<T>();
	V[2] = Vector2<T>();
}

template <typename T>
Triangle2<T>::Triangle2( const Vector2<T>& v1, const Vector2<T>& v2, const Vector2<T>& v3 ) {
	V[0] = v1;
	V[1] = v2;
	V[2] = v3;
}

typedef Triangle2<eeFloat> eeTriangle2f;

}}

#endif