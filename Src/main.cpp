//
//  main.cpp
//  ProtoTension
//
//  Created by Nicholas Ray on 3/23/08.
//  Copyright Windly Games 2008. All rights reserved.
//

#ifdef _MACOSX_
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include <GLUT/glut.h>
#endif
#include "vectormath/scalar/cpp/vectormath_aos.h"
#include <algorithm>

#include "World.h"
#include "Fluid.h"
#include "Collision.h"
#include "GameVars.h"
#include <assert.h>

using namespace std;
using namespace Vectormath::Aos;

Vector3 PlayerLocation;
Vector3 NextPlayerLocation;
Vector3 PlayerVelocity;
Vector3 Input;
bool bCanJump;
bool bOnGround;
Vector3 CameraFocusLocation;
float LastSpeed;

//defined in World.cpp
Vector3 Vector2(float x, float y);

GLvoid DrawScene();
GLvoid ResizeScene(int Width, int Height);
GLvoid InitGL();
GLvoid KeyInput(unsigned char Key, int x, int y);
GLvoid KeyUpInput(unsigned char Key, int x, int y);
void OnTimer(int value);

template<typename t>
t Abs(t var)
{
	return max(var, -var);
}

void ConstrainPlayer(Vector3& Location);


//--------------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(800,	600);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("Surface Tension");
	
	InitGL();
	
	glutDisplayFunc(DrawScene);
	glutIgnoreKeyRepeat(true);
	glutKeyboardFunc(KeyInput);
	glutKeyboardUpFunc(KeyUpInput);
	glutReshapeFunc(ResizeScene);
	glutTimerFunc(20, OnTimer, 1);
	
	InitWorld();
	InitFluids();
	
	CameraFocusLocation = PlayerLocation = Vector2(0.05f, 0.25f);
	ConstrainPlayer(PlayerLocation);
	Input = Vector3(0.0f);
	PlayerVelocity = Vector3(0.0f);

	LastSpeed = 0.0f;
	
	bCanJump = false;
	bOnGround = false;
	
	glutMainLoop();
	
    return 0;
}

GLvoid InitGL()
{
	glShadeModel(GL_SMOOTH);
	glClearColor(0.35f, 0.94f, 1.0f, 0.5f);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	
	
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
}

GLvoid KeyInput(unsigned char Key, int x, int y)
{
	switch(Key)
	{
	case 'a':
		Input.setX(max(Input.getX() - 1.0f, -1.0f));		
		break;
	case 'd':
		Input.setX(min(Input.getX() + 1.0f, 1.0f));
		break;
	//case ' ':
	case 'w':
		Input.setY(min(Input.getY() + 1.0f, 1.0f));
		break;
	case 's':
		Input.setY(max(Input.getY() - 1.0f, -1.0f));
		break;
	default:
		break;
	}
}

GLvoid KeyUpInput(unsigned char Key, int x, int y)
{
	switch(Key)
	{
	case 'a':
		Input.setX(min(Input.getX() + 1.0f, 1.0f));		
		break;
	case 'd':
		Input.setX(max(Input.getX() - 1.0f, -1.0f));
		break;
	//case ' ':
	case 'w':
		Input.setY(max(Input.getY() - 1.0f, -1.0f));
		break;
	case 's':
		Input.setY(min(Input.getY() + 1.0f, 1.0f));
		break;
	default:
		break;
	}
}

struct HitStruct
{
	Vector3 MoveDelta;
	float DeltaTime;
	HitResults Hit;
};

