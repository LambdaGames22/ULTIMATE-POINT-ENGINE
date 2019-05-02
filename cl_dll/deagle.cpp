/***
*
*	Copyright (c) 2016-2020, EETeam. All rights reserved.
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
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"
#include "laserspot.h" // Step4enko

enum deagle_e 
{
	DEAGLE_IDLE1 = 0,
	DEAGLE_IDLE2,
	DEAGLE_IDLE3,
	DEAGLE_IDLE4,
	DEAGLE_IDLE5,
	DEAGLE_SHOOT,
	DEAGLE_SHOOT_EMPTY,
	DEAGLE_RELOAD,
	DEAGLE_RELOAD_NOSHOOT,
	DEAGLE_DRAW,
	DEAGLE_HOLSTER,
	DEAGLE_LASER_SWITCH,
	DEAGLE_START_RUN,
	DEAGLE_RUN,
	DEAGLE_STOP_RUN,
	DEAGLE_FIRST_DRAW
};

LINK_ENTITY_TO_CLASS( weapon_deagle, CDeagle );

int CDeagle::AddToPlayer( CBasePlayer *pPlayer )
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

void CDeagle::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_deagle"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_DEAGLE;
	SET_MODEL(ENT(pev), "models/w_desert_eagle.mdl");
	m_fSpotActive = 0;

	m_iDefaultAmmo = DEAGLE_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

void CDeagle::Precache( void )
{
	PRECACHE_MODEL("models/v_desert_eagle.mdl");
	PRECACHE_MODEL("models/w_desert_eagle.mdl");
	PRECACHE_MODEL("models/p_desert_eagle.mdl");

	PRECACHE_SOUND("weapons/desert_eagle_fire.wav");
	PRECACHE_SOUND("weapons/desert_eagle_fire_empty.wav");

	PRECACHE_SOUND("weapons/desert_eagle_reload1.wav");
	PRECACHE_SOUND("weapons/desert_eagle_reload2.wav");
	PRECACHE_SOUND("weapons/desert_eagle_reload3.wav");

	PRECACHE_SOUND("weapons/desert_eagle_draw.wav");

	PRECACHE_SOUND("weapons/spot_on.wav");
	PRECACHE_SOUND("weapons/spot_off.wav");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl"); // Brass shell

	m_usFireDeagle = PRECACHE_EVENT( 1, "events/eagle.sc" );

	UTIL_PrecacheOther( "laser_spot" );
}

int CDeagle::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "357";
	p->iMaxAmmo1 = _357_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = DEAGLE_MAX_CLIP;
	p->iFlags = 0;
	p->iSlot = 1;
	p->iPosition = 2;
	p->iId = m_iId = WEAPON_DEAGLE;
	p->iWeight = DEAGLE_WEIGHT;

	return 1;
}

BOOL CDeagle::Deploy( )
{
	if ( m_fInFirstDraw = TRUE )
	{
	    return DefaultDeploy( "models/v_desert_eagle.mdl", "models/p_desert_eagle.mdl", DEAGLE_FIRST_DRAW, "onehanded", 3.1875  );
		m_fInFirstDraw = FALSE;
		UpdateSpot( );
	}

	if ( m_fInFirstDraw = FALSE )
	{
	    return DefaultDeploy( "models/v_desert_eagle.mdl", "models/p_desert_eagle.mdl", DEAGLE_DRAW, "onehanded", 0.5  );
		UpdateSpot( );
	}
}

void CDeagle::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE; // Cancel any reload in progress.

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	SendWeaponAnim( DEAGLE_HOLSTER );

	if (m_pSpot)
	{
		m_pSpot->Killed( NULL, GIB_NEVER );
		m_pSpot = NULL;
	}
}

void CDeagle::PrimaryAttack()
{
	if(! ( m_pPlayer->m_afButtonPressed & IN_ATTACK ) )
    return;

	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		if ( m_pSpot && m_fSpotActive )	m_pSpot->Suspend( 1.0 );
		PlayEmptySound( );
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		if (!m_fFireOnEmpty)
			Reload( );
		else
		{
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/357_cock1.wav", 0.8, ATTN_NORM);
			m_flNextPrimaryAttack = 0.15;
		}

		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	// Step4enko
    Vector vecSrc2 = pev->origin + gpGlobals->v_forward * 2;
    MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc2 );
            WRITE_BYTE(TE_DLIGHT);
            WRITE_COORD(vecSrc2.x);	// X
            WRITE_COORD(vecSrc2.y);	// Y
            WRITE_COORD(vecSrc2.z);	// Z
            WRITE_BYTE( RANDOM_LONG(15,20) );		// radius * 0.1 used to be WRITE_BYTE 12
            WRITE_BYTE( RANDOM_LONG(240,255) );		// r
            WRITE_BYTE( RANDOM_LONG(165,185) );		// g
            WRITE_BYTE( RANDOM_LONG(90, 102) );		// b
            //WRITE_BYTE( RANDOM_LONG(1,10) / pev->framerate );		// time * 10 WAS 20
			WRITE_BYTE( 10 );		// time * 10 WAS 20
            WRITE_BYTE( RANDOM_LONG( 180, 195) );		// decay * 0.1
    MESSAGE_END( );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	Vector vecDir;
	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_4DEGREES, 8192, BULLET_PLAYER_357, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );

    int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFireDeagle, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = 0.17; // 0.75
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );

	// Step4enko
	UpdateSpot( );
}

void CDeagle::SecondaryAttack( void )
{
	SendWeaponAnim( DEAGLE_LASER_SWITCH );

	if( m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0 )
	    {
	    if (m_fSpotActive)
	    {
	        EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/spot_off.wav", 1, ATTN_NORM);
	    }

	    if (!m_fSpotActive)
	    {
	        EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/spot_on.wav", 1, ATTN_NORM);
	    }
	}

	m_fSpotActive = ! m_fSpotActive;

#ifndef CLIENT_DLL
	if (!m_fSpotActive && m_pSpot)
	{
		m_pSpot->Killed( NULL, GIB_NORMAL );
		m_pSpot = NULL;
	}
#endif

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0;
}

void CDeagle::Reload( void )
{
	// Step4enko
	//UpdateSpot( );

	if ( m_pPlayer->ammo_9mm <= 0 )
		 return;

	int iResult;

	if (m_iClip == 0)
		iResult = DefaultReload( 7, DEAGLE_RELOAD, 2.71875 );
	else
		iResult = DefaultReload( 7, DEAGLE_RELOAD_NOSHOOT, 2.0625 );

	if (iResult)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	}

	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] == 0) return;
	if ( m_pSpot && m_fSpotActive ) m_pSpot->Suspend( 2.0 );
/*
	if ( m_fSpotActive )
	{
		// no reloading when there are active missiles tracking the designator.
		// ward off future autoreload attempts by setting next attack time into the future for a bit. 
		return;
	}

#ifndef CLIENT_DLL
	if ( m_pSpot && m_fSpotActive )
	{
		m_pSpot->Suspend( 2.1 );
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 2.1;
	}
#endif*/

	//SetThink( &CGlock::DropClip );
}

