#ifndef EE_PHYSICS_SETTINGS_HPP
#define EE_PHYSICS_SETTINGS_HPP

namespace EE { namespace Physics {

//! Comment this if you don't want to use all the rendering stuff from EE. Disabling this will disable any kind of rendering, and the only dependencies with EE will be the templates.
#define PHYSICS_RENDERER_ENABLED

//! Define this to use the template vector from EE
#define USE_EE_VECTOR

//! Define this to use the template AABB from EE
#define USE_EE_AABB

//! Define this if you want to invert the BB Y Axis ( by default BB.Top is the bigger y-axis instead of BB.Bottom )
#define BB_INVERT_Y_AXIS

}}

#endif
