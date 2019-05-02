/***
*
*	Copyright (c) 2018, Step4enko. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Creator contacts:
*   
*   Discord: Step4enko#5003
*   Vk: https://vk.com/scplanst
*   Steam: https://steamcommunity.com/id/scplanst/home
*   
****/

//==============================================================================
// MRFRIENDLY. FULL CODE MADE BY STEP4ENKO. BECAUSE OTHER PEOPLE SUCK AT IT LOL
//==============================================================================

// UNDONE: RAPE ATTACK??? // I'M SUCK AT IT :(

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"nodes.h"
#include	"effects.h"
#include	"decals.h"
#include	"soundent.h"
#include	"game.h"
#include	"weapons.h"

int gmsgParticles;

int g_sModelIndexTinySpit;
int g_sModelIndexSpore1;
int g_sModelIndexSpore2;
int g_sModelIndexSpore3;
int iDefaultSporeExplosion;

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_MRF_SMELLFOOD = LAST_COMMON_SCHEDULE + 1,
	SCHED_MRF_EAT,
	SCHED_MRF_SNIFF_AND_EAT,
	SCHED_MRF_WALLOW,
};


//=========================================================
// MR.FRIENDLY (SEXY MONSTER) made by Step4enko
//=========================================================
#define MRF_AE_SLASH_RIGHT	( 1 )
#define MRF_AE_SLASH_LEFT	( 2 )
#define MRF_AE_THROW		( 4 )
#define MRF_AE_EARTHQUAKE   ( 6 ) 
#define MRF_AE_RAPE         ( 7 ) // UNDONE

#define MRF_FLINCH_DELAY			1.5	// One flinch every n secs
#define	MRF_SPRINT_DIST	            256 // How close the Mr.Friendly has to get before starting to sprint and refusing to swerve

#define	MRF_TOLERANCE_MELEE1_RANGE	95
#define	MRF_TOLERANCE_MELEE2_RANGE	85

#define	MRF_TOLERANCE_MELEE1_DOT		0.7
#define	MRF_TOLERANCE_MELEE2_DOT		0.7

#define	MRF_MELEE_ATTACK_RADIUS		70
#define MRF_MAX_ATTACK_RADIUS       384

//=========================================================
// MR.FRIENDLY SPORE SPIT CLASS
//=========================================================
class CSporeGrenade : public CBaseMonster 
{
	public:
		void Spawn(void);
		void Precache(void);
		void Glow(void);
		void Explode(void);

		void EXPORT ExplodeThink(CBaseEntity *pOther);
		void EXPORT BounceThink(CBaseEntity *pOther);
		void EXPORT FlyThink(void);

		static CSporeGrenade *ShootTimed(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time); // Explode as grenade
		static CSporeGrenade *ShootContact(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity); // Explode right after contact will wall or entity
	private:
		CSprite *m_pSprite;
};

LINK_ENTITY_TO_CLASS(spore, CSporeGrenade);

CSporeGrenade *CSporeGrenade::ShootTimed(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time) 
{
	CSporeGrenade *pSpore = GetClassPtr((CSporeGrenade *)NULL);
	UTIL_SetOrigin(pSpore->pev, vecStart);
	pSpore->pev->movetype = MOVETYPE_BOUNCE;
	pSpore->pev->owner = ENT(pevOwner);
	pSpore->pev->classname = MAKE_STRING("spore");
	pSpore->pev->velocity = vecVelocity;
	pSpore->pev->velocity = pSpore->pev->velocity + gpGlobals->v_forward * 700;
	pSpore->pev->angles = UTIL_VecToAngles(pSpore->pev->velocity);
	pSpore->pev->dmgtime = (time <= 0 ? 2.5 : time);
	pSpore->pev->sequence = RANDOM_LONG(3, 6);
	pSpore->pev->framerate = 1.0;
	pSpore->Spawn();
	return pSpore;
}

//=========================================================
// ShootContact
//=========================================================
CSporeGrenade *CSporeGrenade::ShootContact(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity) 
{
	CSporeGrenade *pSpore = GetClassPtr((CSporeGrenade *)NULL);
	UTIL_SetOrigin(pSpore->pev, vecStart);
	pSpore->pev->movetype = MOVETYPE_FLY;
	pSpore->pev->owner = ENT(pevOwner);
	pSpore->pev->classname = MAKE_STRING("spore");
	pSpore->pev->velocity = vecVelocity;
	pSpore->pev->velocity = pSpore->pev->velocity + gpGlobals->v_forward * 1500;
	pSpore->pev->angles = UTIL_VecToAngles(pSpore->pev->velocity);
	pSpore->Spawn();
	return pSpore;
}

void CSporeGrenade::Spawn(void) 
{
	Precache();

	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/spore.mdl");
	UTIL_SetSize(pev, Vector(-4, -4, -4), Vector(4, 4, 4));

	UTIL_SetOrigin(pev, pev->origin);
	UTIL_MakeVectors(pev->angles);

	pev->gravity = 0.5;
	pev->friction = 0.2;
	Glow();

	if (pev->movetype == MOVETYPE_FLY) 
	{
		SetThink(&CSporeGrenade::FlyThink);
		SetTouch(&CSporeGrenade::ExplodeThink);
	} 
	else 
	{
		SetThink(&CSporeGrenade::FlyThink);
		SetTouch(&CSporeGrenade::BounceThink);
	}

	pev->dmgtime = (gpGlobals->time + pev->dmgtime);
	pev->dmg = 60;
	pev->nextthink = pev->ltime + 0.1;
}

