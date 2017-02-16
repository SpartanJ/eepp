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
		
		void Color(ColorAf Color, Float alphaDecay);
		ColorAf Color() const { return mColor; }
		
		Float r() { return mColor.r(); }
		Float g() { return mColor.g(); }
		Float b() { return mColor.b(); }
		Float a() { return mColor.a(); }
		
		void reset(const Float &x, const Float &y, const Float &xspeed, const Float &yspeed, const Float &xacc, const Float &yacc, const Float size = 16);
		void update(const Float &pTime);
		
		void X(const Float x) { mX = x; }
		Float X() const { return mX; }
		
		void Y(const Float y) { mY = y; }
		Float Y() const { return mY; }
		
		void xSpeed(const Float xspeed) { mXSpeed = xspeed; }
		Float xSpeed() const { return mXSpeed; }
		
		void ySpeed(const Float yspeed) { mYSpeed = yspeed; }
		Float ySpeed() const { return mYSpeed; }
		
		void xAcc(const Float xacc) { mXAcc = xacc; }
		Float xAcc() const { return mXAcc; }
		
		void yAcc(const Float yacc) { mYAcc = yacc; }
		Float yAcc() const { return mYAcc; }
		
		void alphaDecay(const Float alphadecay) { mAlphaDecay = alphadecay; }
		Float alphaDecay() const { return mAlphaDecay; }
		
		void size(const Float size) { if (size>0) mSize = size; }
		Float size() const { return mSize; }
		
		void used(const bool used) { mUsed = used; }
		bool used() const { return mUsed; }
		
		void id(const Uint32 id) { mId = id; }
		Uint32 id() const { return mId; }
	private:
		Float mX, mY;
		ColorAf mColor;
		
		Float mXSpeed, mYSpeed, mXAcc, mYAcc, mAlphaDecay, mSize;
		bool mUsed;
		Uint32 mId;
};

}}
#endif
