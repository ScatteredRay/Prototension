#include "Collision.h"
#include "World.h"
#include <algorithm>
#include <assert.h>

using namespace std;

Vector3 GetNormal(Vector3 P1, Vector3 P2)
{
	Vector3 Normal = normalize(P1 - P2);

	float Temp = Normal.getX();
	Normal.setX(Normal.getY());
	Normal.setY(-Temp);

	return Normal;
}

Vector3 GetOpposingFaceNormal(const Triangle& Tri, Vector3 Delta)
{
	Vector3 BestNormal;
	Vector3 TempNormal;
	Vector3 DeltaNormal = normalize(Delta);
	float BestDot;
	float TempDot;

	BestNormal = GetNormal(Tri.p1, Tri.p2);
	BestDot = dot(DeltaNormal, BestNormal);
	
	TempNormal = GetNormal(Tri.p2, Tri.p3);
	TempDot = dot(DeltaNormal, TempNormal);
	BestNormal = (TempDot < BestDot) ? TempNormal : BestNormal;
	BestDot = (TempDot < BestDot) ? TempDot : BestDot;

	TempNormal = GetNormal(Tri.p3, Tri.p1);
	TempDot = dot(DeltaNormal, TempNormal);
	BestNormal = (TempDot < BestDot) ? TempNormal : BestNormal;

	return BestNormal;
}

float GetPointLineDistance(const HalfSpace& Segment, Vector3 Point)
{
	Vector3 HalfSpaceNormal = GetNormal(Segment.p1, Segment.p2);
	Vector3 PointOffset = Point - Segment.p2;
	float Dist = dot(normalize(PointOffset), HalfSpaceNormal) * length(PointOffset);
	return Dist;
}

Vector3 GetClosestFaceNormal(const Triangle& Tri, Vector3 Point, float* OutDist)
{
	Vector3 BestNormal;
	float BestDist;
	Vector3 TempNormal;
	float TempDist;
	bool bBetter;

	BestNormal = GetNormal(Tri.p1, Tri.p2);
	BestDist = GetPointLineDistance(HalfSpace(Tri.p1, Tri.p2), Point);

	TempNormal = GetNormal(Tri.p2, Tri.p3);
	TempDist = GetPointLineDistance(HalfSpace(Tri.p2, Tri.p3), Point);
	bBetter = (abs(TempDist) < abs(BestDist));
	BestNormal = bBetter ? TempNormal : BestNormal;
	BestDist = bBetter ? TempDist : BestDist;

	TempNormal = GetNormal(Tri.p3, Tri.p1);
	TempDist = GetPointLineDistance(HalfSpace(Tri.p3, Tri.p1), Point);
	bBetter = (abs(TempDist) < abs(BestDist));
	BestNormal = bBetter ? TempNormal : BestNormal;
	BestDist = bBetter ? TempDist : BestDist;

	if(OutDist)
		*OutDist = BestDist;
	return BestNormal;

}

Vector3 GetBestRemovalFaceNormal(const Triangle& Tri, Vector3 Point, Vector3 Delta, float* OutDist)
{
	Vector3 DeltaNormal = normalize(Delta);
	Vector3 BestNormal;
	float BestDist;
	float BestDot;
	Vector3 TempNormal;
	float TempDist;
	float TempDot;
	bool bBetter;

	BestNormal = GetNormal(Tri.p1, Tri.p2);
	BestDot = dot(DeltaNormal, BestNormal);
	BestDist = GetPointLineDistance(HalfSpace(Tri.p1, Tri.p2), Point);

	TempNormal = GetNormal(Tri.p2, Tri.p3);
	TempDot = dot(DeltaNormal, TempNormal);
	TempDist = GetPointLineDistance(HalfSpace(Tri.p2, Tri.p3), Point);
	bBetter = (abs(TempDist) < abs(BestDist)) && TempDot <= 0.0f;
	BestNormal = (bBetter) ? TempNormal : BestNormal;
	BestDot = (bBetter) ? TempDot : BestDot;
	BestDist = (bBetter) ? TempDist : BestDist;

	TempNormal = GetNormal(Tri.p3, Tri.p1);
	TempDot = dot(DeltaNormal, TempNormal);
	TempDist = GetPointLineDistance(HalfSpace(Tri.p3, Tri.p1), Point);
	bBetter = (abs(TempDist) < abs(BestDist)) && TempDot <= 0.0f;
	BestNormal = (bBetter) ? TempNormal : BestNormal;
	BestDot = (bBetter) ? TempDot : BestDot;
	BestDist = (bBetter) ? TempDist : BestDist;

	if(OutDist)
		*OutDist = BestDist;
	return BestNormal;

}

HalfSpace GetFaceHalfSpaceFromNormal(const Triangle& Tri, Vector3 Normal)
{
	if(length(Normal - GetNormal(Tri.p1, Tri.p2)) == 0.0f)
		return HalfSpace(Tri.p1, Tri.p2);
	else if(length(Normal - GetNormal(Tri.p2, Tri.p3)) == 0.0f)
		return HalfSpace(Tri.p2, Tri.p3);
	else// if(length(Normal - GetNormal(Tri.p3, Tri.p1)) == 0.0f)
		return HalfSpace(Tri.p3, Tri.p1);
}

bool CheckSphereHalfspace(const Sphere& Sph, const Vector3& P1, const Vector3& P2)
{
	/*Vector3 EdgePerp = GetNormal(P1, P2);
	
	Vector3 SphereOffset = Sph.Point - P2;
	float SphereDistance = dot(normalize(SphereOffset), EdgePerp) * length(SphereOffset);

	return SphereDistance - Sph.Radius <= 0.0f;*/
	float Dist = GetPointLineDistance(HalfSpace(P1, P2), Sph.Point);
	return Dist <= Sph.Radius;

}

