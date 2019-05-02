/***
*
*	Copyright (c) 2016-2019, EETeam. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This code is made by EETeam (Half-Life: Ultimate Point developers)
*   Coders: Step4enko / LambdaGames22
*
*   https://www.moddb.com/mods/half-life-ultimate-point
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "customentity.h"
#include "effects.h"
#include "weapons.h"
#include "decals.h"
#include "func_break.h"
#include "shake.h"
#include "player.h"

//=========================================================
// Generic Item
//=========================================================
class CItemGeneric : public CBaseAnimating
{
public:
	void	Spawn( void );
	void	Precache( void );
	void	KeyValue( KeyValueData *pkvd );

	virtual int		ObjectCaps( void ) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	static	TYPEDESCRIPTION m_SaveData[];
};

LINK_ENTITY_TO_CLASS( item_generic, CItemGeneric );


void CItemGeneric :: KeyValue( KeyValueData *pkvd )
{
	CBaseAnimating::KeyValue( pkvd );
}

void CItemGeneric :: Precache ( void )
{
	PRECACHE_MODEL( (char *)STRING(pev->model) );
}

void CItemGeneric::Spawn( void )
{
	if ( pev->targetname )
	{
		Precache();
		SET_MODEL( ENT(pev), STRING(pev->model));
	}
	else
	{
		UTIL_Remove(this);
	}

	pev->movetype = MOVETYPE_NONE;

	pev->framerate = 1.0;
}