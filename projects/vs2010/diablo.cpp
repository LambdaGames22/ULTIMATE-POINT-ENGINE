/***
*
*	Copyright (c) 2018,  UPTeam. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Creators: Step4enko
*   
****/

//=========================================================
// Diablo - Step4enko monster
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"soundent.h"
#include	"weapons.h"

//=========================================================
// Animation Events
//=========================================================
#define	DIABLO_AE_ATTACK_LEFT		0x01
#define	DIABLO_AE_ATTACK_BOTH		0x02
#define	DIABLO_AE_ATTACK_RIGHT		0x03
#define DIABLO_AE_STEP				0x04

#define DIABLO_FLINCH_DELAY				2
#define	DIABLO_MELEE_ATTACK_RADIUS		70
#define	DIABLO_TOLERANCE_MELEE2_RANGE	85

//=========================================================
// Class registration
//=========================================================
class CDiablo : public CBaseMonster 
{
	public:
		void Spawn(void);
		void Precache(void);
		void SetYawSpeed(void);
		int  Classify(void);
		void HandleAnimEvent(MonsterEvent_t *pEvent);
		int IgnoreConditions(void);
		int  ISoundMask(void);

		float m_flNextFlinch;

		void PainSound(void);
		void AlertSound(void);
		void IdleSound(void);
		void AttackSound(void);

		BOOL CheckRangeAttack1(float flDot, float flDist);
		BOOL CheckRangeAttack2(float flDot, float flDist);
		BOOL CheckMeleeAttack1(float flDot, float flDist);
		BOOL CheckMeleeAttack2(float flDot, float flDist);

		static const char *pAttackSounds[];
		static const char *pStepSounds[];
		static const char *pIdleSounds[];
		static const char *pAlertSounds[];
		static const char *pPainSounds[];
		static const char *pAttackHitSounds[];
		static const char *pAttackMissSounds[];
		static const char *pAttackHitSoundsEXT[];
		static const char *pAttackMissSoundsEXT[];

		int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
		void TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

		void SetActivity(Activity NewActivity);

		Schedule_t	*GetSchedule(void);
		Schedule_t  *GetScheduleOfType(int Type);

		CUSTOM_SCHEDULES;

	protected:
		BOOL m_flDebug;
		float m_flBulletDR;
		float m_flHitgroupHead;
		float m_flHitgroupChest;
		float m_flHitgroupStomach;
		float m_flHitgroupLeg;
};

//=========================================================
// New Schedule types
//=========================================================
enum 
{
	SCHED_DIABLO_RANGE_ATTACK1,
	SCHED_DIABLO_RANGE_ATTACK2,
};

//=========================================================
// LINK ENTITY TO CLASS
//=========================================================
LINK_ENTITY_TO_CLASS(monster_diablo, CDiablo);

