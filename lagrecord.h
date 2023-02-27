#pragma once

// pre-declare.
class LagRecord;

class BackupRecord {
public:
	BoneArray* m_bones;
	int        m_bone_count;
	vec3_t     m_origin, m_abs_origin;
	vec3_t     m_mins;
	vec3_t     m_maxs;
	ang_t      m_abs_ang;

public:
	__forceinline void store( Player* player ) {
		// get bone cache ptr.
		CBoneCache* cache = &player->m_BoneCache( );

		// store bone data.
		m_bones      = cache->m_pCachedBones;
		m_bone_count = cache->m_CachedBoneCount;
		m_origin     = player->m_vecOrigin( );
		m_mins       = player->m_vecMins( );
		m_maxs       = player->m_vecMaxs( );
		m_abs_origin = player->GetAbsOrigin( );
		m_abs_ang    = player->GetAbsAngles( );
	}

	__forceinline void restore( Player* player ) {
		// get bone cache ptr.
		CBoneCache* cache = &player->m_BoneCache( );

		cache->m_pCachedBones    = m_bones;
		cache->m_CachedBoneCount = m_bone_count;

		player->m_vecOrigin( ) = m_origin;
		player->m_vecMins( )   = m_mins;
		player->m_vecMaxs( )   = m_maxs;
		player->SetAbsAngles( m_abs_ang );
		player->SetAbsOrigin( m_origin );
	}
};

class LagRecord {
public:
	// data.
	Player* m_player;
	float   m_immune;
	int     m_tick;
	int     m_lag;
	bool    m_dormant;

	// netvars.
	float  m_sim_time;
	float  m_old_sim_time;
	float m_lag_time;
	int    m_flags;
	vec3_t m_origin;
	vec3_t m_old_origin;
	vec3_t m_velocity;
	int    m_shift;
	vec3_t m_mins;
	vec3_t m_maxs;
	ang_t  m_eye_angles;
	ang_t  m_abs_ang;
	float  m_body;
	float  m_duck;

	// anim stuff.
	C_AnimationLayer m_layers[ 13 ];
	float            m_poses[ 24 ];
	vec3_t           m_anim_velocity;

	// bone stuff.
	bool       m_setup;
	BoneArray* m_bones;

	// lagfix stuff.
	bool m_iDistorting[65];
	int m_iDistortCheck;
	bool m_iRestartDistortCheck;
	bool m_iFirstCheck;
	float m_iDistortTiming;
	bool   m_broke_lc;
	vec3_t m_pred_origin;
	vec3_t m_pred_velocity;
	float  m_pred_time;
	int    m_pred_flags;
	bool m_fake_flick;
	bool m_invalid;


	// resolver stuff.
	size_t m_mode;
	bool   m_fake_walk;
	bool   m_shot;
	float  m_away;
	float  m_anim_time;

	// other stuff.
	float  m_interp_time;
public:

	// default ctor.
	__forceinline LagRecord( ) : 
		m_setup{ false }, 
		m_broke_lc{ false },
		m_fake_walk{ false }, 
		m_shot{ false }, 
		m_lag{}, 
		m_bones{} {}

	// ctor.
	__forceinline LagRecord( Player* player ) : 
		m_setup{ false }, 
		m_broke_lc{ false },
		m_fake_walk{ false },
		m_shot{ false }, 
		m_lag{}, 
		m_bones{} {

		store( player );
	}

	// dtor.
	__forceinline ~LagRecord( ){
		// free heap allocated game mem.
		g_csgo.m_mem_alloc->Free( m_bones );
	}

	__forceinline void invalidate( ) {
		// free heap allocated game mem.
		g_csgo.m_mem_alloc->Free( m_bones );

		// mark as not setup.
		m_setup = false;

		// allocate new memory.
		m_bones = ( BoneArray* )g_csgo.m_mem_alloc->Alloc( sizeof( BoneArray ) * 128 );
	}

	__forceinline bool broke_lc( ) {
		return ( this->m_origin - this->m_old_origin ).length_2d_sqr( ) > 4096.f;
	}

