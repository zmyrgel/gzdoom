// 
//---------------------------------------------------------------------------
//
// Copyright(C) 2002-2016 Christoph Oelckers
// All rights reserved.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/
//
//--------------------------------------------------------------------------
//
/*
** gl_light.cpp
** Light level / fog management / dynamic lights
**
*/

#include "gl/system/gl_system.h"
#include "c_dispatch.h"
#include "p_local.h"
#include "p_effect.h"
#include "vectors.h"
#include "gl/gl_functions.h"
#include "g_level.h"

#include "gl/system/gl_cvars.h"
#include "gl/renderer/gl_renderer.h"
#include "gl/renderer/gl_lightdata.h"
#include "gl/data/gl_data.h"
#include "gl/dynlights/gl_dynlight.h"
#include "gl/scene/gl_drawinfo.h"
#include "gl/scene/gl_portal.h"
#include "gl/shaders/gl_shader.h"
#include "gl/textures/gl_material.h"
#include "gl/dynlights/gl_lightbuffer.h"

FDynLightData modellightdata;
int modellightindex = -1;

CVAR(Float, gl_sunlight_x, 1.5, 0);
CVAR(Float, gl_sunlight_y, 1.5, 0);
CVAR(Float, gl_sunlight_z, 2.0, 0);
CVAR(Float, gl_sunlight_str, 0.5, 0);
CVAR(Float, gl_sunlight_r, 1, 0);
CVAR(Float, gl_sunlight_g, 0.95f, 0);
CVAR(Float, gl_sunlight_b, 0.9, 0);

//==========================================================================
//
// Sets a single light value from all dynamic lights affecting the specified location
//
//==========================================================================

void gl_SetDynSpriteLight(AActor *self, float x, float y, float z, subsector_t * subsec)
{
	ADynamicLight *light;
	float frac, lr, lg, lb;
	float radius;
	float out[3] = { 0.0f, 0.0f, 0.0f };
	
	// Go through both light lists
	FLightNode * node = subsec->lighthead;
	while (node)
	{
		light=node->lightsource;
		if (light->visibletoplayer && !(light->flags2&MF2_DORMANT) && (!(light->lightflags&LF_DONTLIGHTSELF) || light->target != self) && !(light->lightflags&LF_DONTLIGHTACTORS))
		{
			float dist;

			// This is a performance critical section of code where we cannot afford to let the compiler decide whether to inline the function or not.
			// This will do the calculations explicitly rather than calling one of AActor's utility functions.
			if (Displacements.size > 0)
			{
				int fromgroup = light->Sector->PortalGroup;
				int togroup = subsec->sector->PortalGroup;
				if (fromgroup == togroup || fromgroup == 0 || togroup == 0) goto direct;

				DVector2 offset = Displacements.getOffset(fromgroup, togroup);
				dist = FVector3(x - light->X() - offset.X, y - light->Y() - offset.Y, z - light->Z()).LengthSquared();
			}
			else
			{
			direct:
				dist = FVector3(x - light->X(), y - light->Y(), z - light->Z()).LengthSquared();
			}

			radius = light->GetRadius();

			if (dist < radius * radius)
			{
				dist = sqrtf(dist);	// only calculate the square root if we really need it.

				frac = 1.0f - (dist / radius);

				if (frac > 0 && GLRenderer->mShadowMap.ShadowTest(light, { x, y, z }))
				{
					lr = light->GetRed() / 255.0f;
					lg = light->GetGreen() / 255.0f;
					lb = light->GetBlue() / 255.0f;
					if (light->IsSubtractive())
					{
						float bright = FVector3(lr, lg, lb).Length();
						FVector3 lightColor(lr, lg, lb);
						lr = (bright - lr) * -1;
						lg = (bright - lg) * -1;
						lb = (bright - lb) * -1;
					}

					out[0] += lr * frac;
					out[1] += lg * frac;
					out[2] += lb * frac;
				}
			}
		}
		node = node->nextLight;
	}
	gl_RenderState.SetDynLight(out[0], out[1], out[2]);
	modellightindex = -1;
}

void gl_SetDynSpriteLight(AActor *thing, particle_t *particle)
{
	if (thing != NULL)
	{
		gl_SetDynSpriteLight(thing, thing->X(), thing->Y(), thing->Center(), thing->subsector);
	}
	else if (particle != NULL)
	{
		gl_SetDynSpriteLight(NULL, particle->Pos.X, particle->Pos.Y, particle->Pos.Z, particle->subsector);
	}
}