//=========================================================
// Diablo Sounds
//=========================================================
const char *CDiablo::pAttackHitSoundsEXT[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CDiablo::pAttackMissSoundsEXT[] = 
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CDiablo::pAttackHitSounds[] = 
{
	"diablo/diablo_claw1.wav",
	"diablo/diablo_claw2.wav",
	"diablo/diablo_claw3.wav"
};

const char *CDiablo::pAttackMissSounds[] = 
{
	"diablo/diablo_claw_miss1.wav",
	"diablo/diablo_claw_miss2.wav"
};

const char *CDiablo::pAttackSounds[] = 
{
	"diablo/diablo_attack1.wav",
	"diablo/diablo_attack2.wav"
};

const char *CDiablo::pIdleSounds[] = 
{
	"diablo/diablo_idle1.wav",
	"diablo/diablo_idle2.wav",
	"diablo/diablo_idle3.wav"
};

const char *CDiablo::pAlertSounds[] = 
{
	"diablo/diablo_alert10.wav",
	"diablo/diablo_alert20.wav",
	"diablo/diablo_alert30.wav"
};

const char *CDiablo::pPainSounds[] = 
{
	"diablo/diablo_pain1.wav",
	"diablo/diablo_pain2.wav"
};

const char *CDiablo::pStepSounds[] = 
{
	"player/pl_step1.wav",
	"player/pl_step2.wav",
	"player/pl_step3.wav",
	"player/pl_step4.wav"
};

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CDiablo::ISoundMask(void) 
{
	return	bits_SOUND_WORLD |
			bits_SOUND_COMBAT |
			bits_SOUND_CARCASS |
			bits_SOUND_MEAT |
			bits_SOUND_GARBAGE |
			bits_SOUND_PLAYER;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CDiablo::Classify(void) 
{
	return m_iClass ? m_iClass : CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CDiablo::SetYawSpeed(void) 
{
	switch (m_Activity) 
	{
		default: pev->yaw_speed = 600; break;
	}
}

//=========================================================
// TakeDamage
//=========================================================
int CDiablo::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType) 
{
	if (pev->spawnflags & SF_MONSTER_SPAWNFLAG_64) 
	{
		if (m_flDebug)
			ALERT(at_console, "%s:TakeDamage:SF_MONSTER_SPAWNFLAG_64\n", STRING(pev->classname));

		CBaseEntity *pEnt = CBaseEntity::Instance(pevAttacker);
		if (pEnt->IsPlayer()) {
			pev->health = pev->max_health / 2;
			if (flDamage > 0) //Override all damage
				SetConditions(bits_COND_LIGHT_DAMAGE);

			if (flDamage >= 20) //Override all damage
				SetConditions(bits_COND_HEAVY_DAMAGE);

			return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
		}

		if (pevAttacker->owner) 
		{
			pEnt = CBaseEntity::Instance(pevAttacker->owner);
			if (pEnt->IsPlayer()) 
			{
				pev->health = pev->max_health / 2;
				if (flDamage > 0) //Override all damage
					SetConditions(bits_COND_LIGHT_DAMAGE);

				if (flDamage >= 20) //Override all damage
					SetConditions(bits_COND_HEAVY_DAMAGE);

				return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
			}
		}
	}

	// Take xx% damage from bullets
	if (bitsDamageType == DMG_BULLET && m_flBulletDR != 0) 
	{
		if (m_flDebug)
			ALERT(at_console, "%s:TakeDamage:DMG_BULLET:Reduce Damage\n", STRING(pev->classname));

		Vector vecDir = pev->origin - (pevInflictor->absmin + pevInflictor->absmax) * 0.5;
		vecDir = vecDir.Normalize();
		float flForce = DamageForce(flDamage);
		pev->velocity = pev->velocity + vecDir * flForce;
		flDamage *= m_flBulletDR;
	}

	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

//=========================================================
// TraceAttack - Damage based on Hitgroups
//=========================================================
void CDiablo::TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType) 
{
	if (!IsAlive()) 
	{
		CBaseMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
		return;
	}

	if (pev->takedamage) 
	{
		if (IsAlive() && RANDOM_LONG(0, 4) <= 2) { PainSound(); }
		if (pev->spawnflags & SF_MONSTER_SPAWNFLAG_64) 
		{
			CBaseEntity *pEnt = CBaseEntity::Instance(pevAttacker);
			if (pEnt->IsPlayer()) { return; }
			if (pevAttacker->owner) 
			{
				pEnt = CBaseEntity::Instance(pevAttacker->owner);
				if (pEnt->IsPlayer()) { return; }
			}
		}

		switch (ptr->iHitgroup) 
		{
		case HITGROUP_HEAD:
			if (m_flDebug)
				ALERT(at_console, "%s:TraceAttack:HITGROUP_HEAD\n", STRING(pev->classname));
			flDamage = m_flHitgroupHead*flDamage;
			break;
		case HITGROUP_CHEST:
			if (m_flDebug)
				ALERT(at_console, "%s:TraceAttack:HITGROUP_CHEST\n", STRING(pev->classname));
			flDamage = m_flHitgroupChest*flDamage;
			break;
		case HITGROUP_STOMACH:
			if (m_flDebug)
				ALERT(at_console, "%s:TraceAttack:HITGROUP_STOMACH\n", STRING(pev->classname));
			flDamage = m_flHitgroupStomach*flDamage;
			break;
		case HITGROUP_LEFTARM:
		case HITGROUP_RIGHTARM:
		case HITGROUP_LEFTLEG:
		case HITGROUP_RIGHTLEG:
			if (m_flDebug)
				ALERT(at_console, "%s:TraceAttack:HITGROUP_LEG\n", STRING(pev->classname));
			flDamage = m_flHitgroupLeg*flDamage;
			break;
		}
	}

	SpawnBlood(ptr->vecEndPos, BloodColor(), flDamage);// a little surface blood.
	TraceBleed(flDamage, vecDir, ptr, bitsDamageType);
	CBaseMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CDiablo::CheckRangeAttack1(float flDot, float flDist) 
{
	if (flDist >= 128) 
	{
		return TRUE;
	}

	return FALSE;
}

//=========================================================
// CheckRangeAttack2
//=========================================================
BOOL CDiablo::CheckRangeAttack2(float flDot, float flDist) 
{
	if (flDist < 128) 
	{
		return TRUE;
	}

	return FALSE;
}

//=========================================================
// CheckMeleeAttack1
//=========================================================
BOOL CDiablo::CheckMeleeAttack1(float flDot, float flDist) 
{
	if (flDist <= 64 && flDot >= 0.7 && m_hEnemy != NULL && FBitSet(m_hEnemy->pev->flags, FL_ONGROUND)) 
	{
		return TRUE;
	}

	return FALSE;
}

//=========================================================
// CheckMeleeAttack2
//=========================================================
BOOL CDiablo::CheckMeleeAttack2(float flDot, float flDist) 
{
	if (flDist <= 92 && flDot >= 0.7 && m_hEnemy != NULL && FBitSet(m_hEnemy->pev->flags, FL_ONGROUND)) 
	{
		return TRUE;
	}

	return FALSE;
}

//=========================================================
// PainSound
//=========================================================
void CDiablo::PainSound(void) 
{
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pPainSounds);
}

//=========================================================
// AlertSound
//=========================================================
void CDiablo::AlertSound(void) 
{
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pAlertSounds);
}

//=========================================================
// IdleSound
//=========================================================
void CDiablo::IdleSound(void) 
{
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pIdleSounds);
}

