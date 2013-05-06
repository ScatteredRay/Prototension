#include "Fluid.h"
#include "World.h"
#ifdef _MACOSX_
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>
#endif
#include <vector>
#include <assert.h>

#include "GameVars.h"
#include "World.h"


using namespace std;

struct FluidSpring
{
	unsigned int ParticleA;
	unsigned int ParticleB;
	float RestLength;
};
vector<Vector3> ParticleLocations;
vector<Vector3> PrevParticleLocations;
vector<Vector3> ParticleVelocities;
vector<Vector3> ParticleImpulses;
vector<FluidSpring> ParticleSprings;
float ElapsedWaveTime;


void InitFluids()
{
	unsigned int StartingParticles = 500;
	unsigned int StackDepth = 4;
	ParticleLocations.reserve(StartingParticles);
	PrevParticleLocations.reserve(StartingParticles);
	ParticleVelocities.reserve(StartingParticles);
	ParticleImpulses.reserve(StartingParticles);
	ParticleSprings.reserve(StartingParticles); // Why not, resonable starting number.
	ElapsedWaveTime = 0.0f;
	
	float FluidDepth = (WorldMaxY - WorldMinY) / 4.0f; 
	float YParticlePadding = FluidDepth/(((float)StackDepth)*2.0f);
	float XParticlePadding = (WorldMaxX - WorldMinX)/((StartingParticles/StackDepth)*2.0f);
	
	Vector3 LastParticle = Vector2(WorldMinX + XParticlePadding, WorldMinY + YParticlePadding);

	for(unsigned  int i=0; i<StartingParticles; i++)
	{
		ParticleLocations.push_back(LastParticle);
		ParticleVelocities.push_back(Vector2(0.0f, 0.0f));
		PrevParticleLocations.push_back(LastParticle);
		ParticleImpulses.push_back(Vector2(0.0f, 0.0f));

		LastParticle.setX(LastParticle.getX() + XParticlePadding * 2.0f);
		if(LastParticle.getX() >= WorldMaxX)
		{
			LastParticle.setX(WorldMinX + XParticlePadding);
			LastParticle.setY(LastParticle.getY() + YParticlePadding * 2.0f);
		}
	}
}

void ParticleViscosity(float DeltaTime)
{
	for(unsigned int i=0; i<ParticleLocations.size(); i++)
	{
		for(unsigned int j=i+1; j<ParticleLocations.size(); j++)
		{
			float Dist = length(ParticleLocations[i] - ParticleLocations[j]);
			if(Dist > FluidRadius)
				continue;
			float q = Dist/FluidRadius;
			if(q<1.0f)
			{
				Vector3 IJVect = normalize(ParticleLocations[i] - ParticleLocations[j]);
				float u = dot(normalize(ParticleVelocities[i] - ParticleVelocities[j]), IJVect);
				if(u > 0)
				{
					Vector3 Impulse = DeltaTime*(1-q)*(ViscousO*u + ViscousB*u*u)*IJVect;
					ParticleVelocities[i] += Impulse/2;
					ParticleVelocities[j] -= Impulse/2;
				}
			}
		}
	}
}

void AdjustSprings(float DeltaTime)
{
	ParticleSprings.clear();
	for(unsigned int i=0; i<ParticleLocations.size(); i++)
	{
		for(unsigned int j=i+1; j<ParticleLocations.size(); j++)
		{
			Vector3 DiffVect = ParticleLocations[i] - ParticleLocations[j];
			float Dist = length(DiffVect);
			float q = Dist/FluidRadius;
			if(q<1.0f)
			{
				ParticleSprings.push_back(FluidSpring());
				FluidSpring& Spring = ParticleSprings.back();
				Spring.ParticleA = i;
				Spring.ParticleB = j;
				Spring.RestLength = FluidRadius;

				float d = YieldRatio * Spring.RestLength;

				if(Dist > Spring.RestLength + d)
					Spring.RestLength += DeltaTime*PlasticityConstant*(Dist-Spring.RestLength-d);
				else if(Dist < Spring.RestLength - d)
					Spring.RestLength -= DeltaTime*PlasticityConstant*(Spring.RestLength-d-Dist);
			}
		}
	}
}

