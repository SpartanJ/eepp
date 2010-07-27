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
tSize<T>::tSize() {
	this->x = 0;
	this->y = 0;
}

template <typename T>
tSize<T>::tSize( const T& Width, const T& Height ) {
	this->x = Width;
	this->y = Height;
}

template <typename T>
tSize<T>::tSize( const tSize<T>& Size ) {
	this->x = Size.Width();
	this->y = Size.Height();
}

template <typename T>
tSize<T>::tSize( const Vector2<T>& Vec ) {
	this->x = Vec.x;
	this->y = Vec.y;
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

}}

#endif
