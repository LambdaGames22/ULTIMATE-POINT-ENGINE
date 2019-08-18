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

===== doors.cpp ========================================================

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "doors.h"
#include "explode.h" // Step4enko

#define SF_BREAK_CROWBAR		  256

extern void SetMovedir(entvars_t* ev);

#define noiseMoving noise1
#define noiseArrived noise2

class CBaseDoor : public CBaseToggle
{
public:
	void Spawn( void );
	void Precache( void );
	virtual void KeyValue( KeyValueData *pkvd );
	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual void Blocked( CBaseEntity *pOther );

    inline int ExplosionMagnitude( void ) { return pev->impulse; }

    static const char **MaterialSoundList( Materials precacheMaterial, int &soundCount );
    int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
    void DamageSound( void );
    void MaterialSoundPrecache( Materials precacheMaterial );
    void BreakTouch( CBaseEntity *pOther );
    void Die( void );

	virtual int	ObjectCaps( void ) 
	{ 
		if (pev->spawnflags & SF_ITEM_USE_ONLY)
			return (CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_IMPULSE_USE;
		else
			return (CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION);
	};
	virtual int	Save( CSave &save );
	virtual int	Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];
	
	virtual void SetToggleState( int state );

	// Used to selectivly override defaults.
	void EXPORT DoorTouch( CBaseEntity *pOther );

	// Local functions.
	int DoorActivate( );
	void EXPORT DoorGoUp( void );
	void EXPORT DoorGoDown( void );
	void EXPORT DoorHitTop( void );
	void EXPORT DoorHitBottom( void );
	
	static const char *pSoundsWood[];
	static const char *pSoundsFlesh[];
	static const char *pSoundsGlass[];
	static const char *pSoundsMetal[];
	static const char *pSoundsConcrete[];
	static const char *pSpawnObjects[];

	BYTE	m_bHealthValue;// some doors are medi-kit doors, they give players health
	
	BYTE	m_bMoveSnd;			// sound a door makes while moving
	BYTE	m_bStopSnd;			// sound a door makes when it stops

	locksound_t m_ls;			// door lock sounds
	
	BYTE	m_bLockedSound;		// ordinals from entity selection
	BYTE	m_bLockedSentence;	
	BYTE	m_bUnlockedSound;
	BYTE	m_bUnlockedSentence;

    int       m_idShard; // Step4enko
    Materials m_Material; // Step4enko
    bool      m_bBreakable; // Step4enko
    bool      m_bExplodable; // Step4enko
	string_t  TargetOnBreak; // Step4enko
	int		  m_iszGibModel; // Step4enko
	int       m_iszSound1; // Step4enko
	int       m_iszSound2; // Step4enko

	string_t  TargetOnOpenStart; // Step4enko
	string_t  TargetOnCloseStart; // Step4enko

	int		  m_iSecondSpeed; // Step4enko
	float     m_fSecondSpeedDelay; // Step4enko
};


TYPEDESCRIPTION	CBaseDoor::m_SaveData[] = 
{
	DEFINE_FIELD( CBaseDoor, m_bHealthValue, FIELD_CHARACTER ),
	DEFINE_FIELD( CBaseDoor, m_bMoveSnd, FIELD_CHARACTER ),
	DEFINE_FIELD( CBaseDoor, m_bStopSnd, FIELD_CHARACTER ),
	
	DEFINE_FIELD( CBaseDoor, m_bLockedSound, FIELD_CHARACTER ),
	DEFINE_FIELD( CBaseDoor, m_bLockedSentence, FIELD_CHARACTER ),
	DEFINE_FIELD( CBaseDoor, m_bUnlockedSound, FIELD_CHARACTER ),
	DEFINE_FIELD( CBaseDoor, m_bUnlockedSentence, FIELD_CHARACTER ),

    DEFINE_FIELD( CBaseDoor, m_Material, FIELD_INTEGER ), // Step4enko
    DEFINE_FIELD( CBaseDoor, m_bBreakable, FIELD_BOOLEAN ), // Step4enko
    DEFINE_FIELD( CBaseDoor, m_bExplodable, FIELD_BOOLEAN ), // Step4enko
    DEFINE_FIELD( CBaseDoor, TargetOnBreak, FIELD_STRING ), // Step4enko
    DEFINE_FIELD( CBaseDoor, m_iszGibModel, FIELD_STRING ), // Step4enko
	DEFINE_FIELD( CBaseDoor, m_iszSound1, FIELD_STRING ), // Step4enko
	DEFINE_FIELD( CBaseDoor, m_iszSound2, FIELD_STRING ), // Step4enko

    DEFINE_FIELD( CBaseDoor, TargetOnOpenStart, FIELD_STRING ), // Step4enko
    DEFINE_FIELD( CBaseDoor, TargetOnCloseStart, FIELD_STRING ), // Step4enko

    DEFINE_FIELD( CBaseDoor, m_iSecondSpeed, FIELD_INTEGER ), // Step4enko
    DEFINE_FIELD( CBaseDoor, m_fSecondSpeedDelay, FIELD_FLOAT ), // Step4enko
};

IMPLEMENT_SAVERESTORE( CBaseDoor, CBaseToggle );


#define DOOR_SENTENCEWAIT	6
#define DOOR_SOUNDWAIT		3
#define BUTTON_SOUNDWAIT	0.5

// play door or button locked or unlocked sounds. 
// pass in pointer to valid locksound struct. 
// if flocked is true, play 'door is locked' sound,
// otherwise play 'door is unlocked' sound
// NOTE: this routine is shared by doors and buttons

void PlayLockSounds(entvars_t *pev, locksound_t *pls, int flocked, int fbutton)
{
	// LOCKED SOUND
	
	// CONSIDER: consolidate the locksound_t struct (all entries are duplicates for lock/unlock)
	// CONSIDER: and condense this code.
	float flsoundwait;

	if (fbutton)
		flsoundwait = BUTTON_SOUNDWAIT;
	else
		flsoundwait = DOOR_SOUNDWAIT;

	if (flocked)
	{
		int fplaysound = (pls->sLockedSound && gpGlobals->time > pls->flwaitSound);
		int fplaysentence = (pls->sLockedSentence && !pls->bEOFLocked && gpGlobals->time > pls->flwaitSentence);
		float fvol;

		if (fplaysound && fplaysentence)
			fvol = 0.25;
		else
			fvol = 1.0;

		// if there is a locked sound, and we've debounced, play sound
		if (fplaysound)
		{
			// play 'door locked' sound
			EMIT_SOUND(ENT(pev), CHAN_ITEM, (char*)STRING(pls->sLockedSound), fvol, ATTN_NORM);
			pls->flwaitSound = gpGlobals->time + flsoundwait;
		}

		// if there is a sentence, we've not played all in list, and we've debounced, play sound
		if (fplaysentence)
		{
			// play next 'door locked' sentence in group
			int iprev = pls->iLockedSentence;
			
			pls->iLockedSentence = SENTENCEG_PlaySequentialSz(ENT(pev), STRING(pls->sLockedSentence), 
					  0.85, ATTN_NORM, 0, 100, pls->iLockedSentence, FALSE);
			pls->iUnlockedSentence = 0;

			// make sure we don't keep calling last sentence in list
			pls->bEOFLocked = (iprev == pls->iLockedSentence);
		
			pls->flwaitSentence = gpGlobals->time + DOOR_SENTENCEWAIT;
		}
	}
	else
	{
		// UNLOCKED SOUND

		int fplaysound = (pls->sUnlockedSound && gpGlobals->time > pls->flwaitSound);
		int fplaysentence = (pls->sUnlockedSentence && !pls->bEOFUnlocked && gpGlobals->time > pls->flwaitSentence);
		float fvol;

		// if playing both sentence and sound, lower sound volume so we hear sentence
		if (fplaysound && fplaysentence)
			fvol = 0.25;
		else
			fvol = 1.0;

		// play 'door unlocked' sound if set
		if (fplaysound)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, (char*)STRING(pls->sUnlockedSound), fvol, ATTN_NORM);
			pls->flwaitSound = gpGlobals->time + flsoundwait;
		}

		// play next 'door unlocked' sentence in group
		if (fplaysentence)
		{
			int iprev = pls->iUnlockedSentence;
			
			pls->iUnlockedSentence = SENTENCEG_PlaySequentialSz(ENT(pev), STRING(pls->sUnlockedSentence), 
					  0.85, ATTN_NORM, 0, 100, pls->iUnlockedSentence, FALSE);
			pls->iLockedSentence = 0;

			// make sure we don't keep calling last sentence in list
			pls->bEOFUnlocked = (iprev == pls->iUnlockedSentence);
			pls->flwaitSentence = gpGlobals->time + DOOR_SENTENCEWAIT;
		}
	}
}

