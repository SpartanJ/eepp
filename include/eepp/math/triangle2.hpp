#ifndef EE_MATHTRIANGLE2_HPP
#define EE_MATHTRIANGLE2_HPP

#include <eepp/math/vector2.hpp>

namespace EE { namespace Math {

/** @brief Utility template class for manipulating triangles. */
template <typename T>
class Triangle2 {
	public:
		/** Default constructor ( creates 3 empty Vector3(0,0,0) */
		Triangle2();

		/** Create a triangle from 3 vectors */
		Triangle2( const Vector2<T>& v1, const Vector2<T>& v2, const Vector2<T>& v3 );

		Vector2<T> V[3];

		/** @return The vector index ( between 0 and 2 ) */
		Vector2<T>& GetAt( Uint32 Index ) { return V[Index]; }
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

typedef Triangle2<Float> eeTriangle2f;

}}

#endif
