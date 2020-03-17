#ifndef EE_GRAPHICSTORIGINPOINT_HPP
#define EE_GRAPHICSTORIGINPOINT_HPP

#include <eepp/config.hpp>
#include <eepp/core/string.hpp>
#include <eepp/math/vector2.hpp>

namespace EE { namespace Math {

/** @brief Helper class to define the origin point of a translation/rotation/scaling */
template <typename T> class tOriginPoint : public Vector2<T> {
  public:
	enum OriginTypes { OriginCenter, OriginTopLeft, OriginCustom, OriginEquation };

	OriginTypes OriginType;

	/** By default creates a origin point centered */
	tOriginPoint() : Vector2<T>(), OriginType( OriginCenter ) {}

	/** Creates a custom origin point */
	tOriginPoint( T X, T Y ) : Vector2<T>( X, Y ), OriginType( OriginCustom ) {}

	/** Creates a origin point type */
	tOriginPoint( OriginTypes type ) : Vector2<T>( (T)0, (T)0 ), OriginType( type ) {}

	std::string toString() const;

	tOriginPoint<T>& operator=( const Vector2<T>& v );

	const std::string& getXEq() const;

	void setXEq( const std::string& xEq );

	const std::string& getYEq() const;

	void setYEq( const std::string& yEq );

  protected:
	std::string mXEq;
	std::string mYEq;
};

template <typename T> tOriginPoint<T>& tOriginPoint<T>::operator=( const Vector2<T>& v ) {
	this->x = v.x;
	this->y = v.y;
	return *this;
}

template <typename T>
const std::string& tOriginPoint<T>::getXEq() const {
	return mXEq;
}

template <typename T>
void tOriginPoint<T>::setXEq( const std::string& xEq ) {
	OriginType = OriginEquation;
	mXEq = xEq;
}

template <typename T>
const std::string& tOriginPoint<T>::getYEq() const {
	return mYEq;
}

template <typename T>
void tOriginPoint<T>::setYEq( const std::string& yEq ) {
	OriginType = OriginEquation;
	mYEq = yEq;
}

template <typename T> std::string tOriginPoint<T>::toString() const {
	if ( OriginType == OriginCenter )
		return "center";
	else if ( OriginType == OriginTopLeft )
		return "topleft";
	else if ( OriginType == OriginEquation )
		return mXEq + " " + mYEq;
	return String::toStr( this->x ) + " " + String::toStr( this->y );
}

typedef tOriginPoint<Float> OriginPoint;

}} // namespace EE::Math

#endif
