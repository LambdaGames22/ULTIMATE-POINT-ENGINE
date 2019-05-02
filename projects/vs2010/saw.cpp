/***
*
*	Copyright (c) 2016-2020, UPTeam. All rights reserved.
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
/*
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"

enum saw_e 
{
	SAW_SLOWIDLE = 0,
	SAW_IDLE2,
	SAW_RELOAD_START,
	SAW_RELOAD_END,
	SAW_HOLSTER,
	SAW_DRAW,
	SAW_SHOOT1,
	SAW_SHOOT2,
	SAW_SHOOT3
};

TYPEDESCRIPTION	CSaw::m_SaveData[] = 
{
	DEFINE_FIELD( CSaw, m_flReloadEnd, FIELD_FLOAT ),
	DEFINE_FIELD( CSaw, m_bReloading, FIELD_BOOLEAN ),
	DEFINE_FIELD( CSaw, m_iFire, FIELD_INTEGER ),
	DEFINE_FIELD( CSaw, m_iSmoke, FIELD_INTEGER ),
	DEFINE_FIELD( CSaw, m_iLink, FIELD_INTEGER ),
	DEFINE_FIELD( CSaw, m_iShell, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CSaw, CBasePlayerWeapon );

LINK_ENTITY_TO_CLASS( weapon_saw, CSaw );
LINK_ENTITY_TO_CLASS( weapon_m249, CSaw );

int CSaw::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

void CSaw::Spawn()
{
	pev->classname = MAKE_STRING("weapon_saw"); // hack to allow for old names
	Precache();
	m_iId = WEAPON_SAW;
	SET_MODEL(ENT(pev), "models/w_saw.mdl" );

	m_iDefaultAmmo = SAW_DEFAULT_GIVE;

	m_bAlternatingEject = false;

	FallInit(); // get ready to fall down.
}

void CSaw::Precache()
{
	PRECACHE_MODEL( "models/v_saw.mdl" );
	PRECACHE_MODEL( "models/w_saw.mdl" );
	PRECACHE_MODEL( "models/p_saw.mdl" );

	m_iShell = PRECACHE_MODEL( "models/saw_shell.mdl" );
	m_iLink = PRECACHE_MODEL( "models/saw_link.mdl" );
	m_iSmoke = PRECACHE_MODEL( "sprites/wep_smoke_01.spr" );
	m_iFire = PRECACHE_MODEL( "sprites/xfire.spr" );

	PRECACHE_SOUND( "weapons/saw_reload.wav" );
	PRECACHE_SOUND( "weapons/saw_reload2.wav" );
	PRECACHE_SOUND( "weapons/saw_fire1.wav" );

	m_usFireM249 = PRECACHE_EVENT( 1, "events/m249.sc" );
}

bool CSaw::AddDuplicate( CBasePlayerWeapon* pOriginal )
{
// UNDONE CLIP
}

BOOL CSaw::Deploy()
{
	return DefaultDeploy( "models/v_saw.mdl", "models/p_saw.mdl", SAW_DRAW, "mp5" );
}

void CSaw::Holster()
{
	SetThink( nullptr );

	SendWeaponAnim( SAW_HOLSTER );

	m_bReloading = false;

	m_fInReload = false;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10.0, 15.0 );
}

void CSaw::WeaponIdle()
{
	ResetEmptySound();

	if( m_bReloading && UTIL_WeaponTimeBase() >= m_flReloadEnd )
	{
		m_bReloading = false;

		pev->body = 0;

		SendWeaponAnim( SAW_RELOAD_END, 0 );
	}

	if( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		const float flNextIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0, 1.0 );

		int iAnim;

		if( flNextIdle <= 0.95 )
		{
			iAnim = SAW_SLOWIDLE;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0;
		}
		else
		{
			iAnim = SAW_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 6.16;
		}

		SendWeaponAnim( iAnim, 0 );
	}
}

void CSaw::PrimaryAttack()
{
	if ( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound();

		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
		return;
	}

	if( m_iClip <= 0 )
	{
		if( !m_fInReload )
		{
			PlayEmptySound();

			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
		}

		return;
	}

	--m_iClip;

	pev->body = ( RecalculateBody( m_iClip  ) );

	m_bAlternatingEject = !m_bAlternatingEject;

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	m_flNextAnimTime = UTIL_WeaponTimeBase() + 0.2;

	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );

	Vector vecSrc = m_pPlayer->GetGunPosition();

	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	Vector vecSpread;

	if( bIsMultiplayer() )
	{
		if( m_pPlayer->GetButtons().Any( IN_DUCK ) )
		{
			vecSpread = VECTOR_CONE_3DEGREES;
		}
		else if( m_pPlayer->GetButtons().Any( IN_MOVERIGHT |
										   IN_MOVELEFT | 
										   IN_FORWARD | 
										   IN_BACK ) )
		{
			vecSpread = VECTOR_CONE_15DEGREES;
		}
		else
		{
			vecSpread = VECTOR_CONE_6DEGREES;
		}
	}
	else
	{
		if( m_pPlayer->GetButtons().Any( IN_DUCK ) )
		{
			vecSpread = VECTOR_CONE_4DEGREES;
		}
		else if( m_pPlayer->GetButtons().Any( IN_MOVERIGHT |
										   IN_MOVELEFT |
										   IN_FORWARD |
										   IN_BACK ) )
		{
			vecSpread = VECTOR_CONE_10DEGREES;
		}
		else
		{
			vecSpread = VECTOR_CONE_2DEGREES;
		}
	}

	Vector vecDir = m_pPlayer->FireBulletsPlayer( 
		1, 
		vecSrc, vecAiming, vecSpread, 
		8192.0, BULLET_PLAYER_556, 2, 0, 
		m_pPlayer, m_pPlayer->random_seed );

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( 
		flags, m_pPlayer->edict(), m_usFireM249, 0, 
		g_vecZero, g_vecZero, 
		vecDir.x, vecDir.y, 
		GetBody(), 0, 
		m_bAlternatingEject ? 1 : 0, 0 );

	if( !m_iClip )
	{
		if( m_pPlayer->m_rgAmmo[ PrimaryAmmoIndex() ] <= 0 )
		{
			m_pPlayer->SetSuitUpdate( "!HEV_AMO0", SUIT_SENTENCE, SUIT_REPEAT_OK );
		}
	}

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.067;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.2;

#ifndef CLIENT_DLL
	Vector vecPunchAngle = m_pPlayer->GetPunchAngle();

	vecPunchAngle.x = UTIL_RandomFloat( -2, 2 );

	vecPunchAngle.y = UTIL_RandomFloat( -1, 1 );

	m_pPlayer->SetPunchAngle( vecPunchAngle );

	UTIL_MakeVectors( m_pPlayer->GetViewAngle() + m_pPlayer->GetPunchAngle() );

	const Vector& vecVelocity = m_pPlayer->GetAbsVelocity();

	const float flZVel = vecVelocity.z;

	//TODO: magic number - Solokiller
	Vector vecInvPushDir = gpGlobals->v_forward * 35.0;

	float flNewZVel = CVAR_GET_FLOAT( "sv_maxspeed" );

	if( vecInvPushDir.z >= 10.0 )
		flNewZVel = vecInvPushDir.z;

	if( !g_pGameRules->IsDeathmatch() )
	{
		Vector vecNewVel = vecVelocity - vecInvPushDir;

		//Restore Z velocity to make deathmatch easier.
		vecNewVel.z = flZVel;

		//m_pPlayer->SetAbsVelocity( vecNewVel );
	}
	else
	{
		Vector vecNewVel = vecVelocity;

		const float flZTreshold = -( flNewZVel + 100.0 );

		if( vecVelocity.x > flZTreshold )
		{
			vecNewVel.x -= vecInvPushDir.x;
		}

		if( vecVelocity.y > flZTreshold )
		{
			vecNewVel.y -= vecInvPushDir.y;
		}

		vecNewVel.z -= vecInvPushDir.z;
	}
#endif
}

void CM249::Reload()
{
	if( DefaultReload( M249_RELOAD_START, 1.0, GetBody() ) )
	{
		m_bReloading = true;

		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 3.78;

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.78;

		m_flReloadEnd = UTIL_WeaponTimeBase() + 1.33;
	}
}

void CM249::GetWeaponData( weapon_data_t& data )
{
	BaseClass::GetWeaponData( data );

	//Needed because the client may not get a chance to process the reload end if ping is low enough. - Solokiller
	data.iuser2 = GetBody();
	data.fuser2 = m_flReloadEnd;
}

void CM249::SetWeaponData( const weapon_data_t& data )
{
	BaseClass::SetWeaponData( data );

	SetBody( data.iuser2 );
	m_flReloadEnd = data.fuser2;
}

void CM249::DecrementTimers( float flTime )
{
	BaseClass::DecrementTimers( flTime );

	m_flReloadEnd = max( m_flReloadEnd - flTime, -0.001f );
}

int CM249::RecalculateBody( int iClip )
{
	if( iClip == 0 )
	{
		return 8;
	}
	else if( iClip >= 0 && iClip <= 7 )
	{
		return 9 - iClip;
	}
	else
	{
		return 0;
	}
}*/