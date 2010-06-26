#ifndef EE_UTILS_EASINGEASING_H
#define EE_UTILS_EASINGEASING_H

#include "base.hpp"

namespace EE { namespace Utils { namespace easing {

typedef eeFloat( *easingCbFunc )( eeFloat, eeFloat, eeFloat, eeFloat );

extern easingCbFunc easingCb[];

/**
*  Calculate the position of a point from a linear interpolation.
*
*  @param t Specifies the current time, between 0 and duration inclusive.
*  @param b Specifies the initial value of the animation property.
*  @param c Specifies the total change in the animation property.
*  @param d Specifies the duration of the motion.
*  @return The value of the interpolated property at the specified time.         
*/ 
eeFloat LinearInterpolation( eeFloat t, eeFloat b, eeFloat c, eeFloat d );

/**
*  The <code>QuadraticIn()</code> method starts motion from a zero velocity 
*  and then accelerates motion as it executes. 
*  @param t Specifies the current time, between 0 and duration inclusive.
*  @param b Specifies the initial value of the animation property.
*  @param c Specifies the total change in the animation property.
*  @param d Specifies the duration of the motion.
*  @return The value of the interpolated property at the specified time.
*/  
eeFloat QuadraticIn( eeFloat t, eeFloat b, eeFloat c, eeFloat d );

/**
*  The <code>QuadraticOut()</code> method starts motion fast 
*  and then decelerates motion to a zero velocity as it executes. 
*
*  @param t Specifies the current time, between 0 and duration inclusive.
*  @param b Specifies the initial value of the animation property.
*  @param c Specifies the total change in the animation property.
*  @param d Specifies the duration of the motion.
*  @return The value of the interpolated property at the specified time.
*/  
eeFloat QuadraticOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d );

/**
*  The <code>QuadraticInOut()</code> method combines the motion
*  of the <code>QuadraticIn()</code> and <code>QuadraticOut()</code> methods
*  to start the motion from a zero velocity, 
*  accelerate motion, then decelerate to a zero velocity. 
*
*  @param t Specifies the current time, between 0 and duration inclusive.
*  @param b Specifies the initial value of the animation property.
*  @param c Specifies the total change in the animation property.
*  @param d Specifies the duration of the motion.
*  @return The value of the interpolated property at the specified time.
*/  
eeFloat QuadraticInOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d );

/**
*  The <code>SineIn()</code> method starts motion from zero velocity 
*  and then accelerates motion as it executes. 
*
*  @param t Specifies the current time, between 0 and duration inclusive.
*  @param b Specifies the initial value of the animation property.
*  @param c Specifies the total change in the animation property.
*  @param d Specifies the duration of the motion.
*  @return The value of the interpolated property at the specified time.      
*/  
eeFloat SineIn( eeFloat t, eeFloat b, eeFloat c, eeFloat d );

/**
*  The <code>easeOut()</code> method starts motion fast 
*  and then decelerates motion to a zero velocity as it executes. 
*
*  @param t Specifies the current time, between 0 and duration inclusive.
*  @param b Specifies the initial value of the animation property.
*  @param c Specifies the total change in the animation property.
*  @param d Specifies the duration of the motion.
*  @return The value of the interpolated property at the specified time.      
*/  
eeFloat SineOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d );

/**
*  The <code>easeInOut()</code> method combines the motion
*  of the <code>easeIn()</code> and <code>easeOut()</code> methods
*  to start the motion from a zero velocity, accelerate motion, 
*  then decelerate to a zero velocity. 
*
*  @param t Specifies the current time, between 0 and duration inclusive.
*  @param b Specifies the initial value of the animation property.
*  @param c Specifies the total change in the animation property.
*  @param d Specifies the duration of the motion.
*  @return The value of the interpolated property at the specified time.     
*/  
eeFloat SineInOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d );

/**
*  The <code>ExponentialIn()</code> method starts motion slowly 
*  and then accelerates motion as it executes. 
*
*  @param t Specifies the current time, between 0 and duration inclusive.
*  @param b Specifies the initial position of a component.
*  @param c Specifies the total change in position of the component.
*  @param d Specifies the duration of the effect, in milliseconds.
*  @return Number corresponding to the position of the component.        
*/  
eeFloat ExponentialIn( eeFloat t, eeFloat b, eeFloat c, eeFloat d );

/**
*  The <code>ExponentialOut()</code> method starts motion fast 
*  and then decelerates motion to a zero velocity as it executes. 
*
*  @param t Specifies the current time, between 0 and duration inclusive.
*  @param b Specifies the initial value of the animation property.
*  @param c Specifies the total change in the animation property.
*  @param d Specifies the duration of the motion.
*  @return The value of the interpolated property at the specified time.        
*/  
eeFloat ExponentialOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d );

/**
*  The <code>ExponentialInOut()</code> method combines the motion
*  of the <code>ExponentialIn()</code> and <code>ExponentialOut()</code> methods
*  to start the motion from a zero velocity, accelerate motion, 
*  then decelerate to a zero velocity. 
*
*  @param t Specifies the current time, between 0 and duration inclusive.
*  @param b Specifies the initial value of the animation property.
*  @param c Specifies the total change in the animation property.
*  @param d Specifies the duration of the motion.
*  @return The value of the interpolated property at the specified time.         
*/  
eeFloat ExponentialInOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d );

eeFloat QuarticIn( eeFloat t, eeFloat b, eeFloat c, eeFloat d );

eeFloat QuarticOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d );

eeFloat QuarticInOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d );

eeFloat QuinticIn( eeFloat t, eeFloat b, eeFloat c, eeFloat d );

eeFloat QuinticOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d );

eeFloat QuinticInOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d );

eeFloat CircularIn( eeFloat t, eeFloat b, eeFloat c, eeFloat d );

eeFloat CircularOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d );

eeFloat CircularInOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d );

eeFloat CubicIn( eeFloat t, eeFloat b, eeFloat c, eeFloat d );

eeFloat CubicOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d );

eeFloat CubicInOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d );

eeFloat BackIn( eeFloat t, eeFloat b, eeFloat c, eeFloat d );

eeFloat BackOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d );

eeFloat BackInOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d );

eeFloat BounceIn( eeFloat t, eeFloat b, eeFloat c, eeFloat d );

eeFloat BounceOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d );

eeFloat BounceInOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d );

eeFloat ElasticIn( eeFloat t, eeFloat b, eeFloat c, eeFloat d );

eeFloat ElasticOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d );

eeFloat ElasticInOut( eeFloat t, eeFloat b, eeFloat c, eeFloat d );

}}}

#endif
