/***
*
*	Copyright (c) 2016-2020, UPTeam. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This code is made by UPTeam (Half-Life: Ultimate Point developers)
*
*   https://www.moddb.com/mods/half-life-ultimate-point
*
****/

// + Means that feature is implemented
// ? Means that feature is not implemented
// - Means that feature is removed

//=========================================================
// Step4enko: It's very easy to implement them all, but it's not really in the high priority.
//
// TODO:
// + m_iHead instead pev->body (DONE) (Cuz I h8 entvars variables. Fuck them all.)
// + fix bullshit with bloody skins (DONE)
// + wpn drop fix (DONE)
// + wpn pick up fix (DONE)
// + Shell ejection (DONE)
// + Limping Barney (DONE)
// + Bloody skins for dead Barney and limping Barney (DONE)
//
// ? Point entity with name like env_chargerpoint (with monster_ type to use) or something. 
//   So our lovely Barney will use func_healthcharger on it.
//   I think it will be the best way. I removed my old code because it was buggy.
//
// ? RPG/SAW/SHOTGUN/MP5(M4)/CROWBAR for Barney. 
//   (Such shit as MP5(M4) or shotgun should use new attachment to avoid problems with muzzleflash) 
//
// ? GREN_TOSS stuff. Mapper should choose amount of grenades for Barney. Amount of grenades should also affect Barney's bodygroup
//   and change it: If Barney has 4 grenades - there should be selected body group win 4 grenades and e.t.c.
//
// ? Add a check like if ( FClassnameIs( m_hEnemy->pev, "monster_zombie" ) ) then Barney should aim on zombie's head.
//   Because they always saying like "Aim for the head!"
//=========================================================

//=========================================================
// Monster Barney
//=========================================================
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"weapons.h"
#include	"soundent.h"
#include        "barney.h" // Step4enko
#include        "items.h" // Step4enko
#include	"animation.h" // Step4enko

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		BARNEY_AE_DRAW		( 2 )
#define		BARNEY_AE_SHOOT		( 3 )
#define		BARNEY_AE_HOLSTER	( 4 )
#define         BARNEY_AE_RELOAD        ( 5 ) // Step4enko
#define		BARNEY_AE_KICK		( 6 ) // Step4enko
#define		BARNEY_AE_WPNDROP	( 7 ) // Step4enko
#define		BARNEY_AE_WPNPICKUP	( 8 ) // Step4enko
#define		HGRUNT_AE_GREN_TOSS     ( 9 ) // Step4enko

#define BARNEY_LIMP_HEALTH			20

#define	BARNEY_BODY_GUNHOLSTERED	0
#define	BARNEY_BODY_GUNDRAWN		1
#define	BARNEY_BODY_357HOLSTERED	2
#define BARNEY_BODY_357DRAWN		3
#define	BARNEY_BODY_DEAGLEHOLSTERED	4
#define BARNEY_BODY_DEAGLEDRAWN		5
#define BARNEY_BODY_GUNGONE		6

#define	NUM_BARNEY_HEADS	        3

enum 
{ 
	HEAD_NORMAL = 0, 
	HEAD_MOUSTACHE = 1, 
	HEAD_BLACK = 2
};

//=========================================================
// Monster-specific schedule types
//=========================================================
typedef enum
{
    SCHED_BARNEY_HOLSTER = LAST_TALKMONSTER_SCHEDULE + 1,
};

//=========================================================
// Monster-specific tasks
//=========================================================
typedef enum
{
    TASK_BARNEY_HOLSTER = LAST_TALKMONSTER_TASK + 1,
};

LINK_ENTITY_TO_CLASS( monster_barney, CBarney );

const char *CBarney::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CBarney::pAttackMissSounds[] = 
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

