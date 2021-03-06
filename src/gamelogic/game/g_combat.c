/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2009 Darklegion Development

This file is part of Daemon.

Daemon is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "g_local.h"

damageRegion_t g_damageRegions[ PCL_NUM_CLASSES ][ MAX_DAMAGE_REGIONS ];
int            g_numDamageRegions[ PCL_NUM_CLASSES ];

damageRegion_t g_armourRegions[ UP_NUM_UPGRADES ][ MAX_DAMAGE_REGIONS ];
int            g_numArmourRegions[ UP_NUM_UPGRADES ];

/*
============
AddScoreHelper

Helper function for G_AddCreditsToScore and G_AddConfidenceToScore.
============
*/
static void AddScoreHelper( gentity_t *self, float score )
{
	if ( !self->client )
	{
		return;
	}

	self->client->ps.persistant[ PERS_SCORE ] += ( int )( score + 0.5f );

	CalculateRanks();
}

/*
============
G_AddCreditsToScore

Adds score to the client, input represents a credit value.
============
*/
void G_AddCreditsToScore( gentity_t *self, int credits )
{
	AddScoreHelper( self, credits * SCORE_PER_CREDIT );
}

/*
============
G_AddConfidenceToScore

Adds score to the client, input represents a confidence value.
============
*/
void G_AddConfidenceToScore( gentity_t *self, float confidence )
{
	AddScoreHelper( self, confidence * SCORE_PER_CONFIDENCE );
}

/*
==================
LookAtKiller
==================
*/
void LookAtKiller( gentity_t *self, gentity_t *inflictor, gentity_t *attacker )
{
	if ( attacker && attacker != self )
	{
		self->client->ps.stats[ STAT_VIEWLOCK ] = attacker - g_entities;
	}
	else if ( inflictor && inflictor != self )
	{
		self->client->ps.stats[ STAT_VIEWLOCK ] = inflictor - g_entities;
	}
	else
	{
		self->client->ps.stats[ STAT_VIEWLOCK ] = self - g_entities;
	}
}

// these are just for logging, the client prints its own messages
static const char *const modNames[] =
{
	"MOD_UNKNOWN",
	"MOD_SHOTGUN",
	"MOD_BLASTER",
	"MOD_PAINSAW",
	"MOD_MACHINEGUN",
	"MOD_CHAINGUN",
	"MOD_PRIFLE",
	"MOD_MDRIVER",
	"MOD_LASGUN",
	"MOD_LCANNON",
	"MOD_LCANNON_SPLASH",
	"MOD_FLAMER",
	"MOD_FLAMER_SPLASH",
	"MOD_BURN",
	"MOD_WEIGHT_H",

	"MOD_GRENADE",
	"MOD_WATER",
	"MOD_SLIME",
	"MOD_LAVA",
	"MOD_CRUSH",
	"MOD_TELEFRAG",
	"MOD_FALLING",
	"MOD_SUICIDE",
	"MOD_TARGET_LASER",
	"MOD_TRIGGER_HURT",

	"MOD_ABUILDER_CLAW",
	"MOD_LEVEL0_BITE",
	"MOD_LEVEL1_CLAW",
	"MOD_LEVEL1_PCLOUD",
	"MOD_LEVEL3_CLAW",
	"MOD_LEVEL3_POUNCE",
	"MOD_LEVEL3_BOUNCEBALL",
	"MOD_LEVEL2_CLAW",
	"MOD_LEVEL2_ZAP",
	"MOD_LEVEL4_CLAW",
	"MOD_LEVEL4_TRAMPLE",
	"MOD_WEIGHT_A",

	"MOD_SLOWBLOB",
	"MOD_POISON",
	"MOD_SWARM",

	"MOD_HSPAWN",
	"MOD_TESLAGEN",
	"MOD_MGTURRET",
	"MOD_REACTOR",

	"MOD_ASPAWN",
	"MOD_ATUBE",
	"MOD_OVERMIND",
	"MOD_DECONSTRUCT",
	"MOD_REPLACE",
	"MOD_NOCREEP"
};

