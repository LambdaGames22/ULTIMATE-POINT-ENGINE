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
};


LINK_ENTITY_TO_CLASS( item_healthkit, CHealthKit );

void CHealthKit :: Spawn( void )
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model));
	else
	    SET_MODEL(ENT(pev), "models/w_medkit.mdl");

	CItem::Spawn();
}

void CHealthKit::Precache( void )
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model));
	else
	    PRECACHE_MODEL("models/w_medkit.mdl");

	char* szSoundFile = (char*) STRING( m_IszSound );

	// Step4enko
	if (FStringNull( m_IszSound ))
		PRECACHE_SOUND("items/smallmedkit1.wav");
	else
		PRECACHE_SOUND( szSoundFile );
}

BOOL CHealthKit::MyTouch( CBasePlayer *pPlayer )
{
	char* szSoundFile = (char*) STRING( m_IszSound );

	if ( pPlayer->pev->deadflag != DEAD_NO )
	{
		return FALSE;
	}

    if (FStringNull(pev->health))
	{
		if ( pPlayer->TakeHealth( gSkillData.healthkitCapacity, DMG_GENERIC ) )
		{		
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

			if (FStringNull( m_IszSound ))
		        EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, "items/smallmedkit1.wav", 1, ATTN_NORM);
			else
		        EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, szSoundFile, 1, ATTN_NORM);

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
	}
	else
	{
	    if ( pPlayer->TakeHealth( pev->health, DMG_GENERIC ) )
	    {
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

			if (FStringNull( m_IszSound ))
		        EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, "items/smallmedkit1.wav", 1, ATTN_NORM);
			else
		        EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, szSoundFile, 1, ATTN_NORM);

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

	float   m_flNextCharge;
	int		m_iReactivate; // DeathMatch Delay until reactvated
	int		m_iJuice;
	int		m_iOn;			// 0 = off, 1 = startup, 2 = going
	float   m_flSoundTime;

	int     m_iJuiceAmount; // Step4enko
	int	    m_iszDeniedSound; // Step4enko
	int	    m_iszStartSound; // Step4enko
	int	    m_iszLoopSound; // Step4enko
	int     m_iszRechargeSound; // Step4enko
	float   m_fChargeSpeed; // Step4enko
	string_t     TargetOnEmpty; // Step4enko
	string_t     TargetOnRecharged; // Step4enko
	string_t     TargetOnUse; // Step4enko
};