void CollideElement(IndexGrid* GridElement, void* UserData)
{
	HitStruct* Hit = (HitStruct*)UserData;
	for(int i=0; i<CollisionReferencesPerGrid; i++)
	{
		if(GridElement->Idx[i] == 0)
			break;
		
		if(CheckSphereTriangle(Sphere(PlayerLocation+Hit->MoveDelta, PlayerRadius), *GetTri(GridElement->Idx[i])))
		{
			float DistFromFace;
			Vector3 HitFaceNormal = GetBestRemovalFaceNormal(*GetTri(GridElement->Idx[i]), PlayerLocation+Hit->MoveDelta, Hit->MoveDelta, &DistFromFace);
			HalfSpace HitHalfSpace = GetFaceHalfSpaceFromNormal(*GetTri(GridElement->Idx[i]), HitFaceNormal);
			//HitHalfSpace.p1 += HitFaceNormal * PlayerRadius;
			//HitHalfSpace.p2 += HitFaceNormal * PlayerRadius;
			//float DistFromFace = GetPointLineDistance(HitHalfSpace, PlayerLocation+Hit->MoveDelta);
			assert(DistFromFace <= PlayerRadius);
			Hit->MoveDelta = Hit->MoveDelta - (HitFaceNormal * (DistFromFace - PlayerRadius));
			if(length(PlayerVelocity) != 0.0f)
				PlayerVelocity -= HitFaceNormal * length(PlayerVelocity) * dot(normalize(PlayerVelocity), HitFaceNormal);
			if(dot(HitFaceNormal, normalize(Gravity)) <= 0.0f)
				bCanJump = true;
			if(dot(HitFaceNormal, normalize(Gravity)) < GroundDotThreshold)
				bOnGround = true;
			//PlayerVelocity = Hit->MoveDelta / Hit->DeltaTime;
			//PlayerVelocity = Vector3(0.0f);
		}

	}

}

void MovePlayer(float DeltaTime, Vector3& DeltaMove)
{
	HitStruct HitStruct;
	HitResults* Hit = &HitStruct.Hit;
	HitStruct.MoveDelta = DeltaMove;
	HitStruct.DeltaTime = DeltaTime;
	if(length(DeltaMove) < 0.001f)
		return;
	OnGridElements(PlayerLocation.getX() - PlayerRadius, PlayerLocation.getY() - PlayerRadius, PlayerLocation.getX() + PlayerRadius, PlayerLocation.getY() + PlayerRadius, CollideElement, &HitStruct);
	if(0)
	{
		SweepSphereHalfspace(Sphere(PlayerLocation, PlayerRadius), DeltaMove, Vector2(0.0f, 0.18f), Vector2(1.0f, 0.18f), *Hit);
		if(Hit->HitTime < 1.0f)
		{
			PlayerVelocity.setY(0.0f);
			bOnGround = true;
		}
		SweepSphereHalfspace(Sphere(PlayerLocation, PlayerRadius), DeltaMove, Vector2(0.6f, 0.0f), Vector2(0.6f, 1.0f), *Hit);
		if(Hit->HitTime < 1.0f)
			PlayerVelocity.setX(0.0f);
	}
	PlayerLocation += HitStruct.MoveDelta;
	if(Hit->HitTime < 1.0f)
	{
		//PlayerVelocity += PlayerVelocity * dot(normalize(PlayerVelocity), Hit->HitNormal) * length(PlayerVelocity);
	}
	Vector3 FluidImpulse = CollideFluidSphere(PlayerLocation, PlayerRadius);
	if(length(FluidImpulse) > 0.0f)
	{
		bOnGround = true;
		PlayerLocation += FluidImpulse;
		//PlayerVelocity += normalize(FluidImpulse) * length(PlayerVelocity) * dot(FluidImpulse, normalize(PlayerVelocity)) * -1.0f; 
	}
	
}

void ConstrainPlayer(Vector3& Location)
{
	Location.setX(min(Location.getX() + PlayerRadius, WorldMaxX) - PlayerRadius); 
	Location.setX(max(Location.getX() - PlayerRadius, WorldMinX) + PlayerRadius);
	
	Location.setY(min(Location.getY() + PlayerRadius, WorldMaxY) - PlayerRadius); 
	Location.setY(max(Location.getY() - PlayerRadius, WorldMinY) + PlayerRadius);
}

