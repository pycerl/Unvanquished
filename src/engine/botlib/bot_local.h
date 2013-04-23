/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2012 Unvanquished Developers

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

extern "C"
{
#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
}

#include "../../libs/detour/DetourNavMeshBuilder.h"
#include "../../libs/detour/DetourNavMeshQuery.h"
#include "../../libs/detour/DetourPathCorridor.h"
#include "../../libs/detour/DetourCommon.h"
#include "../../libs/detour/DetourTileCache.h"
#include "../../libs/detour/DetourTileCacheBuilder.h"

#include "bot_types.h"
#include "bot_api.h"

const int MAX_NAV_DATA = 16;
const int MAX_BOT_PATH = 512;
const int MAX_PATH_LOOKAHEAD = 5;
const int MAX_CORNERS = 5;

typedef struct
{
	dtTileCache      *cache;
	dtNavMesh        *mesh;
	dtNavMeshQuery   *query;
	dtQueryFilter    filter;
} NavData_t;

typedef struct
{
	NavData_t         *nav;
	dtPathCorridor    corridor;
	int               clientNum;
	qboolean          needReplan;
	int               lastRouteTime;
	float             cornerVerts[ MAX_CORNERS * 3 ];
	unsigned char     cornerFlags[ MAX_CORNERS ];
	dtPolyRef         cornerPolys[ MAX_CORNERS ];
	int               numCorners;
} Bot_t;

extern int numNavData;
extern NavData_t BotNavData[ MAX_NAV_DATA ];
extern Bot_t agents[ MAX_CLIENTS ];

//coordinate conversion
static inline void quake2recast( vec3_t vec )
{
	vec_t temp = vec[1];
	vec[0] = -vec[0];
	vec[1] = vec[2];
	vec[2] = -temp;
}

static inline void recast2quake( vec3_t vec )
{
	vec_t temp = vec[1];
	vec[0] = -vec[0];
	vec[1] = -vec[2];
	vec[2] = temp;
}

static inline void recast2quakeExtents( vec3_t qmins, vec3_t qmaxs )
{
	vec3_t mins, maxs;
	VectorCopy( qmins, mins );
	VectorCopy( qmaxs, maxs );

	recast2quake( mins );
	recast2quake( maxs );

	ClearBounds( qmins, qmaxs );
	AddPointToBounds( mins, qmins, qmaxs );
	AddPointToBounds( maxs, qmins, qmaxs );
}

static inline void quake2recastExtents( vec3_t qmins, vec3_t qmaxs )
{
	vec3_t mins, maxs;
	VectorCopy( qmins, mins );
	VectorCopy( qmaxs, maxs );

	quake2recast( mins );
	quake2recast( maxs );

	ClearBounds( qmins, qmaxs );
	AddPointToBounds( mins, qmins, qmaxs );
	AddPointToBounds( maxs, qmins, qmaxs );
}

static inline void quake2recastTarget( botRouteTarget_t *target )
{
	quake2recast( target->pos );
	quake2recast( target->polyExtents );
	target->polyExtents[ 0 ] = fabsf( target->polyExtents[ 0 ] );
	target->polyExtents[ 1 ] = fabsf( target->polyExtents[ 1 ] );
	target->polyExtents[ 2 ] = fabsf( target->polyExtents[ 2 ] );
	quake2recastExtents( target->mins, target->maxs );
}

// all functions here use detour's coordinate system
// callers should use quake2recast and recast2quake where appropriate to convert vectors
void         BotCalcSteerDir( Bot_t *bot, vec3_t dir );
void         FindWaypoints( Bot_t *bot, float *corners, unsigned char *cornerFlags, dtPolyRef *cornerPolys, int *numCorners, int maxCorners );
qboolean     PointInPolyExtents( Bot_t *bot, dtPolyRef ref, const vec3_t point, const vec3_t mins, const vec3_t maxs );
qboolean     PointInPoly( Bot_t *bot, dtPolyRef ref, const vec3_t point );
qboolean     BotFindNearestPoly( Bot_t *bot, const vec3_t coord, dtPolyRef *nearestPoly, vec3_t nearPoint );
unsigned int FindRoute( Bot_t *bot, const vec3_t s, const botRouteTarget_t *target );