//=========================================================
// Precache
//=========================================================
void CSporeGrenade::Precache(void) 
{
	PRECACHE_MODEL("models/spore.mdl");

	PRECACHE_MODEL("sprites/glow02.spr");

	PRECACHE_SOUND("mrfriendly/splauncher_impact.wav");

	PRECACHE_SOUND("mrfriendly/spore_hit1.wav");
	PRECACHE_SOUND("mrfriendly/spore_hit2.wav");
	PRECACHE_SOUND("mrfriendly/spore_hit3.wav");
}

//=========================================================
// Glow
//=========================================================
void CSporeGrenade::Glow(void) 
{
	if (!m_pSprite) 
	{
		m_pSprite = CSprite::SpriteCreate("sprites/glow02.spr", pev->origin, FALSE);
		m_pSprite->SetAttachment(edict(), 0);
		m_pSprite->pev->scale = 0.75;
		m_pSprite->SetTransparency(kRenderTransAdd, 150, 158, 19, 155, kRenderFxNoDissipation);
		m_pSprite->pev->spawnflags |= SF_SPRITE_TEMPORARY;
		m_pSprite->pev->flags |= FL_SKIPLOCALHOST;
	}
}

//=========================================================
// FlyThink
//=========================================================
void CSporeGrenade::FlyThink(void) 
{
	TraceResult tr;
	UTIL_TraceLine(pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT(pev), &tr);
	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_SPRITE_SPRAY);
		WRITE_COORD(pev->origin.x + RANDOM_LONG(-5, 5));
		WRITE_COORD(pev->origin.y + RANDOM_LONG(-5, 5));
		WRITE_COORD(pev->origin.z + RANDOM_LONG(-5, 5));
		WRITE_COORD(tr.vecPlaneNormal.Normalize().x);
		WRITE_COORD(tr.vecPlaneNormal.Normalize().y);
		WRITE_COORD(tr.vecPlaneNormal.Normalize().z);
		WRITE_SHORT(g_sModelIndexTinySpit);
		WRITE_BYTE(RANDOM_LONG(3, 8)); // count
		WRITE_BYTE(RANDOM_FLOAT(10, 15)); // speed
		WRITE_BYTE(RANDOM_FLOAT(4, 9) * 100);
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD(pev->origin.x);	// X
		WRITE_COORD(pev->origin.y);	// Y
		WRITE_COORD(pev->origin.z);	// Z
		WRITE_BYTE(8);     // radius
		WRITE_BYTE(0);		// r
		WRITE_BYTE(100);	// g
		WRITE_BYTE(0);	// b
		WRITE_BYTE(1);     // life * 10
		WRITE_BYTE(0); // decay
	MESSAGE_END();

	if (pev->movetype == MOVETYPE_BOUNCE) 
	{
		if (pev->dmgtime <= gpGlobals->time)
			Explode();
	}

	pev->nextthink = pev->ltime + 0.001;
}

//=========================================================
// BounceThink
//=========================================================
void CSporeGrenade::BounceThink(CBaseEntity *pOther) 
{
	if (pOther->pev->flags & FL_MONSTER || pOther->IsPlayer()) 
	{
		Explode();
	}

	if (UTIL_PointContents(pev->origin) == CONTENT_SKY) 
	{
		if (m_pSprite) 
		{
			UTIL_Remove(m_pSprite);
			m_pSprite = NULL;
		}
		UTIL_Remove(this);
		return;
	}

	// only do damage if we're moving fairly fast
	if (m_flNextAttack < gpGlobals->time && pev->velocity.Length() > 100) 
	{
		entvars_t *pevOwner = VARS(pev->owner);
		if (pevOwner) 
		{
			TraceResult tr = UTIL_GetGlobalTrace();
			ClearMultiDamage();
			pOther->TraceAttack(pevOwner, 1, gpGlobals->v_forward, &tr, DMG_GENERIC);
			ApplyMultiDamage(pev, pevOwner);
		}
		m_flNextAttack = gpGlobals->time + 1.0;
	}

	Vector vecTestVelocity;
	vecTestVelocity = pev->velocity;
	vecTestVelocity.z *= 0.45;

	if (pev->flags & FL_ONGROUND) 
	{
		pev->velocity = pev->velocity * 0.8;
		pev->sequence = RANDOM_LONG(1, 1);
	} 
	else 
	{
		// play bounce sound
		switch (RANDOM_LONG(0, 2)) 
		{
			case 0: EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/spore_hit1.wav", 0.25, ATTN_NORM); break;
			case 1: EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/spore_hit2.wav", 0.25, ATTN_NORM); break;
			case 2: EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/spore_hit3.wav", 0.25, ATTN_NORM); break;
		}
	}

	pev->framerate = pev->velocity.Length() / 200.0;
	if (pev->framerate > 1.0)
		pev->framerate = 1;
	else if (pev->framerate < 0.5)
		pev->framerate = 0;

}

//=========================================================
// ExplodeThink
//=========================================================
void CSporeGrenade::ExplodeThink(CBaseEntity *pOther) 
{
	if (UTIL_PointContents(pev->origin) == CONTENT_SKY) 
	{
		if (m_pSprite) 
		{
			UTIL_Remove(m_pSprite);
			m_pSprite = NULL;
		}
		UTIL_Remove(this);
		return;
	}

	Explode();
}