TYPEDESCRIPTION	CBarney::m_SaveData[] = 
{
	DEFINE_FIELD( CBarney, m_fGunDrawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBarney, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( CBarney, m_checkAttackTime, FIELD_TIME ),
	DEFINE_FIELD( CBarney, m_lastAttackCheck, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBarney, m_flPlayerDamage, FIELD_FLOAT ),
        DEFINE_FIELD( CBarney, m_iClipSize, FIELD_INTEGER ), // Step4enko
	DEFINE_FIELD( CBarney, m_flNextHolsterTime, FIELD_TIME ), // Step4enko
	DEFINE_FIELD( CBarney, m_iWeapon, FIELD_INTEGER ), // Step4enko
	DEFINE_FIELD( CBarney, m_iHead, FIELD_INTEGER ), // Step4enko
	DEFINE_FIELD( CBarney, m_iGrenades, FIELD_INTEGER ), // Step4enko
};

IMPLEMENT_SAVERESTORE( CBarney, CTalkMonster );

void CBarney :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "TriggerTarget"))
	{
		m_iszTriggerTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "TriggerCondition") )
	{
		m_iTriggerCondition = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iClass") )
	{
		m_iClass = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iWeapon") )
	{
		m_iWeapon = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iHead") )
	{
		m_iHead = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iGrenades") )
	{
		m_iGrenades = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t	tlBaFollow[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE,(float)128		},	// Move within 128 of target ent (client)
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};

Schedule_t	slBaFollow[] =
{
	{
		tlBaFollow,
		ARRAYSIZE ( tlBaFollow ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"Follow"
	},
};

//=========================================================
// BarneyDraw- much better looking draw schedule for when
// barney knows who he's gonna attack.
//=========================================================
Task_t	tlBarneyEnemyDraw[] =
{
	{ TASK_STOP_MOVING,					0				},
	{ TASK_FACE_ENEMY,					0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float) ACT_ARM },
};

Schedule_t slBarneyEnemyDraw[] = 
{
	{
		tlBarneyEnemyDraw,
		ARRAYSIZE ( tlBarneyEnemyDraw ),
		0,
		0,
		"Barney Enemy Draw"
	}
};

//=========================================================
// BarneyHolster
//=========================================================
Task_t    tlBarneyHolster[] =
{
    { TASK_STOP_MOVING,0    },
    { TASK_PLAY_SEQUENCE,(float) ACT_DISARM    },
};

Schedule_t slBarneyHolster[] =
{
    {
        tlBarneyHolster,
        ARRAYSIZE ( tlBarneyHolster ),
        bits_COND_HEAVY_DAMAGE,
        
        0,
        "Barney Holster"
    }
};

Task_t	tlBaFaceTarget[] =
{
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE },
};

Schedule_t	slBaFaceTarget[] =
{
	{
		tlBaFaceTarget,
		ARRAYSIZE ( tlBaFaceTarget ),
		bits_COND_CLIENT_PUSH	|
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"FaceTarget"
	},
};


Task_t	tlIdleBaStand[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		}, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET,		(float)0		}, // reset head position
};

Schedule_t	slIdleBaStand[] =
{
	{ 
		tlIdleBaStand,
		ARRAYSIZE ( tlIdleBaStand ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND	|
		bits_COND_SMELL			|
		bits_COND_PROVOKED,

		bits_SOUND_COMBAT		|// sound flags - change these, and you'll break the talking code.
		//bits_SOUND_PLAYER		|
		bits_SOUND_WORLD		| // FORTEST
		
		bits_SOUND_DANGER		|
		bits_SOUND_MEAT			|// scents
		bits_SOUND_CARCASS		|
		bits_SOUND_GARBAGE,
		"IdleStand"
	},
};

DEFINE_CUSTOM_SCHEDULES( CBarney )
{
	slBaFollow,
	slBarneyEnemyDraw,
	slBaFaceTarget,
	slIdleBaStand,
	slBarneyHolster, // Step4enko
};


IMPLEMENT_CUSTOM_SCHEDULES( CBarney, CTalkMonster );

void CBarney :: StartTask( Task_t *pTask )
{
	CTalkMonster::StartTask( pTask );	
}

void CBarney :: RunTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		if (m_hEnemy != NULL && (m_hEnemy->IsPlayer()))
		{
			pev->framerate = 1.5;
		}
		CTalkMonster::RunTask( pTask );
		break;
	default:
		CTalkMonster::RunTask( pTask );
		break;
	}
}

// Step4enko
void CBarney :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_RUN:
		if ( pev->health <= BARNEY_LIMP_HEALTH )
		{
			iSequence = LookupActivity ( ACT_RUN_HURT );
		}
		else
		{
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_WALK:
		if ( pev->health <= BARNEY_LIMP_HEALTH )
		{
			iSequence = LookupActivity ( ACT_WALK_HURT );
		}
		else
		{
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	default:
		iSequence = LookupActivity ( NewActivity );
		break;
	}
	m_Activity = NewActivity;

	// Set to the desired animation, or default anim if the desired is not present.
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if ( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			pev->frame = 0;
		}

		pev->sequence		= iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo( );
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT ( at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity );
		pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}
}


//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CBarney :: ISoundMask ( void) 
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_CARCASS	|
			bits_SOUND_MEAT		|
			bits_SOUND_GARBAGE	|
			bits_SOUND_DANGER	|
			bits_SOUND_PLAYER;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CBarney :: Classify ( void )
{
	return m_iClass?m_iClass:CLASS_PLAYER_ALLY;
}

