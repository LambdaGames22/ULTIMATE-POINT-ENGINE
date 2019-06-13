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
#include "items.h"
#include "gamerules.h"

#define SF_CHARGER_ONLYDIRECT	    16	// Step4enko: Can't use healthcharger thorugh the wall.
#define SF_CHARGER_NOT_SOLID		128	// Step4enko: Charger is not solid.

extern int gmsgItemPickup;

class CHealthKit : public CItem
{
	void Spawn( void );
	void Precache( void );
	BOOL MyTouch( CBasePlayer *pPlayer );

/*
	virtual int		Save( CSave &save ); 
	virtual int		Restore( CRestore &restore );
	
	static	TYPEDESCRIPTION m_SaveData[];
*/

};


LINK_ENTITY_TO_CLASS( item_healthkit, CHealthKit );

/*
TYPEDESCRIPTION	CHealthKit::m_SaveData[] = 
{

};


IMPLEMENT_SAVERESTORE( CHealthKit, CItem);
*/

void CHealthKit :: Spawn( void )
{
	Precache( );
	SET_MODEL(ENT(pev), "models/w_medkit.mdl");

	CItem::Spawn();
}

void CHealthKit::Precache( void )
{
	PRECACHE_MODEL("models/w_medkit.mdl");
	PRECACHE_SOUND("items/smallmedkit1.wav");
}

BOOL CHealthKit::MyTouch( CBasePlayer *pPlayer )
{
	if ( pPlayer->pev->deadflag != DEAD_NO )
	{
		return FALSE;
	}

	if ( pPlayer->TakeHealth( gSkillData.healthkitCapacity, DMG_GENERIC ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
			WRITE_STRING( STRING(pev->classname) );
		MESSAGE_END();

		EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, "items/smallmedkit1.wav", 1, ATTN_NORM);

		if ( g_pGameRules->ItemShouldRespawn( this ) )
		{
			Respawn();
		}
		else
		{
			UTIL_Remove(this);	
		}

		return TRUE;
	}

	return FALSE;
}



//=============================================================
// Wall mounted health kit
//=============================================================
class CWallHealth : public CBaseToggle
{
public:
	void Spawn( );
	void Precache( void );
	void EXPORT Off(void);
	void EXPORT Recharge(void);
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int	ObjectCaps( void );
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	float            m_flNextCharge; 
	int		         m_iReactivate ; // DeathMatch Delay until reactvated
	int		         m_iJuice;
	int		         m_iOn;			// 0 = off, 1 = startup, 2 = going
	float            m_flSoundTime;
	unsigned int     m_IszJuiceValue; // Step4enko
	int	             m_IszDeniedSound; // Step4enko
	int	             m_IszStartSound; // Step4enko
	int	             m_IszLoopSound; // Step4enko
};

// Step4enko
int CWallHealth::ObjectCaps( void )
{
	return (CBaseToggle :: ObjectCaps() | FCAP_CONTINUOUS_USE) & ~FCAP_ACROSS_TRANSITION |	
		(pev->spawnflags & SF_CHARGER_ONLYDIRECT?FCAP_ONLYDIRECT_USE:0);
}

TYPEDESCRIPTION CWallHealth::m_SaveData[] =
{
	DEFINE_FIELD( CWallHealth, m_flNextCharge, FIELD_TIME),
	DEFINE_FIELD( CWallHealth, m_iReactivate, FIELD_INTEGER),
	DEFINE_FIELD( CWallHealth, m_iJuice, FIELD_INTEGER),
	DEFINE_FIELD( CWallHealth, m_iOn, FIELD_INTEGER),
	DEFINE_FIELD( CWallHealth, m_flSoundTime, FIELD_TIME),
	DEFINE_FIELD( CWallHealth, m_IszJuiceValue, FIELD_INTEGER), // Step4enko
	DEFINE_FIELD( CWallHealth, m_IszDeniedSound, FIELD_STRING ), // Step4enko
	DEFINE_FIELD( CWallHealth, m_IszStartSound, FIELD_STRING ), // Step4enko
	DEFINE_FIELD( CWallHealth, m_IszLoopSound, FIELD_STRING ), // Step4enko
};

IMPLEMENT_SAVERESTORE( CWallHealth, CBaseEntity );

LINK_ENTITY_TO_CLASS(func_healthcharger, CWallHealth);


