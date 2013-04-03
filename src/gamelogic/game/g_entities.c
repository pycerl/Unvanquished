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

#include "g_local.h"
#include "g_entities.h"

/*
=================================================================================

basic gentity lifecycle handling

=================================================================================
*/

void G_InitGentity( gentity_t *entity )
{
	entity->inuse = qtrue;
	entity->enabled = qtrue;
	entity->classname = "noclass";
	entity->s.number = entity - g_entities;
	entity->r.ownerNum = ENTITYNUM_NONE;
	entity->creationTime = level.time;
}

/*
=================
G_Spawn

Either finds a free entity, or allocates a new one.

  The slots from 0 to MAX_CLIENTS-1 are always reserved for clients, and will
never be used by anything else.

Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
gentity_t *G_NewEntity( void )
{
	int       i, force;
	gentity_t *newEntity;

	newEntity = NULL; // shut up warning
	i = 0; // shut up warning

	for ( force = 0; force < 2; force++ )
	{
		// if we go through all entities first and can't find a free one,
		// then try again a second time, this time ignoring times
		newEntity = &g_entities[ MAX_CLIENTS ];

		for ( i = MAX_CLIENTS; i < level.num_entities; i++, newEntity++ )
		{
			if ( newEntity->inuse )
			{
				continue;
			}

			// the first couple seconds of server time can involve a lot of
			// freeing and allocating, so relax the replacement policy
			if ( !force && newEntity->freetime > level.startTime + 2000 && level.time - newEntity->freetime < 1000 )
			{
				continue;
			}

			// reuse this slot
			G_InitGentity( newEntity );
			return newEntity;
		}

		if ( i != MAX_GENTITIES )
		{
			break;
		}
	}

	if ( i == ENTITYNUM_MAX_NORMAL )
	{
		for ( i = 0; i < MAX_GENTITIES; i++ )
		{
			G_Printf( "%4i: %s\n", i, g_entities[ i ].classname );
		}

		G_Error( "G_Spawn: no free entities" );
	}

	// open up a new slot
	level.num_entities++;

	// let the server system know that there are more entities
	trap_LocateGameData( level.gentities, level.num_entities, sizeof( gentity_t ),
	                     &level.clients[ 0 ].ps, sizeof( level.clients[ 0 ] ) );

	G_InitGentity( newEntity );
	return newEntity;
}

/*
=================
G_FreeEntity

Marks the entity as free
=================
*/
void G_FreeEntity( gentity_t *entity )
{
	trap_UnlinkEntity( entity );  // unlink from world

	if ( entity->neverFree )
	{
		return;
	}

	if ( g_debugEntities.integer > 2 )
		G_Printf(S_DEBUG "Freeing Entity %s\n", etos(entity));

	if( entity->eclass && entity->eclass->instanceCounter > 0)
		entity->eclass->instanceCounter--;

	memset( entity, 0, sizeof( *entity ) );
	entity->classname = "freent";
	entity->freetime = level.time;
	entity->inuse = qfalse;
}


/*
=================
G_TempEntity

Spawns an event entity that will be auto-removed
The origin will be snapped to save net bandwidth, so care
must be taken if the origin is right on a surface (snap towards start vector first)
=================
*/
gentity_t *G_NewTempEntity( const vec3_t origin, int event )
{
	gentity_t *newEntity;
	vec3_t    snapped;

	newEntity = G_NewEntity();
	newEntity->s.eType = ET_EVENTS + event;

	newEntity->classname = "tempEntity";
	newEntity->eventTime = level.time;
	newEntity->freeAfterEvent = qtrue;

	VectorCopy( origin, snapped );
	SnapVector( snapped );  // save network bandwidth
	G_SetOrigin( newEntity, snapped );

	// find cluster for PVS
	trap_LinkEntity( newEntity );

	return newEntity;
}

/*
=================================================================================

gentity debuging

=================================================================================
*/