//=========================================================
// Kick
//=========================================================
CBaseEntity *CBarney :: Kick( void )
{
	TraceResult tr;

	UTIL_MakeVectors( pev->angles );
	Vector vecStart = pev->origin;
	vecStart.z += pev->size.z * 0.5;
	Vector vecEnd = vecStart + (gpGlobals->v_forward * 70);

	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr );
	
	if ( tr.pHit )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		return pEntity;
	}

	return NULL;
}

//=========================================================
// ALertSound - barney says "Freeze!"
//=========================================================
void CBarney :: AlertSound( void )
{
	if ( m_hEnemy != NULL )
	{
		if ( F0kToSpeak() )
		{
			PlaySentence( "BA_ATTACK", RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE );
		}
	}
}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CBarney :: SetYawSpeed ( void )
{
	int ys;

	ys = 0;

	switch ( m_Activity )
	{
	case ACT_IDLE:		
		ys = 600;
		break;
	case ACT_WALK:
		ys = 600;
		break;
	case ACT_RUN:
		ys = 600;
		break;
	default:
		ys = 600;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CBarney :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( flDist <= 1024 && flDot >= 0.5 )
	{
		if ( flDist <= 64 )
		{
			// kick nonclients who are close enough, but don't shoot at them.
			return FALSE;
		}

		if ( gpGlobals->time > m_checkAttackTime )
		{
			TraceResult tr;
			
			Vector shootOrigin = pev->origin + Vector( 0, 0, 55 );
			CBaseEntity *pEnemy = m_hEnemy;
			Vector shootTarget = ( (pEnemy->BodyTarget( shootOrigin ) - pEnemy->pev->origin) + m_vecEnemyLKP );
			UTIL_TraceLine( shootOrigin, shootTarget, dont_ignore_monsters, ENT(pev), &tr );
			m_checkAttackTime = gpGlobals->time + 1;
			if ( tr.flFraction == 1.0 || (tr.pHit != NULL && CBaseEntity::Instance(tr.pHit) == pEnemy) )
				m_lastAttackCheck = TRUE;
			else
				m_lastAttackCheck = FALSE;
			m_checkAttackTime = gpGlobals->time + 1.5;
		}
		return m_lastAttackCheck;
	}
	return FALSE;
}

// Step4enko: SHITT
/*
BOOL CBarney::ItemTouch( CBaseEntity *pOther )
{
	if ( &CBarney::Killed )
	{
		return FALSE;
	}

	if ( CBarney::TakeDamage( NULL, NULL, gSkillData.healthkitCapacity, DMG_GENERIC) )
	{
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "items/smallmedkit1.wav", 1, ATTN_NORM, 0, 100 );
		UTIL_Remove(this);

		return TRUE;
	}

	return FALSE;
}*/

//=========================================================
// BarneyFirePistol - shoots one round from the pistol at
// the enemy barney is facing.
//=========================================================
void CBarney :: BarneyFirePistol ( void )
{
	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = pev->origin + Vector( 0, 0, 55 ); // 55
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
	pev->effects = EF_MUZZLEFLASH;

	FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_2DEGREES, 1024, BULLET_MONSTER_9MM );
	
    if ( m_cAmmoLoaded <= 0 )
        SetConditions(bits_COND_NO_AMMO_LOADED);

	int pitchShift = RANDOM_LONG( 0, 20 );
	
	// Only shift about half the time
	if ( pitchShift > 10 )
		pitchShift = 0;
	else
		pitchShift -= 5;

	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/pl_gun3.wav", 1, ATTN_NORM, 0, 100 + pitchShift );

	Vector vecShootShell = pev->origin + Vector( -20, 0, 55 );
	Vector vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 100) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass(vecShootShell, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL);

    MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecShootOrigin );
        WRITE_BYTE(TE_DLIGHT);
        WRITE_COORD(vecShootOrigin.x);
        WRITE_COORD(vecShootOrigin.y);
        WRITE_COORD(vecShootOrigin.z);
        WRITE_BYTE( RANDOM_LONG(15,20) );		// radius
        WRITE_BYTE( RANDOM_LONG(240,255) );		// r
        WRITE_BYTE( RANDOM_LONG(165,185) );		// g
        WRITE_BYTE( RANDOM_LONG(90, 102) );		// b
        //WRITE_BYTE( RANDOM_LONG(1,10) / pev->framerate );		// time * 10 WAS 20
		WRITE_BYTE( 10 );		// time * 10 WAS 20
        WRITE_BYTE( RANDOM_LONG( 180, 195) );		// decay * 0.1
    MESSAGE_END( );

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
	m_cAmmoLoaded--;
}

