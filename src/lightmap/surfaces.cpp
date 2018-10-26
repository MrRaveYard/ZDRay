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
// DESCRIPTION: Prepares geometry from map structures
//
//-----------------------------------------------------------------------------

#include "common.h"
#include "mapData.h"
#include "surfaces.h"

//#define EXPORT_OBJ

kexArray<surface_t*> surfaces;

//
// Surface_AllocateFromSeg
//

static void Surface_AllocateFromSeg(FLevel &doomMap, MapSegGLEx *seg)
{
    IntSideDef *side;
    surface_t *surf;
    float top, bTop;
    float bottom, bBottom;
    IntSector *front;
    IntSector *back;

    if(seg->linedef == NO_LINE_INDEX)
    {
        return;
    }

    side = doomMap.GetSideDef(seg);
    front = doomMap.GetFrontSector(seg);
    back = doomMap.GetBackSector(seg);

    top = front->data.ceilingheight;
    bottom = front->data.floorheight;

    FloatVertex v1 = doomMap.GetSegVertex(seg->v1);
	FloatVertex v2 = doomMap.GetSegVertex(seg->v2);

    if(back)
    {
        bTop = back->data.ceilingheight;
        bBottom = back->data.floorheight;

        if(bTop == top && bBottom == bottom)
        {
            return;
        }

        // bottom seg
        if(bottom < bBottom)
        {
            if(side->bottomtexture[0] != '-')
            {
                surf = (surface_t*)Mem_Calloc(sizeof(surface_t), hb_static);
                surf->numVerts = 4;
                surf->verts = (kexVec3*)Mem_Calloc(sizeof(kexVec3) * 4, hb_static);

                surf->verts[0].x = surf->verts[2].x = v1.x;
                surf->verts[0].y = surf->verts[2].y = v1.y;
                surf->verts[1].x = surf->verts[3].x = v2.x;
                surf->verts[1].y = surf->verts[3].y = v2.y;
                surf->verts[0].z = surf->verts[1].z = bottom;
                surf->verts[2].z = surf->verts[3].z = bBottom;

                surf->plane.SetNormal(surf->verts[0], surf->verts[1], surf->verts[2]);
                surf->plane.SetDistance(surf->verts[0]);
                surf->type = ST_LOWERSEG;
                surf->typeIndex = seg - doomMap.GLSegs;
                surf->subSector = &doomMap.GLSubsectors[doomMap.segLeafLookup[seg - doomMap.GLSegs]];

                doomMap.segSurfaces[1][surf->typeIndex] = surf;
                surf->data = (MapSegGLEx*)seg;

                surfaces.Push(surf);
            }

            bottom = bBottom;
        }

        // top seg
        if(top > bTop)
        {
            bool bSky = false;
            int frontidx = front - &doomMap.Sectors[0];
            int backidx = back - &doomMap.Sectors[0];

            if(doomMap.bSkySectors[frontidx] && doomMap.bSkySectors[backidx])
            {
                if(front->data.ceilingheight != back->data.ceilingheight && side->toptexture[0] == '-')
                {
                    bSky = true;
                }
            }

            if(side->toptexture[0] != '-' || bSky)
            {
                surf = (surface_t*)Mem_Calloc(sizeof(surface_t), hb_static);
                surf->numVerts = 4;
                surf->verts = (kexVec3*)Mem_Calloc(sizeof(kexVec3) * 4, hb_static);

                surf->verts[0].x = surf->verts[2].x = v1.x;
                surf->verts[0].y = surf->verts[2].y = v1.y;
                surf->verts[1].x = surf->verts[3].x = v2.x;
                surf->verts[1].y = surf->verts[3].y = v2.y;
                surf->verts[0].z = surf->verts[1].z = bTop;
                surf->verts[2].z = surf->verts[3].z = top;

                surf->plane.SetNormal(surf->verts[0], surf->verts[1], surf->verts[2]);
                surf->plane.SetDistance(surf->verts[0]);
                surf->type = ST_UPPERSEG;
                surf->typeIndex = seg - doomMap.GLSegs;
                surf->bSky = bSky;
                surf->subSector = &doomMap.GLSubsectors[doomMap.segLeafLookup[seg - doomMap.GLSegs]];

                doomMap.segSurfaces[2][surf->typeIndex] = surf;
                surf->data = (MapSegGLEx*)seg;

                surfaces.Push(surf);
            }

            top = bTop;
        }
    }

    // middle seg
    if(back == NULL)
    {
        surf = (surface_t*)Mem_Calloc(sizeof(surface_t), hb_static);
        surf->numVerts = 4;
        surf->verts = (kexVec3*)Mem_Calloc(sizeof(kexVec3) * 4, hb_static);

        surf->verts[0].x = surf->verts[2].x = v1.x;
        surf->verts[0].y = surf->verts[2].y = v1.y;
        surf->verts[1].x = surf->verts[3].x = v2.x;
        surf->verts[1].y = surf->verts[3].y = v2.y;
        surf->verts[0].z = surf->verts[1].z = bottom;
        surf->verts[2].z = surf->verts[3].z = top;

        surf->plane.SetNormal(surf->verts[0], surf->verts[1], surf->verts[2]);
        surf->plane.SetDistance(surf->verts[0]);
        surf->type = ST_MIDDLESEG;
        surf->typeIndex = seg - doomMap.GLSegs;
        surf->subSector = &doomMap.GLSubsectors[doomMap.segLeafLookup[seg - doomMap.GLSegs]];

        doomMap.segSurfaces[0][surf->typeIndex] = surf;
        surf->data = (MapSegGLEx*)seg;

        surfaces.Push(surf);
    }
}

