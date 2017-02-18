#ifndef CPARTICLE_H
#define CPARTICLE_H

#include <eepp/graphics/base.hpp>

namespace EE { namespace Graphics {

/** @brief A simple particle class used by the particle system.
**	Probably not useful for the end user. */
class EE_API Particle{
	public:
		Particle();
		~Particle();
		
		void setColor(ColorAf Color, Float alphaDecay);

		const ColorAf& getColor() const { return mColor; }
		
		Float r() { return mColor.r(); }
		Float g() { return mColor.g(); }
		Float b() { return mColor.b(); }
		Float a() { return mColor.a(); }
		
		void reset(const Float &x, const Float &y, const Float &xspeed, const Float &yspeed, const Float &xacc, const Float &yacc, const Float size = 16);

		void update(const Float &pTime);
		
		void setX(const Float& x) { mX = x; }

		const Float& getX() const { return mX; }
		
		void setY(const Float& y) { mY = y; }

		const Float& getY() const { return mY; }
		
		void setXSpeed(const Float xspeed) { mXSpeed = xspeed; }

		const Float& getXSpeed() const { return mXSpeed; }
		
		void setYSpeed(const Float& yspeed) { mYSpeed = yspeed; }

		const Float& getYSpeed() const { return mYSpeed; }
		
		void setXAcc(const Float& xacc) { mXAcc = xacc; }

		const Float& getXAcc() const { return mXAcc; }
		
		void setYAcc(const Float& yacc) { mYAcc = yacc; }

		const Float& getYAcc() const { return mYAcc; }
		
		void setAlphaDecay(const Float& alphadecay) { mAlphaDecay = alphadecay; }

		const Float& getAlphaDecay() const { return mAlphaDecay; }
		
		void setSize(const Float& size) { if (size>0) mSize = size; }

		const Float& getSize() const { return mSize; }
		
		void setUsed(const bool& used) { mUsed = used; }

		bool isUsed() const { return mUsed; }
		
		void setId(const Uint32& id) { mId = id; }

		const Uint32& getId() const { return mId; }
	private:
		Float mX, mY;
		ColorAf mColor;
		
		Float mXSpeed, mYSpeed, mXAcc, mYAcc, mAlphaDecay, mSize;
		bool mUsed;
		Uint32 mId;
};

}}
#endif
