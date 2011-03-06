#ifndef EE_UTILSSIZE_HPP
#define EE_UTILSSIZE_HPP

namespace EE { namespace Utils {

template<typename T>
class tSize : public Vector2<T>
{
	public:
		tSize();
		tSize( const T& Width, const T& Height );
		tSize( const tSize<T>& Size );
		tSize( const Vector2<T>& Vec );

		const T& Width() const;
		const T& Height() const;
		void Width( const T& width );
		void Height( const T& height );
};

template <typename T>
tSize<T>::tSize() :
	Vector2<T>( 0, 0 )
{
}

template <typename T>
tSize<T>::tSize( const T& Width, const T& Height ) :
	Vector2<T>( Width, Height )
{
}

template <typename T>
tSize<T>::tSize( const tSize<T>& Size ) :
	Vector2<T>( Size.Width(), Size.Height() )
{
}

template <typename T>
tSize<T>::tSize( const Vector2<T>& Vec ) :
	Vector2<T>( Vec.x, Vec.y )
{
}

template <typename T>
const T& tSize<T>::Width() const {
	return this->x;
}

template <typename T>
const T& tSize<T>::Height() const {
	return this->y;
}

template <typename T>
void tSize<T>::Width( const T& width ) {
	this->x = width;
}

template <typename T>
void tSize<T>::Height( const T& height ) {
	this->y = height;
}

typedef tSize<eeInt> eeSize;
typedef tSize<eeFloat> eeSizef;

}}

#endif
