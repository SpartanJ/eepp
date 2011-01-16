#ifndef EE_PHYSICS_SETTINGS_HPP
#define EE_PHYSICS_SETTINGS_HPP

//! Comment this if you don't want to use all the rendering stuff from EE. Disabling this will disable any kind of rendering, and the only dependencies with EE will be the templates.
#define PHYSICS_RENDERER_ENABLED

//! Safe delete a pointer, check if not null, delete it, and set the variable to NULL. ( Must be like this: cpSAFE_DELETE( ptr ) )
#define cpSAFE_DELETE eeSAFE_DELETE

//! new Allocator used by the wrapper, by default use the EE allocator. ( Must be like this: cpNew( classType, constructor ) )
#define cpNew eeNew

//! Dynamic Library Export/Import symbols
#define CP_API EE_API

//! Define this to use the template vector from EE
#define USE_EE_VECTOR

//! Define this to use the templace aabb from EE
#define USE_EE_AABB

//! Define this if you want to invert the BB Y Axis ( by default BB.Top is the bigger y-axis instead of BB.Bottom )
#define BB_INVERT_Y_AXIS

//! Namespace name begin
#define CP_NAMESPACE_BEGIN namespace EE { namespace Physics {

//! Namespace name end
#define CP_NAMESPACE_END }}

#endif
