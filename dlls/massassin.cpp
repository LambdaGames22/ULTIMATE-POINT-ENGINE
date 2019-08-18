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


#include	"extdll.h"
#include	"plane.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"
#include	"squadmonster.h"
#include	"weapons.h"
#include	"talkmonster.h"
#include	"soundent.h"
#include	"effects.h"
#include	"customentity.h"
#include	"scripted.h"
#include	"hgrunt.h" // Step4enko
#include	"massassin.h" // Step4enko

#define	MASSN_CLIP_SIZE				36

// Weapon flags.
#define MASSN_9MMAR					(1 << 0) // 1
#define MASSN_HANDGRENADE			(1 << 1) // 2
#define MASSN_GRENADELAUNCHER		(1 << 2) // 4
#define MASSN_SNIPERRIFLE			(1 << 4) // 16

// Body groups.
#define HEAD_GROUP					1
#define GUN_GROUP					2

// Head values.
#define HEAD_WHITE					0
#define HEAD_BLACK					1
#define HEAD_GOGGLES				2

// Gun values.
#define GUN_MP5						0
#define GUN_SNIPERRIFLE				1
#define GUN_NONE					2

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		MASSN_AE_KICK			( 3 )
#define		MASSN_AE_BURST1			( 4 )
#define		MASSN_AE_CAUGHT_ENEMY	( 10 )
#define		MASSN_AE_DROP_GUN		( 11 )

//=========================================================
// Link Entity To Class
//=========================================================
LINK_ENTITY_TO_CLASS( monster_male_assassin, CMassn );

//=========================================================
// Save/Restore
//=========================================================
TYPEDESCRIPTION	CMassn::m_SaveData[] = 
{
	DEFINE_FIELD( CMassn, m_iHead, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CMassn, CHGrunt );

//=========================================================
// FOkToSpeak Override
//=========================================================
BOOL CMassn::FOkToSpeak()
{
	return FALSE;
}

//=========================================================
// Key Value
//=========================================================
void CMassn::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "m_iHead"))
	{
		m_iHead = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else 
		CSquadMonster::KeyValue( pkvd );
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CMassn::Classify() 
{
	return m_iClass ? m_iClass : CLASS_BLACK_OPS;
}

//=========================================================
// Idle Sound Override
//=========================================================
void CMassn::IdleSound()
{

}

//=========================================================
// Shoot
//=========================================================
void CMassn::Sniperrifle()
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	UTIL_MakeVectors(pev->angles);

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass(vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL);
	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_1DEGREES, 2048, BULLET_PLAYER_357, 0); // shoot +-7.5 degrees

	pev->effects |= EF_MUZZLEFLASH;

	m_cAmmoLoaded--; // Take away a bullet!

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CMassn::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch (pEvent->event)
	{
	case MASSN_AE_DROP_GUN:
	{
		Vector	vecGunPos;
		Vector	vecGunAngles;

		GetAttachment(0, vecGunPos, vecGunAngles);

		// Switch to body group with no gun.
		SetBodygroup(GUN_GROUP, GUN_NONE);

		// Now spawn a gun.
		if (FBitSet(pev->weapons, MASSN_SNIPERRIFLE))
			DropItem("weapon_sniperrifle", vecGunPos, vecGunAngles);
		else
			DropItem("weapon_9mmAR", vecGunPos, vecGunAngles);

		if (FBitSet(pev->weapons, MASSN_GRENADELAUNCHER))
			DropItem("ammo_ARgrenades", BodyTarget(pev->origin), vecGunAngles);
	}
	break;


	case MASSN_AE_BURST1:
	{
		if (FBitSet(pev->weapons, MASSN_9MMAR))
		{
			//Shoot();

			// the first round of the three round burst plays the sound and puts a sound in the world sound list.
			if (RANDOM_LONG(0, 1))
			{
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hgrunt/gr_mgun1.wav", VOL_NORM, ATTN_NORM);
			}
			else
			{
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hgrunt/gr_mgun2.wav", VOL_NORM, ATTN_NORM);
			}
		}
		else if (FBitSet(pev->weapons, MASSN_SNIPERRIFLE))
		{
			Sniperrifle();

			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/sniper_bolt1.wav", VOL_NORM, ATTN_NORM);
		}

		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);
	}
	break;

	case MASSN_AE_KICK:
	{
		CBaseEntity *pHurt = Kick();

		if (pHurt)
		{
			// SOUND HERE!
			UTIL_MakeVectors(pev->angles);
			pHurt->pev->punchangle.x = 20;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 150 + gpGlobals->v_up * 80;
			pHurt->TakeDamage(pev, pev, gSkillData.hgruntDmgKick, DMG_CLUB);
		}
	}
	break;

	case MASSN_AE_CAUGHT_ENEMY:
		break;

	default:
		CHGrunt::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CMassn::Spawn()
{
	Precache();

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model));
	else
		SET_MODEL(ENT(pev), "models/massn.mdl");

	if (pev->health == 0)
		pev->health = 65;

	// Step4enko: Random head.
	if (m_iHead == -1)
	{
		switch (RANDOM_LONG(0,2))
		{
		case 0:	SetBodygroup( HEAD_GROUP, HEAD_WHITE ); break;
		case 1:	SetBodygroup( HEAD_GROUP, HEAD_BLACK ); break;
		case 2:	SetBodygroup( HEAD_GROUP, HEAD_GOGGLES ); break;
		}
	}

	// Step4enko: White head.
	if (m_iHead == 0)
		SetBodygroup( HEAD_GROUP, HEAD_WHITE );

	// Step4enko: Black head.
	if (m_iHead == 1)
		SetBodygroup( HEAD_GROUP, HEAD_BLACK );

	// Step4enko: Head with goggles.
	if (m_iHead == 2)
		SetBodygroup( HEAD_GROUP, HEAD_GOGGLES );

	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = 0;
	m_flFieldOfView = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_flNextGrenadeCheck = UTIL_GlobalTimeBase() + 1;
	m_flNextPainTime = UTIL_GlobalTimeBase();
	m_iSentence = -1;

	m_afCapability = bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_fEnemyEluded = FALSE;
	m_fFirstEncounter = TRUE;// this is true when the grunt spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector(0, 0, 55);

	if (pev->weapons == 0)
	{
		// initialize to original values
		pev->weapons = MASSN_9MMAR | MASSN_HANDGRENADE;
		// pev->weapons = HGRUNT_SHOTGUN;
		// pev->weapons = HGRUNT_9MMAR | HGRUNT_GRENADELAUNCHER;
	}

	if (FBitSet(pev->weapons, MASSN_SNIPERRIFLE))
	{
		SetBodygroup(GUN_GROUP, GUN_SNIPERRIFLE);
		m_cClipSize = 5;
	}
	else
		m_cClipSize = MASSN_CLIP_SIZE;

	m_cAmmoLoaded = m_cClipSize;

	if (RANDOM_LONG(0, 99) < 80)
		pev->skin = 0;	// light skin
	else
		pev->skin = 1;	// dark skin

	CTalkMonster::g_talkWaitTime = 0;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CMassn::Precache()
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model));
	else
		PRECACHE_MODEL("models/massn.mdl");

	PRECACHE_SOUND("hgrunt/gr_mgun1.wav");
	PRECACHE_SOUND("hgrunt/gr_mgun2.wav");

	PRECACHE_SOUND("hgrunt/gr_reload1.wav");

	PRECACHE_SOUND("weapons/glauncher.wav");

	PRECACHE_SOUND("weapons/sniper_bolt1.wav");

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	m_voicePitch = 109 + RANDOM_LONG(0, 16); // get voice pitch
	m_iBrassShell = PRECACHE_MODEL("models/shell.mdl");// brass shell
}

