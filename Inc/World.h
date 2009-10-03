/*
 *  World.h
 *  ProtoTension
 *
 *  Created by Nicholas Ray on 3/23/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */
#ifndef _WORLD_H_
#define _WORLD_H_
#include "vectormath/scalar/cpp/vectormath_aos.h"
#include "Collision.h"
using namespace Vectormath::Aos;

extern float WorldMinX;
extern float WorldMinY;
extern float WorldMaxX;
extern float WorldMaxY;

void InitWorld();
void DrawWorld();

// This gives us ~4KB usage for the grid.
const int CollisionGridSubDivisionsX = 128;
const int CollisionGridSubDivisionsY = 32;
const int CollisionReferencesPerGrid = 8; 
typedef unsigned char TriIndex;
const unsigned int MaxIndex = 255; // char minus one for null index.

typedef Triangle WorldTri;

struct IndexGrid
{
	TriIndex Idx[CollisionReferencesPerGrid];
};

inline Vector3 Vector2(float x, float y)
{
	return Vector3(x, y, 0.0f);
}

typedef void (*GridElementCallback)(IndexGrid* GridElement, void* UserData);

WorldTri* GetTri(TriIndex Idx);
void OnGridElements(float MinX, float MinY, float MaxX, float MaxY, GridElementCallback Callback, void* UserData);

#endif //_WORLD_H_