void CBarney :: BarneyFirePython ( void )
{
	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = pev->origin + Vector( 0, 0, 55 );
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
	pev->effects = EF_MUZZLEFLASH;

	FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_3DEGREES, 1024, BULLET_PLAYER_357 );
	
    if ( m_cAmmoLoaded <= 0 )
        SetConditions(bits_COND_NO_AMMO_LOADED);

	int pitchShift = RANDOM_LONG( 0, 20 );
	
	// Only shift about half the time
	if ( pitchShift > 10 )
		pitchShift = 0;
	else
		pitchShift -= 5;

	switch(RANDOM_LONG(0,1))
	{	
	case 0:	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/357_shot1.wav", 1, ATTN_NORM, 0, 100 + pitchShift ); break;
	case 1:	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/357_shot2.wav", 1, ATTN_NORM, 0, 100 + pitchShift ); break;
	}

	Vector vecShootShell = pev->origin + Vector( -20, 0, 55 );
	Vector vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 100) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass(vecShootShell, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL);

    MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecShootOrigin );
        WRITE_BYTE(TE_DLIGHT);
        WRITE_COORD(vecShootOrigin.x);
        WRITE_COORD(vecShootOrigin.y);
        WRITE_COORD(vecShootOrigin.z);
        WRITE_BYTE( RANDOM_LONG(15,20) );		// radius
        WRITE_BYTE( RANDOM_LONG(240,255) );		// r
        WRITE_BYTE( RANDOM_LONG(165,185) );		// g
        WRITE_BYTE( RANDOM_LONG(90, 102) );		// b
        //WRITE_BYTE( RANDOM_LONG(1,10) / pev->framerate );		// time * 10 WAS 20
		WRITE_BYTE( 10 );		// time * 10 WAS 20
        WRITE_BYTE( RANDOM_LONG( 180, 195) );		// decay * 0.1
    MESSAGE_END( );

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
	m_cAmmoLoaded--;
}