	// function: allocates memory for SetupBones and stores relevant data.
	void store( Player* player ) {
		// allocate game heap.
		m_bones = ( BoneArray* )g_csgo.m_mem_alloc->Alloc( sizeof( BoneArray ) * 128 );

		// player data.
		m_player    = player;
		m_immune    = player->m_fImmuneToGunGameDamageTime( );
		m_tick      = g_csgo.m_cl->m_server_tick;
	
		// netvars.
		m_pred_time     = m_sim_time = player->m_flSimulationTime( );
		m_old_sim_time  = player->m_flOldSimulationTime( );
		m_pred_flags    = m_flags  = player->m_fFlags( );
		m_pred_origin   = m_origin = player->m_vecOrigin( );
		m_old_origin    = player->m_vecOldOrigin( );
		m_eye_angles    = player->m_angEyeAngles( );
		m_abs_ang       = player->GetAbsAngles( );
		m_body          = player->m_flLowerBodyYawTarget( );
		m_mins          = player->m_vecMins( );
		m_maxs          = player->m_vecMaxs( );
		m_duck          = player->m_flDuckAmount( );
		m_pred_velocity = m_velocity = player->m_vecVelocity( );

		// save networked animlayers.
		player->GetAnimLayers( m_layers );

		// normalize eye angles.
		m_eye_angles.normalize( );
		math::clamp( m_eye_angles.x, -90.f, 90.f );

		// get lag.
		m_lag = game::TIME_TO_TICKS( m_sim_time - m_old_sim_time );

		// compute animtime.
		m_anim_time = m_old_sim_time + g_csgo.m_globals->m_interval;
	}

	// function: restores 'predicted' variables to their original.
	__forceinline void predict( ) {
		m_broke_lc      = broke_lc( );
		m_pred_origin   = m_origin;
		m_pred_velocity = m_velocity;
		m_pred_time     = m_sim_time;
		m_pred_flags    = m_flags;
	}

	// function: writes current record to bone cache.
	__forceinline void cache( ) {
		// get bone cache ptr.
		CBoneCache* cache = &m_player->m_BoneCache( );

		cache->m_pCachedBones    = m_bones;
		cache->m_CachedBoneCount = 128;

		m_player->m_vecOrigin( ) = m_pred_origin;
		m_player->m_vecMins( )   = m_mins;
		m_player->m_vecMaxs( )   = m_maxs;

		m_player->SetAbsAngles( m_abs_ang );
		m_player->SetAbsOrigin( m_pred_origin );
	}

	__forceinline bool dormant( ) {
		return m_dormant;
	}

	__forceinline bool immune( ) {
		return m_immune > 0.f;
	}

	// function: checks if LagRecord obj is hittable if we were to fire at it now.
	bool valid() {
		if (!this)
			return false;

		INetChannel* info = g_csgo.m_engine->GetNetChannelInfo();
		if (!info || !g_cl.m_local || dormant())
			return false;

		// get correct based on out latency + in latency + lerp time and clamp on sv_maxunlag
		float correct = 0;

		// add out latency
		correct += info->GetLatency(INetChannel::FLOW_OUTGOING);

		// add in latency
		correct += info->GetLatency(INetChannel::FLOW_INCOMING);

		// add interpolation amount
		correct += g_cl.m_lerp;

		// clamp this shit
		correct = std::clamp(correct, 0.f, g_csgo.sv_maxunlag->GetFloat());

		// def cur time
		float curtime = g_cl.m_local->alive() ? game::TICKS_TO_TIME(g_cl.m_local->m_nTickBase()) : g_csgo.m_globals->m_curtime;

		// simtime -> tickcount -> target time
		float target_time = game::TICKS_TO_TIME(game::TIME_TO_TICKS(this->m_sim_time));

		// get delta time
		// lol this looks cancer but thats the way it is ._.
		// https://github.com/perilouswithadollarsign/cstrike15_src/blob/f82112a2388b841d72cb62ca48ab1846dfcc11c8/game/server/player_lagcompensation.cpp#L287
		float delta_time = correct - (curtime - target_time);

		// if out of range
		// https://github.com/perilouswithadollarsign/cstrike15_src/blob/f82112a2388b841d72cb62ca48ab1846dfcc11c8/game/server/player_lagcompensation.cpp#L292
		if (fabs(delta_time) > 0.19f)
			return false;

		// just ignore dead time or not?
		return true;
	}
};