//=========================================================
// Explode
//=========================================================
void CSporeGrenade::Explode(void) 
{
	SetTouch(NULL);
	SetThink(NULL);
	EMIT_SOUND(ENT(pev), CHAN_ITEM, "mrfriendly/splauncher_impact.wav", VOL_NORM, ATTN_NORM);

	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_SPRITE);		
	WRITE_COORD(pev->origin.x);	
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);

	switch (RANDOM_LONG(0, 1)) 
		{
			case 0:
				WRITE_SHORT(g_sModelIndexSpore1);
			break;
			default:
			case 1:
				WRITE_SHORT(g_sModelIndexSpore3);
			break;
		}

	WRITE_BYTE(25); // scale * 10
	WRITE_BYTE(155); // framerate
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_EXPLOSION);		// This makes a dynamic light and the explosion sprites/sound
		WRITE_COORD(pev->origin.x);	// Send to PAS because of the sound
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_SHORT(g_sModelIndexSpore1);
		WRITE_BYTE((pev->dmg - 50) * .60); // scale * 10
		WRITE_BYTE(15); // framerate
		WRITE_BYTE(TE_EXPLFLAG_NOSOUND);
	MESSAGE_END();

	TraceResult tr;
	UTIL_TraceLine(pev->origin, pev->origin + pev->velocity * 15, dont_ignore_monsters, ENT(pev), &tr);

	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_SPRITE_SPRAY);		// This makes a dynamic light and the explosion sprites/sound
		WRITE_COORD(pev->origin.x);	// Send to PAS because of the sound
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_COORD(tr.vecPlaneNormal.x);
		WRITE_COORD(tr.vecPlaneNormal.y);
		WRITE_COORD(tr.vecPlaneNormal.z);
		WRITE_SHORT(g_sModelIndexTinySpit);
		WRITE_BYTE(50); // count
		WRITE_BYTE(30); // speed
		WRITE_BYTE(640);
	MESSAGE_END();

	if (CVAR_GET_FLOAT("r_particles")) 
	{
		MESSAGE_BEGIN(MSG_ALL, gmsgParticles);
			WRITE_SHORT(0);
			WRITE_BYTE(0);
			WRITE_COORD(pev->origin.x);
			WRITE_COORD(pev->origin.y);
			WRITE_COORD(pev->origin.z);
			WRITE_COORD(0);
			WRITE_COORD(0);
			WRITE_COORD(0);
			WRITE_SHORT(iDefaultSporeExplosion);
		MESSAGE_END();
	}

	// Step4enko: Dynamic light
	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD(pev->origin.x);	// X
		WRITE_COORD(pev->origin.y);	// Y
		WRITE_COORD(pev->origin.z);	// Z
		WRITE_BYTE(20);		// radius * 0.1
		WRITE_BYTE(0);		// r
		WRITE_BYTE(180);		// g
		WRITE_BYTE(0);		// b
		WRITE_BYTE(20);		// time * 10
		WRITE_BYTE(20);		// decay * 0.1
	MESSAGE_END();

	entvars_t *pevOwner;
	if (pev->owner)
		pevOwner = VARS(pev->owner);
	else
		pevOwner = NULL;

	::RadiusDamage(pev->origin, pev, pevOwner, pev->dmg, 128, CLASS_PLAYER_BIOWEAPON, DMG_GENERIC);

	if (pev->movetype == MOVETYPE_FLY) 
	{
		TraceResult tr;
		UTIL_TraceLine(pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT(pev), &tr);
		UTIL_DecalTrace(&tr, DECAL_SPIT1 + RANDOM_LONG(0, 2));
	}

	pev->velocity = g_vecZero;

	if(m_pSprite) 
	{
		UTIL_Remove(m_pSprite);
		m_pSprite = NULL;
	}

	pev->nextthink = pev->ltime + 0.3;
	UTIL_Remove(this);
}

//=========================================================
// MR.FRIENDLY CLASS
//=========================================================
class CMRFriendly : public CBaseMonster 
{
	public:
		void Spawn(void);
		void Precache(void);
		void SetYawSpeed(void);
		void Earthquake(void);
		void EffectStomp(void);

		int  ISoundMask(void);

		int  Classify(void);
		void HandleAnimEvent(MonsterEvent_t *pEvent);
		void PainSound(void);
		void DeathSound(void);
		void AlertSound(void);
		void IdleSound(void);
		void AttackSound(void);
		void StartTask(Task_t *pTask);

		void Killed( entvars_t *pevAttacker, int iGib );

		int	Save(CSave &save);
		int Restore(CRestore &restore);
		static TYPEDESCRIPTION m_SaveData[];

		Schedule_t *GetSchedule(void);
		Schedule_t *GetScheduleOfType(int Type);

		static const char *pAttackSounds[];
		static const char *pIdleSounds[];
		static const char *pPainSounds[];
		static const char *pDeathSounds[];
		static const char *pAttackHitSounds[];
		static const char *pAttackMissSounds[];

		BOOL CheckMeleeAttack1(float flDot, float flDist);
		BOOL CheckMeleeAttack2(float flDot, float flDist);
		BOOL CheckRangeAttack1(float flDot, float flDist);
		void RunAI(void);

		int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
		int IgnoreConditions(void);
		MONSTERSTATE GetIdealState(void);
		CUSTOM_SCHEDULES;