void CBarney :: BarneyFireDeagle ( void )
{
	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = pev->origin + Vector( 0, 0, 55 );
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
	pev->effects = EF_MUZZLEFLASH;

	FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_3DEGREES, 1024, BULLET_PLAYER_357 );
	
    if ( m_cAmmoLoaded <= 0 )
        SetConditions(bits_COND_NO_AMMO_LOADED);

	int pitchShift = RANDOM_LONG( 0, 20 );
	
	// Only shift about half the time
	if ( pitchShift > 10 )
		pitchShift = 0;
	else
		pitchShift -= 5;
	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/desert_eagle_fire.wav", 1, ATTN_NORM, 0, 100 + pitchShift );

	Vector vecShootShell = pev->origin + Vector( -20, 0, 55 );
	Vector vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 100) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass(vecShootShell, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL);

    MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecShootOrigin );
        WRITE_BYTE(TE_DLIGHT);
        WRITE_COORD(vecShootOrigin.x);
        WRITE_COORD(vecShootOrigin.y);
        WRITE_COORD(vecShootOrigin.z);
        WRITE_BYTE( RANDOM_LONG(15,20) );		// radius
        WRITE_BYTE( RANDOM_LONG(240,255) );		// r
        WRITE_BYTE( RANDOM_LONG(165,185) );		// g
        WRITE_BYTE( RANDOM_LONG(90, 102) );		// b
        //WRITE_BYTE( RANDOM_LONG(1,10) / pev->framerate );		// time * 10 WAS 20
		WRITE_BYTE( 10 );		// time * 10 WAS 20
        WRITE_BYTE( RANDOM_LONG( 180, 195) );		// decay * 0.1
    MESSAGE_END( );

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
	m_cAmmoLoaded--;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CBarney :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case BARNEY_AE_SHOOT:
		if (m_iWeapon == 1 || FStringNull(m_iWeapon))
			BarneyFirePistol();
		if (m_iWeapon == 2)
			BarneyFirePython();
		if (m_iWeapon == 3)
			BarneyFireDeagle();
		break;

	case BARNEY_AE_WPNDROP:
		{
			Vector vecGunPos;
			Vector vecGunAngles;
	
			pev->body = BARNEY_BODY_GUNGONE;

			GetAttachment( 0, vecGunPos, vecGunAngles );

			if (m_iWeapon == 1 || FStringNull(m_iWeapon))
			{
				CBaseEntity *pGun = DropItem( "weapon_9mmhandgun", vecGunPos, vecGunAngles );
				pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
				pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
			}
			if (m_iWeapon == 2)
			{
				CBaseEntity *pGun = DropItem( "weapon_357", vecGunPos, vecGunAngles );
				pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
				pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
			}
			if (m_iWeapon == 3)
			{
				CBaseEntity *pGun = DropItem( "weapon_357", vecGunPos, vecGunAngles ); // deagle
				pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
				pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
			}
			if (m_iWeapon == 4)
			{
				CBaseEntity *pGun = DropItem( "weapon_shotgun", vecGunPos, vecGunAngles );
				pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
				pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
			}
			if (m_iWeapon == 5)
			{
				CBaseEntity *pGun = DropItem( "weapon_9mmAR", vecGunPos, vecGunAngles );
				pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
				pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
			}
			if (m_iWeapon == 6)
			{
				CBaseEntity *pGun = DropItem( "weapon_crowbar", vecGunPos, vecGunAngles );
				pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
				pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
			}
			if (m_iWeapon == 7)
			{
				CBaseEntity *pGun = DropItem( "weapon_rpg", vecGunPos, vecGunAngles );
				pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
				pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
			}
			if (m_iWeapon == 8)
			{
				CBaseEntity *pGun = DropItem( "weapon_saw", vecGunPos, vecGunAngles );
				pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
				pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
			}
		}
		break;

	case BARNEY_AE_WPNPICKUP:
		{
			EMIT_SOUND( ENT(pev), CHAN_WEAPON, "item/gunpickup2.wav", 1, ATTN_NORM );
		}
		break;

	case BARNEY_AE_DRAW:
		// Barney's bodygroup switches here so he can pull gun from holster.
		if (m_iWeapon == 1 || FStringNull(m_iWeapon))
			pev->body = BARNEY_BODY_GUNDRAWN;
		if (m_iWeapon == 2)
			pev->body = BARNEY_BODY_357DRAWN;
		if (m_iWeapon == 3)
			pev->body = BARNEY_BODY_DEAGLEDRAWN;
		m_fGunDrawn = TRUE;
		break;

	case BARNEY_AE_HOLSTER:
		// Change bodygroup to replace gun in holster.
		if (m_iWeapon == 1 || FStringNull(m_iWeapon))
			pev->body = BARNEY_BODY_GUNHOLSTERED;
		if (m_iWeapon == 2)
			pev->body = BARNEY_BODY_357HOLSTERED;
		if (m_iWeapon == 3)
			pev->body = BARNEY_BODY_DEAGLEHOLSTERED;

		m_fGunDrawn = FALSE;
		break;

	// Step4enko
	case BARNEY_AE_RELOAD:
        EMIT_SOUND( ENT(pev), CHAN_WEAPON, "hgrunt/gr_reload1.wav", 1, ATTN_NORM );
        m_cAmmoLoaded = m_iClipSize;
        ClearConditions( bits_COND_NO_AMMO_LOADED );
        break;

	// Step4enko
	case BARNEY_AE_KICK:
		{
			CBaseEntity *pHurt = Kick();

			if ( pHurt )
			{
				// Play a random attack hit sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

				// SOUND HERE!
				UTIL_MakeVectors( pev->angles );
				pHurt->pev->punchangle.x = 140; // 15                              // 100
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 200 + gpGlobals->v_up * 50; // * 50
				pHurt->TakeDamage( pev, pev, gSkillData.hgruntDmgKick + RANDOM_LONG( 5, 15 ), DMG_CLUB );	
			}
			else
			{
				// Play a random attack miss sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}		
		}
		break;

	default:
		CTalkMonster::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// Spawn
//=========================================================
void CBarney :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model));
	else
	    SET_MODEL(ENT(pev), "models/barney.mdl");

	if (pev->health == 0)
	    pev->health			= gSkillData.barneyHealth + 5;

	if (pev->health <= BARNEY_LIMP_HEALTH)
		pev->skin = RANDOM_LONG(1,4);

	if (m_iHead == -1)
		SetBodygroup(2, RANDOM_LONG(0, NUM_BARNEY_HEADS-1));

	if (m_iHead == 0)
		SetBodygroup(2, 0);

	if (m_iHead == 1)
		SetBodygroup(2, 1);

	if (m_iHead == 2)
		SetBodygroup(2, 2);

	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;

	pev->view_ofs		= Vector ( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;
	
	m_fGunDrawn			= FALSE;

	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	if (m_iWeapon == 1 || FStringNull(m_iWeapon))
	{
		m_iClipSize = 17;
		SetBodygroup(1, 0);
	}

	if (m_iWeapon == 2)
	{
		m_iClipSize = 6;
		SetBodygroup(1, 2);
	}

	if (m_iWeapon == 3)
	{
		m_iClipSize = 7;
		SetBodygroup(1, 4);
	}

	if (m_iWeapon == 4)
		m_iClipSize = 8;

	if (m_iWeapon == 5)
		m_iClipSize = 30;

	if (m_iWeapon == 6)
		m_iClipSize = -1;

	if (m_iWeapon == 7)
		m_iClipSize = 1;

	if (m_iWeapon == 8)
		m_iClipSize = 100;

	if (m_iWeapon == 9)
		m_iWeapon = RANDOM_LONG(0,8);

    m_cAmmoLoaded = m_iClipSize;
	m_flNextHolsterTime = gpGlobals->time;

	MonsterInit();
	SetUse( &CBarney::FollowerUse );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CBarney :: Precache()
{
	int i;

	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model));
	else
	    PRECACHE_MODEL("models/barney.mdl");

	m_iBrassShell = PRECACHE_MODEL("models/shell.mdl");

	for ( i = 0; i < ARRAYSIZE( pAttackHitSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackHitSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackMissSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackMissSounds[i]);

	if (m_iWeapon == 1 || FStringNull(m_iWeapon))
		PRECACHE_SOUND("weapons/pl_gun3.wav" );

	if (m_iWeapon == 2)
	{
		PRECACHE_SOUND("weapons/357_shot1.wav" );
		PRECACHE_SOUND("weapons/357_shot2.wav" );
	}

	if (m_iWeapon == 3)
		PRECACHE_SOUND("weapons/desert_eagle_fire.wav" );

	PRECACHE_SOUND("hgrunt/gr_reload1.wav");

	PRECACHE_SOUND("barney/ba_attack1.wav" );
	PRECACHE_SOUND("barney/ba_attack2.wav" );

	PRECACHE_SOUND("barney/ba_pain1.wav");
	PRECACHE_SOUND("barney/ba_pain2.wav");
	PRECACHE_SOUND("barney/ba_pain3.wav");
	PRECACHE_SOUND("barney/ba_pain4.wav");
	PRECACHE_SOUND("barney/ba_pain5.wav");

	PRECACHE_SOUND("barney/ba_die1.wav");
	PRECACHE_SOUND("barney/ba_die2.wav");
	PRECACHE_SOUND("barney/ba_die3.wav");
	PRECACHE_SOUND("barney/ba_die4.wav");
	
	// every new barney must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	CTalkMonster::Precache();
}	