//=========================================================
// CAssassinRepel - when triggered, spawns a monster_male_assassin
// repelling down a line.
//=========================================================
class CAssassinRepel : public CHGruntRepel
{
public:
	void Precache();
	void EXPORT RepelUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};

LINK_ENTITY_TO_CLASS(monster_assassin_repel, CAssassinRepel);

void CAssassinRepel::Precache()
{
	UTIL_PrecacheOther("monster_male_assassin");
	m_iSpriteTexture = PRECACHE_MODEL("sprites/rope.spr");
}

void CAssassinRepel::RepelUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	TraceResult tr;
	UTIL_TraceLine(pev->origin, pev->origin + Vector(0, 0, -4096.0), dont_ignore_monsters, ENT(pev), &tr);
	/*
	if ( tr.pHit && Instance( tr.pHit )->pev->solid != SOLID_BSP)
	return NULL;
	*/

	CBaseEntity *pEntity = Create("monster_male_assassin", pev->origin, pev->angles);
	CBaseMonster *pGrunt = pEntity->MyMonsterPointer();
	pGrunt->pev->movetype = MOVETYPE_FLY;
	pGrunt->pev->velocity = Vector(0, 0, RANDOM_FLOAT(-196, -128));
	pGrunt->SetActivity(ACT_GLIDE);
	// UNDONE: position?
	pGrunt->m_vecLastPosition = tr.vecEndPos;

	CBeam *pBeam = CBeam::BeamCreate("sprites/rope.spr", 10);
	pBeam->PointEntInit(pev->origin + Vector(0, 0, 112), pGrunt->entindex());
	pBeam->SetFlags(BEAM_FSOLID);
	pBeam->SetColor(255, 255, 255);
	pBeam->SetThink(&CBeam::SUB_Remove);
	pBeam->pev->nextthink = UTIL_GlobalTimeBase() + -4096.0 * tr.flFraction / pGrunt->pev->velocity.z + 0.5;

	UTIL_Remove(this);
}