void UpdatePlayer(float DeltaTime)
{
	Vector3 PlayerThrust = Input;
	PlayerThrust.setY(0.0f);
	
	PlayerVelocity += PlayerThrust * DeltaTime * PlayerAccel;
	PlayerVelocity.setX(min(PlayerVelocity.getX(), MaxPlayerSpeed));
	PlayerVelocity.setX(max(PlayerVelocity.getX(), -MaxPlayerSpeed));
	
	if(bOnGround)
	{
		PlayerVelocity.setX(PlayerVelocity.getX() * GroundCoefficient);
		
		bool bBrake = lengthSqr(PlayerThrust) == 0.0f;
		
		if(Abs(PlayerVelocity.getX()) <= StopSpeed && bBrake)
			PlayerVelocity.setX(0.0f);
		
		if(dot(PlayerVelocity, Gravity) > 0.0f)
			PlayerVelocity -= dot(PlayerVelocity, Gravity) * normalize(Gravity) * length(PlayerVelocity);
		//PlayerVelocity.setY(0.0f);

		bCanJump = true;
		bOnGround = false;
	}
	else
	{
		PlayerVelocity += Gravity * DeltaTime;
	}

	if(Input.getY() == 1.0f && bCanJump)
	{
		float JumpDirSpeed = dot(normalize(PlayerJumpImpulse), normalize(PlayerVelocity)) * length(PlayerVelocity);
		if(JumpDirSpeed < length(PlayerJumpImpulse))
			PlayerVelocity += normalize(PlayerJumpImpulse) * (length(PlayerJumpImpulse) - JumpDirSpeed);
		bCanJump = false;
	}
	bCanJump = false;
	
	NextPlayerLocation = PlayerLocation + PlayerVelocity * DeltaTime;
	
	// Constrain within world.
	ConstrainPlayer(NextPlayerLocation);
	
	Vector3 DeltaMove = NextPlayerLocation - PlayerLocation;

	MovePlayer(DeltaTime, DeltaMove);
	
	if(PlayerLocation.getX() + PlayerRadius >= WorldMaxX - SmallNumber * (Abs(WorldMaxX)+1.0f))
		PlayerVelocity.setX(min(0.0f, PlayerVelocity.getX()));
	if(PlayerLocation.getX() - PlayerRadius <= WorldMinX + SmallNumber * (Abs(WorldMinX)+1.0f))
		PlayerVelocity.setX(max(0.0f, PlayerVelocity.getX()));
	if(PlayerLocation.getY() + PlayerRadius >= WorldMaxY - SmallNumber * (Abs(WorldMaxY)+1.0f))
		PlayerVelocity.setY(min(0.0f, PlayerVelocity.getY()));
	
	if(PlayerLocation.getY() - PlayerRadius <= WorldMinY + SmallNumber * (Abs(WorldMinY) + 1.0f))
	{
		bOnGround = true;
		PlayerVelocity.setY(0.0f);
	}
}

void OnTimer(int value)
{
	float DeltaTime = 0.02f;
	
	float SpeedDiff = max(MaxPlayerSpeed - length(PlayerVelocity), 0.0f);
	SpeedDiff /= MaxPlayerSpeed;
	float DeltaSpeedDiff = SpeedDiff - LastSpeed;
	LastSpeed += DeltaSpeedDiff * 0.4f;
	SimulateFluids(DeltaTime * LastSpeed);
	UpdatePlayer(DeltaTime);
	
	glutPostRedisplay();
	glutTimerFunc(20, OnTimer, 1);
}


void DrawPlayer()
{
	glPushMatrix();
	glTranslatef(PlayerLocation.getX(), PlayerLocation.getY(), 0.0f);
	glScalef(0.01f, 0.02f, 1.0f);

	glBegin(GL_QUADS);
		glVertex3f(-1.0f, -1.0f, 0.0f);
		glVertex3f(-1.0f, 1.0f, 0.0f);
		glVertex3f(1.0f, 1.0f, 0.0f);
		glVertex3f(1.0f, -1.0f, 0.0f);
	glEnd();
	
	glPopMatrix();
}

GLvoid DrawScene()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	glScalef( 1/ViewWidth, 1/(ViewWidth*AspectHeight/AspectWidth), 1.0f);
	Vector3 DeltaCamera = PlayerLocation - CameraFocusLocation;
	DeltaCamera *= length(DeltaCamera)*CameraApproach/ViewWidth;
	CameraFocusLocation += DeltaCamera;
	glTranslatef( -CameraFocusLocation.getX(), -CameraFocusLocation.getY(), 0.0f); 
	
	DrawFluids();
	DrawWorld();
	DrawPlayer();
	
	glFinish();
	glutSwapBuffers();
}

GLvoid ResizeScene(int Width, int Height)
{
}