// Init talk data
void CBarney :: TalkInit()
{
	CTalkMonster::TalkInit();

	// scientists speach group names (group names are in sentences.txt)

	m_szGrp[TLK_ANSWER]  =	"BA_ANSWER";
	m_szGrp[TLK_QUESTION] =	"BA_QUESTION";
	m_szGrp[TLK_IDLE] =		"BA_IDLE";
	m_szGrp[TLK_STARE] =	"BA_STARE";
	m_szGrp[TLK_USE] =		"BA_OK";
	m_szGrp[TLK_UNUSE] =	"BA_WAIT";
	m_szGrp[TLK_STOP] =		"BA_STOP";

	m_szGrp[TLK_NOSHOOT] =	"BA_SCARED";
	m_szGrp[TLK_HELLO] =	"BA_HELLO";

	m_szGrp[TLK_PLHURT1] =	"BA_CUREA";
	m_szGrp[TLK_PLHURT2] =	"BA_CUREB"; 
	m_szGrp[TLK_PLHURT3] =	"BA_CUREC";

	m_szGrp[TLK_PHELLO] =	NULL;	//"BA_PHELLO";		// UNDONE
	m_szGrp[TLK_PIDLE] =	NULL;	//"BA_PIDLE";			// UNDONE
	m_szGrp[TLK_PQUESTION] = "BA_PQUEST";		// UNDONE

	m_szGrp[TLK_SMELL] =	"BA_SMELL";
	
	m_szGrp[TLK_WOUND] =	"BA_WOUND";
	m_szGrp[TLK_MORTAL] =	"BA_MORTAL";

//	m_szGrp[TLK_SORRY]  =	"BA_SORRY"; // Step4enko

	// Get voice for head.
	if (pev->body == 2)
		m_voicePitch = 93;
	else
		m_voicePitch = 100;

	switch (pev->body % 3)
	{
	default:
	case HEAD_NORMAL:	 m_voicePitch = 100; break;
	case HEAD_MOUSTACHE: m_voicePitch = 100; break;
	case HEAD_BLACK:	 m_voicePitch = 93;  break;
	}
}


static BOOL IsFacing( entvars_t *pevTest, const Vector &reference )
{
	Vector vecDir = (reference - pevTest->origin);
	vecDir.z = 0;
	vecDir = vecDir.Normalize();
	Vector forward, angle;
	angle = pevTest->v_angle;
	angle.x = 0;
	UTIL_MakeVectorsPrivate( angle, forward, NULL, NULL );
	// He's facing me, he meant it
	if ( DotProduct( forward, vecDir ) > 0.96 )	// +/- 15 degrees or so
	{
		return TRUE;
	}
	return FALSE;
}


