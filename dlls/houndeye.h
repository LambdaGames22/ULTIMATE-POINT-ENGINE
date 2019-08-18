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

#ifndef HOUNDEYE_H
#define HOUNDEYE_H

//=========================================================
// Houndeye 
//=========================================================
class CHoundeye : public CSquadMonster
{
public:
	void Spawn( void );
	void Precache( void );
	int  Classify ( void );
	int  ISoundMask( void );
	int  IgnoreConditions ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void SetYawSpeed ( void );
	void WarmUpSound ( void );
	void AlertSound( void );
	void DeathSound( void );
	void WarnSound( void );
	void PainSound( void );
	void IdleSound( void );
	void StartTask( Task_t *pTask );
	void RunTask ( Task_t *pTask );
	void SonicAttack( void );
	void PrescheduleThink( void );
	void SetActivity ( Activity NewActivity );
	void WriteBeamColor ( void );

	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL FValidateHintType ( short sHint );
	BOOL FCanActiveIdle ( void );
	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *CHoundeye :: GetSchedule( void );

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	int 	m_iSpriteTexture;
	int		m_iMode; // SAVE RESTORE NEED?

	BOOL m_fAsleep;// some houndeyes sleep in idle mode if this is set, the houndeye is lying down
	BOOL m_fDontBlink;// don't try to open/close eye if this bit is set!
	Vector	m_vecPackCenter; // the center of the pack. The leader maintains this by averaging the origins of all pack members.
};

#endif // HOUNDEYE_H