/*
=============
EntityToString

Convenience function for printing entities
=============
*/
//assuming MAX_GENTITIES to be 5 digits or less
#define MAX_ETOS_LENGTH MAX_NAME_LENGTH + 5 * 2 + 4 + 1 + 5
char *etos( const gentity_t *entity )
{
	static  int  index;
	static  char str[ 4 ][ MAX_ETOS_LENGTH ];
	char         *resultString;

	if(!entity)
		return "<NULL>";

	// use an array so that multiple etos have smaller chance of colliding
	resultString = str[ index ];
	index = ( index + 1 ) & 3;

	Com_sprintf( resultString, MAX_ETOS_LENGTH,
			"%s%s" S_COLOR_WHITE "(" S_COLOR_CYAN "%s" S_COLOR_WHITE "|" S_COLOR_CYAN "#%i" S_COLOR_WHITE ")",
			entity->names[0], entity->names[0] ? " " : "", entity->classname, entity->s.number
			);

	return resultString;
}

void G_PrintEntityNameList(gentity_t *entity)
{
	int i;
	char string[ MAX_STRING_CHARS ];
	char nameSegment[ MAX_STRING_CHARS ];

	if(!entity)
	{
		G_Printf("<NULL>");
		return;
	}
	if(!entity->names[0])
	{
		return;
	}

	for (i = 0; i < MAX_ENTITY_ALIASES && entity->names[i]; ++i)
	{
		Com_sprintf(nameSegment, sizeof(nameSegment), "%s%s", i == 0 ? "": ", ", entity->names[i]);
		Q_strcat(string, sizeof(string), nameSegment);
	}
	G_Printf("{ %s }\n", string);
}

/*
=================================================================================

gentity list handling and searching

=================================================================================
*/

/*
=============
G_IterateEntities

Iterates through all active enities optionally filtered by classname
and a fieldoffset (set via FOFS() macro) of the callers choosing.

Iteration will continue to return the gentity following the "previous" parameter that fullfill these conditions
or NULL if there are no further matching gentities.

Set NULL as previous gentity to start the iteration from the beginning
=============
*/
gentity_t *G_IterateEntities( gentity_t *entity, const char *classname, qboolean skipdisabled, size_t fieldofs, const char *match )
{
	char *fieldString;

	if ( !entity )
	{
		entity = g_entities;
		//start after the reserved player slots, if we are not searching for a player
		if ( classname && !strcmp(classname, S_PLAYER_CLASSNAME) )
			entity += MAX_CLIENTS;
	}
	else
	{
		entity++;
	}

	for ( ; entity < &g_entities[ level.num_entities ]; entity++ )
	{
		if ( !entity->inuse )
			continue;

		if( skipdisabled && !entity->enabled)
			continue;


		if ( classname && Q_stricmp( entity->classname, classname ) )
			continue;

		if ( fieldofs && match )
		{
			fieldString = * ( char ** )( ( byte * ) entity + fieldofs );
			if ( Q_stricmp( fieldString, match ) )
				continue;
		}

		return entity;
	}

	return NULL;
}

gentity_t *G_IterateEntitiesOfClass( gentity_t *entity, const char *classname )
{
	return G_IterateEntities( entity, classname, qtrue, 0, NULL );
}

/*
=============
G_IterateEntitiesWithField

Searches all active entities for the next one that holds
the matching string at fieldofs (use the FOFS() macro) in the structure.

Searches beginning at the entity after from, or the beginning if NULL
NULL will be returned if the end of the list is reached.

if we are not searching for player entities it is recommended to start searching from gentities[MAX_CLIENTS - 1]

=============
*/
gentity_t *G_IterateEntitiesWithField( gentity_t *entity, size_t fieldofs, const char *match )
{
	return G_IterateEntities( entity, NULL, qtrue, fieldofs, match );
}