int CBarney :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// make sure friends talk about it if player hurts talkmonsters...
	int ret = CTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
	if ( !IsAlive() || pev->deadflag == DEAD_DYING )
		return ret;

	if ( m_MonsterState != MONSTERSTATE_PRONE && (pevAttacker->flags & FL_CLIENT) )
	{
		m_flPlayerDamage += flDamage;

		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if ( m_hEnemy == NULL )
		{
			// If the player was facing directly at me, or I'm already suspicious, get mad
			if ( (m_afMemory & bits_MEMORY_SUSPICIOUS) || IsFacing( pevAttacker, pev->origin ) )
			{
				// Alright, now I'm pissed!
				PlaySentence( "BA_MAD", 4, VOL_NORM, ATTN_NORM );

				Remember( bits_MEMORY_PROVOKED );
				StopFollowing( TRUE );
			}
			else
			{
				// Hey, be careful with that
				PlaySentence( "BA_SHOT", 4, VOL_NORM, ATTN_NORM );
				Remember( bits_MEMORY_SUSPICIOUS );
			}
		}
		else if ( !(m_hEnemy->IsPlayer()) && pev->deadflag == DEAD_NO )
		{
			PlaySentence( "BA_SHOT", 4, VOL_NORM, ATTN_NORM );
		}
	}

	return ret;
}

	
//=========================================================
// PainSound
//=========================================================
void CBarney :: PainSound ( void )
{
	if (gpGlobals->time < m_painTime)
		return;
	
	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	switch (RANDOM_LONG(0,4))
	{
	case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_pain2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 2: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_pain3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 3: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_pain4.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 4: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_pain5.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CBarney :: DeathSound ( void )
{
	switch (RANDOM_LONG(0,3))
	{
	case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_die1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_die2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 2: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_die3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 3: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_die4.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}


void CBarney::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	switch( ptr->iHitgroup)
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		if (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_BLAST))
		{
			flDamage = flDamage / 2;
		}
		break;
	case 10:
		if (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_CLUB))
		{
			flDamage -= 20;
			if (flDamage <= 0)
			{
				UTIL_Ricochet( ptr->vecEndPos, 1.0 );
				flDamage = 0.01;
			}
		}
		// always a head shot
		ptr->iHitgroup = HITGROUP_HEAD;
		break;
	}

	CTalkMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


