#ifndef EE_MATHSIZE_HPP
#define EE_MATHSIZE_HPP

#include <eepp/math/vector2.hpp>

namespace EE { namespace Math {

/** @brief A template class to manipulate sizes */
template <typename T> class tSize : public Vector2<T> {
  public:
	/** Default constructor, creates a tSize(0,0) */
	tSize();

	/** Creates a tSize of the width and height */
	tSize( const T& width, const T& height );

	/** Creates a copy of a size */
	tSize( const tSize<T>& Size );

	/** Creates a size from a Vector2 */
	tSize( const Vector2<T>& Vec );

	/** @return The size width */
	const T& getWidth() const;

	/** @return The size height */
	const T& getHeight() const;

	/** Set a new width */
	void setWidth( const T& width );

	/** Set a new height */
	void setHeight( const T& height );

	tSize<T>& operator=( const tSize<T>& right );
};

template <typename T> tSize<T>::tSize() : Vector2<T>( 0, 0 ) {}

template <typename T>
tSize<T>::tSize( const T& width, const T& height ) : Vector2<T>( width, height ) {}

template <typename T>
tSize<T>::tSize( const tSize<T>& size ) : Vector2<T>( size.getWidth(), size.getHeight() ) {}

template <typename T> tSize<T>::tSize( const Vector2<T>& vec ) : Vector2<T>( vec.x, vec.y ) {}

template <typename T> const T& tSize<T>::getWidth() const {
	return this->x;
}

template <typename T> const T& tSize<T>::getHeight() const {
	return this->y;
}

template <typename T> void tSize<T>::setWidth( const T& width ) {
	this->x = width;
}

template <typename T> void tSize<T>::setHeight( const T& height ) {
	this->y = height;
}

template <typename T> tSize<T>& tSize<T>::operator=( const tSize<T>& right ) {
	this->x = right.x;
	this->y = right.y;
	return *this;
}

typedef tSize<int> Sizei;
typedef tSize<unsigned int> Sizeu;
typedef tSize<Float> Sizef;

}} // namespace EE::Math

#endif
