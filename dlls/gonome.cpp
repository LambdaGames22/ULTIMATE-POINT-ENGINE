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

//=========================================================
// Step4enko: TODO:
// JUMP ATTACK?
//=========================================================

//=========================================================
// Gonome
//=========================================================
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"nodes.h"
#include	"effects.h"
#include	"soundent.h"
#include	"game.h"
#include	"gonome.h" // Step4enko
#include	"proj_gonomespit.h" // Step4enko

#define		GONOME_SPRINT_DIST	256 

int			iGonomeSpitSprite;
	
//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_GONOME_SMELLFOOD = LAST_COMMON_SCHEDULE + 1,
	SCHED_GONOME_EAT,
	SCHED_GONOME_SNIFF_AND_EAT,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum 
{
	TASK_GONOME_SMELLFOOD = LAST_COMMON_SCHEDULE + 1,
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		GONOMEE_AE_SLASH1	( 1 )
#define		GONOMEE_AE_SLASH2	( 2 )
#define		GONOMEE_AE_GUTS1	( 3 )
#define		GONOMEE_AE_GUTS2	( 4 )
#define		GONOMEE_AE_JUMP1	( 10 )
#define		GONOMEE_AE_JUMP2	( 11 )
#define		GONOMEE_AE_JUMP3	( 12 )
#define		GONOMEE_AE_JUMP4	( 13 )
#define		GONOMEE_AE_JUMP5	( 14 )
#define		GONOMEE_AE_JUMP6	( 15 )
#define		GONOMEE_AE_JUMP7	( 16 )
#define		GONOMEE_AE_JUMP8	( 17 )
#define		GONOMEE_AE_JUMP9	( 18 )
#define		GONOMEE_AE_BITE1	( 19 )
#define		GONOMEE_AE_BITE2	( 20 )
#define		GONOMEE_AE_BITE3	( 21 )
#define		GONOMEE_AE_BITE4	( 22 )

#define GONOME_FLINCH_DELAY			2		// at most one flinch every n secs

LINK_ENTITY_TO_CLASS( monster_gonome, CGonome );

TYPEDESCRIPTION	CGonome::m_SaveData[] = 
{
	DEFINE_FIELD( CGonome, m_fCanThreatDisplay, FIELD_BOOLEAN ),
	DEFINE_FIELD( CGonome, m_flLastHurtTime, FIELD_TIME ),
	DEFINE_FIELD( CGonome, m_flNextSpitTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CGonome, CBaseMonster );

const char *CGonome::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CGonome::pAttackMissSounds[] = 
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

//=========================================================
// IgnoreConditions 
//=========================================================
int CGonome::IgnoreConditions ( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if ((m_Activity == ACT_MELEE_ATTACK1) || (m_Activity == ACT_MELEE_ATTACK1))
	{
#if 0
		if (pev->health < 20)
			iIgnore |= (bits_COND_LIGHT_DAMAGE|bits_COND_HEAVY_DAMAGE);
		else
#endif			
		if (m_flNextFlinch >= gpGlobals->time)
			iIgnore |= (bits_COND_LIGHT_DAMAGE|bits_COND_HEAVY_DAMAGE);
	}

	// Step4enko: Do no try to smell or eat something if we have an enemy.
	if ( m_hEnemy != NULL )
		iIgnore = bits_COND_SMELL | bits_COND_SMELL_FOOD;

	if ((m_Activity == ACT_SMALL_FLINCH) || (m_Activity == ACT_BIG_FLINCH))
	{
		if (m_flNextFlinch < gpGlobals->time)
			m_flNextFlinch = gpGlobals->time + GONOME_FLINCH_DELAY;
	}

	return iIgnore;
}
//=========================================================
// IRelationship - overridden for gonome so that it can
// be made to ignore its love of headcrabs for a while.
//=========================================================
int CGonome::IRelationship ( CBaseEntity *pTarget )
{
	return CBaseMonster :: IRelationship ( pTarget );
}

//=========================================================
// TakeDamage - overridden for gonome so we can keep track
// of how much time has passed since it was last injured
//=========================================================
int CGonome :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	float flDist;
	Vector vecApex;

	// if the gonome is running, has an enemy, was hurt by the enemy, hasn't been hurt in the last 3 seconds, and isn't too close to the enemy,
	// it will swerve. (whew).
	if ( m_hEnemy != NULL && IsMoving() && pevAttacker == m_hEnemy->pev && gpGlobals->time - m_flLastHurtTime > 3 )
	{
		flDist = ( pev->origin - m_hEnemy->pev->origin ).Length2D();
		
		if ( flDist > GONOME_SPRINT_DIST )
		{
			flDist = ( pev->origin - m_Route[ m_iRouteIndex ].vecLocation ).Length2D();// reusing flDist. 

			if ( FTriangulate( pev->origin, m_Route[ m_iRouteIndex ].vecLocation, flDist * 0.5, m_hEnemy, &vecApex ) )
			{
				InsertWaypoint( vecApex, bits_MF_TO_DETOUR | bits_MF_DONT_SIMPLIFY );
			}
		}
	}

	return CBaseMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CGonome :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( IsMoving() && flDist >= 512 )
	{
		// gonome will far too far behind if he stops running to spit at this distance from the enemy.
		return FALSE;
	}

	if ( flDist > 64 && flDist <= 784 && flDot >= 0.5 && gpGlobals->time >= m_flNextSpitTime )
	{

		if ( IsMoving() )
		{
			// don't spit again for a long time, resume chasing enemy.
			m_flNextSpitTime = gpGlobals->time + 5;
		}
		else
		{
			// not moving, so spit again pretty soon.
			m_flNextSpitTime = gpGlobals->time + 0.5;
		}

		return TRUE;
	}

	return FALSE;

}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CGonome :: ISoundMask ( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_CARCASS	|
			bits_SOUND_MEAT		|
			bits_SOUND_GARBAGE	|
			bits_SOUND_PLAYER;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CGonome :: Classify ( void )
{
	return m_iClass ? m_iClass : CLASS_ALIEN_MONSTER;
}

//=========================================================
// IdleSound 
//=========================================================
#define GONOME_ATTN_IDLE	(float)1.5
void CGonome :: IdleSound ( void )
{
	switch ( RANDOM_LONG(0,2) )
	{
	case 0:	
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "gonome/gonome_idle1.wav", 1, GONOME_ATTN_IDLE );	
		break;
	case 1:	
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "gonome/gonome_idle2.wav", 1, GONOME_ATTN_IDLE );	
		break;
	case 2:	
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "gonome/gonome_idle3.wav", 1, GONOME_ATTN_IDLE );	
		break;
	}
}

