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

#ifndef BARNEY_H
#define BARNEY_H

#include "talkmonster.h"

//=========================================================
// Barney
//=========================================================
class CBarney : public CTalkMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  ISoundMask( void );
	void KeyValue( KeyValueData *pkvd );
	void BarneyFirePistol( void );
	void BarneyFirePython( void );
	void BarneyFireDeagle( void );
	void AlertSound( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	
	void SetActivity ( Activity NewActivity );

	void RunTask( Task_t *pTask );
	void StartTask( Task_t *pTask );
	virtual int	ObjectCaps( void ) { return CTalkMonster :: ObjectCaps() | FCAP_IMPULSE_USE; }
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	BOOL CheckRangeAttack1 ( float flDot, float flDist );

    //BOOL ItemTouch( CBaseEntity *pOther ); // UNDONE
	//BOOL WeaponTouch( CBaseEntity *pOther ); // UNDONE
	
	void DeclineFollowing( void );

	// Override these to set behavior
	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );
	MONSTERSTATE GetIdealState ( void );

	void DeathSound( void );
	void PainSound( void );
	
	void TalkInit( void );

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void Killed( entvars_t *pevAttacker, int iGib );
	
	CBaseEntity	*Kick( void );
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	BOOL	m_fGunDrawn;
	float	m_painTime;
	float	m_checkAttackTime;
	BOOL	m_lastAttackCheck;

	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];

	// UNDONE: What is this for?  It isn't used?
	float	m_flPlayerDamage;// how much pain has the player inflicted on me?
	float   m_flNextHolsterTime; // Step4enko
	int     m_iClipSize; // Step4enko
	int     m_iWeapon; // Step4enko
	int     m_iGrenades; // Step4enko

	CUSTOM_SCHEDULES;

	int		m_iBrassShell; // Step4enko
protected:
	int		m_iHead; // Step4enko
};

//=========================================================
// Dead Barney
//=========================================================
class CDeadBarney : public CBaseMonster
{
public:
	void Spawn( void );
	int	Classify ( void ) { return	CLASS_PLAYER_ALLY; }

	void KeyValue( KeyValueData *pkvd );

	int	m_iPose;// which sequence to display	-- temporary, don't need to save
	static char *m_szPoses[3];
};

#endif // BARNEY_H