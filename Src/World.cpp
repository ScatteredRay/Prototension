/*
 *  World.cpp
 *  ProtoTension
 *
 *  Created by Nicholas Ray on 3/23/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "World.h"
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
#include <vector>
#include <assert.h>

using namespace std;

float WorldMinX;
float WorldMinY;
float WorldMaxX;
float WorldMaxY;

IndexGrid CollisionGrid[CollisionGridSubDivisionsX][CollisionGridSubDivisionsY];

IndexGrid* GetGridElement(int x, int y)
{
	assert(x < CollisionGridSubDivisionsX);
	assert(y < CollisionGridSubDivisionsY);
	IndexGrid* Index = &CollisionGrid[x][y];
	return Index;
}

void AddGridIdx(IndexGrid* Index, TriIndex idx)
{
	for(int i=0; i < CollisionReferencesPerGrid; i++)
	{
		if(Index->Idx[i] == 0)
		{
			Index->Idx[i] = idx;
			return;
		}
	}

	assert(false && "Grid element full!");
}

vector<WorldTri*> CollisionTriangles;

WorldTri* GetTri(TriIndex Idx)
{
	//assert(Idx <= MaxIndex);
	assert(Idx < CollisionTriangles.size());
	return CollisionTriangles[Idx];
}

void OnGridElements(float MinX, float MinY, float MaxX, float MaxY, GridElementCallback Callback, void* UserData)
{

	// Build world to grid conversion.
	
	float XAxisFToI = ((float)CollisionGridSubDivisionsX)/(WorldMaxX - WorldMinX);
	float YAxisFToI = ((float)CollisionGridSubDivisionsY)/(WorldMaxY - WorldMinY);
	
	// Convert Bounds to grid coords.

	int LowX = (int)floor((MinX - WorldMinX) * XAxisFToI);
	int LowY = (int)floor((MinY - WorldMinY) * YAxisFToI);

	int HighX = (int)floor((MaxX - WorldMinX) * XAxisFToI);
	int HighY = (int)floor((MaxY - WorldMinY) * YAxisFToI);

	LowX = max(min(LowX, CollisionGridSubDivisionsX), 0);
	LowY = max(min(LowY, CollisionGridSubDivisionsY), 0);
	HighX = max(min(HighX, CollisionGridSubDivisionsX), 0);
	HighY = max(min(HighY, CollisionGridSubDivisionsY), 0);

	assert(0 <= LowX && LowX <= HighX && HighX <= CollisionGridSubDivisionsX);
	assert(0 <= LowY && LowY <= HighY && HighY <= CollisionGridSubDivisionsY);

	HighX = min(HighX, CollisionGridSubDivisionsX-1);
	HighY = min(HighY, CollisionGridSubDivisionsY-1);


	for(int x = LowX; x <= HighX; x++)
	{
		for(int y = LowY; y <= HighY; y++)
		{
			Callback(GetGridElement(x, y), UserData);
		}
	}
}

void AddTriCallback(IndexGrid* GridElement, void* UserData)
{
	TriIndex TmpIdx = (TriIndex)(unsigned int)UserData;

	AddGridIdx(GridElement, TmpIdx);
}

void AddTri(const Vector3& P1, const Vector3& P2, const Vector3& P3)
{
	WorldTri* TmpTri = new WorldTri(P1, P2, P3);
	CollisionTriangles.push_back(TmpTri);
	assert(CollisionTriangles.size()-1 <= MaxIndex);
	TriIndex TmpIdx = (unsigned char)(CollisionTriangles.size()-1);

	// Build bounds, just to speed up a bit.

	float LowX = min(P1.getX(), min(P2.getX(), P3.getX()));
	float LowY = min(P1.getY(), min(P2.getY(), P3.getY()));

	float HighX = max(P1.getX(), max(P2.getX(), P3.getX()));
	float HighY = max(P1.getY(), max(P2.getY(), P3.getY()));

	OnGridElements(LowX, LowY, HighX, HighY, &AddTriCallback, (void*)TmpIdx);


}

void AddQuad(const Vector3& P1, const Vector3& P2, const Vector3& P3, const Vector3& P4)
{
	AddTri(P1, P2, P3);
	AddTri(P1, P3, P4);
}

void AddAAQuad(float minX, float minY, float maxX, float maxY)
{
	assert(minX <= maxX);
	assert(minY <= maxY);
	
	AddQuad(Vector2(minX, maxY), Vector2(maxX, maxY), Vector2(maxX, minY), Vector2(minX, minY));
}

void InitEntryLevel();

void InitWorld()
{
	memset(CollisionGrid, 0, sizeof(CollisionTriangles));
	InitEntryLevel();
	
}

void DoGLVector(const Vector3& Vect)
{
	glVertex3f(Vect.getX(), Vect.getY(), Vect.getZ());
}

void DrawWorld()
{
	// Draw Limits
	glColor3f(0.0f, 0.0f, 0.0f);
	glTranslatef(0.0f, 0.0f, 0.0f);
	glBegin(GL_LINE_LOOP);
		glVertex3f(WorldMinX, WorldMinY, 0.0f);
		glVertex3f(WorldMaxX, WorldMinY, 0.0f);
		glVertex3f(WorldMaxX, WorldMaxY, 0.0f);
		glVertex3f(WorldMinX, WorldMaxY, 0.0f);
	glEnd();

	glColor3f(0.4f, 0.4f, 0.4f);

	glBegin(GL_TRIANGLES);
	for(vector<WorldTri*>::iterator it = CollisionTriangles.begin(); it != CollisionTriangles.end(); it++)
	{
		glVertex3f((*it)->p1.getX(), (*it)->p1.getY(), (*it)->p1.getZ());
		glVertex3f((*it)->p2.getX(), (*it)->p2.getY(), (*it)->p2.getZ());
		glVertex3f((*it)->p3.getX(), (*it)->p3.getY(), (*it)->p3.getZ());
	}
	glEnd();

	glColor3f(1.0f, 0.0f, 0.0f);

#if 0
	glBegin(GL_LINES);
	for(vector<WorldTri*>::iterator it = CollisionTriangles.begin(); it != CollisionTriangles.end(); it++)
	{
		{
			Vector3 Origin = ((*it)->p1 + (*it)->p2) / 2;
			DoGLVector(Origin);
			DoGLVector(Origin + GetNormal((*it)->p1, (*it)->p2) * 0.05f);
		}
		
		{
			Vector3 Origin = ((*it)->p2 + (*it)->p3) / 2;
			DoGLVector(Origin);
			DoGLVector(Origin + GetNormal((*it)->p2, (*it)->p3) * 0.05f);
		}

		{
			Vector3 Origin = ((*it)->p3 + (*it)->p1) / 2;
			DoGLVector(Origin);
			DoGLVector(Origin + GetNormal((*it)->p3, (*it)->p1) * 0.05f);
		}
	}
	glEnd();
#endif
	
}

void InitEntryLevel()
{
	WorldMinX = -1.0f;
	WorldMaxX = 1.0f;
	WorldMinY = 0.0f;
	WorldMaxY = 1.0f;

	AddQuad(Vector2(0.2f, 0.1f), Vector2(0.6f, 0.1f), Vector2(0.6f, 0.08f), Vector2(0.2f, 0.08f));
	
	AddQuad(Vector2(0.0f, 0.2f), Vector2(0.2f, 0.2f), Vector2(0.2f, 0.18f), Vector2(0.0f, 0.18f));

	AddQuad(Vector2(-0.2f, 0.18f), Vector2(-0.17f, 0.18f), Vector2(-0.17f, 0.0f), Vector2(-0.2f, 0.0f)); 

	AddTri(Vector2(-0.6f, 0.3f), Vector2(-0.2f, 0.3f), Vector2(-0.6f, 0.08f));

	AddAAQuad(0.98f, 0.0f, 1.2f, 1.0f);
	
	AddAAQuad(-1.2f, 0.0f, -0.98f, 1.0f);
	
}
