#ifndef _COLLISION_H_
#define _COLLISION_H_

#include "vectormath/scalar/cpp/vectormath_aos.h"
using namespace Vectormath::Aos;

// Triangles must be ordered clockwise.
struct Triangle
{
	Vector3 p1;
	Vector3 p2;
	Vector3 p3;
	Triangle(const Vector3& P1, const Vector3& P2, const Vector3& P3) : p1(P1), p2(P2), p3(P3) {}
};

struct HalfSpace
{
	Vector3 p1;
	Vector3 p2;
	HalfSpace(const Vector3& P1, const Vector3& P2) :  p1(P1), p2(P2) {}
};

struct Sphere
{
	Vector3 Point;
	float Radius;
	Sphere(const Vector3& P, float R) : Point(P), Radius(R) {}
};

struct HitResults
{
	float HitTime;
	Vector3 HitNormal;
	HitResults() : HitTime(1.0f) {}
	HitResults(const HitResults& C) : HitTime(C.HitTime), HitNormal(C.HitNormal) {}
	bool operator>(const HitResults& C) const
	{
		return HitTime > C.HitTime;
	}
	bool operator<(const HitResults& C) const
	{
		return HitTime < C.HitTime;
	}
};

Vector3 GetNormal(Vector3 P1, Vector3 P2);
Vector3 GetOpposingFaceNormal(const Triangle& Tri, Vector3 Delta);
Vector3 GetClosestFaceNormal(const Triangle& Tri, Vector3 Point, float* OutDist = 0);
Vector3 GetBestRemovalFaceNormal(const Triangle& Tri, Vector3 Point, Vector3 Delta, float* OutDist = 0);
HalfSpace GetFaceHalfSpaceFromNormal(const Triangle& Tri, Vector3 Normal);
float GetPointLineDistance(const HalfSpace& Segment, Vector3 Point);

bool CheckSphereHalfspace(const Sphere& Sph, const Vector3& P1, const Vector3& P2);
bool CheckSphereTriangle(const Sphere& Sph, const Triangle& Tri);

bool SweepSphereHalfspace(const Sphere& Sph, const Vector3& Delta, const Vector3& P1, const Vector3& P2, HitResults& Hit);
bool SweepSphereTriangle(const Sphere& Sph, const Vector3& Delta, const Triangle& Tri, HitResults& Hit);

#endif //_COLLISION_H_