//=========================================================
// AttackSound 
//=========================================================
void CDiablo::AttackSound(void) 
{
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pAttackSounds);
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CDiablo::HandleAnimEvent(MonsterEvent_t *pEvent) 
{
	switch (pEvent->event) 
	{
		case DIABLO_AE_ATTACK_RIGHT: 
			{
			CBaseEntity *pHurt = CheckTraceHullAttack(DIABLO_MELEE_ATTACK_RADIUS, 25, DMG_SLASH | DMG_NEVERGIB | DMG_CLUB);
			if (pHurt) 
			{
				if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT)) 
				{
					pHurt->pev->punchangle.z = -20;
					pHurt->pev->punchangle.x = 15;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * -200;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 100;
				}

				if (RANDOM_LONG(0, 1))
					EMIT_SOUND_ARRAY_DYN(CHAN_WEAPON, pAttackHitSounds);

				EMIT_SOUND_ARRAY_DYN(CHAN_WEAPON, pAttackHitSoundsEXT);
			} 
			else 
			{
				if (RANDOM_LONG(0, 1))
					EMIT_SOUND_ARRAY_DYN(CHAN_WEAPON, pAttackMissSounds);

				EMIT_SOUND_ARRAY_DYN(CHAN_WEAPON, pAttackMissSoundsEXT);
			}

			if (RANDOM_LONG(0, 1)) { AttackSound(); }
		}
		break;
		case DIABLO_AE_ATTACK_LEFT: 
			{
			CBaseEntity *pHurt = CheckTraceHullAttack(DIABLO_MELEE_ATTACK_RADIUS, 25, DMG_SLASH | DMG_NEVERGIB | DMG_CLUB);
			if (pHurt) 
			{
				if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT)) 
				{
					pHurt->pev->punchangle.z = 20;
					pHurt->pev->punchangle.x = 15;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 200;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 100;
				}
				if (RANDOM_LONG(0, 1))
					EMIT_SOUND_ARRAY_DYN(CHAN_WEAPON, pAttackHitSounds);

				EMIT_SOUND_ARRAY_DYN(CHAN_WEAPON, pAttackHitSoundsEXT);
			} 
			else 
			{
				if (RANDOM_LONG(0, 1))
					EMIT_SOUND_ARRAY_DYN(CHAN_WEAPON, pAttackMissSounds);

				EMIT_SOUND_ARRAY_DYN(CHAN_WEAPON, pAttackMissSoundsEXT);
			}
		}
		break;
		case DIABLO_AE_ATTACK_BOTH: 
			{
			CBaseEntity *pHurt = CheckTraceHullAttack(DIABLO_TOLERANCE_MELEE2_RANGE, 25, DMG_SLASH | DMG_NEVERGIB | DMG_CLUB);
			if (pHurt) {
				pHurt->pev->punchangle.z = RANDOM_LONG(-10, 10);
				pHurt->pev->punchangle.x = RANDOM_LONG(-20, -30);
				pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_forward * -100;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 70;
				if (RANDOM_LONG(0, 1))
					EMIT_SOUND_ARRAY_DYN(CHAN_WEAPON, pAttackHitSounds);

				EMIT_SOUND_ARRAY_DYN(CHAN_WEAPON, pAttackHitSoundsEXT);
			} 
			else 
			{
				if (RANDOM_LONG(0, 1))
					EMIT_SOUND_ARRAY_DYN(CHAN_WEAPON, pAttackMissSounds);

				EMIT_SOUND_ARRAY_DYN(CHAN_WEAPON, pAttackMissSoundsEXT);
			}

			if (RANDOM_LONG(0, 1)) { AttackSound(); }
		}
		break;
		case DIABLO_AE_STEP: 
			{
			EMIT_SOUND_ARRAY_DYN(CHAN_BODY, pStepSounds);
		}
		break;
		default:
			CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CDiablo::Spawn(void) 
{
	Precache();

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model));
	else
		SET_MODEL(ENT(pev), "models/diablo.mdl");

	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_YELLOW;

	if (pev->health == 0)
		pev->health = 80;

	pev->effects = 0;
	pev->view_ofs = VEC_VIEW; // position of the eyes relative to monster's origin.

	m_flFieldOfView = VIEW_FIELD_FULL;
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_HEAR |
					 bits_CAP_RANGE_ATTACK1 |
					 bits_CAP_MELEE_ATTACK1 |
					 bits_CAP_MELEE_ATTACK2;

	m_flBulletDR = 0.3; //damage from bullets
	m_flDebug = false; //Debug Massages

	m_flHitgroupHead = 30;
	m_flHitgroupChest = 15;
	m_flHitgroupStomach = 25;
	m_flHitgroupLeg = 10;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CDiablo::Precache(void) 
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model));
	else
		PRECACHE_MODEL("models/diablo.mdl");

	PRECACHE_SOUND_ARRAY(pAttackHitSounds);
	PRECACHE_SOUND_ARRAY(pAttackMissSounds);
	PRECACHE_SOUND_ARRAY(pAttackHitSoundsEXT);
	PRECACHE_SOUND_ARRAY(pAttackMissSoundsEXT);
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pStepSounds);
}