void CDeagle::WeaponIdle( void )
{
	UpdateSpot( );

	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	// only idle if the slid isn't back
	if (m_iClip != 0)
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0, 1.0 );

		if (flRand <= 0.3 + 0 * 0.75)
		{
			iAnim = DEAGLE_IDLE1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 61.0 / 30;
		}
		else if (flRand <= 0.6 + 0 * 0.875)
		{
			iAnim = DEAGLE_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 61.0 / 24.0;
		}
		else
		{
			switch (RANDOM_LONG(0,2))
			{
			    case 0 : 
			    {
			        iAnim = DEAGLE_IDLE3; 
			        m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 50.0 / 30.0;
					break;
			    } 
			
			    case 1 : 
			    {
			        iAnim = DEAGLE_IDLE4; 
			        m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 61.0 / 30.0;
					break;
			    }

			    case 2 : 
			    {
			        iAnim = DEAGLE_IDLE5; 
			        m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 61.0 / 30.0;
					break;
			    }
			}
		}
		SendWeaponAnim( iAnim, 1 );
	}
}

void CDeagle::UpdateSpot( void )
{
#ifndef CLIENT_DLL
	if (m_fSpotActive)
	{
		if (!m_pSpot)
		{
			m_pSpot = CLaserSpot::CreateSpot();
		}

		UTIL_MakeVectors( m_pPlayer->pev->v_angle );
		Vector vecSrc = m_pPlayer->GetGunPosition( );;
		Vector vecAiming = gpGlobals->v_forward;

		TraceResult tr;
		UTIL_TraceLine ( vecSrc, vecSrc + vecAiming * 8192, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr );
		
		UTIL_SetOrigin( m_pSpot->pev, tr.vecEndPos );

		// Step4enko
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, tr.vecEndPos );
			WRITE_BYTE(TE_DLIGHT);
			WRITE_COORD(tr.vecEndPos.x);	// X
			WRITE_COORD(tr.vecEndPos.y);	// Y
			WRITE_COORD(tr.vecEndPos.z);	// Z
			WRITE_BYTE( 4.5 );		// radius * 0.1 used to be WRITE_BYTE 12
			WRITE_BYTE( 220 );		// r
			WRITE_BYTE( 11 );		// g
			WRITE_BYTE( 11 );		// b
			WRITE_BYTE( 1 );		// time * 10 WAS 20
			WRITE_BYTE( RANDOM_LONG( 180, 195) );		// decay * 0.1
		MESSAGE_END( );
	}
#endif
}

//=========================================================
// Create Spot
//=========================================================
CLaserSpot *CLaserSpot::CreateSpot( void )
{
	CLaserSpot *pSpot = GetClassPtr( (CLaserSpot *)NULL );
	pSpot->Spawn();

	pSpot->pev->classname = MAKE_STRING("laser_spot");

	return pSpot;
}

//=========================================================
// Spawn
//=========================================================
void CLaserSpot::Spawn( void )
{
	Precache( );
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;

	pev->rendermode = kRenderGlow;
	pev->renderfx = kRenderFxNoDissipation;
	pev->renderamt = 255;

	SET_MODEL(ENT(pev), "sprites/laserdot.spr");
	UTIL_SetOrigin( pev, pev->origin );
};

//=========================================================
// Suspend- make the laser sight invisible. 
//=========================================================
void CLaserSpot::Suspend( float flSuspendTime )
{
	pev->effects |= EF_NODRAW;
	
	SetThink( &CLaserSpot::Revive );
	pev->nextthink = gpGlobals->time + flSuspendTime;
}

//=========================================================
// Revive - bring a suspended laser sight back.
//=========================================================
void CLaserSpot::Revive( void )
{
	pev->effects &= ~EF_NODRAW;

	SetThink( NULL );
}

void CLaserSpot::Precache( void )
{
	PRECACHE_MODEL("sprites/laserdot.spr");
};