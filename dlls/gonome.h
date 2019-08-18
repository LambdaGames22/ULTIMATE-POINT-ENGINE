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

#ifndef GONOME_H
#define GONOME_H

//=========================================================
// CGonome
//=========================================================
class CGonome : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  ISoundMask( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void IdleSound( void );
	void PainSound( void );
	void DeathSound( void );
	void AlertSound ( void );
	void AttackSound( void );
	void StartTask ( Task_t *pTask );
	void RunTask ( Task_t *pTask );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	void RunAI( void );
	Schedule_t *GetSchedule( void );
	Schedule_t *GetScheduleOfType ( int Type );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	int IRelationship ( CBaseEntity *pTarget );
	int IgnoreConditions ( void );
	MONSTERSTATE GetIdealState ( void );

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	BOOL m_fCanThreatDisplay;

	float m_flLastHurtTime;
	float m_flNextSpitTime;
	float m_flNextFlinch;

	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];
};

#endif // GONOME_H