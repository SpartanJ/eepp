#ifndef EE_GRAPHICSTORIGINPOINT_HPP 
#define EE_GRAPHICSTORIGINPOINT_HPP

#include <eepp/config.hpp>
#include <eepp/math/vector2.hpp>
#include <eepp/core/string.hpp>

namespace EE { namespace Math {

/** @brief Helper class to define the origin point of a translation/rotation/scaling */
template <typename T>
class tOriginPoint : public Vector2<T> {
	public:
		enum OriginTypes
		{
			OriginCenter,
			OriginTopLeft,
			OriginCustom
		};
		
		OriginTypes OriginType;

		/** By default creates a origin point centered */
		tOriginPoint() :
			Vector2<T>(),
			OriginType( OriginCenter )
		{
		}

		/** Creates a custom origin point */
		tOriginPoint(T X, T Y) :
			Vector2<T>( X, Y ),
			OriginType( OriginCustom )
		{
		}

		/** Creates a origin point type */
		tOriginPoint( OriginTypes type ) :
			Vector2<T>( (T)0, (T)0 ),
			OriginType( type )
		{
		}

		std::string toString() const;

		tOriginPoint<T>& operator=(const Vector2<T>& v);
};

template<typename T>
tOriginPoint<T>& tOriginPoint<T>::operator=(const Vector2<T>& v) {
	this->x = v.x;
	this->y = v.y;
	return *this;
}

template<typename T>
std::string tOriginPoint<T>::toString() const {
	if ( OriginType == OriginCenter ) return "center";
	else if ( OriginType == OriginTopLeft ) return "topleft";
	return String::toStr(this->x) + "," + String::toStr(this->y);
}

typedef tOriginPoint<Float>		OriginPoint;
typedef tOriginPoint<int>		OriginPointi;

}}

#endif
