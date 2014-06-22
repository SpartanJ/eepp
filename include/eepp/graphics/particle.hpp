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
		
		Float R() { return mColor.R(); }
		Float G() { return mColor.G(); }
		Float B() { return mColor.B(); }
		Float A() { return mColor.A(); }
		
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
		
		void Used(const bool used) { cUsed = used; }
		bool Used() const { return cUsed; }
		
		void Id(const Uint32 Id) { cId = Id; }
		Uint32 Id() const { return cId; }
	private:
		Float mX, mY;
		ColorAf mColor;
		
		Float mXSpeed, mYSpeed, mXAcc, mYAcc, mAlphaDecay, mSize;
		bool cUsed;
		Uint32 cId;
};

}}
#endif