//=========================================================
// SetActivity
//=========================================================
void CDiablo::SetActivity(Activity newActivity) 
{
	if (m_MonsterState == MONSTERSTATE_COMBAT) 
	{
		if (newActivity == ACT_WALK)
			newActivity = ACT_RUN;
	}

	CBaseMonster::SetActivity(newActivity);
}

//=========================================================
// AI Schedules
//=========================================================
Task_t	tlDiabloRangeAttack1[] = {
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_TAKE_COVER_FROM_ENEMY },
	{ TASK_GET_PATH_TO_ENEMY,	(float)0 },
	{ TASK_RUN_PATH,			(float)0 },
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0 },

};

Schedule_t	slDiabloRangeAttack1[] = 
{
	{
		tlDiabloRangeAttack1,
		ARRAYSIZE(tlDiabloRangeAttack1),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_ENEMY_OCCLUDED |
		bits_COND_NO_AMMO_LOADED |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_HEAR_SOUND,
	},
};

Task_t	tlDiabloRangeAttack2[] = {
	{ TASK_GET_PATH_TO_ENEMY,	(float)0 },
	{ TASK_WALK_PATH,			(float)0 },
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0 },
};

Schedule_t	slDiabloRangeAttack2[] = 
{
	{
		tlDiabloRangeAttack2,
		ARRAYSIZE(tlDiabloRangeAttack2),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_ENEMY_OCCLUDED |
		bits_COND_NO_AMMO_LOADED |
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_HEAR_SOUND,
	},
};

