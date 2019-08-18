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
/*

===== h_battery.cpp ========================================================

  battery-related code

*/

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

class CItemBattery : public CItem
{
	void Spawn( void );
	void Precache( void );
	BOOL MyTouch( CBasePlayer *pPlayer );
};

LINK_ENTITY_TO_CLASS(item_battery, CItemBattery);

void CItemBattery::Spawn( void )
{ 
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model));
	else
	    SET_MODEL(ENT(pev), "models/w_battery.mdl");

	CItem::Spawn( );
}

void CItemBattery::Precache( void )
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model));
	else
	    PRECACHE_MODEL ("models/w_battery.mdl");

	char* szSoundFile = (char*) STRING( m_IszSound );

	// Step4enko
	if (FStringNull( m_IszSound ))
		PRECACHE_SOUND("items/gunpickup2.wav");
	else
		PRECACHE_SOUND( szSoundFile );
}

BOOL CItemBattery::MyTouch( CBasePlayer *pPlayer )
{
	char* szSoundFile = (char*) STRING( m_IszSound );

	if ( pPlayer->pev->deadflag != DEAD_NO )
	{
		return FALSE;
	}

	if (FStringNull(pev->health))
	{
		if ((pPlayer->pev->armorvalue < MAX_NORMAL_BATTERY) &&
			(pPlayer->pev->weapons & (1<<WEAPON_SUIT)))
		{
			int pct;
			char szcharge[64];

			pPlayer->pev->armorvalue += gSkillData.batteryCapacity;
			pPlayer->pev->armorvalue = min(pPlayer->pev->armorvalue, MAX_NORMAL_BATTERY);

			if (FStringNull( m_IszSound ))
				EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );
			else
				EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, szSoundFile, 1, ATTN_NORM ); 

			if (FStringNull( m_iszCustomHudIcon ))
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
					WRITE_STRING( STRING(pev->classname) );
				MESSAGE_END();
			}
			else
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
					WRITE_STRING( STRING(m_iszCustomHudIcon) );
				MESSAGE_END();
			}
		
			// Suit reports new power level
			// For some reason this wasn't working in release build -- round it.
			pct = (int)( (float)(pPlayer->pev->armorvalue * 100.0) * (1.0/MAX_NORMAL_BATTERY) + 0.5);
			pct = (pct / 5);
			if (pct > 0)
				pct--;
		
			sprintf( szcharge,"!HEV_%1dP", pct );
			
			//EMIT_SOUND_SUIT(ENT(pev), szcharge);
			pPlayer->SetSuitUpdate(szcharge, FALSE, SUIT_NEXT_IN_30SEC);
			return TRUE;		
		}
	}
	else
	{
		if ((pPlayer->pev->armorvalue < MAX_NORMAL_BATTERY) &&
			(pPlayer->pev->weapons & (1<<WEAPON_SUIT)))
		{
			int pct;
			char szcharge[64];

			pPlayer->pev->armorvalue += pev->health;
			pPlayer->pev->armorvalue = min(pPlayer->pev->armorvalue, MAX_NORMAL_BATTERY);

			if (FStringNull( m_IszSound ))
				EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );
			else
				EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, szSoundFile, 1, ATTN_NORM );

			if (FStringNull( m_iszCustomHudIcon ))
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
					WRITE_STRING( STRING(pev->classname) );
				MESSAGE_END();
			}
			else
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
					WRITE_STRING( STRING(m_iszCustomHudIcon) );
				MESSAGE_END();
			}
		
			// Suit reports new power level
			// For some reason this wasn't working in release build -- round it.
			pct = (int)( (float)(pPlayer->pev->armorvalue * 100.0) * (1.0/MAX_NORMAL_BATTERY) + 0.5);
			pct = (pct / 5);
			if (pct > 0)
				pct--;
		
			sprintf( szcharge,"!HEV_%1dP", pct );
			
			//EMIT_SOUND_SUIT(ENT(pev), szcharge);
			pPlayer->SetSuitUpdate(szcharge, FALSE, SUIT_NEXT_IN_30SEC);
			return TRUE;		
		}
	}
	return FALSE;
}

class CRecharge : public CBaseToggle
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

	float   m_flNextCharge; 
	int		m_iReactivate ; // DeathMatch Delay until reactvated
	int		m_iJuice;
	int		m_iOn;			// 0 = off, 1 = startup, 2 = going
	float   m_flSoundTime;
	int     m_IszChargeAmount; // Step4enko
	int	    m_IszDeniedSound; // Step4enko
	int	    m_IszStartSound; // Step4enko
	int	    m_IszLoopSound; // Step4enko
	int     m_IszRechargeSound; // Step4enko
	float   m_IszChargeSpeed; // Step4enko
	string_t     TargetOnEmpty; // Step4enko
	string_t     TargetOnRecharged; // Step4enko
	string_t     TargetOnUse; // Step4enko
};

