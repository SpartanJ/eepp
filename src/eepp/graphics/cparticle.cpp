#include <eepp/graphics/cparticle.hpp>

namespace EE { namespace Graphics {

cParticle::cParticle() {
	Reset(0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
}

cParticle::~cParticle() {}

void cParticle::Color(eeColorAf Color, Float AlphaDecay) {
	mColor = Color;
	mAlphaDecay = AlphaDecay;
}

void cParticle::Reset(const Float &x, const Float &y, const Float &xspeed, const Float &yspeed, const Float &xacc, const Float &yacc, const Float size) {
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

void cParticle::Update(const Float &pTime) {
	mX = mX + mXSpeed * pTime;
    mY = mY + mYSpeed * pTime;
    mXSpeed = mXSpeed + mXAcc * pTime;
    mYSpeed = mYSpeed + mYAcc * pTime;
    mColor.Alpha -= mAlphaDecay * pTime;
    if (mColor.Alpha < 0) mColor.Alpha = 0;
}

}}
