#include <eepp/graphics/particle.hpp>

namespace EE { namespace Graphics {

Particle::Particle() {
	reset(0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
}

Particle::~Particle() {}

void Particle::setColor(ColorAf Color, Float AlphaDecay) {
	mColor = Color;
	mAlphaDecay = AlphaDecay;
}

void Particle::reset(const Float &x, const Float &y, const Float &xspeed, const Float &yspeed, const Float &xacc, const Float &yacc, const Float size) {
	mX = x;
	mY = y;
	mXSpeed = xspeed;
	mYSpeed = yspeed;
	mXAcc = xacc;
	mYAcc = yacc;
	mSize = size;
	mColor = ColorAf(1.0f,1.0f,1.0f,1.0f);
	mAlphaDecay = 0.01f;
}

void Particle::update(const Float &pTime) {
	mX = mX + mXSpeed * pTime;
	mY = mY + mYSpeed * pTime;
	mXSpeed = mXSpeed + mXAcc * pTime;
	mYSpeed = mYSpeed + mYAcc * pTime;
	mColor.a -= mAlphaDecay * pTime;
	if (mColor.a < 0) mColor.a = 0;
}

}}