//=========================================================
// PainSound 
//=========================================================
void CGonome :: PainSound ( void )
{
	int iPitch = RANDOM_LONG( 85, 120 );

	switch ( RANDOM_LONG(0,3) )
	{
	case 0:	
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "gonome/gonome_pain1.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	case 1:	
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "gonome/gonome_pain2.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	case 2:	
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "gonome/gonome_pain3.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	case 3:	
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "gonome/gonome_pain4.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	}
}

//=========================================================
// AlertSound
//=========================================================
void CGonome :: AlertSound ( void )
{
	int iPitch = RANDOM_LONG( 140, 160 );

	switch ( RANDOM_LONG ( 0, 2  ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "gonome/gonome_idle1.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	case 1:
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "gonome/gonome_idle2.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	case 2:
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "gonome/gonome_idle3.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	}
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CGonome :: SetYawSpeed ( void )
{
	int ys;

	ys = 0;

	switch ( m_Activity )
	{
	case	ACT_WALK:			ys = 600;	break;
	case	ACT_RUN:			ys = 600;	break;
	case	ACT_IDLE:			ys = 600;	break;
	case	ACT_RANGE_ATTACK1:	ys = 600;	break;
	default:
		ys = 600;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CGonome :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case GONOMEE_AE_GUTS1:
		{
		Vector	vecSpitOffset;
		Vector  vecGunPos, vecGunDir;

		UTIL_MakeVectors ( pev->angles );

		GetAttachment ( 0, vecGunPos, vecGunDir );

			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecGunPos/*vecSpitOffset*/ );
				WRITE_BYTE( TE_SPRITE );
				WRITE_COORD( pev->angles.x );	// pos
				WRITE_COORD( pev->angles.y );	
				WRITE_COORD( pev->angles.z );	
				WRITE_SHORT( iGonomeSpitSprite );		// model
				WRITE_BYTE( 6 );				// size * 10
				WRITE_BYTE( 128 );			// brightness
			MESSAGE_END();

		}
		break;

		case GONOMEE_AE_GUTS2:
		{
			Vector	vecSpitOffset;
			Vector	vecSpitDir;
			Vector  vecGunPos, vecGunDir;
			Vector	vecDirToEnemy;

			UTIL_MakeAimVectors ( pev->angles );

			// !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
			// we should be able to read the position of bones at runtime for this info.
			vecDirToEnemy = ( (( m_vecEnemyLKP ) - pev->origin) + gpGlobals->v_up * -96 +gpGlobals->v_right * -35 ).Normalize();

			GetAttachment ( 0, vecGunPos, vecGunDir );

			CGonomeSpit::Shoot( pev, vecGunPos, vecDirToEnemy * 900 );
		}
		break;

		case GONOMEE_AE_BITE1:
		case GONOMEE_AE_BITE2:
		case GONOMEE_AE_BITE3:
		case GONOMEE_AE_BITE4:
		{
			// SOUND HERE!
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, 15, DMG_SLASH );
			
			if ( pHurt )
			{
				pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_forward * 100;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 100;

				// Play a random attack hit sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else 
			{
				// Play a random attack miss sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
		}
		break;

		case GONOMEE_AE_SLASH1:
		{
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, 30, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = -18;
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 100;
				}
				// Play a random attack hit sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else 
			{
				// Play a random attack miss sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
		}
		break;
		case GONOMEE_AE_SLASH2:
		{
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, 30, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = 18;
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 100;
				}
				// Play a random attack hit sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else 
			{
				// Play a random attack miss sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
		}
		break;

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// Spawn
//=========================================================
void CGonome :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model));
	else
		SET_MODEL(ENT(pev), "models/gonome.mdl");

	if (pev->health == 0)
		pev->health			= 200;

	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->effects		= 0;
	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	m_fCanThreatDisplay	= TRUE;
	m_flNextSpitTime = gpGlobals->time;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CGonome :: Precache()
{
	int i;

	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model));
	else
		PRECACHE_MODEL("models/gonome.mdl");
	
	for ( i = 0; i < ARRAYSIZE( pAttackHitSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackHitSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackMissSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackMissSounds[i]);

	PRECACHE_MODEL("sprites/blood_chnk.spr");
	
	iGonomeSpitSprite = PRECACHE_MODEL("sprites/blood_tinyspit.spr");

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	PRECACHE_SOUND("gonome/gonome_melee1.wav");
	PRECACHE_SOUND("gonome/gonome_melee2.wav");
	
	PRECACHE_SOUND("gonome/gonome_death2.wav");
	PRECACHE_SOUND("gonome/gonome_death3.wav");
	PRECACHE_SOUND("gonome/gonome_death4.wav");
	
	PRECACHE_SOUND("gonome/gonome_idle1.wav");
	PRECACHE_SOUND("gonome/gonome_idle2.wav");
	PRECACHE_SOUND("gonome/gonome_idle3.wav");
	
	PRECACHE_SOUND("gonome/gonome_pain1.wav");
	PRECACHE_SOUND("gonome/gonome_pain2.wav");
	PRECACHE_SOUND("gonome/gonome_pain3.wav");
	PRECACHE_SOUND("gonome/gonome_pain4.wav");
	
	PRECACHE_SOUND("gonome/gonome_jumpattack.wav");

	PRECACHE_SOUND("gonome/gonome_run.wav");
	PRECACHE_SOUND("gonome/gonome_eat.wav");

	PRECACHE_SOUND("bullchicken/bc_spithit1.wav");
	PRECACHE_SOUND("bullchicken/bc_spithit2.wav");
}	

//=========================================================
// DeathSound
//=========================================================
void CGonome :: DeathSound ( void )
{
	switch ( RANDOM_LONG(0,2) )
	{
	case 0:	
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "gonome/gonome_death2.wav", 1, ATTN_NORM );	
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "gonome/gonome_death3.wav", 1, ATTN_NORM );	
		break;
	case 2:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "gonome/gonome_death4.wav", 1, ATTN_NORM );	
		break;
	}
}