//
// Surface_AllocateFromLeaf
//
// Plane normals should almost always be known
// unless slopes are involved....
//

static void Surface_AllocateFromLeaf(FLevel &doomMap)
{
    surface_t *surf;
    leaf_t *leaf;
    IntSector *sector = NULL;
    int i;
    int j;

    printf("------------- Building leaf surfaces -------------\n");

    doomMap.leafSurfaces[0] = (surface_t**)Mem_Calloc(sizeof(surface_t*) * doomMap.NumGLSubsectors, hb_static);
    doomMap.leafSurfaces[1] = (surface_t**)Mem_Calloc(sizeof(surface_t*) * doomMap.NumGLSubsectors, hb_static);

    for(i = 0; i < doomMap.NumGLSubsectors; i++)
    {
        printf("subsectors: %i / %i\r", i+1, doomMap.NumGLSubsectors);

        if(doomMap.ssLeafCount[i] < 3)
        {
            continue;
        }

        sector = doomMap.GetSectorFromSubSector(&doomMap.GLSubsectors[i]);

        // I will be NOT surprised if some users tries to do something stupid with
        // sector hacks
        if(sector == NULL)
        {
            Error("Surface_AllocateFromLeaf: subsector %i has no sector\n", i);
            return;
        }

        surf = (surface_t*)Mem_Calloc(sizeof(surface_t), hb_static);
        surf->numVerts = doomMap.ssLeafCount[i];
        surf->verts = (kexVec3*)Mem_Calloc(sizeof(kexVec3) * surf->numVerts, hb_static);
        surf->subSector = &doomMap.GLSubsectors[i];

        // floor verts
        for(j = 0; j < surf->numVerts; j++)
        {
            leaf = &doomMap.leafs[doomMap.ssLeafLookup[i] + (surf->numVerts - 1) - j];

            surf->verts[j].x = leaf->vertex.x;
            surf->verts[j].y = leaf->vertex.y;
            surf->verts[j].z = sector->data.floorheight;
        }

        surf->plane.SetNormal(kexVec3(0, 0, 1));
        surf->plane.SetDistance(surf->verts[0]);
        surf->type = ST_FLOOR;
        surf->typeIndex = i;

        doomMap.leafSurfaces[0][i] = surf;
        surf->data = (IntSector*)sector;

        surfaces.Push(surf);

        surf = (surface_t*)Mem_Calloc(sizeof(surface_t), hb_static);
        surf->numVerts = doomMap.ssLeafCount[i];
        surf->verts = (kexVec3*)Mem_Calloc(sizeof(kexVec3) * surf->numVerts, hb_static);
        surf->subSector = &doomMap.GLSubsectors[i];

        if(doomMap.bSkySectors[sector-&doomMap.Sectors[0]])
        {
            surf->bSky = true;
        }

        // ceiling verts
        for(j = 0; j < surf->numVerts; j++)
        {
            leaf = &doomMap.leafs[doomMap.ssLeafLookup[i] + j];

            surf->verts[j].x = leaf->vertex.x;
            surf->verts[j].y = leaf->vertex.y;
            surf->verts[j].z = sector->data.ceilingheight;
        }

        surf->plane.SetNormal(kexVec3(0, 0, -1));
        surf->plane.SetDistance(surf->verts[0]);
        surf->type = ST_CEILING;
        surf->typeIndex = i;

        doomMap.leafSurfaces[1][i] = surf;
        surf->data = (IntSector*)sector;

        surfaces.Push(surf);
    }

    printf("\nLeaf surfaces: %i\n", surfaces.Length() - doomMap.NumGLSubsectors);
}

