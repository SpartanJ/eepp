#ifndef CPARTICLE_H
#define CPARTICLE_H

#include <eepp/graphics/base.hpp>

namespace EE { namespace Graphics {

class EE_API cParticle{
	public:
		cParticle();
		~cParticle();
		
		void Color(eeColorAf Color, eeFloat AlphaDecay);
		eeColorAf Color() const { return mColor; }
		
		eeFloat R() { return mColor.R(); }
		eeFloat G() { return mColor.G(); }
		eeFloat B() { return mColor.B(); }
		eeFloat A() { return mColor.A(); }
		
		void Reset(const eeFloat &x, const eeFloat &y, const eeFloat &xspeed, const eeFloat &yspeed, const eeFloat &xacc, const eeFloat &yacc, const eeFloat size = 16);
		void Update(const eeFloat &pTime);
		
		void X(const eeFloat x) { mX = x; }
		eeFloat X() const { return mX; }
		
		void Y(const eeFloat y) { mY = y; }
		eeFloat Y() const { return mY; }
		
		void Speed(const eeFloat xspeed) { mXSpeed = xspeed; }
		eeFloat XSpeed() const { return mXSpeed; }
		
		void YSpeed(const eeFloat yspeed) { mYSpeed = yspeed; }
		eeFloat YSpeed() const { return mYSpeed; }
		
		void XAcc(const eeFloat xacc) { mXAcc = xacc; }
		eeFloat XAcc() const { return mXAcc; }
		
		void YAcc(const eeFloat yacc) { mYAcc = yacc; }
		eeFloat YAcc() const { return mYAcc; }
		
		void AlphaDecay(const eeFloat alphadecay) { mAlphaDecay = alphadecay; }
		eeFloat AlphaDecay() const { return mAlphaDecay; }
		
		void Size(const eeFloat size) { if (size>0) mSize = size; }
		eeFloat Size() const { return mSize; }
		
		void Used(const bool used) { cUsed = used; }
		bool Used() const { return cUsed; }
		
		void Id(const Uint32 Id) { cId = Id; }
		Uint32 Id() const { return cId; }
	private:
		eeFloat mX, mY;
		eeColorAf mColor;
		
		eeFloat mXSpeed, mYSpeed, mXAcc, mYAcc, mAlphaDecay, mSize;
		bool cUsed;
		Uint32 cId;
};

}}
#endif
