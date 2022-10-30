//-----------------------------------------------------------------------------
// Note: this is a modified version of dlight. It is not the original software.
//-----------------------------------------------------------------------------
//
// Copyright (c) 2013-2014 Samuel Villarreal
// svkaiser@gmail.com
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
//    1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
//
//   2. Altered source versions must be plainly marked as such, and must not be
//   misrepresented as being the original software.
//
//    3. This notice may not be removed or altered from any source
//    distribution.
//

#pragma once

#include <vector>
#include <memory>
#include <string>
#include <cstring>

#include "framework/tarray.h"
#include "framework/halffloat.h"
#include "lightmaptexture.h"
#include "math/mathlib.h"
#include "collision.h"

#include "dp_rect_pack/dp_rect_pack.h"

typedef dp::rect_pack::RectPacker<int> RectPacker;

struct MapSubsectorEx;
struct IntSector;
struct IntSideDef;
struct IntLineDef;
struct FLevel;
struct ThingLight;
class FWadWriter;

enum SurfaceType
{
	ST_UNKNOWN,
	ST_MIDDLESIDE,
	ST_UPPERSIDE,
	ST_LOWERSIDE,
	ST_CEILING,
	ST_FLOOR
};

struct Portal
{
	mat4 transformation = mat4::identity();
	int sourceSectorGroup = 0;
	int targetSectorGroup = 0;

	inline vec3 TransformPosition(const vec3& pos) const
	{
		auto v = transformation * vec4(pos, 1.0);
		return vec3(v.x, v.y, v.z);
	}

	inline vec3 TransformRotation(const vec3& dir) const
	{
		auto v = transformation * vec4(dir, 0.0);
		return vec3(v.x, v.y, v.z);
	}
 
	// Check if the portal is inverse
	inline bool IsInversePortal(const Portal& portal) const
	{
		auto diff = portal.TransformPosition(TransformPosition(vec3(0)));
		return abs(diff.x) < 0.001 && abs(diff.y) < 0.001 && abs(diff.z) < 0.001;
	}

	// Check if the portal transformation is equal
	inline bool IsEqualPortal(const Portal& portal) const
	{
		auto diff = portal.TransformPosition(vec3(0)) - TransformPosition(vec3(0));
		return (abs(diff.x) < 0.001 && abs(diff.y) < 0.001 && abs(diff.z) < 0.001);
	}

};


	
// for use with std::set to recursively go through portals
struct RejectRecursivePortals
{
	bool operator()(const Portal& a, const Portal& b) const
	{
		return !(a.IsInversePortal(b) || a.IsEqualPortal(b)) && std::memcmp(&a.transformation, &b.transformation, sizeof(mat4)) < 0;
	}
};

struct Surface
{
	// Surface geometry
	SurfaceType type = ST_UNKNOWN;
	std::vector<vec3> verts;
	Plane plane;
	BBox bounds;

	// Surface material
	std::string material;
	std::vector<vec2> texUV;

	// Surface properties
	int typeIndex = 0;
	IntSector* controlSector = nullptr;
	int sampleDimension = 0;
	bool bSky = false;

	// Portal
	int portalDestinationIndex = -1; // line or sector index
	int portalIndex = -1;

	// Touching light sources
	std::vector<ThingLight*> LightList;

	// Lightmap world coordinates for the texture
	vec3 worldOrigin = { 0.0f };
	vec3 worldStepX = { 0.0f };
	vec3 worldStepY = { 0.0f };

	// Calculate world coordinates to UV coordinates
	vec3 translateWorldToLocal = { 0.0f };
	vec3 projLocalToU = { 0.0f };
	vec3 projLocalToV = { 0.0f };

	// Output lightmap for the surface
	int texWidth = 0;
	int texHeight = 0;
	std::vector<vec3> texPixels;

	// UV coordinates for the vertices
	std::vector<vec2> lightUV;

	// Placement in final texture atlas
	int atlasPageIndex = -1;
	int atlasX = 0;
	int atlasY = 0;

	// Smoothing group surface is to be rendered with
	int smoothingGroupIndex = -1;
};

class LevelMesh
{
public:
	LevelMesh(FLevel &doomMap, int sampleDistance, int textureSize);

	void CreateTextures();
	void AddLightmapLump(FWadWriter& wadFile);
	void Export(std::string filename);

	FLevel* map = nullptr;

	std::vector<std::unique_ptr<Surface>> surfaces;

	std::vector<std::unique_ptr<LightmapTexture>> textures;

	std::vector<Plane> smoothingGroups;

	std::vector<std::unique_ptr<Portal>> portals;

	int defaultSamples = 16;
	int textureWidth = 128;
	int textureHeight = 128;

	TArray<vec3> MeshVertices;
	TArray<int> MeshUVIndex;
	TArray<unsigned int> MeshElements;
	TArray<int> MeshSurfaces;

	std::unique_ptr<TriangleMeshShape> Collision;

private:
	std::vector<std::unique_ptr<ThingLight>> portalLights; // Portal generated lights

	void CreateSubsectorSurfaces(FLevel &doomMap);
	void CreateCeilingSurface(FLevel &doomMap, MapSubsectorEx *sub, IntSector *sector, int typeIndex, bool is3DFloor);
	void CreateFloorSurface(FLevel &doomMap, MapSubsectorEx *sub, IntSector *sector, int typeIndex, bool is3DFloor);
	void CreateSideSurfaces(FLevel &doomMap, IntSideDef *side);

	void BuildSurfaceParams(Surface* surface);
	BBox GetBoundsFromSurface(const Surface* surface);

	void BuildLightLists(FLevel &doomMap);
	void PropagateLight(FLevel& doomMap, ThingLight* thing);

	void BlurSurfaces();
	void FinishSurface(RectPacker& packer, Surface* surface);

	static bool IsDegenerate(const vec3 &v0, const vec3 &v1, const vec3 &v2);

	int CheckAndMakePortal(FLevel& doomMap, MapSubsectorEx* sub, IntSector* sector, int typeIndex, int plane);

	int CreateLinePortal(FLevel &doomMap, const IntLineDef& srcLine, const IntLineDef& dstLine);
	int CreatePlanePortal(FLevel &doomMap, const IntLineDef& srcLine, const IntLineDef& dstLine);
};