//
// Surface_AllocateFromMap
//

void Surface_AllocateFromMap(FLevel &doomMap)
{
    doomMap.segSurfaces[0] = (surface_t**)Mem_Calloc(sizeof(surface_t*) * doomMap.NumGLSegs, hb_static);
    doomMap.segSurfaces[1] = (surface_t**)Mem_Calloc(sizeof(surface_t*) * doomMap.NumGLSegs, hb_static);
    doomMap.segSurfaces[2] = (surface_t**)Mem_Calloc(sizeof(surface_t*) * doomMap.NumGLSegs, hb_static);

    printf("------------- Building seg surfaces -------------\n");

    for(int i = 0; i < doomMap.NumGLSegs; i++)
    {
        Surface_AllocateFromSeg(doomMap, &doomMap.GLSegs[i]);
        printf("segs: %i / %i\r", i+1, doomMap.NumGLSegs);
    }

    printf("\nSeg surfaces: %i\n", surfaces.Length());

#ifdef EXPORT_OBJ
    FILE *f = fopen("level.obj", "w");
    int curLen = surfaces.Length();
    for(unsigned int i = 0; i < surfaces.Length(); i++)
    {
        for(int j = 0; j < surfaces[i]->numVerts; j++)
        {
            fprintf(f, "v %f %f %f\n",
                    -surfaces[i]->verts[j].y / 256.0f,
                    surfaces[i]->verts[j].z / 256.0f,
                    -surfaces[i]->verts[j].x / 256.0f);
        }
    }

    int tri;

    for(unsigned int i = 0; i < surfaces.Length(); i++)
    {
        fprintf(f, "o surf%i_seg%i\n", i, i);
        fprintf(f, "f %i %i %i\n", 0+(i*4)+1, 1+(i*4)+1, 2+(i*4)+1);
        fprintf(f, "f %i %i %i\n", 1+(i*4)+1, 3+(i*4)+1, 2+(i*4)+1);

        tri = 3+(i*4)+1;
    }

    tri++;
#endif

    Surface_AllocateFromLeaf(doomMap);

    printf("Surfaces total: %i\n\n", surfaces.Length());

#ifdef EXPORT_OBJ
    for(unsigned int i = curLen; i < surfaces.Length(); i++)
    {
        for(int j = 0; j < surfaces[i]->numVerts; j++)
        {
            fprintf(f, "v %f %f %f\n",
                    -surfaces[i]->verts[j].y / 256.0f,
                    surfaces[i]->verts[j].z / 256.0f,
                    -surfaces[i]->verts[j].x / 256.0f);
        }
    }

    for(unsigned int i = curLen; i < surfaces.Length(); i++)
    {
        fprintf(f, "o surf%i_ssect%i\n", i, i - curLen);
        fprintf(f, "f ");
        for(int j = 0; j < surfaces[i]->numVerts; j++)
        {
            fprintf(f, "%i ", tri++);
        }
        fprintf(f, "\n");
    }

    fclose(f);
#endif
}