// Step4enko
int CRecharge::ObjectCaps( void )
{
	return (CBaseToggle :: ObjectCaps() | FCAP_CONTINUOUS_USE) & ~FCAP_ACROSS_TRANSITION |	
		(pev->spawnflags & SF_CHARGER_ONLYDIRECT?FCAP_ONLYDIRECT_USE:0);
}

TYPEDESCRIPTION CRecharge::m_SaveData[] =
{
	DEFINE_FIELD( CRecharge, m_flNextCharge, FIELD_TIME ),
	DEFINE_FIELD( CRecharge, m_iReactivate, FIELD_INTEGER),
	DEFINE_FIELD( CRecharge, m_iJuice, FIELD_INTEGER),
	DEFINE_FIELD( CRecharge, m_iOn, FIELD_INTEGER),
	DEFINE_FIELD( CRecharge, m_flSoundTime, FIELD_TIME ),
	DEFINE_FIELD( CRecharge, m_IszChargeAmount, FIELD_INTEGER ), // Step4enko
	DEFINE_FIELD( CRecharge, m_IszDeniedSound, FIELD_STRING ), // Step4enko
	DEFINE_FIELD( CRecharge, m_IszStartSound, FIELD_STRING ), // Step4enko
	DEFINE_FIELD( CRecharge, m_IszLoopSound, FIELD_STRING ), // Step4enko
	DEFINE_FIELD( CRecharge, m_IszRechargeSound, FIELD_STRING ), // Step4enko
	DEFINE_FIELD( CRecharge, m_IszChargeSpeed, FIELD_FLOAT ), // Step4enko
	DEFINE_FIELD( CRecharge, TargetOnEmpty, FIELD_STRING ), // Step4enko
	DEFINE_FIELD( CRecharge, TargetOnRecharged, FIELD_STRING ), // Step4enko
	DEFINE_FIELD( CRecharge, TargetOnUse, FIELD_STRING ), // Step4enko
};

IMPLEMENT_SAVERESTORE( CRecharge, CBaseEntity );

LINK_ENTITY_TO_CLASS(func_recharge, CRecharge);


void CRecharge::KeyValue( KeyValueData *pkvd )
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
	else if (FStrEq(pkvd->szKeyName, "m_IszChargeAmount"))
	{
		m_IszChargeAmount = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_IszChargeSpeed"))
	{
		m_IszChargeSpeed = atof(pkvd->szValue);
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
	else if (FStrEq(pkvd->szKeyName, "m_IszRechargeSound"))
	{
		m_IszRechargeSound = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "TargetOnEmpty"))
	{
		TargetOnEmpty = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "TargetOnRecharged"))
	{
		TargetOnRecharged = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "TargetOnUse"))
	{
		TargetOnUse = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}

void CRecharge::Spawn()
{
	Precache( );

	// Step4enko
	if ( FBitSet ( pev->spawnflags, SF_CHARGER_NOT_SOLID ) )
	    pev->solid		= SOLID_NOT;
	else
		pev->solid		= SOLID_BSP;

	// Step4enko: Shut down charger imideately if juice amount is less than 1.
	if (m_iJuice <= 0)
	{
		pev->frame = 1;			
		Off();
	}

	pev->movetype	= MOVETYPE_PUSH;

	UTIL_SetOrigin(pev, pev->origin);		// set size and link into world
	UTIL_SetSize(pev, pev->mins, pev->maxs);
	SET_MODEL(ENT(pev), STRING(pev->model) );

	// Step4enko
	if (m_IszChargeAmount == 0)
	    m_iJuice = gSkillData.suitchargerCapacity;
	else
		m_iJuice = m_IszChargeAmount;

	pev->frame = 0;			
}

void CRecharge::Precache()
{
	char* szSoundFile1 = (char*) STRING( m_IszDeniedSound );
	char* szSoundFile2 = (char*) STRING( m_IszStartSound );
	char* szSoundFile3 = (char*) STRING( m_IszLoopSound );
	char* szSoundFile4 = (char*) STRING( m_IszRechargeSound );

	if (!FStringNull(m_IszStartSound))
		PRECACHE_SOUND( szSoundFile2 );
	else
	    PRECACHE_SOUND("items/suitchargeok1.wav" );

	if (!FStringNull(m_IszLoopSound))
		PRECACHE_SOUND( szSoundFile3 );
	else
		PRECACHE_SOUND("items/suitcharge1.wav" );

	if (!FStringNull(m_IszDeniedSound))
		PRECACHE_SOUND( szSoundFile1 );
	else
		PRECACHE_SOUND("items/suitchargeno1.wav" );

	if (!FStringNull(m_IszRechargeSound))
		PRECACHE_SOUND( szSoundFile4 );
	else
	    PRECACHE_SOUND("items/suitchargeok1.wav" );
}