// from quakestyle.telefragged.com
// (NOBODY): Code helper function
//
gentity_t *G_IterateEntitiesWithinRadius( gentity_t *entity, vec3_t origin, float radius )
{
	vec3_t eorg;
	int    j;

	if ( !entity )
	{
		entity = g_entities;
	}
	else
	{
		entity++;
	}

	for ( ; entity < &g_entities[ level.num_entities ]; entity++ )
	{
		if ( !entity->inuse )
		{
			continue;
		}

		for ( j = 0; j < 3; j++ )
		{
			eorg[ j ] = origin[ j ] - ( entity->r.currentOrigin[ j ] + ( entity->r.mins[ j ] + entity->r.maxs[ j ] ) * 0.5 );
		}

		if ( VectorLength( eorg ) > radius )
		{
			continue;
		}

		return entity;
	}

	return NULL;
}

/*
===============
G_ClosestEnt

Test a list of entities for the closest to a particular point
===============
*/
gentity_t *G_FindClosestEntity( vec3_t origin, gentity_t **entities, int numEntities )
{
	int       i;
	float     nd, d;
	gentity_t *closestEnt;

	if ( numEntities <= 0 )
	{
		return NULL;
	}

	closestEnt = entities[ 0 ];
	d = DistanceSquared( origin, closestEnt->s.origin );

	for ( i = 1; i < numEntities; i++ )
	{
		gentity_t *ent = entities[ i ];

		nd = DistanceSquared( origin, ent->s.origin );

		if ( nd < d )
		{
			d = nd;
			closestEnt = ent;
		}
	}

	return closestEnt;
}

gentity_t *G_PickRandomEntity( const char *classname, size_t fieldofs, const char *match )
{
	gentity_t *foundEntity = NULL;
	int       totalChoiceCount = 0;
	gentity_t *choices[ MAX_GENTITIES - 2 - MAX_CLIENTS ];

	//collects the targets
	while( ( foundEntity = G_IterateEntities( foundEntity, classname, qtrue, fieldofs, match ) ) != NULL )
		choices[ totalChoiceCount++ ] = foundEntity;

	if ( !totalChoiceCount )
	{

		if ( g_debugEntities.integer > -1 )
			G_Printf( S_WARNING "Could not find any entity matching \"" S_COLOR_CYAN "%s" S_COLOR_WHITE "\"\n", match );

		return NULL;
	}

	//return a random one from among the choices
	return choices[ rand() / ( RAND_MAX / totalChoiceCount + 1 ) ];
}

gentity_t *G_PickRandomEntityOfClass( const char *classname )
{
	return G_PickRandomEntity(classname, 0, NULL);
}

gentity_t *G_PickRandomEntityWithField( size_t fieldofs, const char *match )
{
	return G_PickRandomEntity(NULL, fieldofs, match);
}

/*
=================================================================================

gentity chain handling

=================================================================================
*/

typedef struct
{
	const char *key;
	gentityCallEvent_t eventType;
} entityCallEventDescription_t;

static const entityCallEventDescription_t gentityEventDescriptions[] =
{
		{ "onAct",       ON_ACT       },
		{ "onDie",       ON_DIE       },
		{ "onDisable",   ON_DISABLE   },
		{ "onEnable",    ON_ENABLE    },
		{ "onFree",      ON_FREE      },
		{ "onReach",     ON_REACH     },
		{ "onReset",     ON_RESET     },
		{ "onSpawn",     ON_SPAWN     },
		{ "onTouch",     ON_TOUCH     },
		{ "onUse",       ON_USE       },
		{ "target",      ON_DEFAULT   },
};

gentityCallEvent_t G_GetCallEventTypeFor( const char* event )
{
	entityCallEventDescription_t *foundDescription;

	if(!event)
		return ON_DEFAULT;

	foundDescription = bsearch(event, gentityEventDescriptions, ARRAY_LEN( gentityEventDescriptions ),
		             sizeof( entityCallEventDescription_t ), cmdcmp );

	if(foundDescription && foundDescription->key)
		return foundDescription->eventType;

	return ON_CUSTOM;
}

typedef struct
{
	const char *alias;
	gentityCallActionType_t action;
} entityActionDescription_t;

static const entityActionDescription_t actionDescriptions[] =
{
		{ "act",       ECA_ACT       },
		{ "disable",   ECA_DISABLE   },
		{ "enable",    ECA_ENABLE    },
		{ "free",      ECA_FREE      },
		{ "nop",       ECA_NOP       },
		{ "propagate", ECA_PROPAGATE },
		{ "reset",     ECA_RESET     },
		{ "toggle",    ECA_TOGGLE    },
		{ "use",       ECA_USE       },
};

