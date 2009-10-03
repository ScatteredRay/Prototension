#ifndef _FLUID_H_
#define _FLUID_H_
#include "vectormath/scalar/cpp/vectormath_aos.h"
using namespace Vectormath::Aos;

void InitFluids();
void SimulateFluids(float DeltaTime);
void DrawFluids();
Vector3 CollideFluidSphere(Vector3& Location, float Radius);

#endif //_FLUID_H_