void ApplySprings(float DeltaTime)
{
	for(unsigned int i=0; i<ParticleSprings.size(); i++)
	{
		FluidSpring& Spring = ParticleSprings[i];
		Vector3 DiffVect = ParticleLocations[Spring.ParticleA] - ParticleLocations[Spring.ParticleB];
		float Dist = length(DiffVect);

		Vector3 D = DeltaTime*DeltaTime*FluidStiffness*(1-Spring.RestLength/FluidRadius)*(Spring.RestLength-Dist)*normalize(DiffVect);

		ParticleLocations[Spring.ParticleA] += D/2;
		ParticleLocations[Spring.ParticleB] -= D/2;
	}
}

void DensityRelaxation(float DeltaTime)
{

}

void CollideParticles()
{
	static int iterations = 0;
	for(unsigned int i=0; i<ParticleLocations.size(); i++)
	{
		if(iterations++ > 100)
			ParticleLocations[i].setX(max(WorldMinX+0.05f, ParticleLocations[i].getX()));
		if(iterations > 200)
			iterations = 0;
		ParticleLocations[i].setX(max(WorldMinX, ParticleLocations[i].getX()));
		ParticleLocations[i].setX(min(WorldMaxX, ParticleLocations[i].getX()));
		ParticleLocations[i].setY(max(WorldMinY, ParticleLocations[i].getY()));
		ParticleLocations[i].setY(min(WorldMaxY, ParticleLocations[i].getY()));
	}
}

void CollideParticleOnGridElement(IndexGrid* GridElement, void* UserData)
{
	Vector3* PLoc = (Vector3*)UserData;
	for(int i=0; i<CollisionReferencesPerGrid; i++)
	{
		if(GridElement->Idx[i] == 0)
		   break;
		Triangle* Tri = GetTri(GridElement->Idx[i]);
		if(CheckSphereTriangle(Sphere(*PLoc, 0), *Tri))
		{
			float RemovalDist;
			Vector3 RemovalDir = GetClosestFaceNormal(*Tri, *PLoc, &RemovalDist);
			*PLoc += RemovalDir * (RemovalDist);
		}
	}
}

void CollideParticlesWorld()
{
	for(unsigned int i=0; i<ParticleLocations.size(); i++)
	{
		OnGridElements(
			ParticleLocations[i].getX(),//-FluidRadius,
			ParticleLocations[i].getY(),//-FluidRadius,
			ParticleLocations[i].getX(),//+FluidRadius,
			ParticleLocations[i].getY(),//+FluidRadius,
			CollideParticleOnGridElement,
			&(ParticleLocations[i]));
	}
}

void WaveParticles()
{
	for(unsigned int i=0; i<ParticleLocations.size(); i++)
	{
		ParticleLocations[i].setX(min(ParticleLocations[i].getX(), 0.95f));
	}
}