bool CheckSphereTriangle(const Sphere& Sph, const Triangle& Tri)
{
	return CheckSphereHalfspace(Sph, Tri.p1, Tri.p2) &&  CheckSphereHalfspace(Sph, Tri.p2, Tri.p3) &&  CheckSphereHalfspace(Sph, Tri.p3, Tri.p1);
}

bool SweepSphereHalfspace(const Sphere& Sph, const Vector3& Delta, const Vector3& P1, const Vector3& P2, HitResults& Hit)
{
	bool bHit = false;
	Vector3 Edge = P1 - P2;
	Vector3 EdgePerp = normalize(P1 - P2);

	// Swap X and -Y
	{
		float Temp = EdgePerp.getX();

		EdgePerp.setX(-EdgePerp.getY());
		EdgePerp.setY(Temp);
	}
	
	float DeltaDot = dot(normalize(Delta), EdgePerp);

	Vector3 SphereOffset = Sph.Point - P2;
	float SphereDistance = -1.0f * dot(normalize(SphereOffset), EdgePerp) * length(SphereOffset);
	float DeltaDistance = DeltaDot * length(Delta);

	SphereDistance -= Sph.Radius;
	
	float PerctMove = SphereDistance / DeltaDistance;

	bHit = PerctMove < 1.0f;

	if(SphereDistance <= 0.0f)
		bHit = true;

	// Even though we need to return proper collision, return full move on perp or less collision.
	if(DeltaDot <= 0.0f)
		PerctMove = 1.0f;

	PerctMove = min(max(PerctMove, 0.0f), 1.0f);

	if(PerctMove < Hit.HitTime)
	{
		Hit.HitTime = PerctMove;
		Hit.HitNormal = -EdgePerp;
	}

	return bHit;

}

bool SweepPointSegment(const Vector3& Src, const Vector3& Delta, const Vector3& A, const Vector3& B, HitResults& Hit)
{
	Vector3 Ap = A - Src;
	Vector3 Bp = B - Src;

	Vector3 HitPoint(0.0f);

	float ABDeltaY = Ap.getY() - Bp.getY();
	float BADeltaX = Bp.getX() - Ap.getX();

	float Top = Ap.getX() * ABDeltaY + Ap.getY() * BADeltaX;
	float Denom = Delta.getY() * BADeltaX + Delta.getX() * ABDeltaY;

	HitPoint.setX(Delta.getX() * (Top / Denom));
	HitPoint.setY(Delta.getY() * (Top / Denom));

	float PLen = length(HitPoint);
	float DLen = length(Delta);
	float HitDotDelta = dot(normalize(HitPoint), normalize(Delta));

	float ABLen = length(Ap - Bp);
	float PBLen = length(HitPoint - Bp);

	float ABHitDot = dot(normalize(Ap - Bp), normalize(HitPoint - Bp));

	if(PLen <= DLen &&  HitDotDelta > 0.0f &&
		PBLen <= ABLen && ABHitDot > 0.0f)
	{
		float Time = PLen/DLen;
		if(Time < Hit.HitTime)
		{
			Vector3 Perp = normalize(A - B);

			// Swap X and Y
			{
				float Temp = Perp.getX();

				Perp.setX(Perp.getY());
				Perp.setY(Temp);
			}
	

			Hit.HitTime = Time;
			Hit.HitNormal = Perp;
		}
		return true;
	}

	return false;
}

bool SweepPointSphere(const Vector3& Src, const Vector3& Delta, const Sphere& Sph, HitResults& Hit)
{
	Vector3 PointDist = Src - Sph.Point;
	Vector3 DeltaDir = normalize(Delta);
	float b = dot(PointDist, DeltaDir);
	float c = dot(PointDist, PointDist) - Sph.Radius * Sph.Radius;

	if(c > 0.0f && b > 0.0f)
		return false;

	float d = b*b - c;

	if(d < 0.0f)
		return false;

	float Time = (-b - sqrt(d)) * length(Delta);

	Time = max(Time, 0.0f);

	if(Time < Hit.HitTime)
	{
		Hit.HitTime = Time;
		//Hit.HitNormal = 
	}

	return Time < 1.0f;
}

bool SweepSphereSegmentHelper(const Sphere& Sph, const Vector3& Delta, const Vector3& P1, const Vector3& P2, HitResults& Hit)
{
	Vector3 EdgeNormal = normalize(P1 - P2);
	float Temp = EdgeNormal.getX();
	EdgeNormal.setX(EdgeNormal.getY());
	EdgeNormal.setY(Temp);
	
	EdgeNormal *= Sph.Radius;

	return SweepPointSegment(Sph.Point, Delta, P1 + EdgeNormal, P2 + EdgeNormal, Hit);
}

// This sweep (and what it calls, seems to be broke.
bool SweepSphereTriangle(const Sphere& Sph, const Vector3& Delta, const Triangle& Tri, HitResults& Hit)
{
	bool bHit = false;
	
	bHit = SweepPointSphere(Sph.Point, Delta, Sphere(Tri.p1, Sph.Radius), Hit) ||
		SweepPointSphere(Sph.Point, Delta, Sphere(Tri.p2, Sph.Radius), Hit) ||
		SweepPointSphere(Sph.Point, Delta, Sphere(Tri.p3, Sph.Radius), Hit);



	bHit = bHit || SweepSphereSegmentHelper(Sph, Delta, Tri.p1, Tri.p2, Hit);
	bHit = bHit || SweepSphereSegmentHelper(Sph, Delta, Tri.p2, Tri.p3, Hit);
	bHit = bHit || SweepSphereSegmentHelper(Sph, Delta, Tri.p3, Tri.p1, Hit);

	return bHit;

}