void gl_AddFakeSunLight(subsector_t * subsec, FDynLightData &ldata, bool hudmodel)
{
	// Fake contrast/sun light test
	FVector3 sunlightpos;
	sunlightpos.X = gl_sunlight_x * 10000.0;
	sunlightpos.Y = gl_sunlight_y * 10000.0;
	sunlightpos.Z = gl_sunlight_z * 10000.0;
	if (!hudmodel)
	{
		sunlightpos.X = (float)(sunlightpos.X + r_viewpoint.Pos.X);
		sunlightpos.Y = (float)(sunlightpos.Y + r_viewpoint.Pos.Y);
		sunlightpos.Z = (float)(sunlightpos.Z + r_viewpoint.Pos.Z);
	}
	else
	{
		DVector3 rotation;
		DVector3 localpos((double)sunlightpos.X, (double)sunlightpos.Y, (double)sunlightpos.Z);

		rotation.X = localpos.X * r_viewpoint.Angles.Yaw.Sin() - localpos.Y * r_viewpoint.Angles.Yaw.Cos();
		rotation.Y = localpos.X * r_viewpoint.Angles.Yaw.Cos() + localpos.Y * r_viewpoint.Angles.Yaw.Sin();
		rotation.Z = localpos.Z;
		localpos = rotation;

		rotation.X = localpos.X;
		rotation.Y = localpos.Y * r_viewpoint.Angles.Pitch.Sin() - localpos.Z * r_viewpoint.Angles.Pitch.Cos();
		rotation.Z = localpos.Y * r_viewpoint.Angles.Pitch.Cos() + localpos.Z * r_viewpoint.Angles.Pitch.Sin();
		localpos = rotation;

		rotation.Y = localpos.Y;
		rotation.Z = localpos.Z * r_viewpoint.Angles.Roll.Sin() - localpos.X * r_viewpoint.Angles.Roll.Cos();
		rotation.X = localpos.Z * r_viewpoint.Angles.Roll.Cos() + localpos.X * r_viewpoint.Angles.Roll.Sin();
		localpos = rotation;

		sunlightpos.X = localpos.X;
		sunlightpos.Y = localpos.Y;
		sunlightpos.Z = localpos.Z;
	}
	float sunlightradius = 100000.0;
	float sunlightintensity = subsec->sector->lightlevel / 255.0f * gl_sunlight_str;
	float sunlightred = sunlightintensity * gl_sunlight_r;
	float sunlightgreen = sunlightintensity * gl_sunlight_g;
	float sunlightblue = sunlightintensity * gl_sunlight_b;
	float sunlightshadowIndex = GLRenderer->mShadowMap.ShadowMapIndex(nullptr) + 1.0f;
	sunlightshadowIndex = -sunlightshadowIndex;
	float *data = &modellightdata.arrays[0][modellightdata.arrays[0].Reserve(8)];
	data[0] = sunlightpos.X;
	data[1] = sunlightpos.Z;
	data[2] = sunlightpos.Y;
	data[3] = sunlightradius;
	data[4] = sunlightred;
	data[5] = sunlightgreen;
	data[6] = sunlightblue;
	data[7] = sunlightshadowIndex;
}

void gl_SetDynModelLight(AActor *self, float x, float y, float z, subsector_t * subsec, bool hudmodel)
{
	Plane p;
	p.Set(subsec->sector->ceilingplane); // Is this correct?

	modellightdata.Clear();

	gl_AddFakeSunLight(subsec, modellightdata, hudmodel);

	// Go through both light lists
	FLightNode * node = subsec->lighthead;
	while (node)
	{
		ADynamicLight *light = node->lightsource;
		if (light->visibletoplayer && !(light->flags2&MF2_DORMANT) && (!(light->lightflags&LF_DONTLIGHTSELF) || light->target != self) && !(light->lightflags&LF_DONTLIGHTACTORS))
		{
			gl_GetLight(subsec->sector->PortalGroup, p, node->lightsource, false, modellightdata, false, hudmodel);
		}
		node = node->nextLight;
	}

	gl_RenderState.SetDynLight(0, 0, 0);
	modellightindex = GLRenderer->mLights->UploadLights(modellightdata);
}
