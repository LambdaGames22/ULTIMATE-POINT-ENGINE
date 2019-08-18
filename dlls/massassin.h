/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/

#ifndef MASSASSIN_H
#define MASSASSIN_H

//=========================================================
// Male Assassin
//=========================================================
class CMassn : public CHGrunt 
{
public:
	int  Classify();
	void KeyValue( KeyValueData *pkvd );
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	void Sniperrifle();

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	BOOL FOkToSpeak();

	void Spawn();
	void Precache();

	void IdleSound();

private:
	int 	m_iHead;
};

#endif // MASSASSIN_H