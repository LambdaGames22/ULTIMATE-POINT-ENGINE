/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

extern int gmsgItemPickup;

class CAirtank : public CGrenade
{
	void Spawn( void );
	void Precache( void );
	void EXPORT TankThink( void );
	void EXPORT TankTouch( CBaseEntity *pOther );
	int	 BloodColor( void ) { return DONT_BLEED; };
	void Killed( entvars_t *pevAttacker, int iGib );
	void KeyValue( KeyValueData *pkvd ); // Step4enko

	virtual int		Save( CSave &save ); 
	virtual int		Restore( CRestore &restore );
	
	static	TYPEDESCRIPTION m_SaveData[];

	int	 m_state;
	int  m_iszSound; // Step4enko
	string_t	m_iszCustomHudIcon; // Step4enko
};


LINK_ENTITY_TO_CLASS( item_airtank, CAirtank );
TYPEDESCRIPTION	CAirtank::m_SaveData[] = 
{
	DEFINE_FIELD( CAirtank, m_state, FIELD_INTEGER ),
	DEFINE_FIELD( CAirtank, m_iszSound, FIELD_STRING ), // Step4enko
	DEFINE_FIELD( CAirtank, m_iszCustomHudIcon, FIELD_STRING ), // Step4enko
};

IMPLEMENT_SAVERESTORE( CAirtank, CGrenade );

void CAirtank::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "m_IszSound"))
	{
		m_iszSound = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszCustomHudIcon"))
	{
		m_iszCustomHudIcon = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
}

void CAirtank :: Spawn( void )
{
	Precache( );
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model));
	else
		SET_MODEL(ENT(pev), "models/w_oxygen.mdl");

	if (pev->health == 0)
		pev->health			= 20;

	UTIL_SetSize(pev, Vector( -16, -16, 0), Vector(16, 16, 36));
	UTIL_SetOrigin( pev, pev->origin );

	SetTouch( &CAirtank::TankTouch );
	SetThink( &CAirtank::TankThink );

	pev->flags |= FL_MONSTER;
	pev->takedamage		= DAMAGE_YES;
	pev->dmg			= 50;
	m_state				= 1;
}

void CAirtank::Precache( void )
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model));
	else
		PRECACHE_MODEL("models/w_oxygen.mdl");

	char* szSoundFile = (char*) STRING( m_iszSound );

	// Step4enko
	if (FStringNull( m_iszSound ))
		PRECACHE_SOUND("doors/aliendoor3.wav");
	else
		PRECACHE_SOUND( szSoundFile );
}


void CAirtank :: Killed( entvars_t *pevAttacker, int iGib )
{
	pev->owner = ENT( pevAttacker );

	// UNDONE: this should make a big bubble cloud, not an explosion

	Explode( pev->origin, Vector( 0, 0, -1 ) );
}


void CAirtank::TankThink( void )
{
	// Fire trigger
	m_state = 1;
	SUB_UseTargets( this, USE_TOGGLE, 0 );
}


void CAirtank::TankTouch( CBaseEntity *pOther )
{
	char* szSoundFile = (char*) STRING( m_iszSound );

	if ( !pOther->IsPlayer() )
		return;

	if (!m_state)
	{
		// "no oxygen" sound
		EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_swim2.wav", 1.0, ATTN_NORM );
		return;
	}
	
	// Step4enko
	if (!FStringNull( m_iszCustomHudIcon ))
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pOther->pev );
			WRITE_STRING( STRING(m_iszCustomHudIcon) );
		MESSAGE_END();
	}

	// give player 12 more seconds of air
	pOther->pev->air_finished = gpGlobals->time + 12;

	// Suit recharge sound
	if (FStringNull( m_iszSound ))
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "doors/aliendoor3.wav", 1.0, ATTN_NORM );
	else
		EMIT_SOUND( ENT(pev), CHAN_VOICE, szSoundFile, 1.0, ATTN_NORM );

	// recharge airtank in 30 seconds
	pev->nextthink = gpGlobals->time + 30;
	m_state = 0;
	SUB_UseTargets( this, USE_TOGGLE, 1 );
}