		BOOL m_fCanThreatDisplay;

		int iMrFSpitSprite;
		int iMrFStompEffectSprite;
		float m_flLastHurtTime;
		float m_flNextSpitTime;
		float m_flNextFlinch;
};

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_FR_SMELLFOOD = LAST_COMMON_SCHEDULE + 1,
	SCHED_FR_EAT,
	SCHED_FR_SNIFF_AND_EAT,
	SCHED_FR_WALLOW,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum 
{
	TASK_FR_SMELLFOOD = LAST_COMMON_SCHEDULE + 1,
};

//=========================================================
// CMRFriendly
//=========================================================
LINK_ENTITY_TO_CLASS(monster_mrfriendly, CMRFriendly);

TYPEDESCRIPTION	CMRFriendly::m_SaveData[] = 
{
	DEFINE_FIELD( CMRFriendly, m_flLastHurtTime, FIELD_TIME),
	DEFINE_FIELD( CMRFriendly, m_flNextSpitTime, FIELD_TIME),
	DEFINE_FIELD( CMRFriendly, m_fCanThreatDisplay, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE(CMRFriendly, CBaseMonster);

//=========================================================
// Monster Sounds
//=========================================================
const char *CMRFriendly::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CMRFriendly::pAttackMissSounds[] = 
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CMRFriendly::pIdleSounds[] = 
{
	"mrfriendly/mrf_scream1.wav",
	"mrfriendly/mrf_scream2.wav",
	"mrfriendly/mrf_scream3.wav",
	"mrfriendly/mrf_scream4.wav",
};

const char *CMRFriendly::pPainSounds[] = 
{
	"mrfriendly/mrf_pain2.wav",
	"mrfriendly/mrf_pain3.wav",
};

const char *CMRFriendly::pDeathSounds[] =
{
	"mrfriendly/mrf_die1.wav",
	"mrfriendly/mrf_die2.wav",
};

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CMRFriendly::ISoundMask(void) 
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
int	CMRFriendly::Classify(void) 
{
	return m_iClass ? m_iClass : CLASS_ALIEN_MONSTER;
}

//=========================================================
// IgnoreConditions 
//=========================================================
int CMRFriendly::IgnoreConditions(void) 
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if ((m_Activity == ACT_MELEE_ATTACK1) || (m_Activity == ACT_MELEE_ATTACK1)) 
	{
		if (m_flNextFlinch >= gpGlobals->time)
			iIgnore |= (bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE);
	}

	if ((m_Activity == ACT_SMALL_FLINCH) || (m_Activity == ACT_BIG_FLINCH)) 
	{
		if (m_flNextFlinch < gpGlobals->time)
			m_flNextFlinch = gpGlobals->time + MRF_FLINCH_DELAY;
	}

	return iIgnore;
}

//=========================================================
// Earthquake effect
//=========================================================
void CMRFriendly::EffectStomp( void )
{

}

//=========================================================
// Earthquake attack 
//=========================================================
void CMRFriendly::Earthquake( void )
{
	//EffectStomp();

	TraceResult trace;

	float		flAdjustedDamage;
	float		flDist;

	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "mrfriendly/mrf_earthquake.wav", 1, 0.6); // 0.8

	// Step4enko
	UTIL_ScreenShake( pev->origin, 12.0, 200.0, 2.0, 1200 ); // 100
	/*   // BUGBUGBUG - FIX ME (STEP4ENKO)
	// blast circles
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_BEAMCYLINDER );
		WRITE_COORD( pev->origin.x);
		WRITE_COORD( pev->origin.y);
		WRITE_COORD( pev->origin.z + 16);
		WRITE_COORD( pev->origin.x);
		WRITE_COORD( pev->origin.y);
		WRITE_COORD( pev->origin.z + 16 + MRF_MAX_ATTACK_RADIUS / .2); // reach damage radius over .3 seconds
		WRITE_SHORT( iMrFStompEffectSprite );
		WRITE_BYTE( 0 ); // startframe
		WRITE_BYTE( 0 ); // framerate
		WRITE_BYTE( 2 ); // life
		WRITE_BYTE( 16 );  // width
		WRITE_BYTE( 0 );   // noise

		WRITE_BYTE( 255 ); //brightness
		WRITE_BYTE( 0 );		// speed
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_BEAMCYLINDER );
		WRITE_COORD( pev->origin.x);
		WRITE_COORD( pev->origin.y);
		WRITE_COORD( pev->origin.z + 16);
		WRITE_COORD( pev->origin.x);
		WRITE_COORD( pev->origin.y);
		WRITE_COORD( pev->origin.z + 16 + ( MRF_MAX_ATTACK_RADIUS / 2 ) / .2); // reach damage radius over .3 seconds
		WRITE_SHORT( iMrFStompEffectSprite );
		WRITE_BYTE( 0 ); // startframe
		WRITE_BYTE( 0 ); // framerate
		WRITE_BYTE( 2 ); // life
		WRITE_BYTE( 16 );  // width
		WRITE_BYTE( 0 );   // noise
		
		WRITE_BYTE( 255 ); //brightness
		WRITE_BYTE( 0 );		// speed
	MESSAGE_END();*/

	UTIL_TraceLine( pev->origin, pev->origin - Vector(0,0,20), ignore_monsters, edict(), &trace );
	if ( trace.flFraction < 1.0 )
		UTIL_DecalTrace( &trace, DECAL_GARGSTOMP1 );

	CBaseEntity *pEntity = NULL;
	// iterate on all entities in the vicinity.
	while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, MRF_MAX_ATTACK_RADIUS )) != NULL)
	{
		if ( pEntity->pev->takedamage != DAMAGE_NO )
		{
			if ( !FClassnameIs(pEntity->pev, "monster_mrfriendly") )
			{
				// Mr.Friendly don`t hurt comrades.
				flAdjustedDamage = 100;

				flDist = (pEntity->Center() - pev->origin).Length();

				flAdjustedDamage -= ( flDist / MRF_MAX_ATTACK_RADIUS ) * flAdjustedDamage;

				if ( !FVisible( pEntity ) )
				{
					if ( pEntity->IsPlayer() )
					{
						flAdjustedDamage *= 0.5;
					}
					// Broke func_breakable and func_pushable if Earthquake is near
					else if ( !FClassnameIs( pEntity->pev, "func_breakable" ) && !FClassnameIs( pEntity->pev, "func_pushable" ) ) 
					{
						// do not hurt nonclients through walls, but allow damage to be done to breakables
						flAdjustedDamage = 0;
					}
				}

				//ALERT ( at_aiconsole, "Damage: %f\n", flAdjustedDamage );

				if (flAdjustedDamage > 0 )
				{
					pEntity->TakeDamage ( pev, pev, flAdjustedDamage, DMG_SONIC | DMG_ALWAYSGIB );
				}
			}
		}
	}
}