DEFINE_CUSTOM_SCHEDULES(CDiablo) 
{
	slDiabloRangeAttack1,
};

IMPLEMENT_CUSTOM_SCHEDULES(CDiablo, CBaseMonster);

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
int CDiablo::IgnoreConditions(void) 
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if ((m_Activity == ACT_MELEE_ATTACK1) || (m_Activity == ACT_MELEE_ATTACK1)) 
	{
		if (m_flNextFlinch >= UTIL_WeaponTimeBase())
			iIgnore |= (bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE);
	}

	if ((m_Activity == ACT_SMALL_FLINCH) || (m_Activity == ACT_BIG_FLINCH)) 
	{
		if (m_flNextFlinch < UTIL_WeaponTimeBase())
			m_flNextFlinch = UTIL_WeaponTimeBase() + DIABLO_FLINCH_DELAY;
	}

	return iIgnore;

}

//=========================================================
// GetSchedule
//=========================================================
Schedule_t *CDiablo::GetSchedule(void) 
{
	switch (m_MonsterState) 
	{
		case MONSTERSTATE_COMBAT: 
			{
			if (HasConditions(bits_COND_CAN_MELEE_ATTACK1)) 
			{
				return GetScheduleOfType(SCHED_MELEE_ATTACK1);
			}

			if (HasConditions(bits_COND_CAN_MELEE_ATTACK2)) 
			{
				return GetScheduleOfType(SCHED_MELEE_ATTACK2);
			}

			if (HasConditions(bits_COND_CAN_RANGE_ATTACK1)) 
			{
				return GetScheduleOfType(SCHED_DIABLO_RANGE_ATTACK1);
			}

			if (HasConditions(bits_COND_CAN_RANGE_ATTACK2)) 
			{
				return GetScheduleOfType(SCHED_DIABLO_RANGE_ATTACK2);
			}

			if (pev->health <= 75) 
			{
				return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ENEMY);
			}
		}
	}

	return CBaseMonster::GetSchedule();
}

//=========================================================
// GetScheduleOfType
//=========================================================
Schedule_t* CDiablo::GetScheduleOfType(int Type) 
{
	switch (Type) 
	{
		case SCHED_DIABLO_RANGE_ATTACK1: 
			{
			return &slDiabloRangeAttack1[0];
		}
		case SCHED_DIABLO_RANGE_ATTACK2: 
			{
			return &slDiabloRangeAttack2[0];
		}
		default: 
			{
			return CBaseMonster::GetScheduleOfType(Type);
		}
	}
}