gentityCallActionType_t G_GetCallActionTypeFor( const char* action )
{
	entityActionDescription_t *foundDescription;

	if(!action)
		return ECA_DEFAULT;

	foundDescription = bsearch(action, actionDescriptions, ARRAY_LEN( actionDescriptions ),
		             sizeof( entityActionDescription_t ), cmdcmp );

	if(foundDescription && foundDescription->alias)
		return foundDescription->action;

	return ECA_CUSTOM;
}

gentity_t *G_ResolveEntityKeyword( gentity_t *self, char *keyword )
{
	gentity_t *resolution = NULL;

	if (!Q_stricmp(keyword, "$activator"))
		resolution = self->activator;
	else if (!Q_stricmp(keyword, "$self"))
		resolution = self;
	else if (!Q_stricmp(keyword, "$parent"))
		resolution = self->parent;
	else if (!Q_stricmp(keyword, "$target"))
		resolution = self->target;
	else if (!Q_stricmp(keyword, "$tracker"))
		resolution = self->tracker;

	if(!resolution->inuse)
		return NULL;

	return resolution;
}

gentity_t *G_IterateTargets(gentity_t *entity, int *targetIndex, gentity_t *self)
{
	gentity_t *possibleTarget = NULL;

	if (entity)
		goto cont;

	for (*targetIndex = 0; self->targets[*targetIndex]; ++(*targetIndex))
	{
		if(self->targets[*targetIndex][0] == '$')
		{
			possibleTarget = G_ResolveEntityKeyword( self, self->targets[*targetIndex] );
			if(possibleTarget && possibleTarget->enabled)
				return possibleTarget;
			return NULL;
		}

		for( entity = &g_entities[ MAX_CLIENTS ]; entity < &g_entities[ level.num_entities ]; entity++ )
		{
			if ( !entity->inuse || !entity->enabled)
				continue;

			if( G_MatchesName(entity, self->targets[*targetIndex]) )
				return entity;

			cont: ;
		}
	}
	return NULL;
}

gentity_t *G_IterateCallEndpoints(gentity_t *entity, int *calltargetIndex, gentity_t *self)
{
	if (entity)
		goto cont;

	for (*calltargetIndex = 0; self->calltargets[*calltargetIndex].name; ++(*calltargetIndex))
	{
		if(self->calltargets[*calltargetIndex].name[0] == '$')
			return G_ResolveEntityKeyword( self, self->calltargets[*calltargetIndex].name );

		for( entity = &g_entities[ MAX_CLIENTS ]; entity < &g_entities[ level.num_entities ]; entity++ )
		{
			if ( !entity->inuse )
				continue;

			if( G_MatchesName(entity, self->calltargets[*calltargetIndex].name) )
				return entity;

			cont: ;
		}
	}
	return NULL;
}

/**
 * G_PickRandomTargetFor
 * Selects a random entity from among the targets
 */
gentity_t *G_PickRandomTargetFor( gentity_t *self )
{
	int       targetIndex;
	gentity_t *foundTarget = NULL;
	int       totalChoiceCount = 0;
	gentity_t *choices[ MAX_GENTITIES ];

	//collects the targets
	while( ( foundTarget = G_IterateTargets( foundTarget, &targetIndex, self ) ) != NULL )
		choices[ totalChoiceCount++ ] = foundTarget;

	if ( !totalChoiceCount )
	{
		if ( g_debugEntities.integer > -1 )
		{
			G_Printf( S_WARNING "none of the following targets could be resolved for Entity %s:", etos(self));
			G_PrintEntityNameList( self );
		}
		return NULL;
	}

	//return a random one from among the choices
	return choices[ rand() / ( RAND_MAX / totalChoiceCount + 1 ) ];
}

typedef struct
{
	gentityCallDefinition_t *callDefinition;
	gentity_t *recipient;
} gentityTargetChoice_t;

