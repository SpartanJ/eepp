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
		
		void Color(ColorAf Color, Float AlphaDecay);
		ColorAf Color() const { return mColor; }
		
		Float R() { return mColor.r(); }
		Float G() { return mColor.g(); }
		Float B() { return mColor.b(); }
		Float A() { return mColor.a(); }
		
		void Reset(const Float &x, const Float &y, const Float &xspeed, const Float &yspeed, const Float &xacc, const Float &yacc, const Float size = 16);
		void Update(const Float &pTime);
		
		void X(const Float x) { mX = x; }
		Float X() const { return mX; }
		
		void Y(const Float y) { mY = y; }
		Float Y() const { return mY; }
		
		void Speed(const Float xspeed) { mXSpeed = xspeed; }
		Float XSpeed() const { return mXSpeed; }
		
		void YSpeed(const Float yspeed) { mYSpeed = yspeed; }
		Float YSpeed() const { return mYSpeed; }
		
		void XAcc(const Float xacc) { mXAcc = xacc; }
		Float XAcc() const { return mXAcc; }
		
		void YAcc(const Float yacc) { mYAcc = yacc; }
		Float YAcc() const { return mYAcc; }
		
		void AlphaDecay(const Float alphadecay) { mAlphaDecay = alphadecay; }
		Float AlphaDecay() const { return mAlphaDecay; }
		
		void Size(const Float size) { if (size>0) mSize = size; }
		Float Size() const { return mSize; }
		
		void Used(const bool used) { mUsed = used; }
		bool Used() const { return mUsed; }
		
		void Id(const Uint32 Id) { mId = Id; }
		Uint32 Id() const { return mId; }
	private:
		Float mX, mY;
		ColorAf mColor;
		
		Float mXSpeed, mYSpeed, mXAcc, mYAcc, mAlphaDecay, mSize;
		bool mUsed;
		Uint32 mId;
};

}}
#endif