//=========================================================
// TakeDamage
//=========================================================
int CMRFriendly::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType) 
{
	float flDist;
	Vector vecApex;
	if (pev->spawnflags & SF_MONSTER_SPAWNFLAG_64) // Disable if bugs
	{
		CBaseEntity *pEnt = CBaseEntity::Instance(pevAttacker);
		if (pEnt->IsPlayer()) 
		{
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

	// Take 30% damage from bullets
	if (bitsDamageType == DMG_BULLET) 
	{
		Vector vecDir = pev->origin - (pevInflictor->absmin + pevInflictor->absmax) * 0.5;
		vecDir = vecDir.Normalize();
		float flForce = DamageForce(flDamage);
		pev->velocity = pev->velocity + vecDir * flForce;
		flDamage *= 0.3;
	}

	// if the squid is running, has an enemy, was hurt by the enemy, hasn't been hurt in the last 3 seconds, and isn't too close to the enemy,
	// it will swerve. (whew).
	if (m_hEnemy != NULL && IsMoving() && pevAttacker == m_hEnemy->pev && gpGlobals->time - m_flLastHurtTime > 3) 
	{
		flDist = (pev->origin - m_hEnemy->pev->origin).Length2D();
		if (flDist > MRF_SPRINT_DIST) 
		{
			flDist = (pev->origin - m_Route[m_iRouteIndex].vecLocation).Length2D();
			if (FTriangulate(pev->origin, m_Route[m_iRouteIndex].vecLocation, flDist * 0.5, m_hEnemy, &vecApex)) 
			{
				InsertWaypoint(vecApex, bits_MF_TO_DETOUR | bits_MF_DONT_SIMPLIFY);
			}
		}
	}

	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CMRFriendly::CheckRangeAttack1(float flDot, float flDist)
{
	if (IsMoving() && flDist >= 512)
	{
		// squid will far too far behind if he stops running to spit at this distance from the enemy.
		return FALSE;
	}

	if (flDist > 64 && flDist <= 784 && flDot >= 0.5 && gpGlobals->time >= m_flNextSpitTime)
	{
		if (m_hEnemy != NULL)
		{
			if (fabs(pev->origin.z - m_hEnemy->pev->origin.z) > 256)
			{
				// don't try to spit at someone up really high or down really low.
				return FALSE;
			}
		}

		if (IsMoving())
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
// Check for spit
//=========================================================
BOOL CMRFriendly::CheckMeleeAttack1(float flDot, float flDist) 
{
	if (flDist <= MRF_TOLERANCE_MELEE1_RANGE &&
		flDot >= MRF_TOLERANCE_MELEE1_DOT) 
	{
		return TRUE;
	}

	return FALSE;
}

//=========================================================
// Check for spit
//=========================================================
BOOL CMRFriendly::CheckMeleeAttack2(float flDot, float flDist) 
{
	if (flDist <= MRF_TOLERANCE_MELEE2_RANGE &&
		flDot >= MRF_TOLERANCE_MELEE2_DOT &&
		!HasConditions(bits_COND_CAN_MELEE_ATTACK1)) 
	{
		return TRUE;
	}

	return FALSE;
}

//=========================================================
// IdleSound 
//=========================================================
void CMRFriendly::IdleSound(void) 
{
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pIdleSounds);
}

//=========================================================
// PainSound 
//=========================================================
void CMRFriendly::PainSound(void) 
{
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pPainSounds);
}

//=========================================================
// AlertSound
//=========================================================
void CMRFriendly::AlertSound(void) 
{
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pIdleSounds);
}

//=========================================================
// DeathSound 
//=========================================================
void CMRFriendly::DeathSound(void) 
{
	EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pDeathSounds);
}

//=========================================================
// AttackSound 
//=========================================================
void CMRFriendly::AttackSound(void) 
{
	//EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pAttackSounds);
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CMRFriendly::SetYawSpeed(void) 
{
	switch (m_Activity) 
	{
		default: pev->yaw_speed = 600;   break;
	}
}

void CMRFriendly :: Killed( entvars_t *pevAttacker, int iGib )
{
	CBaseMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CMRFriendly::HandleAnimEvent(MonsterEvent_t *pEvent) 
{
	switch (pEvent->event) 
	{
		case MRF_AE_THROW: 
			{
			Vector	vecSpitOffset;
			Vector	vecSpitDir;

			UTIL_MakeVectors(pev->angles);

			Vector vecArmPos, vecArmAng;
			GetAttachment(0, vecArmPos, vecArmAng);

			vecSpitOffset = vecArmPos;
			vecSpitDir = ((m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs) - vecSpitOffset).Normalize();

			vecSpitDir.x += RANDOM_FLOAT(-0.05, 0.05);
			vecSpitDir.y += RANDOM_FLOAT(-0.05, 0.05);
			vecSpitDir.z += RANDOM_FLOAT(-0.05, 0);

			// spew the spittle temporary ents.
			MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecSpitOffset);
				WRITE_BYTE(TE_SPRITE_SPRAY);
				WRITE_COORD(vecSpitOffset.x);	// pos
				WRITE_COORD(vecSpitOffset.y);
				WRITE_COORD(vecSpitOffset.z);
				WRITE_COORD(vecSpitDir.x);	// dir
				WRITE_COORD(vecSpitDir.y);
				WRITE_COORD(vecSpitDir.z);
				// Step4enko: UNDONE
				WRITE_SHORT(iMrFSpitSprite);	// model
				WRITE_BYTE(15);			// count
				WRITE_BYTE(210);			// speed
				WRITE_BYTE(25);			// noise ( client will divide by 100 )
			MESSAGE_END();

			switch (RANDOM_LONG(0, 1)) 
			{
				case 0:
					EMIT_SOUND(ENT(pev), CHAN_WEAPON, "mrfriendly/splauncher_altfire.wav", VOL_NORM, ATTN_NORM);
				break;
				case 1:
					EMIT_SOUND(ENT(pev), CHAN_WEAPON, "mrfriendly/splauncher_altfire.wav", VOL_NORM, ATTN_NORM);
				break;
			}

			CSporeGrenade::ShootContact(pev, vecSpitOffset, vecSpitDir * 1200);
		}
		break;

		// done???
		case MRF_AE_EARTHQUAKE:
		    {
			Earthquake();
		    }
		    break;

		case MRF_AE_SLASH_LEFT: 
			{
			CBaseEntity *pHurt = CheckTraceHullAttack(MRF_MELEE_ATTACK_RADIUS, 50, DMG_SLASH | DMG_NEVERGIB);
			if (pHurt) 
			{
				if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT)) 
				{
					pHurt->pev->punchangle.z = 22;
					pHurt->pev->punchangle.x = 9;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 200;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 100;
				}
				EMIT_SOUND_ARRAY_DYN(CHAN_WEAPON, pAttackHitSounds);
			} 
			else 
			{
				EMIT_SOUND_ARRAY_DYN(CHAN_WEAPON, pAttackMissSounds);
			}
		}
		break;
		case MRF_AE_SLASH_RIGHT: 
			{
			CBaseEntity *pHurt = CheckTraceHullAttack(MRF_MELEE_ATTACK_RADIUS, 50, DMG_SLASH | DMG_NEVERGIB);
			if (pHurt) 
			{
				if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT)) 
				{
					pHurt->pev->punchangle.z = -22;
					pHurt->pev->punchangle.x = 9;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * -200;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 100;
				}
				EMIT_SOUND_ARRAY_DYN(CHAN_WEAPON, pAttackHitSounds);
			} 
			else 
			{
				EMIT_SOUND_ARRAY_DYN(CHAN_WEAPON, pAttackMissSounds);
			}
		}
		break;

		default:
			CBaseMonster::HandleAnimEvent(pEvent);
	}
}