void G_FireEntityRandomly( gentity_t *entity, gentity_t *activator )
{
	int       targetIndex;
	gentity_t *possibleTarget = NULL;
	int       totalChoiceCount = 0;
	gentityCall_t call;
	gentityTargetChoice_t choices[ MAX_GENTITIES ];
	gentityTargetChoice_t *selectedChoice;

	//collects the targets
	while( ( possibleTarget = G_IterateCallEndpoints( possibleTarget, &targetIndex, entity ) ) != NULL )
	{
		choices[ totalChoiceCount ].recipient = possibleTarget;
		choices[ totalChoiceCount ].callDefinition = &entity->calltargets[targetIndex];
		totalChoiceCount++;
	}

	//return a random one from among the choices
	selectedChoice = &choices[ rand() / ( RAND_MAX / totalChoiceCount + 1 ) ];
	if (!selectedChoice)
		return;

	call.definition = selectedChoice->callDefinition;
	call.caller = entity;
	call.activator = activator;

	G_CallEntity( selectedChoice->recipient, &call );
}

/*
==============================
G_FireAllTargetsOf

"activator" should be set to the entity that initiated the firing.

For all t in the entities, where t.targetnames[i] matches
ent.targets[j] for any (i,j) pairs, call the t.use function.
==============================
*/
void G_EventFireEntity( gentity_t *self, gentity_t *activator, gentityCallEvent_t eventType )
{
	gentity_t *currentTarget = NULL;
	int targetIndex;
	gentityCall_t call;
	call.activator = activator;

	if ( self->shaderKey && self->shaderReplacement )
	{
		G_SetShaderRemap( self->shaderKey, self->shaderReplacement, level.time * 0.001 );
		trap_SetConfigstring( CS_SHADERSTATE, BuildShaderStateConfig() );
	}

	while( ( currentTarget = G_IterateCallEndpoints( currentTarget, &targetIndex, self ) ) != NULL )
	{
		if( eventType && self->calltargets[ targetIndex ].eventType != eventType )
		{
			continue;
		}

		call.caller = self; //reset the caller in case there have been nested calls
		call.definition = &self->calltargets[ targetIndex ];

		G_CallEntity(currentTarget, &call);

		if ( !self->inuse )
		{
			G_Printf( S_WARNING "entity was removed while using targets\n" );
			return;
		}
	}
}

void G_FireEntity( gentity_t *self, gentity_t *activator )
{
	G_EventFireEntity( self, activator, ON_DEFAULT );
}

void G_CallEntity(gentity_t *targetedEntity, gentityCall_t *call)
{
	if ( g_debugEntities.integer > 1 )
	{
		G_Printf(S_DEBUG "[%s] %s calling %s %s:%s\n",
				etos( call->activator ),
				etos( call->caller ),
				call->definition ? call->definition->event : "onUnknown",
				etos( targetedEntity ),
				call->definition && call->definition->action ? call->definition->action : "default");
	}

	if(!targetedEntity->handleCall || !targetedEntity->handleCall(targetedEntity, call))
	{
		switch (call->definition->actionType)
		{
		case ECA_NOP:
			break;

		case ECA_CUSTOM:
			if ( g_debugEntities.integer > -1 )
			{
				G_Printf(S_WARNING "Unknown action \"%s\" for %s\n",
						call->definition->action, etos(targetedEntity));
			}
			return;

		case ECA_FREE:
			G_FreeEntity(targetedEntity);
			return; //we have to handle notification differently in the free-case

		case ECA_PROPAGATE:
			G_FireEntity( targetedEntity, call->activator);
			break;

		case ECA_ENABLE:
			if(!targetedEntity->enabled) //only fire an event if we weren't already enabled
			{
				targetedEntity->enabled = qtrue;
				G_EventFireEntity( targetedEntity, call->activator, ON_ENABLE );
			}
			break;
		case ECA_DISABLE:
			if(targetedEntity->enabled) //only fire an event if we weren't already disabled
			{
				targetedEntity->enabled = qfalse;
				G_EventFireEntity( targetedEntity, call->activator, ON_DISABLE );
			}
			break;
		case ECA_TOGGLE:
			targetedEntity->enabled = !targetedEntity->enabled;
			G_EventFireEntity( targetedEntity, call->activator, targetedEntity->enabled ? ON_ENABLE : ON_DISABLE );
			break;

		case ECA_USE:
			if (!targetedEntity->use)
			{
				if(g_debugEntities.integer >= 0)
					G_Printf(S_WARNING "calling :use on %s, which has no use function!\n", etos(targetedEntity));
				break;
			}
			if(!call->activator || !call->activator->client)
			{
				if(g_debugEntities.integer >= 0)
					G_Printf(S_WARNING "calling %s:use, without a client as activator.\n", etos(targetedEntity));
				break;
			}
			targetedEntity->use(targetedEntity, call->caller, call->activator);
			break;
		case ECA_RESET:
			if (targetedEntity->reset)
			{
				targetedEntity->reset(targetedEntity);
				G_EventFireEntity( targetedEntity, call->activator, ON_RESET );
			}
			break;
		case ECA_ACT:
			if (targetedEntity->act)
				targetedEntity->act(targetedEntity, call->caller, call->activator);
			break;

		default:
			if (targetedEntity->act)
				targetedEntity->act(targetedEntity, call->caller, call->activator);
			break;
		}
	}
	if(targetedEntity->notifyHandler)
		targetedEntity->notifyHandler( targetedEntity, call );
}