/*
==================
G_RewardAttackers

Function to distribute rewards to entities that killed this one.
==================
*/
void G_RewardAttackers( gentity_t *self )
{
	float     value, reward;
	int       playerNum, enemyDamage, maxHealth, damageShare;
	gentity_t *player;
	team_t    ownTeam, playerTeam;
	confidence_reason_t    reason;
	confidence_qualifier_t qualifier;

	// Only reward killing players and buildables
	if ( self->client )
	{
		ownTeam = self->client->pers.teamSelection;
		maxHealth = self->client->ps.stats[ STAT_MAX_HEALTH ];
		value = ( float )BG_GetValueOfPlayer( &self->client->ps );
	}
	else if ( self->s.eType == ET_BUILDABLE )
	{
		ownTeam = self->buildableTeam;
		maxHealth = BG_Buildable( self->s.modelindex )->health;
		value = ( float )BG_Buildable( self->s.modelindex )->value;

		// Give partial credits for buildables in construction
		if ( !self->spawned )
		{
			value *= ( float )( level.time - self->creationTime ) / BG_Buildable( self->s.modelindex )->buildTime;
		}
	}
	else
	{
		return;
	}

	enemyDamage = 0;

	// Sum up damage dealt by enemies
	for ( playerNum = 0; playerNum < level.maxclients; playerNum++ )
	{
		player = &g_entities[ playerNum ];
		playerTeam = player->client->pers.teamSelection;

		// Player must be on the other team
		if ( playerTeam == ownTeam || playerTeam <= TEAM_NONE || playerTeam >= NUM_TEAMS )
		{
			continue;
		}

		enemyDamage += self->credits[ playerNum ];
	}

	if ( enemyDamage <= 0 )
	{
		return;
	}

	// Give individual rewards
	for ( playerNum = 0; playerNum < level.maxclients; playerNum++ )
	{
		player = &g_entities[ playerNum ];
		playerTeam = player->client->pers.teamSelection;
		damageShare = self->credits[ playerNum ];

		// Clear reward array
		self->credits[ playerNum ] = 0;

		// Player must be on the other team
		if ( playerTeam == ownTeam || playerTeam <= TEAM_NONE || playerTeam >= NUM_TEAMS )
		{
			continue;
		}

		// Player must have dealt damage
		if ( damageShare <= 0 )
		{
			continue;
		}

		reward = value * ( damageShare / ( float )maxHealth );

		if ( reward <= 0.0f )
		{
			continue;
		}

		if ( self->s.eType == ET_BUILDABLE )
		{
			G_AddConfidenceToScore( player, reward );

			switch ( self->s.modelindex )
			{
				case BA_A_OVERMIND:
				case BA_H_REACTOR:
					reason = CONF_REAS_DESTR_CRUCIAL;
					break;

				case BA_A_ACIDTUBE:
				case BA_A_TRAPPER:
				case BA_A_HIVE:
				case BA_H_MGTURRET:
				case BA_H_TESLAGEN:
					reason = CONF_REAS_DESTR_AGGRESSIVE;
					break;

				default:
					reason = CONF_REAS_DESTR_SUPPORT;
			}

			qualifier = CONF_QUAL_NONE;

			G_AddConfidence( playerTeam, CONFIDENCE_DESTRUCTION, reason, qualifier, reward, player );
		}
		else
		{
			G_AddCreditsToScore( player, ( int )reward );
			G_AddCreditToClient( player->client, ( short )reward, qtrue );

			// Give confidence for killing non-naked players outside the friendly base
			switch ( self->client->ps.stats[ STAT_CLASS ] )
			{
				case PCL_ALIEN_LEVEL0:
				case PCL_ALIEN_BUILDER0:
				case PCL_ALIEN_BUILDER0_UPG:
					break;

				case PCL_HUMAN:
					// Treat a human just wearing light armor as naked
					if ( ( int )value <= BG_Class( PCL_HUMAN )->value +
					                     ( BG_Upgrade( UP_LIGHTARMOUR )->price / 2 ) )
					{
						break;
					}

				default:
					if ( G_InsideBase( player, qtrue ) || G_InsideBase( self, qfalse ) )
					{
						break;
					}

					qualifier = CONF_QUAL_OUTSIDE_OWN_BASE;

					G_AddConfidence( playerTeam, CONFIDENCE_KILLING, CONF_REAS_KILLING,
					                 qualifier, reward * CONFIDENCE_PER_CREDIT, player );
			}
		}
	}
}