void CRecharge::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{ 
	// If it's not a player, ignore.
	if (!FClassnameIs(pActivator->pev, "player"))
		return;

	// If there is no juice left, turn it off.
	if (m_iJuice <= 0)
	{
		if (!FStringNull(TargetOnEmpty))
	        FireTargets( STRING(TargetOnEmpty), m_hActivator, this, USE_TOGGLE, 0 );
		pev->frame = 1;			
		Off();
	}

	char* szSoundFile1 = (char*) STRING( m_IszDeniedSound );
	char* szSoundFile2 = (char*) STRING( m_IszStartSound );
	char* szSoundFile3 = (char*) STRING( m_IszLoopSound );

	// If the player doesn't have the suit, or there is no juice left, make the deny noise.
	if ((m_iJuice <= 0) || (!(pActivator->pev->weapons & (1<<WEAPON_SUIT))))
	{
		if (m_flSoundTime <= gpGlobals->time)
		{
			m_flSoundTime = gpGlobals->time + 0.62;

			if (!FStringNull( m_IszDeniedSound ))
				EMIT_SOUND(ENT(pev), CHAN_ITEM, szSoundFile1, 1.0, ATTN_NORM );
			else
			    EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/suitchargeno1.wav", 1.0, ATTN_NORM );
		}
		return;
	}

	pev->nextthink = pev->ltime + 0.25;
	SetThink(&CRecharge::Off);

	// Time to recharge yet?
	if (m_flNextCharge >= gpGlobals->time)
		return;

	// Make sure that we have a caller
	if (!pActivator)
		return;

	m_hActivator = pActivator;

	// Only recharge the player.
	if (!m_hActivator->IsPlayer() )
		return;
	
	// Play the on sound or the looping charging sound.
	if (!m_iOn)
	{
		m_iOn++;
	    if (!FStringNull(TargetOnUse))
		    FireTargets( STRING(TargetOnUse), m_hActivator, this, USE_TOGGLE, 0 );

		if (!FStringNull(m_IszStartSound))
			EMIT_SOUND(ENT(pev), CHAN_ITEM, szSoundFile2, 1.0, ATTN_NORM );
		else
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/suitchargeok1.wav", 1.0, ATTN_NORM );

		m_flSoundTime = 0.56 + gpGlobals->time;
	}
	if ((m_iOn == 1) && (m_flSoundTime <= gpGlobals->time))
	{
		m_iOn++;
		if (!FStringNull(m_IszLoopSound))
			EMIT_SOUND(ENT(pev), CHAN_STATIC, szSoundFile3, 1.0, ATTN_NORM );
		else
			EMIT_SOUND(ENT(pev), CHAN_STATIC, "items/suitcharge1.wav", 1.0, ATTN_NORM );
	}

	// Charge the player.
	if (m_hActivator->pev->armorvalue < 100)
	{
		m_iJuice--;
		m_hActivator->pev->armorvalue += 1;

		if (m_hActivator->pev->armorvalue > 100)
			m_hActivator->pev->armorvalue = 100;
	}

	// Step4enko
	if (m_IszChargeSpeed == 0)
		m_flNextCharge = gpGlobals->time + 0.1;
	else
		m_flNextCharge = gpGlobals->time + m_IszChargeSpeed;
}

void CRecharge::Recharge( void )
{
	if (!FStringNull(TargetOnRecharged))
	    FireTargets( STRING(TargetOnRecharged), m_hActivator, this, USE_TOGGLE, 0 );

    char* szSoundFile4 = (char*) STRING( m_IszRechargeSound );

	if (!FStringNull(m_IszRechargeSound))
	    EMIT_SOUND(ENT(pev), CHAN_ITEM, szSoundFile4, 1.0, ATTN_NORM );
	else
	    EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/suitchargeok1.wav", 1.0, ATTN_NORM );

	if (m_IszChargeAmount == 0)
	    m_iJuice = gSkillData.suitchargerCapacity;
	else
		m_iJuice = m_IszChargeAmount;

	pev->frame = 0;			
	SetThink( &CRecharge::SUB_DoNothing );
}

void CRecharge::Off( void )
{
	char* szSoundFile3 = (char*) STRING( m_IszLoopSound );

	// Stop looping sound.
	if (m_iOn > 1)
	{
	    if (!FStringNull(m_IszLoopSound) )
		    STOP_SOUND( ENT(pev), CHAN_STATIC, szSoundFile3 );
		else
			STOP_SOUND( ENT(pev), CHAN_STATIC, "items/suitcharge1.wav" );
	}

	m_iOn = 0;

	if ((!m_iJuice) &&  ( ( m_iReactivate = g_pGameRules->FlHEVChargerRechargeTime() ) > 0) )
	{
		pev->nextthink = pev->ltime + m_iReactivate;
		SetThink(&CRecharge::Recharge);
	}
	else
		SetThink( &CRecharge::SUB_DoNothing );
}