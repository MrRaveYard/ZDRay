#pragma once

#include "lightmap/levelmesh.h"
#include "math/mathlib.h"
#include "delauneytriangulator.h"

class SurfaceClip
{
	std::vector<vec2> vertices;

	mat3 inverseProjection;

	float sampleWidth;
	float sampleHeight;

	BBox bounds;
	float boundsWidth;
	float boundsHeight;
	float offsetW;
	float offsetH;
	float tolerance;

	// Local space
	bool PointInBounds(const vec2& p, float tolerance) const;
public:
	SurfaceClip(Surface* surface);

	// Task XY space. Tolerates points close enough to the surface to avoid missing used samples
	bool SampleIsInBounds(float x, float y) const;
	bool WorldPositionInBounds(const vec3& p) const;

	vec2 MoveSampleOriginToSurfaceBounds(float x, float y) const;

};