//
// Cache user-entity-field values until spawn is called.
//

void CBaseDoor::KeyValue( KeyValueData *pkvd )
{
	int i;

	if (FStrEq(pkvd->szKeyName, "skin"))//skin is used for content type
	{
		pev->skin = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "movesnd"))
	{
		m_bMoveSnd = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "stopsnd"))
	{
		m_bStopSnd = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "healthvalue"))
	{
		m_bHealthValue = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "locked_sound"))
	{
		m_bLockedSound = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "locked_sentence"))
	{
		m_bLockedSentence = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "unlocked_sound"))
	{
		m_bUnlockedSound = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "unlocked_sentence"))
	{
		m_bUnlockedSentence = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "WaveHeight"))
	{
		pev->scale = atof(pkvd->szValue) * (1.0/8.0);
		pkvd->fHandled = TRUE;
	}
    else if (FStrEq(pkvd->szKeyName, "breakable"))
    {
        i = atoi( pkvd->szValue );
        m_bBreakable = (i == 1);
    }
    else if (FStrEq(pkvd->szKeyName, "explodemagnitude"))
    {
        i = atoi( pkvd->szValue );
        if( i < 0)
            i *= -1;
        pev->impulse = i;
        m_bExplodable = (i > 0);
    }
    else if (FStrEq(pkvd->szKeyName, "material"))
    {
        i = atoi( pkvd->szValue );

        if ((i < 0) || (i >= matLastMaterial))
            m_Material = matWood;
        else
            m_Material = (Materials)i;

        pkvd->fHandled = true;
    }
	else if (FStrEq(pkvd->szKeyName, "TargetOnBreak"))
	{
		TargetOnBreak = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "TargetOnOpenStart"))
	{
		TargetOnOpenStart = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "TargetOnCloseStart"))
	{
		TargetOnCloseStart = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "gibmodel") )
	{
		m_iszGibModel = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszSound1"))
	{
		m_iszSound1 = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszSound2"))
	{
		m_iszSound2 = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iSecondSpeed"))
	{
		m_iSecondSpeed = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_fSecondSpeedDelay"))
	{
		m_fSecondSpeedDelay = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}

/*QUAKED func_door (0 .5 .8) ? START_OPEN x DOOR_DONT_LINK TOGGLE
if two doors touch, they are assumed to be connected and operate as a unit.

TOGGLE causes the door to wait in both the start and end states for a trigger event.

START_OPEN causes the door to move to its destination when spawned, and operate in reverse.
It is used to temporarily or permanently close off an area when triggered (not usefull for
touch or takedamage doors).

"angle"         determines the opening direction
"targetname"	if set, no touch field will be spawned and a remote button or trigger
				field activates the door.
"health"        if set, door must be shot open
"speed"         movement speed (100 default)
"wait"          wait before returning (3 default, -1 = never return)
"lip"           lip remaining at end of move (8 default)
"dmg"           damage to inflict when blocked (2 default)
"sounds"
0)      no sound
1)      stone
2)      base
3)      stone chain
4)      screechy metal
*/

LINK_ENTITY_TO_CLASS( func_door, CBaseDoor );
//
// func_water - same as a door. 
//
LINK_ENTITY_TO_CLASS( func_water, CBaseDoor );


void CBaseDoor::Spawn( )
{
	Precache();
	SetMovedir (pev);

	if ( pev->skin == 0 )
	{
		// Normal door.
		if ( FBitSet (pev->spawnflags, SF_DOOR_PASSABLE) )
			pev->solid		= SOLID_NOT;
		else
			pev->solid		= SOLID_BSP;
	}
	else
	{
		// Special contents.
		pev->solid		= SOLID_NOT;
		SetBits( pev->spawnflags, SF_DOOR_SILENT );	// Water is silent for now.
	}

	if (pev->health == 0)
		pev->health = 100;

	pev->takedamage = DAMAGE_YES;
	pev->movetype	= MOVETYPE_PUSH;
	UTIL_SetOrigin(pev, pev->origin);
	SET_MODEL( ENT(pev), STRING(pev->model) );
	
	if (pev->speed == 0)
		pev->speed = 100;
	
	m_vecPosition1	= pev->origin;
	// Subtract 2 from size because the engine expands bboxes by 1 in all directions making the size too big.
	m_vecPosition2	= m_vecPosition1 + (pev->movedir * (fabs( pev->movedir.x * (pev->size.x-2) ) + fabs( pev->movedir.y * (pev->size.y-2) ) + fabs( pev->movedir.z * (pev->size.z-2) ) - m_flLip));
	ASSERTSZ(m_vecPosition1 != m_vecPosition2, "door start/end positions are equal");
	if ( FBitSet (pev->spawnflags, SF_DOOR_START_OPEN) )
	{	
		// Swap pos1 and pos2, put door at pos2.
		UTIL_SetOrigin(pev, m_vecPosition2);
		m_vecPosition2 = m_vecPosition1;
		m_vecPosition1 = pev->origin;
	}

	m_toggle_state = TS_AT_BOTTOM;
	
	// If the door is flagged for USE button activation only, use NULL touch function.
	if ( FBitSet ( pev->spawnflags, SF_DOOR_USE_ONLY ) )
		SetTouch ( NULL );
	else 
		// Touchable button.
		SetTouch( &CBaseDoor::DoorTouch );
}
 
const char *CBaseDoor::pSoundsWood[] = 
{
	"debris/wood1.wav",
	"debris/wood2.wav",
	"debris/wood3.wav",
	"debris/wood4.wav",
};

const char *CBaseDoor::pSoundsFlesh[] = 
{
	"debris/flesh1.wav",
	"debris/flesh2.wav",
	"debris/flesh3.wav",
	"debris/flesh5.wav",
	"debris/flesh6.wav",
	"debris/flesh7.wav",
};

const char *CBaseDoor::pSoundsMetal[] = 
{
	"debris/metal1.wav",
	"debris/metal2.wav",
	"debris/metal3.wav",
};

const char *CBaseDoor::pSoundsConcrete[] = 
{
	"debris/concrete1.wav",
	"debris/concrete2.wav",
	"debris/concrete3.wav",
};


const char *CBaseDoor::pSoundsGlass[] = 
{
	"debris/glass1.wav",
	"debris/glass2.wav",
	"debris/glass3.wav",
	"debris/glass4.wav",
};

const char **CBaseDoor::MaterialSoundList( Materials precacheMaterial, int &soundCount )
{
	const char	**pSoundList = NULL;

    switch ( precacheMaterial ) 
	{
	case matWood:
		pSoundList = pSoundsWood;
		soundCount = ARRAYSIZE(pSoundsWood);
		break;
	case matFlesh:
		pSoundList = pSoundsFlesh;
		soundCount = ARRAYSIZE(pSoundsFlesh);
		break;
	case matComputer:
	case matUnbreakableGlass:
	case matGlass:
		pSoundList = pSoundsGlass;
		soundCount = ARRAYSIZE(pSoundsGlass);
		break;

	case matMetal:
		pSoundList = pSoundsMetal;
		soundCount = ARRAYSIZE(pSoundsMetal);
		break;

	case matCinderBlock:
	case matRocks:
		pSoundList = pSoundsConcrete;
		soundCount = ARRAYSIZE(pSoundsConcrete);
		break;
	
	case matCeilingTile:
	case matNone:
	default:
		soundCount = 0;
		break;
	}

	return pSoundList;
}

int CBaseDoor :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	Vector	vecTemp;

	// if Attacker == Inflictor, the attack was a melee or other instant-hit attack.
	// (that is, no actual entity projectile was involved in the attack so use the shooter's origin). 
	if ( pevAttacker == pevInflictor )	
	{
		vecTemp = pevInflictor->origin - ( pev->absmin + ( pev->size * 0.5 ) );
		
		// if a client hit the breakable with a crowbar, and breakable is crowbar-sensitive, break it now.
		if ( FBitSet ( pevAttacker->flags, FL_CLIENT ) &&
				 FBitSet ( pev->spawnflags, SF_BREAK_CROWBAR ) && (bitsDamageType & DMG_CLUB))
			flDamage = pev->health;
	}
	else
	// an actual missile was involved.
	{
		vecTemp = pevInflictor->origin - ( pev->absmin + ( pev->size * 0.5 ) );
	}
	
	if (!m_bBreakable)
		return 0;

	// Breakables take double damage from the crowbar
	if ( bitsDamageType & DMG_CLUB )
		flDamage *= 2;

	// Boxes / glass / etc. don't take much poison damage, just the impact of the dart - consider that 10%
	if ( bitsDamageType & DMG_POISON )
		flDamage *= 0.1;

// this global is still used for glass and other non-monster killables, along with decals.
	//g_vecAttackDir = vecTemp.Normalize();
		
// do the damage
	pev->health -= flDamage;
	if (pev->health <= 0)
	{
		Killed( pevAttacker, GIB_NORMAL );
		Die();
		return 0;
	}

	// Make a shard noise each time func breakable is hit.
	// Don't play shard noise if cbreakable actually died.

	DamageSound();

	return 1;
}

void CBaseDoor::DamageSound( void )
{
	int pitch;
	float fvol;
	char *rgpsz[6];
	int i;
	int material = m_Material;

//	if (RANDOM_LONG(0,1))
//		return;

	if (RANDOM_LONG(0,2))
		pitch = PITCH_NORM;
	else
		pitch = 95 + RANDOM_LONG(0,34);

	fvol = RANDOM_FLOAT(0.75, 1.0);

	if (material == matComputer && RANDOM_LONG(0,1))
		material = matMetal;

	switch (material)
	{
	case matComputer:
	case matGlass:
	case matUnbreakableGlass:
		rgpsz[0] = "debris/glass1.wav";
		rgpsz[1] = "debris/glass2.wav";
		rgpsz[2] = "debris/glass3.wav";
		rgpsz[3] = "debris/glass4.wav";
		i = 4;
		break;

	case matWood:
		rgpsz[0] = "debris/wood1.wav";
		rgpsz[1] = "debris/wood2.wav";
		rgpsz[2] = "debris/wood3.wav";
		rgpsz[3] = "debris/wood4.wav";
		i = 4;
		break;

	case matMetal:
		rgpsz[0] = "debris/metal1.wav";
		rgpsz[1] = "debris/metal3.wav";
		rgpsz[2] = "debris/metal2.wav";
		i = 2;
		break;

	case matFlesh:
		rgpsz[0] = "debris/flesh1.wav";
		rgpsz[1] = "debris/flesh2.wav";
		rgpsz[2] = "debris/flesh3.wav";
		rgpsz[3] = "debris/flesh5.wav";
		rgpsz[4] = "debris/flesh6.wav";
		rgpsz[5] = "debris/flesh7.wav";
		i = 6;
		break;

	case matRocks:
	case matCinderBlock:
		rgpsz[0] = "debris/concrete1.wav";
		rgpsz[1] = "debris/concrete2.wav";
		rgpsz[2] = "debris/concrete3.wav";
		i = 3;
		break;

	case matCeilingTile:
		// UNDONE: no ceiling tile shard sound yet
		i = 0;
		break;
	}

	if (i)
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, rgpsz[RANDOM_LONG(0,i-1)], fvol, ATTN_NORM, 0, pitch);
}

void CBaseDoor::Die( void )
{
	Vector vecSpot;// shard origin
	Vector vecVelocity;// shard velocity
	CBaseEntity *pEntity = NULL;
	char cFlag = 0;
	int pitch;
	float fvol;
	
	pitch = 95 + RANDOM_LONG(0,29);

	if (pitch > 97 && pitch < 103)
		pitch = 100;

	// The more negative pev->health, the louder
	// the sound should be.

	fvol = RANDOM_FLOAT(0.85, 1.0) + (abs(pev->health) / 100.0);

	if (fvol > 1.0)
		fvol = 1.0;


	switch (m_Material)
	{
	case matGlass:
		switch ( RANDOM_LONG(0,1) )
		{
		case 0:	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustglass1.wav", fvol, ATTN_NORM, 0, pitch);	
			break;
		case 1:	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustglass2.wav", fvol, ATTN_NORM, 0, pitch);	
			break;
		}
		cFlag = BREAK_GLASS;
		break;

	case matWood:
		switch ( RANDOM_LONG(0,1) )
		{
		case 0:	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustcrate1.wav", fvol, ATTN_NORM, 0, pitch);	
			break;
		case 1:	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustcrate2.wav", fvol, ATTN_NORM, 0, pitch);	
			break;
		}
		cFlag = BREAK_WOOD;
		break;

	case matComputer:
	case matMetal:
		switch ( RANDOM_LONG(0,1) )
		{
		case 0:	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustmetal1.wav", fvol, ATTN_NORM, 0, pitch);	
			break;
		case 1:	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustmetal2.wav", fvol, ATTN_NORM, 0, pitch);	
			break;
		}
		cFlag = BREAK_METAL;
		break;

	case matFlesh:
		switch ( RANDOM_LONG(0,1) )
		{
		case 0:	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustflesh1.wav", fvol, ATTN_NORM, 0, pitch);	
			break;
		case 1:	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustflesh2.wav", fvol, ATTN_NORM, 0, pitch);	
			break;
		}
		cFlag = BREAK_FLESH;
		break;

	case matRocks:
	case matCinderBlock:
		switch ( RANDOM_LONG(0,1) )
		{
		case 0:	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustconcrete1.wav", fvol, ATTN_NORM, 0, pitch);	
			break;
		case 1:	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustconcrete2.wav", fvol, ATTN_NORM, 0, pitch);	
			break;
		}
		cFlag = BREAK_CONCRETE;
		break;

	case matCeilingTile:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustceiling.wav", fvol, ATTN_NORM, 0, pitch);
		break;
	}
    
		
//	if (m_Explosion == expDirected)
//		vecVelocity = g_vecAttackDir * 200;
//	else
	{
		vecVelocity.x = 0;
		vecVelocity.y = 0;
		vecVelocity.z = 0;
	}

	vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
		WRITE_BYTE( TE_BREAKMODEL);

		// position
		WRITE_COORD( vecSpot.x );
		WRITE_COORD( vecSpot.y );
		WRITE_COORD( vecSpot.z );

		// size
		WRITE_COORD( pev->size.x);
		WRITE_COORD( pev->size.y);
		WRITE_COORD( pev->size.z);

		// velocity
		WRITE_COORD( vecVelocity.x ); 
		WRITE_COORD( vecVelocity.y );
		WRITE_COORD( vecVelocity.z );

		// randomization
		WRITE_BYTE( 10 ); 

		// Model
		WRITE_SHORT( m_idShard );	//model id#

		// # of shards
		WRITE_BYTE( 0 );	// let client decide

		// duration
		WRITE_BYTE( 25 );// 2.5 seconds

		// flags
		WRITE_BYTE( cFlag );
	MESSAGE_END();

	float size = pev->size.x;
	if ( size < pev->size.y )
		size = pev->size.y;
	if ( size < pev->size.z )
		size = pev->size.z;

	// !!! HACK  This should work!
	// Build a box above the entity that looks like an 8 pixel high sheet
	Vector mins = pev->absmin;
	Vector maxs = pev->absmax;
	mins.z = pev->absmax.z;
	maxs.z += 8;

	// BUGBUG -- can only find 256 entities on a breakable -- should be enough
	CBaseEntity *pList[256];
	int count = UTIL_EntitiesInBox( pList, 256, mins, maxs, FL_ONGROUND );
	if ( count )
	{
		for ( int i = 0; i < count; i++ )
		{
			ClearBits( pList[i]->pev->flags, FL_ONGROUND );
			pList[i]->pev->groundentity = NULL;
		}
	}

	// Don't fire something that could fire myself
	pev->targetname = 0;

	pev->solid = SOLID_NOT;

	// Step4enko: Fire target on break.
    if (!FStringNull(TargetOnBreak))
	    FireTargets( STRING(TargetOnBreak), m_hActivator, this, USE_TOGGLE, 0 );

	SetThink( &CBaseDoor::SUB_Remove );
	pev->nextthink = pev->ltime + 0.1;

	if ( m_bExplodable )
		ExplosionCreate( Center(), pev->angles, edict(), ExplosionMagnitude(), TRUE );
}


void CBaseDoor :: SetToggleState( int state )
{
	if ( state == TS_AT_TOP )
		UTIL_SetOrigin( pev, m_vecPosition2 );
	else
		UTIL_SetOrigin( pev, m_vecPosition1 );
}


void CBaseDoor::Precache( void )
{
	char* szSoundFile1 = (char*) STRING( m_iszSound1 );
	char* szSoundFile2 = (char*) STRING( m_iszSound2 );

	char *pszSound;

    // Step4enko: Precache sounds/models for breakable doors.
 	const char *pGibName;

    switch (m_Material) 
	{
	case matWood:
		pGibName = "models/woodgibs.mdl";
		
		PRECACHE_SOUND("debris/bustcrate1.wav");
		PRECACHE_SOUND("debris/bustcrate2.wav");
		break;
	case matFlesh:
		pGibName = "models/fleshgibs.mdl";
		
		PRECACHE_SOUND("debris/bustflesh1.wav");
		PRECACHE_SOUND("debris/bustflesh2.wav");
		break;
	case matComputer:
		PRECACHE_SOUND("buttons/spark5.wav");
		PRECACHE_SOUND("buttons/spark6.wav");
		pGibName = "models/computergibs.mdl";
		
		PRECACHE_SOUND("debris/bustmetal1.wav");
		PRECACHE_SOUND("debris/bustmetal2.wav");
		break;

	case matUnbreakableGlass:
	case matGlass:
		pGibName = "models/glassgibs.mdl";
		
		PRECACHE_SOUND("debris/bustglass1.wav");
		PRECACHE_SOUND("debris/bustglass2.wav");
		break;
	case matMetal:
		pGibName = "models/metalplategibs.mdl";
		
		PRECACHE_SOUND("debris/bustmetal1.wav");
		PRECACHE_SOUND("debris/bustmetal2.wav");
		break;
	case matCinderBlock:
		pGibName = "models/cindergibs.mdl";
		
		PRECACHE_SOUND("debris/bustconcrete1.wav");
		PRECACHE_SOUND("debris/bustconcrete2.wav");
		break;
	case matRocks:
		pGibName = "models/rockgibs.mdl";
		
		PRECACHE_SOUND("debris/bustconcrete1.wav");
		PRECACHE_SOUND("debris/bustconcrete2.wav");
		break;
	case matCeilingTile:
		pGibName = "models/ceilinggibs.mdl";
		
		PRECACHE_SOUND ("debris/bustceiling.wav");  
		break;
	}
	MaterialSoundPrecache( m_Material );
	if ( m_iszGibModel )
		pGibName = STRING(m_iszGibModel);

	// Step4enko: UNDONE: Custom gibs?
	m_idShard = PRECACHE_MODEL( (char *)pGibName );

	if (FStringNull(m_iszSound1))
	{
		switch (m_bMoveSnd)
		{
		case 0:
			pev->noiseMoving = ALLOC_STRING("common/null.wav");
			break;
		case 1:
			PRECACHE_SOUND ("doors/doormove1.wav");
			pev->noiseMoving = ALLOC_STRING("doors/doormove1.wav");
			break;
		case 2:
			PRECACHE_SOUND ("doors/doormove2.wav");
			pev->noiseMoving = ALLOC_STRING("doors/doormove2.wav");
			break;
		case 3:
			PRECACHE_SOUND ("doors/doormove3.wav");
			pev->noiseMoving = ALLOC_STRING("doors/doormove3.wav");
			break;
		case 4:
			PRECACHE_SOUND ("doors/doormove4.wav");
			pev->noiseMoving = ALLOC_STRING("doors/doormove4.wav");
			break;
		case 5:
			PRECACHE_SOUND ("doors/doormove5.wav");
			pev->noiseMoving = ALLOC_STRING("doors/doormove5.wav");
			break;
		case 6:
			PRECACHE_SOUND ("doors/doormove6.wav");
			pev->noiseMoving = ALLOC_STRING("doors/doormove6.wav");
			break;
		case 7:
			PRECACHE_SOUND ("doors/doormove7.wav");
			pev->noiseMoving = ALLOC_STRING("doors/doormove7.wav");
			break;
		case 8:
			PRECACHE_SOUND ("doors/doormove8.wav");
			pev->noiseMoving = ALLOC_STRING("doors/doormove8.wav");
			break;
		case 9:
			PRECACHE_SOUND ("doors/doormove9.wav");
			pev->noiseMoving = ALLOC_STRING("doors/doormove9.wav");
			break;
		case 10:
			PRECACHE_SOUND ("doors/doormove10.wav");
			pev->noiseMoving = ALLOC_STRING("doors/doormove10.wav");
			break;
		default:
			pev->noiseMoving = ALLOC_STRING("common/null.wav");
			break;
		}
	}
	else
		PRECACHE_SOUND( szSoundFile1 );

	if (FStringNull(m_iszSound2))
	{
		switch (m_bStopSnd)
		{
		case 0:
			pev->noiseArrived = ALLOC_STRING("common/null.wav");
			break;
		case 1:
			PRECACHE_SOUND ("doors/doorstop1.wav");
			pev->noiseArrived = ALLOC_STRING("doors/doorstop1.wav");
			break;
		case 2:
			PRECACHE_SOUND ("doors/doorstop2.wav");
			pev->noiseArrived = ALLOC_STRING("doors/doorstop2.wav");
			break;
		case 3:
			PRECACHE_SOUND ("doors/doorstop3.wav");
			pev->noiseArrived = ALLOC_STRING("doors/doorstop3.wav");
			break;
		case 4:
			PRECACHE_SOUND ("doors/doorstop4.wav");
			pev->noiseArrived = ALLOC_STRING("doors/doorstop4.wav");
			break;
		case 5:
			PRECACHE_SOUND ("doors/doorstop5.wav");
			pev->noiseArrived = ALLOC_STRING("doors/doorstop5.wav");
			break;
		case 6:
			PRECACHE_SOUND ("doors/doorstop6.wav");
			pev->noiseArrived = ALLOC_STRING("doors/doorstop6.wav");
			break;
		case 7:
			PRECACHE_SOUND ("doors/doorstop7.wav");
			pev->noiseArrived = ALLOC_STRING("doors/doorstop7.wav");
			break;
		case 8:
			PRECACHE_SOUND ("doors/doorstop8.wav");
			pev->noiseArrived = ALLOC_STRING("doors/doorstop8.wav");
			break;
		default:
			pev->noiseArrived = ALLOC_STRING("common/null.wav");
			break;
		}
	}
	else
		PRECACHE_SOUND( szSoundFile2 );

	// Get door button sounds, for doors which are directly 'touched' to open.
	if (m_bLockedSound)
	{
		pszSound = ButtonSound( (int)m_bLockedSound );
		PRECACHE_SOUND(pszSound);
		m_ls.sLockedSound = ALLOC_STRING(pszSound);
	}

	if (m_bUnlockedSound)
	{
		pszSound = ButtonSound( (int)m_bUnlockedSound );
		PRECACHE_SOUND(pszSound);
		m_ls.sUnlockedSound = ALLOC_STRING(pszSound);
	}

	// Get sentence group names, for doors which are directly 'touched' to open.
	switch (m_bLockedSentence)
	{
		case 1: m_ls.sLockedSentence = ALLOC_STRING("NA"); break; // access denied
		case 2: m_ls.sLockedSentence = ALLOC_STRING("ND"); break; // security lockout
		case 3: m_ls.sLockedSentence = ALLOC_STRING("NF"); break; // blast door
		case 4: m_ls.sLockedSentence = ALLOC_STRING("NFIRE"); break; // fire door
		case 5: m_ls.sLockedSentence = ALLOC_STRING("NCHEM"); break; // chemical door
		case 6: m_ls.sLockedSentence = ALLOC_STRING("NRAD"); break; // radiation door
		case 7: m_ls.sLockedSentence = ALLOC_STRING("NCON"); break; // gen containment
		case 8: m_ls.sLockedSentence = ALLOC_STRING("NH"); break; // maintenance door
		case 9: m_ls.sLockedSentence = ALLOC_STRING("NG"); break; // broken door
		
		default: m_ls.sLockedSentence = 0; break;
	}

	switch (m_bUnlockedSentence)
	{
		case 1: m_ls.sUnlockedSentence = ALLOC_STRING("EA"); break; // access granted
		case 2: m_ls.sUnlockedSentence = ALLOC_STRING("ED"); break; // security door
		case 3: m_ls.sUnlockedSentence = ALLOC_STRING("EF"); break; // blast door
		case 4: m_ls.sUnlockedSentence = ALLOC_STRING("EFIRE"); break; // fire door
		case 5: m_ls.sUnlockedSentence = ALLOC_STRING("ECHEM"); break; // chemical door
		case 6: m_ls.sUnlockedSentence = ALLOC_STRING("ERAD"); break; // radiation door
		case 7: m_ls.sUnlockedSentence = ALLOC_STRING("ECON"); break; // gen containment
		case 8: m_ls.sUnlockedSentence = ALLOC_STRING("EH"); break; // maintenance door
		
		default: m_ls.sUnlockedSentence = 0; break;
	}
}

void CBaseDoor::BreakTouch( CBaseEntity *pOther )
{
	entvars_t*	pevToucher = pOther->pev;
	
	// only players can break these right now
	if ( !pOther->IsPlayer() || !m_bBreakable )
	{
        return;
	}
}

void CBaseDoor::MaterialSoundPrecache( Materials precacheMaterial )
{
	const char	**pSoundList;
	int			i, soundCount = 0;

	pSoundList = MaterialSoundList( precacheMaterial, soundCount );

	for ( i = 0; i < soundCount; i++ )
	{
		PRECACHE_SOUND( (char *)pSoundList[i] );
	}
}



//
// Doors not tied to anything (e.g. button, another door) can be touched, to make them activate.
//
void CBaseDoor::DoorTouch( CBaseEntity *pOther )
{
	entvars_t*	pevToucher = pOther->pev;
	
	// Ignore touches by anything but players
	if (!FClassnameIs(pevToucher, "player"))
		return;

	// If door has master, and it's not ready to trigger, 
	// play 'locked' sound

	if (m_sMaster && !UTIL_IsMasterTriggered(m_sMaster, pOther))
		PlayLockSounds(pev, &m_ls, TRUE, FALSE);
	
	// If door is somebody's target, then touching does nothing.
	// You have to activate the owner (e.g. button).
	
	if (!FStringNull(pev->targetname))
	{
		// play locked sound
		PlayLockSounds(pev, &m_ls, TRUE, FALSE);
		return; 
	}
	
	m_hActivator = pOther;// remember who activated the door

    BreakTouch( pOther );

	if (DoorActivate( ))
		SetTouch( NULL ); // Temporarily disable the touch function, until movement is finished.
}


//
// Used by SUB_UseTargets, when a door is the target of a button.
//
void CBaseDoor::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	m_hActivator = pActivator;
	// if not ready to be used, ignore "use" command.
	if (m_toggle_state == TS_AT_BOTTOM || FBitSet(pev->spawnflags, SF_DOOR_NO_AUTO_RETURN) && m_toggle_state == TS_AT_TOP)
		DoorActivate();
}

//
// Causes the door to "do its thing", i.e. start moving, and cascade activation.
//
int CBaseDoor::DoorActivate( )
{
	if (!UTIL_IsMasterTriggered(m_sMaster, m_hActivator))
		return 0;

	if (FBitSet(pev->spawnflags, SF_DOOR_NO_AUTO_RETURN) && m_toggle_state == TS_AT_TOP)
	{
		// door should close
		DoorGoDown();
	}
	else
	{
		// door should open

		if ( m_hActivator != NULL && m_hActivator->IsPlayer() )
		{
			// give health if player opened the door (medikit)
		    // VARS( m_eoActivator )->health += m_bHealthValue;
	
			m_hActivator->TakeHealth( m_bHealthValue, DMG_GENERIC );

		}

		// play door unlock sounds
		PlayLockSounds(pev, &m_ls, FALSE, FALSE);
		
		DoorGoUp();
	}

	return 1;
}

extern Vector VecBModelOrigin( entvars_t* pevBModel );

//
// Starts the door going to its "up" position (simply ToggleData->vecPosition2).
//
void CBaseDoor::DoorGoUp( void )
{
	char* szSoundFile1 = (char*) STRING( m_iszSound1 );

	entvars_t	*pevActivator;

	// It could be going-down, if blocked.
	ASSERT(m_toggle_state == TS_AT_BOTTOM || m_toggle_state == TS_GOING_DOWN);

	// emit door moving and stop sounds on CHAN_STATIC so that the multicast doesn't
	// filter them out and leave a client stuck with looping door sounds!
	if ( !FBitSet( pev->spawnflags, SF_DOOR_SILENT ) )
	{
		if (FStringNull(m_iszSound1))
		{
		    if ( m_toggle_state != TS_GOING_UP && m_toggle_state != TS_GOING_DOWN )
			    EMIT_SOUND(ENT(pev), CHAN_STATIC, (char*)STRING(pev->noiseMoving), 1, ATTN_NORM);
		}
		else
		{
			if ( m_toggle_state != TS_GOING_UP && m_toggle_state != TS_GOING_DOWN )
			    EMIT_SOUND(ENT(pev), CHAN_STATIC, szSoundFile1, 1, ATTN_NORM);
		}
	}

	// Step4enko: Fire on Open (Start)
    if (!FStringNull(TargetOnOpenStart))
	    FireTargets( STRING(TargetOnOpenStart), m_hActivator, this, USE_TOGGLE, 0 );

	m_toggle_state = TS_GOING_UP;
	
	SetMoveDone( &CBaseDoor::DoorHitTop );
	if ( FClassnameIs(pev, "func_door_rotating"))		// !!! BUGBUG Triggered doors don't work with this yet
	{
		float	sign = 1.0;

		if ( m_hActivator != NULL )
		{
			pevActivator = m_hActivator->pev;
			
			if ( !FBitSet( pev->spawnflags, SF_DOOR_ONEWAY ) && pev->movedir.y ) 		// Y axis rotation, move away from the player
			{
				Vector vec = pevActivator->origin - pev->origin;
				Vector angles = pevActivator->angles;
				angles.x = 0;
				angles.z = 0;
				UTIL_MakeVectors (angles);
	//			Vector vnext = (pevToucher->origin + (pevToucher->velocity * 10)) - pev->origin;
				UTIL_MakeVectors ( pevActivator->angles );
				Vector vnext = (pevActivator->origin + (gpGlobals->v_forward * 10)) - pev->origin;
				if ( (vec.x*vnext.y - vec.y*vnext.x) < 0 )
					sign = -1.0;
			}
		}
		// Step4enko: Second speed.
		if ( FStringNull( m_iSecondSpeed ) || m_iSecondSpeed == 0 )
			AngularMove(m_vecAngle2*sign, pev->speed);
		else
			if ( m_fSecondSpeedDelay == 0 )
				AngularMove(m_vecAngle2*sign, m_iSecondSpeed);
			else
			{
				AngularMove(m_vecAngle2*sign, pev->speed);

				if ( m_fSecondSpeedDelay >= gpGlobals->time )
					pev->speed = m_iSecondSpeed;
			}
	}
	else
	{
		if ( FStringNull( m_iSecondSpeed ) || m_iSecondSpeed == 0 )
			LinearMove(m_vecPosition2, pev->speed);
		else
			if ( m_fSecondSpeedDelay == 0 )
				LinearMove(m_vecPosition2, m_iSecondSpeed);
			else
			{
				LinearMove(m_vecPosition2, m_iSecondSpeed);
			}
	}
}


//
// The door has reached the "up" position.  Either go back down, or wait for another activation.
//
void CBaseDoor::DoorHitTop( void )
{
	char* szSoundFile1 = (char*) STRING( m_iszSound1 );
	char* szSoundFile2 = (char*) STRING( m_iszSound2 );

	if ( !FBitSet( pev->spawnflags, SF_DOOR_SILENT ) )
	{
		if (FStringNull(m_iszSound1))
		    STOP_SOUND(ENT(pev), CHAN_STATIC, (char*)STRING(pev->noiseMoving) );
		else
			STOP_SOUND(ENT(pev), CHAN_STATIC, szSoundFile1 );

		if (FStringNull(m_iszSound2))
		    EMIT_SOUND(ENT(pev), CHAN_STATIC, (char*)STRING(pev->noiseArrived), 1, ATTN_NORM);
		else
		    EMIT_SOUND(ENT(pev), CHAN_STATIC, szSoundFile2, 1, ATTN_NORM);
	}

	ASSERT(m_toggle_state == TS_GOING_UP);
	m_toggle_state = TS_AT_TOP;
	
	// toggle-doors don't come down automatically, they wait for refire.
	if (FBitSet(pev->spawnflags, SF_DOOR_NO_AUTO_RETURN))
	{
		// Re-instate touch method, movement is complete
		if ( !FBitSet ( pev->spawnflags, SF_DOOR_USE_ONLY ) )
			SetTouch( &CBaseDoor::DoorTouch );
	}
	else
	{
		// In flWait seconds, DoorGoDown will fire, unless wait is -1, then door stays open
		pev->nextthink = pev->ltime + m_flWait;
		SetThink( &CBaseDoor::DoorGoDown );

		if ( m_flWait == -1 )
			pev->nextthink = -1;
	}

	// Fire the close target (if startopen is set, then "top" is closed) - netname is the close target
	if ( pev->netname && (pev->spawnflags & SF_DOOR_START_OPEN) )
		FireTargets( STRING(pev->netname), m_hActivator, this, USE_TOGGLE, 0 );

	SUB_UseTargets( m_hActivator, USE_TOGGLE, 0 ); // this isn't finished
}


//
// Starts the door going to its "down" position (simply ToggleData->vecPosition1).
//
void CBaseDoor::DoorGoDown( void )
{
	char* szSoundFile1 = (char*) STRING( m_iszSound1 );

	if ( !FBitSet( pev->spawnflags, SF_DOOR_SILENT ) )
	{
		if (FStringNull(m_iszSound1))
		{
		    if ( m_toggle_state != TS_GOING_UP && m_toggle_state != TS_GOING_DOWN )
			    EMIT_SOUND(ENT(pev), CHAN_STATIC, (char*)STRING(pev->noiseMoving), 1, ATTN_NORM);
		}
		else
		{
		    if ( m_toggle_state != TS_GOING_UP && m_toggle_state != TS_GOING_DOWN )
			    EMIT_SOUND(ENT(pev), CHAN_STATIC, szSoundFile1, 1, ATTN_NORM);
		}
	}
	
	// Step4enko
    if (!FStringNull(TargetOnCloseStart))
	    FireTargets( STRING(TargetOnCloseStart), m_hActivator, this, USE_TOGGLE, 0 );

#ifdef DOOR_ASSERT
	ASSERT(m_toggle_state == TS_AT_TOP);
#endif // DOOR_ASSERT
	m_toggle_state = TS_GOING_DOWN;

	SetMoveDone( &CBaseDoor::DoorHitBottom );
	if ( FClassnameIs(pev, "func_door_rotating"))//rotating door
		AngularMove( m_vecAngle1, pev->speed);
	else
		LinearMove( m_vecPosition1, pev->speed);
}

//
// The door has reached the "down" position.  Back to quiescence.
//
void CBaseDoor::DoorHitBottom( void )
{
	char* szSoundFile1 = (char*) STRING( m_iszSound1 );
	char* szSoundFile2 = (char*) STRING( m_iszSound2 );

	if ( !FBitSet( pev->spawnflags, SF_DOOR_SILENT ) )
	{
		if (FStringNull(m_iszSound1))
		    STOP_SOUND(ENT(pev), CHAN_STATIC, (char*)STRING(pev->noiseMoving) );
		else
			STOP_SOUND(ENT(pev), CHAN_STATIC, szSoundFile1 );

		if (FStringNull(m_iszSound2))
		    EMIT_SOUND(ENT(pev), CHAN_STATIC, (char*)STRING(pev->noiseArrived), 1, ATTN_NORM);
		else
			EMIT_SOUND(ENT(pev), CHAN_STATIC, szSoundFile2, 1, ATTN_NORM);
	}

	ASSERT(m_toggle_state == TS_GOING_DOWN);
	m_toggle_state = TS_AT_BOTTOM;

	// Re-instate touch method, cycle is complete
	if ( FBitSet ( pev->spawnflags, SF_DOOR_USE_ONLY ) )
	{
		// use only door
		SetTouch ( NULL );
	}
	else // touchable door
		SetTouch( &CBaseDoor::DoorTouch );

	SUB_UseTargets( m_hActivator, USE_TOGGLE, 0 ); // this isn't finished

	// Fire the close target (if startopen is set, then "top" is closed) - netname is the close target
	if ( pev->netname && !(pev->spawnflags & SF_DOOR_START_OPEN) )
		FireTargets( STRING(pev->netname), m_hActivator, this, USE_TOGGLE, 0 );
}

void CBaseDoor::Blocked( CBaseEntity *pOther )
{
	edict_t	*pentTarget = NULL;
	CBaseDoor	*pDoor		= NULL;


	// Hurt the blocker a little.
	if ( pev->dmg )
		pOther->TakeDamage( pev, pev, pev->dmg, DMG_CRUSH );

	// if a door has a negative wait, it would never come back if blocked,
	// so let it just squash the object to death real fast

	if (m_flWait >= 0)
	{
		if (m_toggle_state == TS_GOING_DOWN)
			DoorGoUp();
		else
			DoorGoDown();
	}

	// Block all door pieces with the same targetname here.
	if ( !FStringNull ( pev->targetname ) )
	{
		for (;;)
		{
			pentTarget = FIND_ENTITY_BY_TARGETNAME(pentTarget, STRING(pev->targetname));

			if ( VARS( pentTarget ) != pev )
			{
				if (FNullEnt(pentTarget))
					break;

				if ( FClassnameIs ( pentTarget, "func_door" ) || FClassnameIs ( pentTarget, "func_door_rotating" ) )
				{
				
					pDoor = GetClassPtr( (CBaseDoor *) VARS(pentTarget) );

					if ( pDoor->m_flWait >= 0)
					{
						if (pDoor->pev->velocity == pev->velocity && pDoor->pev->avelocity == pev->velocity)
						{
							// this is the most hacked, evil, bastardized thing I've ever seen. kjb
							if ( FClassnameIs ( pentTarget, "func_door" ) )
							{
								// set origin to realign normal doors
								pDoor->pev->origin = pev->origin;
								pDoor->pev->velocity = g_vecZero;// stop!
							}
							else
							{
								// set angles to realign rotating doors
								pDoor->pev->angles = pev->angles;
								pDoor->pev->avelocity = g_vecZero;
							}
						}

						if ( !FBitSet( pev->spawnflags, SF_DOOR_SILENT ) )
							STOP_SOUND(ENT(pev), CHAN_STATIC, (char*)STRING(pev->noiseMoving) );

						if ( pDoor->m_toggle_state == TS_GOING_DOWN)
							pDoor->DoorGoUp();
						else
							pDoor->DoorGoDown();
					}
				}
			}
		}
	}
}


/*QUAKED FuncRotDoorSpawn (0 .5 .8) ? START_OPEN REVERSE  
DOOR_DONT_LINK TOGGLE X_AXIS Y_AXIS
if two doors touch, they are assumed to be connected and operate as  
a unit.

TOGGLE causes the door to wait in both the start and end states for  
a trigger event.

START_OPEN causes the door to move to its destination when spawned,  
and operate in reverse.  It is used to temporarily or permanently  
close off an area when triggered (not usefull for touch or  
takedamage doors).

You need to have an origin brush as part of this entity.  The  
center of that brush will be
the point around which it is rotated. It will rotate around the Z  
axis by default.  You can
check either the X_AXIS or Y_AXIS box to change that.

"distance" is how many degrees the door will be rotated.
"speed" determines how fast the door moves; default value is 100.

REVERSE will cause the door to rotate in the opposite direction.

"angle"		determines the opening direction
"targetname" if set, no touch field will be spawned and a remote  
button or trigger field activates the door.
"health"	if set, door must be shot open
"speed"		movement speed (100 default)
"wait"		wait before returning (3 default, -1 = never return)
"dmg"		damage to inflict when blocked (2 default)
"sounds"
0)	no sound
1)	stone
2)	base
3)	stone chain
4)	screechy metal
*/
class CRotDoor : public CBaseDoor
{
public:
	void Spawn( void );
	virtual void SetToggleState( int state );
};

LINK_ENTITY_TO_CLASS( func_door_rotating, CRotDoor );


void CRotDoor::Spawn( void )
{
	Precache();
	// set the axis of rotation
	CBaseToggle::AxisDir( pev );

	// check for clockwise rotation
	if ( FBitSet (pev->spawnflags, SF_DOOR_ROTATE_BACKWARDS) )
		pev->movedir = pev->movedir * -1;
	
	//m_flWait			= 2; who the hell did this? (sjb)
	m_vecAngle1	= pev->angles;
	m_vecAngle2	= pev->angles + pev->movedir * m_flMoveDistance;

	ASSERTSZ(m_vecAngle1 != m_vecAngle2, "rotating door start/end positions are equal");
	
	if ( FBitSet (pev->spawnflags, SF_DOOR_PASSABLE) )
		pev->solid		= SOLID_NOT;
	else
		pev->solid		= SOLID_BSP;

	pev->takedamage = DAMAGE_YES;
	pev->movetype	= MOVETYPE_PUSH;
	UTIL_SetOrigin(pev, pev->origin);
	SET_MODEL(ENT(pev), STRING(pev->model) );

	if (pev->speed == 0)
		pev->speed = 100;
	
// DOOR_START_OPEN is to allow an entity to be lighted in the closed position
// but spawn in the open position
	if ( FBitSet (pev->spawnflags, SF_DOOR_START_OPEN) )
	{	// swap pos1 and pos2, put door at pos2, invert movement direction
		pev->angles = m_vecAngle2;
		Vector vecSav = m_vecAngle1;
		m_vecAngle2 = m_vecAngle1;
		m_vecAngle1 = vecSav;
		pev->movedir = pev->movedir * -1;
	}

	m_toggle_state = TS_AT_BOTTOM;

	if ( FBitSet ( pev->spawnflags, SF_DOOR_USE_ONLY ) )
	{
		SetTouch ( NULL );
	}
	else // touchable button
		SetTouch( &CRotDoor::DoorTouch );
}


void CRotDoor :: SetToggleState( int state )
{
	if ( state == TS_AT_TOP )
		pev->angles = m_vecAngle2;
	else
		pev->angles = m_vecAngle1;

	UTIL_SetOrigin( pev, pev->origin );
}


class CMomentaryDoor : public CBaseToggle
{
public:
	void	Spawn( void );
	void Precache( void );

	void	KeyValue( KeyValueData *pkvd );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int	ObjectCaps( void ) { return CBaseToggle :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	virtual int	Save( CSave &save );
	virtual int	Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void EXPORT DoorMoveDone( void );

	BYTE	m_bMoveSnd;			// sound a door makes while moving	
};

LINK_ENTITY_TO_CLASS( momentary_door, CMomentaryDoor );

TYPEDESCRIPTION	CMomentaryDoor::m_SaveData[] = 
{
	DEFINE_FIELD( CMomentaryDoor, m_bMoveSnd, FIELD_CHARACTER ),
};

IMPLEMENT_SAVERESTORE( CMomentaryDoor, CBaseToggle );

void CMomentaryDoor::Spawn( void )
{
	SetMovedir (pev);

	pev->solid		= SOLID_BSP;
	pev->movetype	= MOVETYPE_PUSH;

	UTIL_SetOrigin(pev, pev->origin);
	SET_MODEL( ENT(pev), STRING(pev->model) );
	
	if (pev->speed == 0)
		pev->speed = 100;
	if (pev->dmg == 0)
		pev->dmg = 2;
	
	m_vecPosition1	= pev->origin;
	// Subtract 2 from size because the engine expands bboxes by 1 in all directions making the size too big
	m_vecPosition2	= m_vecPosition1 + (pev->movedir * (fabs( pev->movedir.x * (pev->size.x-2) ) + fabs( pev->movedir.y * (pev->size.y-2) ) + fabs( pev->movedir.z * (pev->size.z-2) ) - m_flLip));
	ASSERTSZ(m_vecPosition1 != m_vecPosition2, "door start/end positions are equal");

	if ( FBitSet (pev->spawnflags, SF_DOOR_START_OPEN) )
	{	// swap pos1 and pos2, put door at pos2
		UTIL_SetOrigin(pev, m_vecPosition2);
		m_vecPosition2 = m_vecPosition1;
		m_vecPosition1 = pev->origin;
	}
	SetTouch( NULL );
	
	Precache();
}
	
void CMomentaryDoor::Precache( void )
{

// set the door's "in-motion" sound
	switch (m_bMoveSnd)
	{
	case	0:
		pev->noiseMoving = ALLOC_STRING("common/null.wav");
		break;
	case	1:
		PRECACHE_SOUND ("doors/doormove1.wav");
		pev->noiseMoving = ALLOC_STRING("doors/doormove1.wav");
		break;
	case	2:
		PRECACHE_SOUND ("doors/doormove2.wav");
		pev->noiseMoving = ALLOC_STRING("doors/doormove2.wav");
		break;
	case	3:
		PRECACHE_SOUND ("doors/doormove3.wav");
		pev->noiseMoving = ALLOC_STRING("doors/doormove3.wav");
		break;
	case	4:
		PRECACHE_SOUND ("doors/doormove4.wav");
		pev->noiseMoving = ALLOC_STRING("doors/doormove4.wav");
		break;
	case	5:
		PRECACHE_SOUND ("doors/doormove5.wav");
		pev->noiseMoving = ALLOC_STRING("doors/doormove5.wav");
		break;
	case	6:
		PRECACHE_SOUND ("doors/doormove6.wav");
		pev->noiseMoving = ALLOC_STRING("doors/doormove6.wav");
		break;
	case	7:
		PRECACHE_SOUND ("doors/doormove7.wav");
		pev->noiseMoving = ALLOC_STRING("doors/doormove7.wav");
		break;
	case	8:
		PRECACHE_SOUND ("doors/doormove8.wav");
		pev->noiseMoving = ALLOC_STRING("doors/doormove8.wav");
		break;
	default:
		pev->noiseMoving = ALLOC_STRING("common/null.wav");
		break;
	}
}

void CMomentaryDoor::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "movesnd"))
	{
		m_bMoveSnd = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "stopsnd"))
	{
//		m_bStopSnd = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "healthvalue"))
	{
//		m_bHealthValue = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}

void CMomentaryDoor::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( useType != USE_SET )		// Momentary buttons will pass down a float in here
		return;

	if ( value > 1.0 )
		value = 1.0;
	if ( value < 0.0 )
		value = 0.0;

	Vector move = m_vecPosition1 + (value * (m_vecPosition2 - m_vecPosition1));
	
	Vector delta = move - pev->origin;
	//float speed = delta.Length() * 10;
	float speed = delta.Length() / 0.1; // move there in 0.1 sec
	if ( speed == 0 )
		return;

	// This entity only thinks when it moves, so if it's thinking, it's in the process of moving
	// play the sound when it starts moving (not yet thinking)
	if ( pev->nextthink < pev->ltime || pev->nextthink == 0 )
		EMIT_SOUND(ENT(pev), CHAN_STATIC, (char*)STRING(pev->noiseMoving), 1, ATTN_NORM);
	// If we already moving to designated point, return
	else if (move == m_vecFinalDest)
		return;
	
	SetMoveDone( &CMomentaryDoor::DoorMoveDone );
	LinearMove( move, speed );
}

//
// The door has reached needed position.
//
void CMomentaryDoor::DoorMoveDone( void )
{
	STOP_SOUND(ENT(pev), CHAN_STATIC, (char*)STRING(pev->noiseMoving) );
	EMIT_SOUND(ENT(pev), CHAN_STATIC, (char*)STRING(pev->noiseArrived), 1, ATTN_NORM);
}