/*
=================================================================================

gentity testing/querying

=================================================================================
*/

qboolean G_MatchesName( gentity_t *entity, const char* name )
{
	int nameIndex;

	for (nameIndex = 0; entity->names[nameIndex]; ++nameIndex)
	{
		if (!Q_stricmp(name, entity->names[nameIndex]))
			return qtrue;
	}
	return qfalse;
}

/*
===============
G_Visible

Test for a LOS between two entities
===============
*/
qboolean G_IsVisible( gentity_t *start, gentity_t *end, int contents )
{
	trace_t trace;

	trap_Trace( &trace, start->s.pos.trBase, NULL, NULL, end->s.pos.trBase,
	            start->s.number, contents );

	return trace.fraction >= 1.0f || trace.entityNum == end - g_entities;
}

/*
=================================================================================

gentity configuration

=================================================================================
*/

/*
===============
G_SetMovedir

The editor only specifies a single value for angles (yaw),
but we have special constants to generate an up or down direction.
Angles will be cleared, because it is being used to represent a direction
instead of an orientation.
===============
*/
void G_SetMovedir( vec3_t angles, vec3_t movedir )
{
	static vec3_t VEC_UP = { 0, -1, 0 };
	static vec3_t MOVEDIR_UP = { 0, 0, 1 };
	static vec3_t VEC_DOWN = { 0, -2, 0 };
	static vec3_t MOVEDIR_DOWN = { 0, 0, -1 };

	if ( VectorCompare( angles, VEC_UP ) )
	{
		VectorCopy( MOVEDIR_UP, movedir );
	}
	else if ( VectorCompare( angles, VEC_DOWN ) )
	{
		VectorCopy( MOVEDIR_DOWN, movedir );
	}
	else
	{
		AngleVectors( angles, movedir, NULL, NULL );
	}

	VectorClear( angles );
}

/*
================
G_SetOrigin

Sets the pos trajectory for a fixed position
================
*/
void G_SetOrigin( gentity_t *self, const vec3_t origin )
{
	VectorCopy( origin, self->s.pos.trBase );
	self->s.pos.trType = TR_STATIONARY;
	self->s.pos.trTime = 0;
	self->s.pos.trDuration = 0;
	VectorClear( self->s.pos.trDelta );

	VectorCopy( origin, self->r.currentOrigin );
	VectorCopy( origin, self->s.origin );
}

/**
 * predefined field interpretations
 */
void G_SetNextthink( gentity_t *self ) {
	self->nextthink = level.time + ( self->config.wait.time + self->config.wait.variance * crandom() ) * 1000;
}