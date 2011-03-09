#include "cparticle.hpp"

namespace EE { namespace Graphics {

cParticle::cParticle() {
	Reset(0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
}

cParticle::~cParticle() {}

void cParticle::Color(eeColorAf Color, eeFloat AlphaDecay) {
	mColor = Color;
	mAlphaDecay = AlphaDecay;
}

void cParticle::Reset(const eeFloat &x, const eeFloat &y, const eeFloat &xspeed, const eeFloat &yspeed, const eeFloat &xacc, const eeFloat &yacc, const eeFloat size) {
	mX = x;
	mY = y;
	mXSpeed = xspeed;
	mYSpeed = yspeed;
	mXAcc = xacc;
	mYAcc = yacc;
	mSize = size;
	mColor = eeColorAf(1.0f,1.0f,1.0f,1.0f);
	mAlphaDecay = 0.01f;
}

void cParticle::Update(const eeFloat &pTime) {
	mX = mX + mXSpeed * pTime;
    mY = mY + mYSpeed * pTime;
    mXSpeed = mXSpeed + mXAcc * pTime;
    mYSpeed = mYSpeed + mYAcc * pTime;
    mColor.Alpha -= mAlphaDecay * pTime;
    if (mColor.Alpha < 0) mColor.Alpha = 0;
}

}}