/*
==================
player_die
==================
*/
void player_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int meansOfDeath )
{
	gentity_t *ent;
	int       anim;
	int       killer;
	int       i;
	const char *killerName, *obit;

	if ( self->client->ps.pm_type == PM_DEAD )
	{
		return;
	}

	if ( level.intermissiontime )
	{
		return;
	}

	self->client->ps.pm_type = PM_DEAD;
	self->suicideTime = 0;

	if ( attacker )
	{
		killer = attacker->s.number;

		if ( attacker->client )
		{
			killerName = attacker->client->pers.netname;
		}
		else
		{
			killerName = "<world>";
		}
	}
	else
	{
		killer = ENTITYNUM_WORLD;
		killerName = "<world>";
	}

	if ( meansOfDeath < 0 || meansOfDeath >= ARRAY_LEN( modNames ) )
	{
		// fall back on the number
		obit = va( "%d", meansOfDeath );
	}
	else
	{
		obit = modNames[ meansOfDeath ];
	}

	G_LogPrintf( "Die: %d %d %s: %s" S_COLOR_WHITE " killed %s\n",
	             killer,
	             ( int )( self - g_entities ),
	             obit,
	             killerName,
	             self->client->pers.netname );

	// deactivate all upgrades
	for ( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
	{
		BG_DeactivateUpgrade( i, self->client->ps.stats );
	}

	// broadcast the death event to everyone
	ent = G_NewTempEntity( self->r.currentOrigin, EV_OBITUARY );
	ent->s.eventParm = meansOfDeath;
	ent->s.otherEntityNum = self->s.number;
	ent->s.otherEntityNum2 = killer;
	ent->r.svFlags = SVF_BROADCAST; // send to everyone

	if ( attacker && attacker->client )
	{
		if ( ( attacker == self || OnSameTeam( self, attacker ) ) )
		{
			//punish team kills and suicides
			if ( attacker->client->ps.stats[ STAT_TEAM ] == TEAM_ALIENS )
			{
				G_AddCreditToClient( attacker->client, -ALIEN_TK_SUICIDE_PENALTY, qtrue );
				G_AddCreditsToScore( attacker, -ALIEN_TK_SUICIDE_PENALTY );
			}
			else if ( attacker->client->ps.stats[ STAT_TEAM ] == TEAM_HUMANS )
			{
				G_AddCreditToClient( attacker->client, -HUMAN_TK_SUICIDE_PENALTY, qtrue );
				G_AddCreditsToScore( attacker, -HUMAN_TK_SUICIDE_PENALTY );
			}
		}
		else if ( g_showKillerHP.integer )
		{
			trap_SendServerCommand( self - g_entities, va( "print_tr %s %s %3i", QQ( N_("Your killer, $1$^7, had $2$ HP.\n") ),
			                        Quote( killerName ),
			                        attacker->health ) );
		}
	}
	else if ( attacker->s.eType != ET_BUILDABLE )
	{
		if ( self->client->ps.stats[ STAT_TEAM ] == TEAM_ALIENS )
		{
			G_AddCreditsToScore( self, -ALIEN_TK_SUICIDE_PENALTY );
		}
		else if ( self->client->ps.stats[ STAT_TEAM ] == TEAM_HUMANS )
		{
			G_AddCreditsToScore( self, -HUMAN_TK_SUICIDE_PENALTY );
		}
	}

	// give credits for killing this player
	G_RewardAttackers( self );

	ScoreboardMessage( self );  // show scores

	// send updated scores to any clients that are following this one,
	// or they would get stale scoreboards
	for ( i = 0; i < level.maxclients; i++ )
	{
		gclient_t *client;

		client = &level.clients[ i ];

		if ( client->pers.connected != CON_CONNECTED )
		{
			continue;
		}

		if ( client->sess.spectatorState == SPECTATOR_NOT )
		{
			continue;
		}

		if ( client->sess.spectatorClient == self->s.number )
		{
			ScoreboardMessage( g_entities + i );
		}
	}

	VectorCopy( self->s.origin, self->client->pers.lastDeathLocation );

	self->takedamage = qfalse; // can still be gibbed

	self->s.weapon = WP_NONE;
	if ( self->client->noclip )
	{
		self->client->cliprcontents = CONTENTS_CORPSE;
	}
	else
	{
		self->r.contents = CONTENTS_CORPSE;
	}

	self->s.angles[ PITCH ] = 0;
	self->s.angles[ ROLL ] = 0;
	self->s.angles[ YAW ] = self->s.apos.trBase[ YAW ];
	LookAtKiller( self, inflictor, attacker );

	VectorCopy( self->s.angles, self->client->ps.viewangles );

	self->s.loopSound = 0;

	self->r.maxs[ 2 ] = -8;

	// don't allow respawn until the death anim is done
	// g_forcerespawn may force spawning at some later time
	self->client->respawnTime = level.time + 1700;

	// clear misc
	memset( self->client->ps.misc, 0, sizeof( self->client->ps.misc ) );

	{
		static int i;

		if ( !( self->client->ps.persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
		{
			switch ( i )
			{
				case 0:
					anim = BOTH_DEATH1;
					break;

				case 1:
					anim = BOTH_DEATH2;
					break;

				case 2:
				default:
					anim = BOTH_DEATH3;
					break;
			}
		}
		else
		{
			switch ( i )
			{
				case 0:
					anim = NSPA_DEATH1;
					break;

				case 1:
					anim = NSPA_DEATH2;
					break;

				case 2:
				default:
					anim = NSPA_DEATH3;
					break;
			}
		}

		self->client->ps.legsAnim =
		  ( ( self->client->ps.legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;

		if ( !( self->client->ps.persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
		{
			self->client->ps.torsoAnim =
			  ( ( self->client->ps.torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;
		}

		// use own entityid if killed by non-client to prevent uint8_t overflow
		G_AddEvent( self, EV_DEATH1 + i,
		            ( killer < MAX_CLIENTS ) ? killer : self - g_entities );

		// globally cycle through the different death animations
		i = ( i + 1 ) % 3;
	}

	trap_LinkEntity( self );

	self->client->pers.infoChangeTime = level.time;
}

/*
===============
G_ParseDmgScript
===============
*/
static int G_ParseDmgScript( damageRegion_t *regions, char *buf )
{
	char  *token;
	float angleSpan, heightSpan;
	int   count;

	for ( count = 0;; count++ )
	{
		token = COM_Parse( &buf );

		if ( !token[ 0 ] )
		{
			break;
		}

		if ( strcmp( token, "{" ) )
		{
			COM_ParseError( "Missing {" );
			break;
		}

		if ( count >= MAX_DAMAGE_REGIONS )
		{
			COM_ParseError( "Max damage regions exceeded" );
			break;
		}

		// defaults
		regions[ count ].name[ 0 ] = '\0';
		regions[ count ].minHeight = 0.0f;
		regions[ count ].maxHeight = 1.0f;
		regions[ count ].minAngle = 0.0f;
		regions[ count ].maxAngle = 360.0f;
		regions[ count ].modifier = 1.0f;
		regions[ count ].crouch = qfalse;

		while ( 1 )
		{
			token = COM_ParseExt( &buf, qtrue );

			if ( !token[ 0 ] )
			{
				COM_ParseError( "Unexpected end of file" );
				break;
			}

			if ( !Q_stricmp( token, "}" ) )
			{
				break;
			}
			else if ( !strcmp( token, "name" ) )
			{
				token = COM_ParseExt( &buf, qfalse );

				if ( token[ 0 ] )
				{
					Q_strncpyz( regions[ count ].name, token,
					            sizeof( regions[ count ].name ) );
				}
			}
			else if ( !strcmp( token, "minHeight" ) )
			{
				token = COM_ParseExt( &buf, qfalse );

				if ( !token[ 0 ] )
				{
					strcpy( token, "0" );
				}

				regions[ count ].minHeight = atof( token );
			}
			else if ( !strcmp( token, "maxHeight" ) )
			{
				token = COM_ParseExt( &buf, qfalse );

				if ( !token[ 0 ] )
				{
					strcpy( token, "100" );
				}

				regions[ count ].maxHeight = atof( token );
			}
			else if ( !strcmp( token, "minAngle" ) )
			{
				token = COM_ParseExt( &buf, qfalse );

				if ( !token[ 0 ] )
				{
					strcpy( token, "0" );
				}

				regions[ count ].minAngle = atoi( token );
			}
			else if ( !strcmp( token, "maxAngle" ) )
			{
				token = COM_ParseExt( &buf, qfalse );

				if ( !token[ 0 ] )
				{
					strcpy( token, "360" );
				}

				regions[ count ].maxAngle = atoi( token );
			}
			else if ( !strcmp( token, "modifier" ) )
			{
				token = COM_ParseExt( &buf, qfalse );

				if ( !token[ 0 ] )
				{
					strcpy( token, "1.0" );
				}

				regions[ count ].modifier = atof( token );
			}
			else if ( !strcmp( token, "crouch" ) )
			{
				regions[ count ].crouch = qtrue;
			}
			else
			{
				COM_ParseWarning( "Unknown token \"%s\"", token );
			}
		}

		// Angle portion covered
		angleSpan = regions[ count ].maxAngle - regions[ count ].minAngle;

		if ( angleSpan < 0.0f )
		{
			angleSpan += 360.0f;
		}

		angleSpan /= 360.0f;

		// Height portion covered
		heightSpan = regions[ count ].maxHeight - regions[ count ].minHeight;

		if ( heightSpan < 0.0f )
		{
			heightSpan = -heightSpan;
		}

		if ( heightSpan > 1.0f )
		{
			heightSpan = 1.0f;
		}

		regions[ count ].area = angleSpan * heightSpan;

		if ( !regions[ count ].area )
		{
			regions[ count ].area = 0.00001f;
		}
	}

	return count;
}

/*
============
GetRegionDamageModifier
============
*/
static float GetRegionDamageModifier( gentity_t *targ, int class, int piece )
{
	damageRegion_t *regions, *overlap;
	float          modifier = 0.0f, areaSum = 0.0f;
	int            j, i;
	qboolean       crouch;

	crouch = targ->client->ps.pm_flags & PMF_DUCKED;
	overlap = &g_damageRegions[ class ][ piece ];

	if ( g_debugDamage.integer > 2 )
	{
		G_Printf( "GetRegionDamageModifier():\n"
		          ".   bodyRegion = [%d %d %f %f] (%s)\n"
		          ".   modifier = %f\n",
		          overlap->minAngle, overlap->maxAngle,
		          overlap->minHeight, overlap->maxHeight,
		          overlap->name, overlap->modifier );
	}

	// Find the armour layer modifier, assuming that none of the armour regions
	// overlap and that any areas that are not covered have a modifier of 1.0
	for ( j = UP_NONE + 1; j < UP_NUM_UPGRADES; j++ )
	{
		if ( !BG_InventoryContainsUpgrade( j, targ->client->ps.stats ) ||
		     !g_numArmourRegions[ j ] )
		{
			continue;
		}

		regions = g_armourRegions[ j ];

		for ( i = 0; i < g_numArmourRegions[ j ]; i++ )
		{
			float overlapMaxA, regionMinA, regionMaxA, angleSpan, heightSpan, area;

			if ( regions[ i ].crouch != crouch )
			{
				continue;
			}

			// Convert overlap angle to 0 to max
			overlapMaxA = overlap->maxAngle - overlap->minAngle;

			if ( overlapMaxA < 0.0f )
			{
				overlapMaxA += 360.0f;
			}

			// Convert region angles to match overlap
			regionMinA = regions[ i ].minAngle - overlap->minAngle;

			if ( regionMinA < 0.0f )
			{
				regionMinA += 360.0f;
			}

			regionMaxA = regions[ i ].maxAngle - overlap->minAngle;

			if ( regionMaxA < 0.0f )
			{
				regionMaxA += 360.0f;
			}

			// Overlapping Angle portion
			if ( regionMinA <= regionMaxA )
			{
				angleSpan = 0.0f;

				if ( regionMinA < overlapMaxA )
				{
					if ( regionMaxA > overlapMaxA )
					{
						regionMaxA = overlapMaxA;
					}

					angleSpan = regionMaxA - regionMinA;
				}
			}
			else
			{
				if ( regionMaxA > overlapMaxA )
				{
					regionMaxA = overlapMaxA;
				}

				angleSpan = regionMaxA;

				if ( regionMinA < overlapMaxA )
				{
					angleSpan += overlapMaxA - regionMinA;
				}
			}

			angleSpan /= 360.0f;

			// Overlapping height portion
			heightSpan = MIN( overlap->maxHeight, regions[ i ].maxHeight ) -
			             MAX( overlap->minHeight, regions[ i ].minHeight );

			if ( heightSpan < 0.0f )
			{
				heightSpan = 0.0f;
			}

			if ( heightSpan > 1.0f )
			{
				heightSpan = 1.0f;
			}

			if ( g_debugDamage.integer > 2 )
			{
				G_Printf( ".   armourRegion = [%d %d %f %f] (%s)\n"
				          ".   .   modifier = %f\n"
				          ".   .   angleSpan = %f\n"
				          ".   .   heightSpan = %f\n",
				          regions[ i ].minAngle, regions[ i ].maxAngle,
				          regions[ i ].minHeight, regions[ i ].maxHeight,
				          regions[ i ].name, regions[ i ].modifier,
				          angleSpan, heightSpan );
			}

			areaSum += area = angleSpan * heightSpan;
			modifier += regions[ i ].modifier * area;
		}
	}

	if ( g_debugDamage.integer > 2 )
	{
		G_Printf( ".   areaSum = %f\n"
		          ".   armourModifier = %f\n", areaSum, modifier );
	}

	return overlap->modifier * ( overlap->area + modifier - areaSum );
}

/*
============
GetNonLocDamageModifier
============
*/
static float GetNonLocDamageModifier( gentity_t *targ, int class )
{
	float    modifier = 0.0f, area = 0.0f, scale = 0.0f;
	int      i;
	qboolean crouch;

	// For every body region, use stretch-armor formula to apply armour modifier
	// for any overlapping area that armour shares with the body region
	crouch = targ->client->ps.pm_flags & PMF_DUCKED;

	for ( i = 0; i < g_numDamageRegions[ class ]; i++ )
	{
		damageRegion_t *region;

		region = &g_damageRegions[ class ][ i ];

		if ( region->crouch != crouch )
		{
			continue;
		}

		modifier += GetRegionDamageModifier( targ, class, i );

		scale += region->modifier * region->area;
		area += region->area;
	}

	modifier = !scale ? 1.0f : 1.0f + ( modifier / scale - 1.0f ) * area;

	if ( g_debugDamage.integer > 1 )
	{
		G_Printf( "GetNonLocDamageModifier() modifier:%f, area:%f, scale:%f\n",
		          modifier, area, scale );
	}

	return modifier;
}

/*
============
GetPointDamageModifier

Returns the damage region given an angle and a height proportion
============
*/
static float GetPointDamageModifier( gentity_t *targ, damageRegion_t *regions,
                                     int len, float angle, float height )
{
	float modifier = 1.0f;
	int   i;

	for ( i = 0; i < len; i++ )
	{
		if ( regions[ i ].crouch != ( targ->client->ps.pm_flags & PMF_DUCKED ) )
		{
			continue;
		}

		// Angle must be within range
		if ( ( regions[ i ].minAngle <= regions[ i ].maxAngle &&
		       ( angle < regions[ i ].minAngle ||
		         angle > regions[ i ].maxAngle ) ) ||
		     ( regions[ i ].minAngle > regions[ i ].maxAngle &&
		       angle > regions[ i ].maxAngle && angle < regions[ i ].minAngle ) )
		{
			continue;
		}

		// Height must be within range
		if ( height < regions[ i ].minHeight || height > regions[ i ].maxHeight )
		{
			continue;
		}

		modifier *= regions[ i ].modifier;
	}

	if ( g_debugDamage.integer )
	{
		G_Printf( "GetDamageRegionModifier(angle = %f, height = %f): %f\n",
		          angle, height, modifier );
	}

	return modifier;
}

/*
============
G_CalcDamageModifier
============
*/
static float G_CalcDamageModifier( vec3_t point, gentity_t *targ, gentity_t *attacker, int class, int dflags )
{
	vec3_t targOrigin, bulletPath, bulletAngle, pMINUSfloor, floor, normal;
	float  clientHeight, hitRelative, hitRatio, modifier;
	int    hitRotation, i;

	if ( point == NULL )
	{
		return 1.0f;
	}

	// Don't need to calculate angles and height for non-locational damage
	if ( dflags & DAMAGE_NO_LOCDAMAGE )
	{
		return GetNonLocDamageModifier( targ, class );
	}

	// Get the point location relative to the floor under the target
	if ( g_unlagged.integer && targ->client && targ->client->unlaggedCalc.used )
	{
		VectorCopy( targ->client->unlaggedCalc.origin, targOrigin );
	}
	else
	{
		VectorCopy( targ->r.currentOrigin, targOrigin );
	}

	BG_GetClientNormal( &targ->client->ps, normal );
	VectorMA( targOrigin, targ->r.mins[ 2 ], normal, floor );
	VectorSubtract( point, floor, pMINUSfloor );

	// Get the proportion of the target height where the hit landed
	clientHeight = targ->r.maxs[ 2 ] - targ->r.mins[ 2 ];

	if ( !clientHeight )
	{
		clientHeight = 1.0f;
	}

	hitRelative = DotProduct( normal, pMINUSfloor ) / VectorLength( normal );

	if ( hitRelative < 0.0f )
	{
		hitRelative = 0.0f;
	}

	if ( hitRelative > clientHeight )
	{
		hitRelative = clientHeight;
	}

	hitRatio = hitRelative / clientHeight;

	// Get the yaw of the attack relative to the target's view yaw
	VectorSubtract( point, targOrigin, bulletPath );
	vectoangles( bulletPath, bulletAngle );

	hitRotation = AngleNormalize360( targ->client->ps.viewangles[ YAW ] -
	                                 bulletAngle[ YAW ] );

	// Get modifiers from the target's damage regions
	modifier = GetPointDamageModifier( targ, g_damageRegions[ class ],
	                                   g_numDamageRegions[ class ],
	                                   hitRotation, hitRatio );

	for ( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
	{
		if ( BG_InventoryContainsUpgrade( i, targ->client->ps.stats ) )
		{
			modifier *= GetPointDamageModifier( targ, g_armourRegions[ i ],
			                                    g_numArmourRegions[ i ],
			                                    hitRotation, hitRatio );
		}
	}

	return modifier;
}

/*
============
G_InitDamageLocations
============
*/
void G_InitDamageLocations( void )
{
	const char   *modelName;
	char         filename[ MAX_QPATH ];
	int          i;
	int          len;
	fileHandle_t fileHandle;
	char         buffer[ MAX_DAMAGE_REGION_TEXT ];

	for ( i = PCL_NONE + 1; i < PCL_NUM_CLASSES; i++ )
	{
		modelName = BG_ClassModelConfig( i )->modelName;
		Com_sprintf( filename, sizeof( filename ),
		             "models/players/%s/locdamage.cfg", modelName );

		len = trap_FS_FOpenFile( filename, &fileHandle, FS_READ );

		if ( !fileHandle )
		{
			G_Printf( S_COLOR_RED "file not found: %s\n", filename );
			continue;
		}

		if ( len >= MAX_DAMAGE_REGION_TEXT )
		{
			G_Printf( S_COLOR_RED "file too large: %s is %i, max allowed is %i\n",
			          filename, len, MAX_DAMAGE_REGION_TEXT );
			trap_FS_FCloseFile( fileHandle );
			continue;
		}

		COM_BeginParseSession( filename );

		trap_FS_Read( buffer, len, fileHandle );
		buffer[ len ] = 0;
		trap_FS_FCloseFile( fileHandle );

		g_numDamageRegions[ i ] = G_ParseDmgScript( g_damageRegions[ i ], buffer );
	}

	for ( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
	{
		modelName = BG_Upgrade( i )->name;
		Com_sprintf( filename, sizeof( filename ), "armour/%s.armour", modelName );

		len = trap_FS_FOpenFile( filename, &fileHandle, FS_READ );

		//no file - no parsage
		if ( !fileHandle )
		{
			continue;
		}

		if ( len >= MAX_DAMAGE_REGION_TEXT )
		{
			G_Printf( S_COLOR_RED "file too large: %s is %i, max allowed is %i\n",
			          filename, len, MAX_DAMAGE_REGION_TEXT );
			trap_FS_FCloseFile( fileHandle );
			continue;
		}

		COM_BeginParseSession( filename );

		trap_FS_Read( buffer, len, fileHandle );
		buffer[ len ] = 0;
		trap_FS_FCloseFile( fileHandle );

		g_numArmourRegions[ i ] = G_ParseDmgScript( g_armourRegions[ i ], buffer );
	}
}

/*
============
T_Damage

targ    entity that is being damaged
inflictor entity that is causing the damage
attacker  entity that caused the inflictor to damage targ
  example: targ=monster, inflictor=rocket, attacker=player

dir     direction of the attack for knockback
point   point at which the damage is being inflicted, used for headshots
damage    amount of damage being inflicted
knockback force to be applied against targ as a result of the damage

inflictor, attacker, dir, and point can be NULL for environmental effects

dflags    these flags are used to control how T_Damage works
  DAMAGE_RADIUS     damage was indirect (from a nearby explosion)
  DAMAGE_NO_ARMOR     armor does not protect from this damage
  DAMAGE_NO_KNOCKBACK   do not affect velocity, just view angles
  DAMAGE_NO_PROTECTION  kills everything except godmode
============
*/

// team is the team that is immune to this damage
void G_SelectiveDamage( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
                        vec3_t dir, vec3_t point, int damage, int dflags, int mod, int team )
{
	if ( targ->client && ( team != targ->client->ps.stats[ STAT_TEAM ] ) )
	{
		G_Damage( targ, inflictor, attacker, dir, point, damage, dflags, mod );
	}
}

void G_Damage( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
               vec3_t dir, vec3_t point, int damage, int dflags, int mod )
{
	gclient_t *client;
	int       take, loss;
	int       asave = 0;
	int       knockback;

	// Can't deal damage sometimes
	if ( !targ->takedamage || targ->health <= 0 || level.intermissionQueued )
	{
		return;
	}

	if ( !inflictor )
	{
		inflictor = &g_entities[ ENTITYNUM_WORLD ];
	}

	if ( !attacker )
	{
		attacker = &g_entities[ ENTITYNUM_WORLD ];
	}

	// shootable doors / buttons don't actually have any health unless they have a die or pain function to handle it
	if ( targ->s.eType == ET_MOVER && !(targ->die || targ->pain ))
	{
		if ( ( targ->moverState == MOVER_POS1 || targ->moverState == ROTATOR_POS1 ) )
		{
			if( targ->act )
				targ->act( targ, inflictor, attacker );
		}

		return;
	}

	client = targ->client;

	if ( client && client->noclip )
	{
		return;
	}

	if ( !dir )
	{
		dflags |= DAMAGE_NO_KNOCKBACK;
	}

	knockback = damage;

	if ( inflictor->s.weapon != WP_NONE )
	{
		knockback = ( int )( ( float ) knockback *
		                     BG_Weapon( inflictor->s.weapon )->knockbackScale );
	}

	if ( targ->client )
	{
		knockback = ( int )( ( float ) knockback *
		                     BG_Class( targ->client->ps.stats[ STAT_CLASS ] )->knockbackScale );
	}

	// Too much knockback from falling really far makes you "bounce" and
	//  looks silly. However, none at all also looks bad. Cap it.
	if ( mod == MOD_FALLING && knockback > 50 )
	{
		knockback = 50;
	}

	if ( knockback > 200 )
	{
		knockback = 200;
	}

	if ( targ->flags & FL_NO_KNOCKBACK )
	{
		knockback = 0;
	}

	if ( dflags & DAMAGE_NO_KNOCKBACK )
	{
		knockback = 0;
	}

	// figure momentum add, even if the damage won't be taken
	if ( knockback && targ->client )
	{
		vec3_t kvel;
		float  mass;

		mass = 200;

		VectorScale( dir, g_knockback.value * ( float ) knockback / mass, kvel );
		VectorAdd( targ->client->ps.velocity, kvel, targ->client->ps.velocity );

		// set the timer so that the other client can't cancel
		// out the movement immediately
		if ( !targ->client->ps.pm_time )
		{
			int t;

			t = knockback * 2;

			if ( t < 50 )
			{
				t = 50;
			}

			if ( t > 200 )
			{
				t = 200;
			}

			targ->client->ps.pm_time = t;
			targ->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		}
	}

	// check for godmode
	if ( targ->flags & FL_GODMODE )
	{
		return;
	}

	// don't do friendly fire on movement attacks
	if ( ( mod == MOD_LEVEL4_TRAMPLE || mod == MOD_LEVEL3_POUNCE ) &&
	     targ->s.eType == ET_BUILDABLE && targ->buildableTeam == TEAM_ALIENS )
	{
		return;
	}

	// check for completely getting out of the damage
	if ( !( dflags & DAMAGE_NO_PROTECTION ) )
	{
		// if TF_NO_FRIENDLY_FIRE is set, don't do damage to the target
		// if the attacker was on the same team
		if ( targ != attacker && OnSameTeam( targ, attacker ) )
		{
			// never do friendly fire on movement attacks
			if ( mod == MOD_LEVEL4_TRAMPLE || mod == MOD_LEVEL3_POUNCE )
			{
				return;
			}

			// if dretchpunt is enabled and this is a dretch, do dretchpunt instead of damage
			if ( g_dretchPunt.integer && targ->client &&
			     ( targ->client->ps.stats[ STAT_CLASS ] == PCL_ALIEN_LEVEL0 ||
			       targ->client->ps.stats[ STAT_CLASS ] == PCL_ALIEN_LEVEL0_UPG ) )
			{
				vec3_t dir, push;

				VectorSubtract( targ->r.currentOrigin, attacker->r.currentOrigin, dir );
				VectorNormalizeFast( dir );
				VectorScale( dir, ( damage * 10.0f ), push );
				push[ 2 ] = 64.0f;
				VectorAdd( targ->client->ps.velocity, push, targ->client->ps.velocity );
				return;
			}

			// check if friendly fire has been disabled
			if ( !g_friendlyFire.integer )
			{
				return;
			}
		}

		if ( targ->s.eType == ET_BUILDABLE && attacker->client &&
		     mod != MOD_DECONSTRUCT && mod != MOD_SUICIDE &&
		     mod != MOD_REPLACE && mod != MOD_NOCREEP )
		{
			if ( targ->buildableTeam == attacker->client->pers.teamSelection &&
			     !g_friendlyBuildableFire.integer )
			{
				return;
			}

			// base is under attack warning if DCC'd
			if ( targ->buildableTeam == TEAM_HUMANS && G_FindDCC( targ ) &&
			     level.time > level.humanBaseAttackTimer )
			{
				level.humanBaseAttackTimer = level.time + DC_ATTACK_PERIOD;
				G_BroadcastEvent( EV_DCC_ATTACK, 0 );
			}
		}
	}

	if ( targ && targ->client && attacker && attacker->client )
	{
		// Update the last combat time.
		targ->client->lastCombatTime = level.time;
		attacker->client->lastCombatTime = level.time;
	}

	take = damage;

	// add to the damage inflicted on a player this frame
	// the total will be turned into screen blends and view angle kicks
	// at the end of the frame
	if ( client )
	{
		if ( attacker )
		{
			client->ps.persistant[ PERS_ATTACKER ] = attacker->s.number;
		}
		else
		{
			client->ps.persistant[ PERS_ATTACKER ] = ENTITYNUM_WORLD;
		}

		client->damage_armor += asave;
		client->damage_blood += take;
		client->damage_knockback += knockback;

		if ( dir )
		{
			VectorCopy( dir, client->damage_from );
			client->damage_fromWorld = qfalse;
		}
		else
		{
			VectorCopy( targ->r.currentOrigin, client->damage_from );
			client->damage_fromWorld = qtrue;
		}

		take = ( int )( take * G_CalcDamageModifier( point, targ, attacker,
		                client->ps.stats[ STAT_CLASS ],
		                dflags ) + 0.5f );

		//if boosted poison every attack
		if ( attacker->client && (attacker->client->ps.stats[ STAT_STATE ] & SS_BOOSTED) )
		{
			if ( targ->client->ps.stats[ STAT_TEAM ] == TEAM_HUMANS &&
			     mod != MOD_LEVEL2_ZAP && mod != MOD_POISON &&
			     mod != MOD_LEVEL1_PCLOUD && mod != MOD_HSPAWN &&
			     mod != MOD_ASPAWN && targ->client->poisonImmunityTime < level.time )
			{
				targ->client->ps.stats[ STAT_STATE ] |= SS_POISONED;
				targ->client->lastPoisonTime = level.time;
				targ->client->lastPoisonClient = attacker;
			}
		}
	}

	if ( take < 1 )
	{
		take = 1;
	}

	if ( g_debugDamage.integer )
	{
		G_Printf( "%i: client:%i health:%i damage:%i armor:%i\n", level.time, targ->s.number,
		          targ->health, take, asave );
	}

	// do the damage
	if ( take )
	{
		targ->health = targ->health - take;

		if ( targ->client )
		{
			targ->client->ps.stats[ STAT_HEALTH ] = targ->health;
			targ->client->pers.infoChangeTime = level.time;
		}

		targ->lastDamageTime = level.time;
		targ->nextRegenTime = level.time + ALIEN_REGEN_DAMAGE_TIME;

		if ( attacker != targ )
		{
			if ( targ->health < 0 )
			{
				loss = ( take + targ->health );
			}
			else
			{
				loss = take;
			}

			// add to the attackers "account" on the target
			if ( attacker->client )
			{
				targ->credits[ attacker->client->ps.clientNum ] += loss;
			}

			// update buildable stats
			if ( attacker->s.eType == ET_BUILDABLE && attacker->health > 0 )
			{
				attacker->buildableStatsTotal += loss;
			}
		}

		if ( targ->health <= 0 )
		{
			if ( client )
			{
				targ->flags |= FL_NO_KNOCKBACK;
			}

			if ( targ->health < -999 )
			{
				targ->health = -999;
			}

			targ->die( targ, inflictor, attacker, mod );

			// update buildable stats
			if ( attacker->s.eType == ET_BUILDABLE && attacker->health > 0 )
			{
				attacker->buildableStatsCount++;
			}

			if(!targ->client) //call the onDie event only on non living entities
			{
				G_EventFireEntity( targ, attacker, ON_DIE );
			}
			return;
		}
		else if ( targ->pain )
		{
			targ->pain( targ, attacker, take );
		}
	}
}

/*
============
CanDamage

Returns qtrue if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/
qboolean CanDamage( gentity_t *targ, vec3_t origin )
{
	vec3_t  dest;
	trace_t tr;
	vec3_t  midpoint;

	// use the midpoint of the bounds instead of the origin, because
	// bmodels may have their origin is 0,0,0
	VectorAdd( targ->r.absmin, targ->r.absmax, midpoint );
	VectorScale( midpoint, 0.5, midpoint );

	VectorCopy( midpoint, dest );
	trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID );

	if ( tr.fraction == 1.0  || tr.entityNum == targ->s.number )
	{
		return qtrue;
	}

	// this should probably check in the plane of projection,
	// rather than in world coordinate, and also include Z
	VectorCopy( midpoint, dest );
	dest[ 0 ] += 15.0;
	dest[ 1 ] += 15.0;
	trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID );

	if ( tr.fraction == 1.0 )
	{
		return qtrue;
	}

	VectorCopy( midpoint, dest );
	dest[ 0 ] += 15.0;
	dest[ 1 ] -= 15.0;
	trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID );

	if ( tr.fraction == 1.0 )
	{
		return qtrue;
	}

	VectorCopy( midpoint, dest );
	dest[ 0 ] -= 15.0;
	dest[ 1 ] += 15.0;
	trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID );

	if ( tr.fraction == 1.0 )
	{
		return qtrue;
	}

	VectorCopy( midpoint, dest );
	dest[ 0 ] -= 15.0;
	dest[ 1 ] -= 15.0;
	trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID );

	if ( tr.fraction == 1.0 )
	{
		return qtrue;
	}

	return qfalse;
}

/*
============
G_SelectiveRadiusDamage
============
*/
qboolean G_SelectiveRadiusDamage( vec3_t origin, gentity_t *attacker, float damage,
                                  float radius, gentity_t *ignore, int mod, int ignoreTeam )
{
	float     points, dist;
	gentity_t *ent;
	int       entityList[ MAX_GENTITIES ];
	int       numListedEntities;
	vec3_t    mins, maxs;
	vec3_t    v;
	int       i, e;
	qboolean  hitClient = qfalse;

	if ( radius < 1 )
	{
		radius = 1;
	}

	for ( i = 0; i < 3; i++ )
	{
		mins[ i ] = origin[ i ] - radius;
		maxs[ i ] = origin[ i ] + radius;
	}

	numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( e = 0; e < numListedEntities; e++ )
	{
		ent = &g_entities[ entityList[ e ] ];

		if ( ent == ignore )
		{
			continue;
		}

		if ( !ent->takedamage )
		{
			continue;
		}

		if ( ent->flags & FL_NOTARGET )
		{
			continue;
		}

		// find the distance from the edge of the bounding box
		for ( i = 0; i < 3; i++ )
		{
			if ( origin[ i ] < ent->r.absmin[ i ] )
			{
				v[ i ] = ent->r.absmin[ i ] - origin[ i ];
			}
			else if ( origin[ i ] > ent->r.absmax[ i ] )
			{
				v[ i ] = origin[ i ] - ent->r.absmax[ i ];
			}
			else
			{
				v[ i ] = 0;
			}
		}

		dist = VectorLength( v );

		if ( dist >= radius )
		{
			continue;
		}

		points = damage * ( 1.0 - dist / radius );

		if ( CanDamage( ent, origin ) && ent->client &&
		     ent->client->ps.stats[ STAT_TEAM ] != ignoreTeam )
		{
			hitClient = qtrue;

			// don't do knockback, since an attack that spares one team is most likely not based on kinetic energy
			G_Damage( ent, NULL, attacker, NULL, origin, ( int ) points,
			          DAMAGE_RADIUS | DAMAGE_NO_LOCDAMAGE | DAMAGE_NO_KNOCKBACK, mod );
		}
	}

	return hitClient;
}

/*
============
G_RadiusDamage
============
*/
qboolean G_RadiusDamage( vec3_t origin, gentity_t *attacker, float damage,
                         float radius, gentity_t *ignore, int mod )
{
	float     points, dist;
	gentity_t *ent;
	int       entityList[ MAX_GENTITIES ];
	int       numListedEntities;
	vec3_t    mins, maxs;
	vec3_t    v;
	vec3_t    dir;
	int       i, e;
	qboolean  hitClient = qfalse;

	if ( radius < 1 )
	{
		radius = 1;
	}

	for ( i = 0; i < 3; i++ )
	{
		mins[ i ] = origin[ i ] - radius;
		maxs[ i ] = origin[ i ] + radius;
	}

	numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( e = 0; e < numListedEntities; e++ )
	{
		ent = &g_entities[ entityList[ e ] ];

		if ( ent == ignore )
		{
			continue;
		}

		if ( !ent->takedamage )
		{
			continue;
		}

		// find the distance from the edge of the bounding box
		for ( i = 0; i < 3; i++ )
		{
			if ( origin[ i ] < ent->r.absmin[ i ] )
			{
				v[ i ] = ent->r.absmin[ i ] - origin[ i ];
			}
			else if ( origin[ i ] > ent->r.absmax[ i ] )
			{
				v[ i ] = origin[ i ] - ent->r.absmax[ i ];
			}
			else
			{
				v[ i ] = 0;
			}
		}

		dist = VectorLength( v );

		if ( dist >= radius )
		{
			continue;
		}

		points = damage * ( 1.0 - dist / radius );

		if ( CanDamage( ent, origin ) )
		{
			VectorSubtract( ent->r.currentOrigin, origin, dir );
			// push the center of mass higher than the origin so players
			// get knocked into the air more
			dir[ 2 ] += 24;
			VectorNormalize( dir );
			hitClient = qtrue;
			G_Damage( ent, NULL, attacker, dir, origin,
			          ( int ) points, DAMAGE_RADIUS | DAMAGE_NO_LOCDAMAGE, mod );
		}
	}

	return hitClient;
}

/*
================
G_LogDestruction

Log deconstruct/destroy events
================
*/
void G_LogDestruction( gentity_t *self, gentity_t *actor, int mod )
{
	buildFate_t fate;

	switch ( mod )
	{
		case MOD_DECONSTRUCT:
			fate = BF_DECONSTRUCT;
			break;

		case MOD_REPLACE:
			fate = BF_REPLACE;
			break;

		case MOD_NOCREEP:
			fate = ( actor->client ) ? BF_UNPOWER : BF_AUTO;
			break;

		default:
			if ( actor->client )
			{
				if ( actor->client->pers.teamSelection ==
				     BG_Buildable( self->s.modelindex )->team )
				{
					fate = BF_TEAMKILL;
				}
				else
				{
					fate = BF_DESTROY;
				}
			}
			else
			{
				fate = BF_AUTO;
			}

			break;
	}

	G_BuildLogAuto( actor, self, fate );

	// don't log when marked structures are removed
	if ( mod == MOD_REPLACE )
	{
		return;
	}

	G_LogPrintf( S_COLOR_YELLOW "Deconstruct: %d %d %s %s: %s %s by %s\n",
	             ( int )( actor - g_entities ),
	             ( int )( self - g_entities ),
	             BG_Buildable( self->s.modelindex )->name,
	             modNames[ mod ],
	             BG_Buildable( self->s.modelindex )->humanName,
	             mod == MOD_DECONSTRUCT ? "deconstructed" : "destroyed",
	             actor->client ? actor->client->pers.netname : "<world>" );

	// No-power deaths for humans come after some minutes and it's confusing
	//  when the messages appear attributed to the deconner. Just don't print them.
	if ( mod == MOD_NOCREEP && actor->client &&
	     actor->client->pers.teamSelection == TEAM_HUMANS )
	{
		return;
	}

	if ( actor->client && actor->client->pers.teamSelection ==
	     BG_Buildable( self->s.modelindex )->team )
	{
		G_TeamCommand( actor->client->ps.stats[ STAT_TEAM ],
		               va( "print_tr %s %s %s", mod == MOD_DECONSTRUCT ? QQ( N_("$1$ ^3DECONSTRUCTED^7 by $2$\n") ) :
						   QQ( N_("$1$ ^3DESTROYED^7 by $2$\n") ),
		                   Quote( BG_Buildable( self->s.modelindex )->humanName ),
		                   Quote( actor->client->pers.netname ) ) );
	}
}