void CBarney::Killed( entvars_t *pevAttacker, int iGib )
{
	if ( pev->body < BARNEY_BODY_GUNGONE && !(pev->spawnflags & SF_MONSTER_NO_WPN_DROP) )
	{
		// drop the gun!
		Vector vecGunPos;
		Vector vecGunAngles;

		pev->body = BARNEY_BODY_GUNGONE;

		GetAttachment( 0, vecGunPos, vecGunAngles );
		
		if (m_iWeapon == 1 || FStringNull(m_iWeapon))
		{
			CBaseEntity *pGun = DropItem( "weapon_9mmhandgun", vecGunPos, vecGunAngles );
			pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
			pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
		}
		if (m_iWeapon == 2)
		{
			CBaseEntity *pGun = DropItem( "weapon_357", vecGunPos, vecGunAngles );
			pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
			pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
		}
		if (m_iWeapon == 3)
		{
			CBaseEntity *pGun = DropItem( "weapon_357", vecGunPos, vecGunAngles ); // deagle
			pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
			pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
		}
		if (m_iWeapon == 4)
		{
			CBaseEntity *pGun = DropItem( "weapon_shotgun", vecGunPos, vecGunAngles );
			pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
			pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
		}
		if (m_iWeapon == 5)
		{
			CBaseEntity *pGun = DropItem( "weapon_9mmAR", vecGunPos, vecGunAngles );
			pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
			pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
		}
		if (m_iWeapon == 6)
		{
			CBaseEntity *pGun = DropItem( "weapon_crowbar", vecGunPos, vecGunAngles );
			pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
			pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
		}
		if (m_iWeapon == 7)
		{
			CBaseEntity *pGun = DropItem( "weapon_rpg", vecGunPos, vecGunAngles );
			pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
			pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
		}
		if (m_iWeapon == 8)
		{
			CBaseEntity *pGun = DropItem( "weapon_saw", vecGunPos, vecGunAngles );
			pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
			pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
		}
	}

	SetUse( NULL );	
	CTalkMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Schedule_t* CBarney :: GetScheduleOfType ( int Type )
{
	Schedule_t *psched;

	switch( Type )
	{
	case SCHED_ARM_WEAPON:
		if ( m_hEnemy != NULL )
		{
			// face enemy, then draw.
			return slBarneyEnemyDraw;
		}
		break;

	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that barney will talk
		// when 'used' 
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slBaFaceTarget;	// override this for different target face behavior
		else
			return psched;

	case SCHED_TARGET_CHASE:
		return slBaFollow;

    case SCHED_BARNEY_HOLSTER:
        if ( m_hEnemy == NULL )
            return slBarneyHolster;
        break;
	case SCHED_IDLE_STAND:
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
		{
			// just look straight ahead.
			return slIdleBaStand;
		}
		else
			return psched;	
	}

	return CTalkMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CBarney :: GetSchedule ( void )
{
	if ( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if ( pSound && (pSound->m_iType & bits_SOUND_DANGER) )
		{
			// Step4enko
		    if ( F0kToSpeak() )
		    {
			    PlaySentence( "BA_GRENADE", 4, VOL_NORM, ATTN_NORM );
			    return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
			}
		}
	}
	if ( HasConditions( bits_COND_ENEMY_DEAD ) && F0kToSpeak() )
	{
		PlaySentence( "BA_KILL", 4, VOL_NORM, ATTN_NORM );
	}

	switch( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
			if (pev->health < BARNEY_LIMP_HEALTH)
				pev->skin = 2;

			// Dead enemy.
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// Call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

			// Dead enemy.
            if ( HasConditions( bits_COND_ENEMY_DEAD ) )
            {
                m_flNextHolsterTime = gpGlobals->time + RANDOM_FLOAT( 5.0, 10.0 );
                return CBaseMonster :: GetSchedule();
            }

			// always act surprized with a new enemy
			if ( HasConditions( bits_COND_NEW_ENEMY ) && HasConditions( bits_COND_LIGHT_DAMAGE) )
				return GetScheduleOfType( SCHED_SMALL_FLINCH );
				
			// wait for one schedule to draw gun
			if (!m_fGunDrawn )
				return GetScheduleOfType( SCHED_ARM_WEAPON );

			if ( HasConditions( bits_COND_HEAVY_DAMAGE ) )
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
			// no ammo
            if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
            {
                return GetScheduleOfType ( SCHED_RELOAD );
            }
		}
		break;

	case MONSTERSTATE_ALERT:				
	case MONSTERSTATE_IDLE:
		if (pev->health < BARNEY_LIMP_HEALTH)
			pev->skin = 2;
		if ( HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
		{
			// flinch if hurt
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		}

		if ( m_fGunDrawn && m_flNextHolsterTime < gpGlobals->time )
            return GetScheduleOfType( SCHED_BARNEY_HOLSTER );

		if ( m_hEnemy == NULL && IsFollowing() )
		{
			if ( !m_hTargetEnt->IsAlive() )
			{
				// UNDONE: Comment about the recently dead player here?
				StopFollowing( FALSE );
				break;
			}
			else
			{
				if ( HasConditions( bits_COND_CLIENT_PUSH ) )
				{
					// Step4enko: Monster apologizes for blocking your way.
					PlaySentence( m_szGrp[TLK_SORRY], RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE );
					return GetScheduleOfType( SCHED_MOVE_AWAY_FOLLOW );
				}
				return GetScheduleOfType( SCHED_TARGET_FACE );
			}
		}

		if (HasConditions( bits_COND_CLIENT_PUSH ))
		{
			// Step4enko: Monster apologizes for blocking your way.
			//PlaySentence( m_szGrp[TLK_SORRY], RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE );
			return GetScheduleOfType( SCHED_MOVE_AWAY );
		}

		// try to say something about smells
		TrySmellTalk();
		break;
	}
	
	return CTalkMonster::GetSchedule();
}

MONSTERSTATE CBarney :: GetIdealState ( void )
{
	return CTalkMonster::GetIdealState();
}

void CBarney::DeclineFollowing( void )
{
	PlaySentence( "BA_POK", 2, VOL_NORM, ATTN_NORM );
}


//=========================================================
// Dead Barney
//=========================================================
char *CDeadBarney::m_szPoses[] = { "lying_on_back", "lying_on_side", "lying_on_stomach" };

void CDeadBarney::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else 
		CBaseMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_barney_dead, CDeadBarney );

//=========================================================
// ********** DeadBarney SPAWN **********
//=========================================================
void CDeadBarney :: Spawn( )
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model));
	else
	    PRECACHE_MODEL("models/barney.mdl");

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model));
	else
	    SET_MODEL(ENT(pev), "models/barney.mdl");

	pev->effects		= 0;
	pev->yaw_speed		= 8;
	pev->sequence		= 0;
	m_bloodColor		= BLOOD_COLOR_RED;

	pev->skin			= RANDOM_LONG(1,4);

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );
	if (pev->sequence == -1)
	{
		ALERT ( at_console, "Dead barney with bad pose\n" );
	}
	
	if (pev->health == 0)
	    pev->health			= 8;

	MonsterInitDead();
}