void SimulateFluids(float DeltaTime)
{
	/*for(unsigned int i=0; i<ParticleLocations.size(); i++)
	{
		ParticleVelocities[i] += DeltaTime * Gravity;
	}
	//ParticleViscosity(DeltaTime);
	for(unsigned int i=0; i<ParticleLocations.size(); i++)
	{
		PrevParticleLocations[i] = ParticleLocations[i];
		ParticleLocations[i] += DeltaTime * ParticleVelocities[i];
	}
	// Springs
	AdjustSprings(DeltaTime);
	ApplySprings(DeltaTime);
	// Density Relaxation
	DensityRelaxation(DeltaTime);
	CollideParticles();
	for(unsigned int i=0; i<ParticleLocations.size(); i++)
	{
		ParticleVelocities[i] = (ParticleLocations[i] - PrevParticleLocations[i])/DeltaTime;
	}*/
	for(unsigned int i=0; i<ParticleLocations.size(); i++)
	{
		ParticleVelocities[i] += DeltaTime * Gravity;
		//ParticleVelocities[i] = Vector2(0.0f, 0.0f);
		ParticleImpulses[i] = Vector2(0.0f, 0.0f);
	}

	float RadSqr = Radius*Radius;

	for(unsigned int x=0; x<ParticleLocations.size(); x++)
	{
		float d = 0;
		float dn = 0;
		for(unsigned int y=x+1; y<ParticleLocations.size(); y++)
		{
			Vector3 Delta = ParticleLocations[x] - ParticleLocations[y];
			float DistSqr = lengthSqr(Delta);

			if(DistSqr < RadSqr)
			{
				float Dist = sqrt(DistSqr);
				float q = q - Dist / Radius;
				float q2 = q * q;
				float q3 = q2 * q;

				d += q2;
				dn += q3;

				ParticleDensity[x] += q2;
				ParticleDensityNear[x] += q3;
			}
		}
		ParticleDensity[x] += d;
		ParticleDensityNear[x] += dn;
	}


	float RestDensity = 1.0f;
	float GasK = 0.75f;
	float GasKNear = 0.9f;
	
	for(unsigned int i=0; i<ParticleLocations.size(); i++)
	{
		ParticlePressure[x] = GasK * (ParticleDensity[x] - RestDensity);
		ParticlePressureNear[x] = GasKNear * ParticleDensityNear[x];
	}

	for(unsigned int x=0; x<ParticleLocations.size(); x++)
	{
		Vector3 DX = Vector3(0.0f, 0.0f, 0.0f);
		for(unsigned int y=x+1; y<ParticleLocations.size(); y++)
		{
			Vector3 Delta = ParticleLocations[x] - ParticleLocations[y];
			float DistSqr = lengthSqr(Delta);

			if(DistSqr < RadSqr)
			{
				float dm = (ParticlePressure[x] + ParticlePressure[y]) * q +
					(ParticlePressureNear[x] + ParticlePressureNear[y]) * q2;
				
				Vector3 D = normal(Delta) * dm;
				DX += D;

				ParticleForce[y] += D;
			}
			
			ParticleForce[x] -= DX;
		}
	}


	for(unsigned int x=0; x<ParticleLocations.size(); x++)
	{
		Vector3 DX = Vector3(0.0f, 0.0f, 0.0f);
		for(unsigned int y=x+1; y<ParticleLocations.size(); y++)
		{
			Vector3 Delta = ParticleLocations[x] - ParticleLocations[y];
			float DistSqr = lengthSqr(Delta);

			if(DistSqr < RadSqr)
			{
				float Dist = sqrt(DistSqr);
				float q = Dist / Radius;

				Vector3 NormalVect = normal(Delta);

				float u = dot(ParticleVelocities[x] - ParticleVelocities[y], NormalVect);

				if(u > 0)
					Vector3 I = (Dist-q) * (Sigma * u + Beta * u * u);
			}
		}
	}
	

	ElapsedWaveTime += DeltaTime;
	if(ElapsedWaveTime >= 1.5f)
		WaveParticles();
	if(ElapsedWaveTime >= 3.0f)
		ElapsedWaveTime = 0.0f;
	CollideParticles();
	CollideParticlesWorld();

	for(unsigned int i=0; i<ParticleLocations.size(); i++)
	{
		ParticleVelocities[i] = (ParticleLocations[i] - PrevParticleLocations[i]) / DeltaTime;
		PrevParticleLocations[i] = ParticleLocations[i];
	}
}

Vector3 CollideFluidSphere(Vector3& Location, float Radius)
{
	Vector3 OutImpulse = Vector2(0.0f, 0.0f);
	
	// Generate impulse against colliding body.
	for(unsigned int i=0; i<ParticleLocations.size(); i++)
	{
		Vector3& P = ParticleLocations[i];
		
		Vector3 PDisp = P - Location;
		if(length(PDisp) < Radius)
		{
			OutImpulse -= normalize(PDisp) * (Radius - length(PDisp)) * FluidMassRatio;
		}
	}

	// Move all the particles out of the new location of the colliding body.
	for(unsigned int i=0; i<ParticleLocations.size(); i++)
	{
		Vector3& P = ParticleLocations[i];

		Vector3 PDisp = P - (Location + OutImpulse);
		
		if(length(PDisp) < Radius)
			P += normalize(PDisp) * (Radius - length(PDisp));
	}

	return OutImpulse;
}

void DrawFluids()
{
	float R = 0.035f;
	
	glColor3f(0.0f, 0.0f, 1.0f);

	for(unsigned int i=0; i<ParticleLocations.size(); i++)
	{
		Vector3& P = ParticleLocations[i];
		glBegin(GL_QUADS);
			glVertex3f(P.getX() - R, P.getY() + R, 0.0f);
			glVertex3f(P.getX() + R, P.getY() + R, 0.0f);
			glVertex3f(P.getX() + R, P.getY() - R, 0.0f);
			glVertex3f(P.getX() - R, P.getY() - R, 0.0f);
		glEnd();
	}
}