//=========================================================
// AttackSound
//=========================================================
void CGonome :: AttackSound ( void )
{

}


//========================================================
// RunAI - overridden for gonome because there are things
// that need to be checked every think.
//========================================================
void CGonome :: RunAI ( void )
{
	// first, do base class stuff
	CBaseMonster :: RunAI();

	if ( m_hEnemy != NULL && m_Activity == ACT_RUN )
	{
		// chasing enemy. Sprint for last bit
		if ( (pev->origin - m_hEnemy->pev->origin).Length2D() < GONOME_SPRINT_DIST )
		{
			pev->framerate = 1.25;
		}
	}
}

//========================================================
// AI Schedules Specific to this monster
//=========================================================

// primary range attack
Task_t	tlGonomeRangeAttack1[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slGonomeRangeAttack1[] =
{
	{ 
		tlGonomeRangeAttack1,
		ARRAYSIZE ( tlGonomeRangeAttack1 ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED,
		0,
		"Gonome Range Attack1"
	},
};

// Chase enemy schedule
Task_t tlGonomeChaseEnemy1[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_RANGE_ATTACK1	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,			(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0					},
};

Schedule_t slGonomeChaseEnemy[] =
{
	{ 
		tlGonomeChaseEnemy1,
		ARRAYSIZE ( tlGonomeChaseEnemy1 ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_SMELL_FOOD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_TASK_FAILED		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER			|
		bits_SOUND_MEAT,
		"Gonome Chase Enemy"
	},
};


// gonome walks to something tasty and eats it.
Task_t tlGonomeEat[] =
{
	{ TASK_STOP_MOVING,				(float)0				},
	{ TASK_EAT,						(float)10				},// this is in case the gonome can't get to the food
	{ TASK_STORE_LASTPOSITION,		(float)0				},
	{ TASK_GET_PATH_TO_BESTSCENT,	(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_EAT,						(float)50				},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t slGonomeEat[] =
{
	{
		tlGonomeEat,
		ARRAYSIZE( tlGonomeEat ),
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_NEW_ENEMY	,
		
		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_MEAT			|
		bits_SOUND_CARCASS,
		"GonomeEat"
	}
};

// this is a bit different than just Eat. We use this schedule when the food is far away, occluded, or behind
// the gonome. This schedule plays a sniff animation before going to the source of food.
Task_t tlGonomeSniffAndEat[] =
{
	{ TASK_STOP_MOVING,				(float)0				},
	{ TASK_EAT,						(float)10				},// this is in case the gonome can't get to the food
	//{ TASK_PLAY_SEQUENCE,			(float)ACT_DETECT_SCENT },
	{ TASK_STORE_LASTPOSITION,		(float)0				},
	{ TASK_GET_PATH_TO_BESTSCENT,	(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_EAT,						(float)50				},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t slGonomeSniffAndEat[] =
{
	{
		tlGonomeSniffAndEat,
		ARRAYSIZE( tlGonomeSniffAndEat ),
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_NEW_ENEMY	,
		
		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_MEAT			|
		bits_SOUND_CARCASS,
		"GonomeSniffAndEat"
	}
};

DEFINE_CUSTOM_SCHEDULES( CGonome ) 
{
	slGonomeRangeAttack1,
	slGonomeChaseEnemy,
	slGonomeEat,
	slGonomeSniffAndEat,
};

IMPLEMENT_CUSTOM_SCHEDULES( CGonome, CBaseMonster );

//=========================================================
// GetSchedule 
//=========================================================
Schedule_t *CGonome :: GetSchedule( void )
{
	switch ( m_MonsterState )
	{
	case MONSTERSTATE_ALERT:
		{
			if ( HasConditions(bits_COND_SMELL_FOOD) )
			{
				CSound *pSound;
				pSound = PBestScent();
				
				if ( pSound && (!FInViewCone ( &pSound->m_vecOrigin ) || !FVisible ( pSound->m_vecOrigin )) )
				{
					// scent is behind or occluded
					return GetScheduleOfType( SCHED_GONOME_SNIFF_AND_EAT );
				}

				// food is right out in the open. Just go get it.
				return GetScheduleOfType( SCHED_GONOME_EAT );
			}
			break;
		}
	case MONSTERSTATE_COMBAT:
		{
			// Dead enemy.
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// Call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{
				if ( m_fCanThreatDisplay && IRelationship( m_hEnemy ) == R_HT )
				{
					return GetScheduleOfType ( SCHED_WAKE_ANGRY );
				}
			}

			if ( HasConditions(bits_COND_SMELL_FOOD) )
			{
				CSound		*pSound;

				pSound = PBestScent();
				
				if ( pSound && (!FInViewCone ( &pSound->m_vecOrigin ) || !FVisible ( pSound->m_vecOrigin )) )
				{
					// scent is behind or occluded
					return GetScheduleOfType( SCHED_GONOME_SNIFF_AND_EAT );
				}

				// food is right out in the open. Just go get it.
				return GetScheduleOfType( SCHED_GONOME_EAT );
			}

			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_RANGE_ATTACK1 );
			}

			if ( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
			}

			if ( HasConditions( bits_COND_CAN_MELEE_ATTACK2 ) )
			{
				return GetScheduleOfType ( SCHED_MELEE_ATTACK2 );
			}
			
			return GetScheduleOfType ( SCHED_CHASE_ENEMY );		
		}
		break;
	}

	return CBaseMonster :: GetSchedule();
}

//=========================================================
// GetScheduleOfType
//=========================================================
Schedule_t* CGonome :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_RANGE_ATTACK1:
		return &slGonomeRangeAttack1[ 0 ];
		break;
	case SCHED_GONOME_EAT:
		return &slGonomeEat[ 0 ];
		break;
	case SCHED_GONOME_SNIFF_AND_EAT:
		return &slGonomeSniffAndEat[ 0 ];
		break;
	case SCHED_CHASE_ENEMY:
		return &slGonomeChaseEnemy[ 0 ];
		break;
	}

	return CBaseMonster :: GetScheduleOfType ( Type );
}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.  OVERRIDDEN for gonome because it needs to
// know explicitly when the last attempt to chase the enemy
// failed, since that impacts its attack choices.
//=========================================================
void CGonome :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_MELEE_ATTACK2:
		{
			CBaseMonster :: StartTask ( pTask );
			break;
		}
	case TASK_GET_PATH_TO_ENEMY:
		{
			if ( BuildRoute ( m_hEnemy->pev->origin, bits_MF_TO_ENEMY, m_hEnemy ) )
			{
				m_iTaskStatus = TASKSTATUS_COMPLETE;
			}
			else
			{
				ALERT ( at_aiconsole, "GetPathToEnemy failed!!\n" );
				TaskFail();
			}
			break;
		}
	default:
		{
			CBaseMonster :: StartTask ( pTask );
			break;
		}
	}
}

//=========================================================
// RunTask
//=========================================================
void CGonome :: RunTask ( Task_t *pTask )
{
	CBaseMonster :: RunTask( pTask );
}


//=========================================================
// GetIdealState - Overridden for Gonome to deal with
// the feature that makes it lose interest in headcrabs for 
// a while if something injures it. 
//=========================================================
MONSTERSTATE CGonome :: GetIdealState ( void )
{
	int	iConditions;

	iConditions = IScheduleFlags();
	
	m_IdealMonsterState = CBaseMonster :: GetIdealState();

	return m_IdealMonsterState;
}