// Step4enko: ObjectCaps override for SF_CHARGER_ONLYDIRECT.
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
	DEFINE_FIELD( CWallHealth, m_iJuiceAmount, FIELD_INTEGER), // Step4enko
	DEFINE_FIELD( CWallHealth, m_iszDeniedSound, FIELD_STRING ), // Step4enko
	DEFINE_FIELD( CWallHealth, m_iszStartSound, FIELD_STRING ), // Step4enko
	DEFINE_FIELD( CWallHealth, m_iszLoopSound, FIELD_STRING ), // Step4enko
	DEFINE_FIELD( CWallHealth, m_iszRechargeSound, FIELD_STRING ), // Step4enko
	DEFINE_FIELD( CWallHealth, m_fChargeSpeed, FIELD_FLOAT ), // Step4enko
	DEFINE_FIELD( CWallHealth, TargetOnEmpty, FIELD_STRING ), // Step4enko
	DEFINE_FIELD( CWallHealth, TargetOnRecharged, FIELD_STRING ), // Step4enko
	DEFINE_FIELD( CWallHealth, TargetOnUse, FIELD_STRING ), // Step4enko
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
	else if (FStrEq(pkvd->szKeyName, "m_IszJuiceAmount"))
	{
		m_iJuiceAmount = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_IszChargeSpeed"))
	{
		m_fChargeSpeed = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_IszDeniedSound"))
	{
		m_iszDeniedSound = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_IszStartSound"))
	{
		m_iszStartSound = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_IszLoopSound"))
	{
		m_iszLoopSound = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_IszRechargeSound"))
	{
		m_iszRechargeSound = ALLOC_STRING(pkvd->szValue);
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

void CWallHealth::Spawn()
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

	UTIL_SetOrigin(pev, pev->origin);		// Set size and link into world.
	UTIL_SetSize(pev, pev->mins, pev->maxs);
	SET_MODEL(ENT(pev), STRING(pev->model) );

	// Step4enko
	if (m_iJuiceAmount == 0)
	    m_iJuice = gSkillData.healthchargerCapacity;
	else
		m_iJuice = m_iJuiceAmount;

	pev->frame = 0;			
}

void CWallHealth::Precache()
{
	char* szSoundFile1 = (char*) STRING( m_iszDeniedSound );
	char* szSoundFile2 = (char*) STRING( m_iszStartSound );
	char* szSoundFile3 = (char*) STRING( m_iszLoopSound );
	char* szSoundFile4 = (char*) STRING( m_iszRechargeSound );

	if (!FStringNull(m_iszStartSound))
		PRECACHE_SOUND( szSoundFile2 );
	else
	    PRECACHE_SOUND("items/medshot4.wav" );

	if (!FStringNull(m_iszLoopSound))
		PRECACHE_SOUND( szSoundFile3 );
	else
		PRECACHE_SOUND("items/medcharge4.wav" );

	if (!FStringNull(m_iszDeniedSound))
		PRECACHE_SOUND( szSoundFile1 );
	else
		PRECACHE_SOUND("items/medshotno1.wav" );

	if (!FStringNull(m_iszRechargeSound))
		PRECACHE_SOUND( szSoundFile4 );
	else
	    PRECACHE_SOUND("items/medshot4.wav" );
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
		if (!FStringNull(TargetOnEmpty))
	        FireTargets( STRING(TargetOnEmpty), m_hActivator, this, USE_TOGGLE, 0 );
		pev->frame = 1;			
		Off();
	}

	char* szSoundFile1 = (char*) STRING( m_iszDeniedSound );
	char* szSoundFile2 = (char*) STRING( m_iszStartSound );
	char* szSoundFile3 = (char*) STRING( m_iszLoopSound );

	// If the player doesn't have the suit, or there is no juice left, make the deny noise.
	if ((m_iJuice <= 0) || (!(pActivator->pev->weapons & (1<<WEAPON_SUIT))))
	{
		if (m_flSoundTime <= gpGlobals->time)
		{
			m_flSoundTime = gpGlobals->time + 0.62;

			if (!FStringNull( m_iszDeniedSound ))
				EMIT_SOUND(ENT(pev), CHAN_ITEM, szSoundFile1, 1.0, ATTN_NORM );
			else
			    EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/medshotno1.wav", 1.0, ATTN_NORM );
		}
		return;
	}

	pev->nextthink = pev->ltime + 0.25;
	SetThink(&CWallHealth::Off);

	// Time to recharge yet?
	if (m_flNextCharge >= gpGlobals->time)
		return;

	// Play the on sound or the looping charging sound.
	if (!m_iOn)
	{
		m_iOn++;
	    if (!FStringNull(TargetOnUse))
		    FireTargets( STRING(TargetOnUse), m_hActivator, this, USE_TOGGLE, 0 );

		if (!FStringNull(m_iszStartSound))
			EMIT_SOUND(ENT(pev), CHAN_ITEM, szSoundFile2, 1.0, ATTN_NORM );
		else
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/medshot4.wav", 1.0, ATTN_NORM );

		m_flSoundTime = 0.56 + gpGlobals->time;
	}
	if ((m_iOn == 1) && (m_flSoundTime <= gpGlobals->time))
	{
		m_iOn++;
		if (!FStringNull(m_iszLoopSound))
			EMIT_SOUND(ENT(pev), CHAN_STATIC, szSoundFile3, 1.0, ATTN_NORM );
		else
			EMIT_SOUND(ENT(pev), CHAN_STATIC, "items/medcharge4.wav", 1.0, ATTN_NORM );
	}

	// Charge the player.
	if ( pActivator->TakeHealth( 1, DMG_GENERIC ) )
	{
		m_iJuice--;
	}

	// Step4enko
	if (m_fChargeSpeed == 0)
		m_flNextCharge = gpGlobals->time + 0.1;
	else
		m_flNextCharge = gpGlobals->time + m_fChargeSpeed;
}

void CWallHealth::Recharge( void )
{
	if (!FStringNull(TargetOnRecharged))
	    FireTargets( STRING(TargetOnRecharged), m_hActivator, this, USE_TOGGLE, 0 );

    char* szSoundFile4 = (char*) STRING( m_iszRechargeSound );

	if (!FStringNull(m_iszRechargeSound))
	    EMIT_SOUND(ENT(pev), CHAN_ITEM, szSoundFile4, 1.0, ATTN_NORM );
	else
	    EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/medshot4.wav", 1.0, ATTN_NORM );

	if (m_iJuiceAmount == 0)
	    m_iJuice = gSkillData.healthchargerCapacity;
	else
		m_iJuice = m_iJuiceAmount;

	pev->frame = 0;
	SetThink( &CWallHealth::SUB_DoNothing );
}

void CWallHealth::Off( void )
{
	char* szSoundFile3 = (char*) STRING( m_iszLoopSound );

	// Stop looping sound.
	if (m_iOn > 1)
	{
	    if (!FStringNull(m_iszLoopSound) )
		    STOP_SOUND( ENT(pev), CHAN_STATIC, szSoundFile3 );
		else
			STOP_SOUND( ENT(pev), CHAN_STATIC, "items/medcharge4.wav" );
	}

	m_iOn = 0;

	if ((!m_iJuice) && ( ( m_iReactivate = g_pGameRules->FlHealthChargerRechargeTime() ) > 0) )
	{
		pev->nextthink = pev->ltime + m_iReactivate;
		SetThink(&CWallHealth::Recharge);
	}
	else
		SetThink( &CWallHealth::SUB_DoNothing );
}

class CWallHealthJarDecay : public CBaseAnimating
{
public:
	void Spawn();
};

void CWallHealthJarDecay::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_FLY;

	SET_MODEL(ENT(pev), "models/health_charger_both.mdl");
	pev->renderamt = 180;
	pev->rendermode = kRenderTransTexture;
	InitBoneControllers();
}

class CWallHealthDecay : public CBaseAnimating
{
public:
	void Spawn();
	void Precache(void);
	void EXPORT SearchForPlayer();
	void EXPORT Off( void );
	void EXPORT Recharge( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int ObjectCaps( void ) { return ( CBaseAnimating::ObjectCaps() | FCAP_CONTINUOUS_USE ) & ~FCAP_ACROSS_TRANSITION; }
	void TurnNeedleToPlayer(const Vector player);
	void SetNeedleState(int state);
	void SetNeedleController(float yaw);

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	enum 
	{
		Still,
		Deploy,
		Idle,
		GiveShot,
		Healing,
		RetractShot,
		RetractArm,
		Inactive
	};

	float m_flNextCharge; 
	int m_iJuice;
	int m_iState;
	float m_flSoundTime;
	CWallHealthJarDecay* m_jar;
	BOOL m_playingChargeSound;

	int     m_iChargeAmount; // Step4enko
	int	    m_iszDeniedSound; // Step4enko
	int	    m_iszStartSound; // Step4enko
	int	    m_iszLoopSound; // Step4enko
	int     m_iszRechargeSound; // Step4enko
	float   m_fChargeSpeed; // Step4enko
	string_t     TargetOnEmpty; // Step4enko
	string_t     TargetOnRecharged; // Step4enko
	string_t     TargetOnUse; // Step4enko

protected:
	void SetMySequence(const char* sequence);
};

TYPEDESCRIPTION CWallHealthDecay::m_SaveData[] =
{
	DEFINE_FIELD( CWallHealthDecay, m_flNextCharge, FIELD_TIME ),
	DEFINE_FIELD( CWallHealthDecay, m_iJuice, FIELD_INTEGER ),
	DEFINE_FIELD( CWallHealthDecay, m_iState, FIELD_INTEGER ),
	DEFINE_FIELD( CWallHealthDecay, m_flSoundTime, FIELD_TIME ),
	DEFINE_FIELD( CWallHealthDecay, m_jar, FIELD_CLASSPTR),
	DEFINE_FIELD( CWallHealthDecay, m_playingChargeSound, FIELD_BOOLEAN),
	DEFINE_FIELD( CWallHealthDecay, m_iChargeAmount, FIELD_INTEGER ), // Step4enko
	DEFINE_FIELD( CWallHealthDecay, m_iszDeniedSound, FIELD_STRING ), // Step4enko
	DEFINE_FIELD( CWallHealthDecay, m_iszStartSound, FIELD_STRING ), // Step4enko
	DEFINE_FIELD( CWallHealthDecay, m_iszLoopSound, FIELD_STRING ), // Step4enko
	DEFINE_FIELD( CWallHealthDecay, m_iszRechargeSound, FIELD_STRING ), // Step4enko
	DEFINE_FIELD( CWallHealthDecay, m_fChargeSpeed, FIELD_FLOAT ), // Step4enko
	DEFINE_FIELD( CWallHealthDecay, TargetOnEmpty, FIELD_STRING ), // Step4enko
	DEFINE_FIELD( CWallHealthDecay, TargetOnRecharged, FIELD_STRING ), // Step4enko
	DEFINE_FIELD( CWallHealthDecay, TargetOnUse, FIELD_STRING ), // Step4enko
};

IMPLEMENT_SAVERESTORE( CWallHealthDecay, CBaseAnimating )

void CWallHealthDecay::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "m_IszChargeAmount"))
	{
		m_iChargeAmount = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_IszChargeSpeed"))
	{
		m_fChargeSpeed = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_IszDeniedSound"))
	{
		m_iszDeniedSound = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_IszStartSound"))
	{
		m_iszStartSound = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_IszLoopSound"))
	{
		m_iszLoopSound = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_IszRechargeSound"))
	{
		m_iszRechargeSound = ALLOC_STRING(pkvd->szValue);
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
		CBaseAnimating::KeyValue( pkvd );
}

void CWallHealthDecay::Spawn()
{
	Precache();

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model));
	else
		SET_MODEL(ENT(pev), "models/health_charger_body.mdl");

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_FLY;

	UTIL_SetSize(pev, Vector(-12, -16, 0), Vector(12, 16, 48));
	UTIL_SetOrigin(pev, pev->origin);

	// Step4enko
	if (m_iChargeAmount == 0)
	    m_iJuice = gSkillData.healthchargerCapacity;
	else
		m_iJuice = m_iChargeAmount;

	pev->skin = 0;

	m_jar = GetClassPtr( (CWallHealthJarDecay *)NULL );
	m_jar->Spawn();
	UTIL_SetOrigin( m_jar->pev, pev->origin );
	m_jar->pev->owner = ENT( pev );
	m_jar->pev->angles = pev->angles;

	InitBoneControllers();

	if (m_iJuice > 0)
	{
		m_iState = Still;
		SetThink(&CWallHealthDecay::SearchForPlayer);
		pev->nextthink = gpGlobals->time + 0.1;
	}
	else
	{
		m_iState = Inactive;
	}
}

LINK_ENTITY_TO_CLASS(item_healthcharger, CWallHealthDecay)

void CWallHealthDecay::Precache(void)
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model));
	else
		PRECACHE_MODEL("models/health_charger_body.mdl");

	PRECACHE_MODEL("models/health_charger_both.mdl");

	char* szSoundFile1 = (char*) STRING( m_iszDeniedSound );
	char* szSoundFile2 = (char*) STRING( m_iszStartSound );
	char* szSoundFile3 = (char*) STRING( m_iszLoopSound );
	char* szSoundFile4 = (char*) STRING( m_iszRechargeSound );

	if (!FStringNull(m_iszStartSound))
		PRECACHE_SOUND( szSoundFile2 );
	else
	    PRECACHE_SOUND("items/medshot4.wav" );

	if (!FStringNull(m_iszLoopSound))
		PRECACHE_SOUND( szSoundFile3 );
	else
		PRECACHE_SOUND("items/medcharge4.wav" );

	if (!FStringNull(m_iszDeniedSound))
		PRECACHE_SOUND( szSoundFile1 );
	else
		PRECACHE_SOUND("items/medshotno1.wav" );

	if (!FStringNull(m_iszRechargeSound))
		PRECACHE_SOUND( szSoundFile4 );
	else
	    PRECACHE_SOUND("items/medshot4.wav" );
}

void CWallHealthDecay::SearchForPlayer()
{
	CBaseEntity* pEntity = 0;
	float delay = 0.05;
	UTIL_MakeVectors( pev->angles );

	while((pEntity = UTIL_FindEntityInSphere(pEntity, Center(), 64)) != 0) 
	{ 
		// this must be in sync with PLAYER_SEARCH_RADIUS from player.cpp
		if (pEntity->IsPlayer()) 
		{
			if (DotProduct(pEntity->pev->origin - pev->origin, gpGlobals->v_forward) < 0) 
			{
				continue;
			}
			TurnNeedleToPlayer(pEntity->pev->origin);
			switch (m_iState) 
			{
			case RetractShot:
				SetNeedleState(Idle);
				break;
			case RetractArm:
				SetNeedleState(Deploy);
				break;
			case Still:
				SetNeedleState(Deploy);
				delay = 0.1;
				break;
			case Deploy:
				SetNeedleState(Idle);
				break;
			case Idle:
				break;
			default:
				break;
			}
		}
		break;
	}
	if (!pEntity || !pEntity->IsPlayer()) 
	{
		switch (m_iState) 
		{
		case Deploy:
		case Idle:
		case RetractShot:
			SetNeedleState(RetractArm);
			delay = 0.2;
			break;
		case RetractArm:
			SetNeedleState(Still);
			break;
		case Still:
			break;
		default:
			break;
		}
	}
	pev->nextthink = gpGlobals->time + delay;
}

void CWallHealthDecay::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	// Make sure that we have a caller
	if( !pActivator )
		return;

	// if it's not a player, ignore
	if( !pActivator->IsPlayer() )
		return;

	if (m_iState != Idle && m_iState != GiveShot && m_iState != Healing && m_iState != Inactive)
		return;

	// If there is no juice left, turn it off
	if( (m_iState == Healing || m_iState == GiveShot) && m_iJuice <= 0 )
	{
		pev->skin = 1;
		SetThink(&CWallHealthDecay::Off);
		pev->nextthink = gpGlobals->time;
	}

	char* szSoundFile1 = (char*) STRING( m_iszDeniedSound );
	char* szSoundFile2 = (char*) STRING( m_iszStartSound );
	char* szSoundFile3 = (char*) STRING( m_iszLoopSound );

	// if the player doesn't have the suit, or there is no juice left, make the deny noise
	if( ( m_iJuice <= 0 ) || ( !( pActivator->pev->weapons & ( 1 << WEAPON_SUIT ) ) ) )
	{
		if( m_flSoundTime <= gpGlobals->time )
		{
			m_flSoundTime = gpGlobals->time + 0.62;

			if (!FStringNull(TargetOnEmpty))
	        	FireTargets( STRING(TargetOnEmpty), pActivator, this, USE_TOGGLE, 0 );

			if (!FStringNull( m_iszDeniedSound ))
				EMIT_SOUND(ENT(pev), CHAN_ITEM, szSoundFile1, 1.0, ATTN_NORM );
			else
			    EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/medshotno1.wav", 1.0, ATTN_NORM );
		}
		return;
	}

	SetThink(&CWallHealthDecay::Off);
	pev->nextthink = gpGlobals->time + 0.25;

	// Time to recharge yet?
	if( m_flNextCharge >= gpGlobals->time )
		return;

	TurnNeedleToPlayer(pActivator->pev->origin);
	switch (m_iState) 
	{
	case Idle:
		m_flSoundTime = 0.56 + gpGlobals->time;
		SetNeedleState(GiveShot);

	    if (!FStringNull(TargetOnUse))
		    FireTargets( STRING(TargetOnUse), pActivator, this, USE_TOGGLE, 0 );

		if (!FStringNull(m_iszStartSound))
			EMIT_SOUND(ENT(pev), CHAN_ITEM, szSoundFile2, 1.0, ATTN_NORM );
		else
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/medshot4.wav", 1.0, ATTN_NORM );
		break;

	case GiveShot:
		m_jar->pev->sequence = LookupSequence( "slosh" );
		SetNeedleState(Healing);
		break;

	case Healing:
		if (!m_playingChargeSound && m_flSoundTime <= gpGlobals->time)
		{
			if (!FStringNull(m_iszLoopSound))
				EMIT_SOUND(ENT(pev), CHAN_STATIC, szSoundFile3, 1.0, ATTN_NORM );
			else
				EMIT_SOUND(ENT(pev), CHAN_STATIC, "items/medcharge4.wav", 1.0, ATTN_NORM );
			m_playingChargeSound = TRUE;
		}
		break;

	default:
		ALERT(at_console, "Unexpected healthcharger state on use: %d\n", m_iState);
		break;
	}

	// charge the player
	if( pActivator->TakeHealth( 1, DMG_GENERIC ) )
	{
		m_iJuice--;
		const float jarBoneControllerValue = (m_iJuice / gSkillData.healthchargerCapacity) * 11 - 11;
		m_jar->SetBoneController(0,  jarBoneControllerValue );
	}

	// Step4enko
	if (m_fChargeSpeed == 0)
		m_flNextCharge = gpGlobals->time + 0.1;
	else
		m_flNextCharge = gpGlobals->time + m_fChargeSpeed;
}

void CWallHealthDecay::Recharge( void )
{
	if (!FStringNull(TargetOnRecharged))
	    FireTargets( STRING(TargetOnRecharged), NULL, this, USE_TOGGLE, 0 ); // Step4enko: UNDONE: Shouldn't be "NULL" activator.

    char* szSoundFile4 = (char*) STRING( m_iszRechargeSound );

	if (!FStringNull(m_iszRechargeSound))
	    EMIT_SOUND(ENT(pev), CHAN_ITEM, szSoundFile4, 1.0, ATTN_NORM );
	else
	    EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/medshot4.wav", 1.0, ATTN_NORM );

	if (m_iChargeAmount == 0)
	    m_iJuice = gSkillData.healthchargerCapacity;
	else
		m_iJuice = m_iChargeAmount;

	m_jar->SetBoneController(0, 0);
	pev->skin = 0;
	SetNeedleState(Still);
	SetThink( &CWallHealthDecay::SearchForPlayer );
	pev->nextthink = gpGlobals->time;
}

void CWallHealthDecay::Off( void )
{
	char* szSoundFile3 = (char*) STRING( m_iszLoopSound );

	switch (m_iState) 
	{
	case GiveShot:
	case Healing:
		if (m_playingChargeSound) 
		{
	    	if (!FStringNull(m_iszLoopSound) )
				STOP_SOUND( ENT(pev), CHAN_STATIC, szSoundFile3 );
			else
				STOP_SOUND( ENT(pev), CHAN_STATIC, "items/medcharge4.wav" );

			m_playingChargeSound = FALSE;
		}
		SetNeedleState(RetractShot);
		pev->nextthink = gpGlobals->time + 0.1;
		break;
	case RetractShot:
		if (m_iJuice > 0) 
		{
			SetNeedleState(Idle);
			SetThink( &CWallHealthDecay::SearchForPlayer );
			pev->nextthink = gpGlobals->time;
		} 
		else 
		{
			SetNeedleState(RetractArm);
			pev->nextthink = gpGlobals->time + 0.2;
		}
		break;
	case RetractArm:
	{
		if( ( m_iJuice <= 0 ) )
		{
			SetNeedleState(Inactive);
			const float rechargeTime = g_pGameRules->FlHealthChargerRechargeTime();
			if (rechargeTime > 0 ) 
			{
				pev->nextthink = gpGlobals->time + rechargeTime;
				SetThink( &CWallHealthDecay::Recharge );
			}
		}
		break;
	}
	default:
		break;
	}
}

void CWallHealthDecay::SetMySequence(const char *sequence)
{
	pev->sequence = LookupSequence( sequence );
	if (pev->sequence == -1) 
	{
		ALERT(at_error, "unknown sequence: %s\n", sequence);
		pev->sequence = 0;
	}
	pev->frame = 0;
	ResetSequenceInfo( );
}

void CWallHealthDecay::SetNeedleState(int state)
{
	m_iState = state;
	if (state == RetractArm)
		SetNeedleController(0);
	switch (state) {
	case Still:
		SetMySequence("still");
		break;
	case Deploy:
		SetMySequence("deploy");
		break;
	case Idle:
		SetMySequence("prep_shot");
		break;
	case GiveShot:
		SetMySequence("give_shot");
		break;
	case Healing:
		SetMySequence("shot_idle");
		break;
	case RetractShot:
		SetMySequence("retract_shot");
		break;
	case RetractArm:
		SetMySequence("retract_arm");
		break;
	case Inactive:
		SetMySequence("inactive");
	default:
		break;
	}
}

void CWallHealthDecay::TurnNeedleToPlayer(const Vector player)
{
	float yaw = UTIL_VecToYaw( player - pev->origin ) - pev->angles.y;

	if( yaw > 180 )
		yaw -= 360;
	if( yaw < -180 )
		yaw += 360;

	SetNeedleController( yaw );
}

void CWallHealthDecay::SetNeedleController(float yaw)
{
	SetBoneController(0, yaw);
}