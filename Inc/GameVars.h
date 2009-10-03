#ifndef _GAMEVARS_H_
#define _GAMEVARS_H_

#include "vectormath/scalar/cpp/vectormath_aos.h"
using namespace Vectormath::Aos;

const float SmallNumber = 0.00001f;

// Rendering Vars
const int AspectWidth = 4;
const int AspectHeight = 3;

// Player Movement Vars
const float PlayerAccel = 0.3f;
const float MaxPlayerSpeed = 1.0f;
const float PlayerRadius = 0.02f;
const float GroundCoefficient = 0.99f;
const float StopSpeed = 0.05f;
const Vector3 PlayerJumpImpulse = Vector3(0.0f, 0.4f, 0.0f);

// World/Environment Vars
const Vector3 Gravity = Vector3(0.0f, -0.5f, 0.0f);
const float GroundDotThreshold = -0.5f;

// Fluid Vars
const float ViscousO = 0.0f;
const float ViscousB = 0.4f;
const float FluidRadius = 0.08f;
const float MaxFluidSpeed = 0.5f;
const float YieldRatio = 0.1f; // generally between 0.0 and 0.2
const float PlasticityConstant = 1.0f;
const float FluidStiffness = 1.0f;
const float FluidMassRatio = 1.0f;

// Camera Vars
const float ViewWidth = 0.6f;
const float CameraApproach = 0.25f;



#endif //_GAMEVARS_H_
