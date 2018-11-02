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
//-----------------------------------------------------------------------------
//
// DESCRIPTION: Special surfaces that contains origin points used for
//              emitting light. Surfaces can be subdivided for more
//              accurate light casting
//
//-----------------------------------------------------------------------------

#include "common.h"
#include "mapdata.h"
#include "trace.h"
#include "lightsurface.h"

kexLightSurface::kexLightSurface()
{
}

kexLightSurface::~kexLightSurface()
{
}

void kexLightSurface::Init(const surfaceLightDef &lightSurfaceDef, surface_t *surface, const bool bWall, const bool bNoCenterPoint)
{
	this->intensity = lightSurfaceDef.intensity;
	this->distance = lightSurfaceDef.distance;
	this->rgb = lightSurfaceDef.rgb;
	this->surface = surface;
	this->bWall = bWall;
	this->bNoCenterPoint = bNoCenterPoint;
}

// Creates a single origin point if we're not intending on subdividing this light surface
void kexLightSurface::CreateCenterOrigin()
{
	if (!bWall)
	{
		kexVec3 center;

		for (int i = 0; i < surface->numVerts; ++i)
		{
			center += surface->verts[i];
		}

		origins.Push(center / (float)surface->numVerts);
	}
	else
	{
		origins.Push(kexVec3((surface->verts[1].x + surface->verts[0].x) * 0.5f,
			(surface->verts[1].y + surface->verts[0].y) * 0.5f,
			(surface->verts[2].z + surface->verts[0].z) * 0.5f));
	}
}

// Splits surface vertices into two groups while adding new ones caused by the split
void kexLightSurface::Clip(vertexBatch_t &points, const kexVec3 &normal, float dist, vertexBatch_t *frontPoints, vertexBatch_t *backPoints)
{
	kexArray<float> dists;
	kexArray<char> sides;

	// determines what sides the vertices lies on
	for (unsigned int i = 0; i < points.Length(); ++i)
	{
		float d = points[i].Dot(normal) - dist;

		dists.Push(d);

		if (d > 0.1f)
		{
			sides.Push(1);      // front
		}
		else if (d < -0.1f)
		{
			sides.Push(-1);     // directly on the split plane
		}
		else
		{
			sides.Push(0);      // back
		}
	}

	// add points
	for (unsigned int i = 0; i < points.Length(); ++i)
	{
		int next;
		float frac;
		kexVec3 pt1, pt2, pt3;

		switch (sides[i])
		{
		case -1:
			backPoints->Push(points[i]);
			break;

		case 1:
			frontPoints->Push(points[i]);
			break;

		default:
			frontPoints->Push(points[i]);
			backPoints->Push(points[i]);

			// point is on the split plane so no new split vertex is needed
			continue;

		}

		// check if the edge crosses the split plane
		next = (i + 1) % points.Length();

		if (sides[next] == 0 || sides[next] == sides[i])
		{
			// didn't cross
			continue;
		}

		pt1 = points[i];
		pt2 = points[next];

		// insert a new point caused by the split
		frac = dists[i] / (dists[i] - dists[next]);
		pt3 = pt1.Lerp(pt2, frac);

		frontPoints->Push(pt3);
		backPoints->Push(pt3);
	}
}

// Recursively divides the surface
bool kexLightSurface::SubdivideRecursion(vertexBatch_t &surfPoints, float divide, kexArray<vertexBatch_t*> &points)
{
	kexBBox bounds;
	kexVec3 splitNormal;
	float dist;
	vertexBatch_t *frontPoints;
	vertexBatch_t *backPoints;

	// get bounds from current set of points
	for (unsigned int i = 0; i < surfPoints.Length(); ++i)
	{
		bounds.AddPoint(surfPoints[i]);
	}

	for (int i = 0; i < 3; ++i)
	{
		// check if its large enough to be divided
		if ((bounds.max[i] - bounds.min[i]) > divide)
		{
			splitNormal.Clear();
			splitNormal[i] = 1;

			dist = (bounds.max[i] + bounds.min[i]) * 0.5f;

			frontPoints = new vertexBatch_t;
			backPoints = new vertexBatch_t;

			// start clipping
			Clip(surfPoints, splitNormal, dist, frontPoints, backPoints);

			if (!SubdivideRecursion(*frontPoints, divide, points))
			{
				points.Push(frontPoints);
			}
			else
			{
				delete frontPoints;
			}

			if (!SubdivideRecursion(*backPoints, divide, points))
			{
				points.Push(backPoints);
			}
			else
			{
				delete backPoints;
			}

			return true;
		}
	}

	return false;
}

void kexLightSurface::Subdivide(const float divide)
{
	kexArray<vertexBatch_t*> points;
	vertexBatch_t surfPoints;

	for (int i = 0; i < surface->numVerts; ++i)
	{
		surfPoints.Push(surface->verts[i]);
	}

	SubdivideRecursion(surfPoints, divide, points);

	// from each group of vertices caused by the split, begin
	// creating a origin point based on the center of that group
	for (unsigned int i = 0; i < points.Length(); ++i)
	{
		vertexBatch_t *vb = points[i];
		kexVec3 center;

		for (unsigned int j = 0; j < vb->Length(); ++j)
		{
			center += (*vb)[j];
		}

		origins.Push(center / (float)vb->Length());
	}

	for (unsigned int i = 0; i < points.Length(); ++i)
	{
		vertexBatch_t *vb = points[i];

		vb->Empty();
		delete vb;
	}
}

float kexLightSurface::TraceSurface(FLevel *doomMap, kexTrace &trace, const surface_t *surf, const kexVec3 &origin)
{
	// light surface will always be fullbright
	if (surf == surface)
	{
		return 1.0f;
	}

	kexVec3 lnormal = surface->plane.Normal();

	kexVec3 normal;
	if (surf)
	{
		normal = surf->plane.Normal();

		if (normal.Dot(lnormal) > 0)
		{
			// not facing the light surface
			return 0.0f;
		}
	}
	else
	{
		normal = kexVec3::vecUp;
	}

	float total = 0.0f;
	for (unsigned int i = 0; i < origins.Length(); ++i)
	{
		kexVec3 center = origins[i];

		if (!bWall && origin.z > center.z)
		{
			// origin is not going to seen or traced by the light surface
			// so don't even bother. this also fixes some bizzare light
			// bleeding issues
			continue;
		}

		kexVec3 dir = (origin - center).Normalize();
		float attenuation = dir.Dot(lnormal);

		if (attenuation <= 0.0f)
			continue; // not even facing the light surface

		if (bWall)
		{
			if (origin.z >= surface->verts[0].z && origin.z <= surface->verts[2].z)
			{
				// since walls are always vertically straight, we can cheat a little by adjusting
				// the sampling point height. this also allows us to do accurate light emitting
				// while just using one sample point
				center.z = origin.z;
			}
		}

		// trace the origin to the center of the light surface. nudge by the normals in
		// case the start/end points are directly on or inside the surface
		trace.Trace(center + lnormal, origin + normal);

		if (trace.fraction < 1.0f)
		{
			// something is obstructing it
			continue;
		}

		float d = origin.Distance(center);
		attenuation *= 1.0f - d / (distance * 2.0f); // 2.0 because gzdoom's dynlights do this and we want them to match
		if (attenuation > 0.0f)
			total += attenuation;
	}

	return total / origins.Length();
}