//=========================================================
// Spawn
//=========================================================
void CMRFriendly::Spawn(void) 
{
	Precache();

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model));
	else
		SET_MODEL(ENT(pev), "models/mrfriendly.mdl");

	if (pev->health == 0)
		pev->health = RANDOM_LONG(150,200);

	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_GREEN;

	pev->view_ofs = VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )

	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP;

	m_fCanThreatDisplay = TRUE;
	m_flNextSpitTime = gpGlobals->time;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CMRFriendly::Precache(void)
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model));
	else
		PRECACHE_MODEL("models/mrfriendly.mdl");

	// Step4enko: spore explosion stuff
	g_sModelIndexSpore1 = PRECACHE_MODEL("sprites/spore_exp_01.spr");
    g_sModelIndexSpore2 = PRECACHE_MODEL("sprites/spore_exp_b_01.spr");
    g_sModelIndexSpore3 = PRECACHE_MODEL("sprites/spore_exp_c_01.spr");

	// Step4enko: stomp effect
	iMrFStompEffectSprite = PRECACHE_MODEL("sprites/effects/beta_sprite.spr");

	UTIL_PrecacheOther("spore");

	PRECACHE_MODEL("models/spore.mdl");

	PRECACHE_SOUND("mrfriendly/splauncher_altfire.wav");
	PRECACHE_SOUND("mrfriendly/mrf_earthquake.wav");


	PRECACHE_SOUND_ARRAY(pAttackHitSounds);
	PRECACHE_SOUND_ARRAY(pAttackMissSounds);