void CWallHealth::KeyValue( KeyValueData *pkvd )
{
	if (	FStrEq(pkvd->szKeyName, "style") ||
				FStrEq(pkvd->szKeyName, "height") ||
				FStrEq(pkvd->szKeyName, "value1") ||
				FStrEq(pkvd->szKeyName, "value2") ||
				FStrEq(pkvd->szKeyName, "value3"))
	{
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "dmdelay"))
	{
		m_iReactivate = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_IszJuiceValue"))
	{
		m_IszJuiceValue = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_IszDeniedSound"))
	{
		m_IszDeniedSound = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_IszStartSound"))
	{
		m_IszStartSound = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_IszLoopSound"))
	{
		m_IszLoopSound = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}

void CWallHealth::Spawn()
{
	Precache( );

	// Step4enko
	if ( FBitSet ( pev->spawnflags, SF_CHARGER_NOT_SOLID ) )
	    pev->solid		= SOLID_NOT;
	else
		pev->solid		= SOLID_BSP;

	// Step4enko: Shut down charger imideately if juice is less than 1.
	if (m_iJuice <= 0)
	{
		pev->frame = 1;			
		Off();
	}

	pev->movetype	= MOVETYPE_PUSH;

	UTIL_SetOrigin(pev, pev->origin);		// Set size and link into world.
	UTIL_SetSize(pev, pev->mins, pev->maxs);
	SET_MODEL(ENT(pev), STRING(pev->model) );

	// Step4enko
	if (m_IszJuiceValue == 0)
	    m_iJuice = gSkillData.healthchargerCapacity;
	else
		m_iJuice = m_IszJuiceValue;

	pev->frame = 0;			

}

void CWallHealth::Precache()
{
	char* szSoundFile1 = (char*) STRING( m_IszDeniedSound );
	char* szSoundFile2 = (char*) STRING( m_IszStartSound );
	char* szSoundFile3 = (char*) STRING( m_IszLoopSound );

	if ( m_IszStartSound == 0 )
	    PRECACHE_SOUND("items/medshot4.wav");
	else
		PRECACHE_SOUND( szSoundFile2 );

	if ( m_IszLoopSound == 0 )
	    PRECACHE_SOUND("items/medcharge4.wav");
	else
		PRECACHE_SOUND( szSoundFile3 );

	if ( m_IszDeniedSound == 0 )
	    PRECACHE_SOUND("items/medshotno1.wav");
	else
		PRECACHE_SOUND( szSoundFile1 );
}


void CWallHealth::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{ 
	// Make sure that we have a caller.
	if (!pActivator)
		return;
	// If it's not a player, ignore.
	if ( !pActivator->IsPlayer() )
		return;

	// If there is no juice left, turn it off.
	if (m_iJuice <= 0)
	{
		pev->frame = 1;			
		Off();
	}

	// if the player doesn't have the suit, or there is no juice left, make the deny noise
	if ((m_iJuice <= 0) || (!(pActivator->pev->weapons & (1<<WEAPON_SUIT))))
	{
		if (m_flSoundTime <= gpGlobals->time)
		{
			m_flSoundTime = gpGlobals->time + 0.62;
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/medshotno1.wav", 1.0, ATTN_NORM );
		}
		return;
	}

	pev->nextthink = pev->ltime + 0.25;
	SetThink(&CWallHealth::Off);

	// Time to recharge yet?
	if (m_flNextCharge >= gpGlobals->time)
		return;

	char* szSoundFile1 = (char*) STRING( m_IszDeniedSound );
	char* szSoundFile2 = (char*) STRING( m_IszStartSound );
	char* szSoundFile3 = (char*) STRING( m_IszLoopSound );

	// Play the on sound or the looping charging sound.
	if (!m_iOn)
	{
		m_iOn++;

		// Step4enko
		if ( m_IszStartSound == 0 )
		    EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/medshot4.wav", 1.0, ATTN_NORM );
		else
			EMIT_SOUND(ENT(pev), CHAN_ITEM, szSoundFile2, 1.0, ATTN_NORM );

		m_flSoundTime = 0.56 + gpGlobals->time;
	}
	if ((m_iOn == 1) && (m_flSoundTime <= gpGlobals->time))
	{
		m_iOn++;

		// Step4enko
		if ( m_IszLoopSound )
		    EMIT_SOUND(ENT(pev), CHAN_STATIC, "items/medcharge4.wav", 1.0, ATTN_NORM );
		else
			EMIT_SOUND(ENT(pev), CHAN_STATIC, szSoundFile3, 1.0, ATTN_NORM );
	}

	// Charge the player.
	if ( pActivator->TakeHealth( 1, DMG_GENERIC ) )
	{
		m_iJuice--;
	}

	// Govern the rate of charge.
	m_flNextCharge = gpGlobals->time + 0.1;
}

void CWallHealth::Recharge(void)
{
	EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/medshot4.wav", 1.0, ATTN_NORM );

	// Step4enko
	if (m_IszJuiceValue == 0)
	    m_iJuice = gSkillData.healthchargerCapacity;
	else
		m_iJuice = m_IszJuiceValue;

	pev->frame = 0;			
	SetThink( &CWallHealth::SUB_DoNothing );
}

void CWallHealth::Off(void)
{
	// Stop looping sound.
	if (m_iOn > 1)
		STOP_SOUND( ENT(pev), CHAN_STATIC, "items/medcharge4.wav" );

	m_iOn = 0;

	if ((!m_iJuice) &&  ( ( m_iReactivate = g_pGameRules->FlHealthChargerRechargeTime() ) > 0) )
	{
		pev->nextthink = pev->ltime + m_iReactivate;
		SetThink(&CWallHealth::Recharge);
	}
	else
		SetThink( &CWallHealth::SUB_DoNothing );
}