//	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);
}

//========================================================
// RunAI
//========================================================
void CMRFriendly::RunAI(void) 
{
	// first, do base class stuff
	CBaseMonster::RunAI();

	if (m_hEnemy != NULL && m_Activity == ACT_RUN) 
	{
		// chasing enemy. Sprint for last bit
		if ((pev->origin - m_hEnemy->pev->origin).Length2D() < MRF_SPRINT_DIST) 
		{
			pev->framerate = 1.25;
		}
	}

}

//========================================================
// AI Schedules Specific to this monster
//=========================================================

// primary range attack
Task_t	tlFRRangeAttack1[] =
{
	{ TASK_STOP_MOVING,			0 },
	{ TASK_FACE_IDEAL,			(float)0 },
	{ TASK_RANGE_ATTACK1,		(float)0 },
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
};

Schedule_t	slFRRangeAttack1[] =
{
	{
		tlFRRangeAttack1,
		ARRAYSIZE(tlFRRangeAttack1),
	bits_COND_NEW_ENEMY |
	bits_COND_ENEMY_DEAD |
	bits_COND_HEAVY_DAMAGE |
	bits_COND_ENEMY_OCCLUDED |
	bits_COND_NO_AMMO_LOADED,
	0,
	"FR Range Attack1"
	},
};

// Chase enemy schedule
Task_t tlFRChaseEnemy1[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_RANGE_ATTACK1 },
	{ TASK_GET_PATH_TO_ENEMY,	(float)0 },
	{ TASK_RUN_PATH,			(float)0 },
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0 },
};

Schedule_t slFRChaseEnemy[] =
{
	{
	tlFRChaseEnemy1,
	ARRAYSIZE(tlFRChaseEnemy1),
	bits_COND_NEW_ENEMY |
	bits_COND_ENEMY_DEAD |
	bits_COND_SMELL_FOOD |
	bits_COND_CAN_RANGE_ATTACK1 |
	bits_COND_CAN_MELEE_ATTACK1 |
	bits_COND_CAN_MELEE_ATTACK2 |
	bits_COND_TASK_FAILED |
	bits_COND_HEAR_SOUND,

	bits_SOUND_DANGER |
	bits_SOUND_MEAT,
	"FR Chase Enemy"
	},
};


// mrf walks to something tasty and eats it.
Task_t tlFREat[] =
{
	{ TASK_STOP_MOVING,				(float)0 },
	{ TASK_EAT,						(float)10 },
	{ TASK_STORE_LASTPOSITION,		(float)0 },
	{ TASK_GET_PATH_TO_BESTSCENT,	(float)0 },
	{ TASK_WALK_PATH,				(float)0 },
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0 },
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT },
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT },
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT },
	{ TASK_EAT,						(float)50 },
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0 },
	{ TASK_WALK_PATH,				(float)0 },
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0 },
	{ TASK_CLEAR_LASTPOSITION,		(float)0 },
};

Schedule_t slFREat[] =
{
	{
		tlFREat,
		ARRAYSIZE(tlFREat),

	// Step4enko UNDONE: Eat sequence???	
	bits_COND_LIGHT_DAMAGE |
	bits_COND_HEAVY_DAMAGE |
	bits_COND_NEW_ENEMY	,

	// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
	// here or the monster won't detect these sounds at ALL while running this schedule.
	bits_SOUND_MEAT |
	bits_SOUND_CARCASS,
	"MrFEat"
	
	}
};

// this is a bit different than just Eat. We use this schedule when the food is far away, occluded, or behind
// This schedule plays a sniff animation before going to the source of food.
Task_t tlFRSniffAndEat[] =
{
	{ TASK_STOP_MOVING,				(float)0 },
	{ TASK_EAT,						(float)10 },
	{ TASK_PLAY_SEQUENCE,			(float)ACT_DETECT_SCENT },
	{ TASK_STORE_LASTPOSITION,		(float)0 },
	{ TASK_GET_PATH_TO_BESTSCENT,	(float)0 },
	{ TASK_WALK_PATH,				(float)0 },
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0 },
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT },
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT },
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT },
	{ TASK_EAT,						(float)50 },
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0 },
	{ TASK_WALK_PATH,				(float)0 },
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0 },
	{ TASK_CLEAR_LASTPOSITION,		(float)0 },
};

Schedule_t slFRSniffAndEat[] =
{
	{
		tlFRSniffAndEat,
		ARRAYSIZE(tlFRSniffAndEat),
	bits_COND_LIGHT_DAMAGE |
	bits_COND_HEAVY_DAMAGE |
	bits_COND_NEW_ENEMY	,

	// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
	// here or the monster won't detect these sounds at ALL while running this schedule.
	bits_SOUND_MEAT |
	bits_SOUND_CARCASS,
	"FRSniffAndEat"
	}
};

// Does this to stinky things. 
Task_t tlFRWallow[] =
{
	{ TASK_STOP_MOVING,				(float)0 },
	{ TASK_EAT,						(float)10 },
	{ TASK_STORE_LASTPOSITION,		(float)0 },
	{ TASK_GET_PATH_TO_BESTSCENT,	(float)0 },
	{ TASK_WALK_PATH,				(float)0 },
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0 },
	{ TASK_PLAY_SEQUENCE,			(float)ACT_INSPECT_FLOOR },
	{ TASK_EAT,						(float)50 },
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0 },
	{ TASK_WALK_PATH,				(float)0 },
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0 },
	{ TASK_CLEAR_LASTPOSITION,		(float)0 },
};

Schedule_t slFRWallow[] =
{
	{
		tlFRWallow,
		ARRAYSIZE(tlFRWallow),
	bits_COND_LIGHT_DAMAGE |
	bits_COND_HEAVY_DAMAGE |
	bits_COND_NEW_ENEMY	,

	// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
	// here or the monster won't detect these sounds at ALL while running this schedule.
	bits_SOUND_GARBAGE,

	"FRWallow"
	}
};

DEFINE_CUSTOM_SCHEDULES(CMRFriendly)
{
	slFRRangeAttack1,
	slFRChaseEnemy,
	slFREat,
	slFRSniffAndEat,
	slFRWallow
};

IMPLEMENT_CUSTOM_SCHEDULES(CMRFriendly, CBaseMonster);

//=========================================================
// GetSchedule 
//=========================================================
Schedule_t *CMRFriendly::GetSchedule(void)
{
	switch (m_MonsterState)
	{
	case MONSTERSTATE_ALERT:
	{

		if (HasConditions(bits_COND_SMELL_FOOD))
		{
			CSound		*pSound;

			pSound = PBestScent();

			if (pSound && (!FInViewCone(&pSound->m_vecOrigin) || !FVisible(pSound->m_vecOrigin)))
			{
				// scent is behind or occluded
				return GetScheduleOfType(SCHED_MRF_SNIFF_AND_EAT);
			}

			// food is right out in the open. Just go get it.
			return GetScheduleOfType(SCHED_MRF_EAT);
		}

		if (HasConditions(bits_COND_SMELL))
		{
			// there's something stinky. 
			CSound		*pSound;

			pSound = PBestScent();
			if (pSound)
				return GetScheduleOfType(SCHED_MRF_WALLOW);
		}

		break;
	}
	case MONSTERSTATE_COMBAT:
	{
		// dead enemy
		if (HasConditions(bits_COND_ENEMY_DEAD))
		{
			// call base class, all code to handle dead enemies is centralized there.
			return CBaseMonster::GetSchedule();
		}

		if (HasConditions(bits_COND_NEW_ENEMY))
		{
			if (m_fCanThreatDisplay && IRelationship(m_hEnemy) == R_HT)
			{
				return GetScheduleOfType(SCHED_WAKE_ANGRY);
			}
		}

		if (HasConditions(bits_COND_SMELL_FOOD))
		{
			CSound		*pSound;

			pSound = PBestScent();

			if (pSound && (!FInViewCone(&pSound->m_vecOrigin) || !FVisible(pSound->m_vecOrigin)))
			{
				// scent is behind or occluded
				//return GetScheduleOfType(SCHED_MRF_SNIFF_AND_EAT);
			}

			// food is right out in the open. Just go get it.
			return GetScheduleOfType(SCHED_MRF_EAT);
		}

		if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
		{
			return GetScheduleOfType(SCHED_RANGE_ATTACK1);
		}

		if (HasConditions(bits_COND_CAN_MELEE_ATTACK1))
		{
			return GetScheduleOfType(SCHED_MELEE_ATTACK1);
		}

		if (HasConditions(bits_COND_CAN_MELEE_ATTACK2))
		{
			return GetScheduleOfType(SCHED_MELEE_ATTACK2);
		}

		return GetScheduleOfType(SCHED_CHASE_ENEMY);

		break;
	}
	}

	return CBaseMonster::GetSchedule();
}

//=========================================================
// GetScheduleOfType
//=========================================================
Schedule_t* CMRFriendly::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_RANGE_ATTACK1:
		return &slFRRangeAttack1[0];
		break;
	case SCHED_FR_EAT:
		return &slFREat[0];
		break;
//	case SCHED_FR_SNIFF_AND_EAT:
//		return &slFRSniffAndEat[0];
//		break;
	case SCHED_FR_WALLOW:
		return &slFRWallow[0];
		break;
	case SCHED_CHASE_ENEMY:
		return &slFRChaseEnemy[0];
		break;
	}

	return CBaseMonster::GetScheduleOfType(Type);
}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.  OVERRIDDEN for bullsquid because it needs to
// know explicitly when the last attempt to chase the enemy
// failed, since that impacts its attack choices.
//=========================================================
void CMRFriendly::StartTask(Task_t *pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch (pTask->iTask)
	{
		case TASK_MELEE_ATTACK1:
		{
			CBaseMonster::StartTask(pTask);
		}
		break;
		case TASK_MELEE_ATTACK2:
		{
			CBaseMonster::StartTask(pTask);
		}
		break;
		case TASK_GET_PATH_TO_ENEMY:
		{
			if (BuildRoute(m_hEnemy->pev->origin, bits_MF_TO_ENEMY, m_hEnemy))
			{
				m_iTaskStatus = TASKSTATUS_COMPLETE;
			}
			else
			{
				ALERT(at_aiconsole, "GetPathToEnemy failed!!\n");
				TaskFail();
			}
		}
		break;
		default:
			CBaseMonster::StartTask(pTask);
		break;

	}
}

//=========================================================
// GetIdealState
//=========================================================
MONSTERSTATE CMRFriendly::GetIdealState(void) 
{
	int	iConditions;
	iConditions = IScheduleFlags();
	m_IdealMonsterState = CBaseMonster::GetIdealState();
	return m_IdealMonsterState;
}