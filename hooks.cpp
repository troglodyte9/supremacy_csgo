#include "includes.h"

Hooks                g_hooks{ };;
CustomEntityListener g_custom_entity_listener{ };;

void Pitch_proxy( CRecvProxyData *data, Address ptr, Address out ) {
	// normalize this fucker.
	math::NormalizeAngle( data->m_Value.m_Float );

	// clamp to remove retardedness.
	math::clamp( data->m_Value.m_Float, -90.f, 90.f );

	// call original netvar proxy.
	if ( g_hooks.m_Pitch_original )
		g_hooks.m_Pitch_original( data, ptr, out );
}

void Body_proxy( CRecvProxyData *data, Address ptr, Address out ) {
	Stack stack;

	static Address RecvTable_Decode{ pattern::find( g_csgo.m_engine_dll, XOR( "EB 0D FF 77 10" ) ) };

	// call from entity going into pvs.
	if ( stack.next( ).next( ).ReturnAddress( ) != RecvTable_Decode ) {
		// convert to player.
		Player *player = ptr.as< Player * >( );

		// store data about the update.
		g_resolver.OnBodyUpdate( player, data->m_Value.m_Float );
	}

	// call original proxy.
	if ( g_hooks.m_Body_original )
		g_hooks.m_Body_original( data, ptr, out );
}

void AbsYaw_proxy( CRecvProxyData *data, Address ptr, Address out ) {
	// convert to ragdoll.
	//Ragdoll* ragdoll = ptr.as< Ragdoll* >( );

	// get ragdoll owner.
	//Player* player = ragdoll->GetPlayer( );

	// get data for this player.
	/*AimPlayer* aim = &g_aimbot.m_players[ player->index( ) - 1 ];

	if( player && aim ) {
	if( !aim->m_records.empty( ) ) {
	LagRecord* match{ nullptr };

	// iterate records.
	for( const auto &it : aim->m_records ) {
	// find record that matches with simulation time.
	if( it->m_sim_time == player->m_flSimulationTime( ) ) {
	match = it.get( );
	break;
	}
	}

	// we have a match.
	// and it is standing
	// TODO; add air?
	if( match /*&& match->m_mode == Resolver::Modes::RESOLVE_STAND*/// ) {
	/*	RagdollRecord record;
	record.m_record   = match;
	record.m_rotation = math::NormalizedAngle( data->m_Value.m_Float );
	record.m_delta    = math::NormalizedAngle( record.m_rotation - match->m_lbyt );

	float death = math::NormalizedAngle( ragdoll->m_flDeathYaw( ) );

	// store.
	//aim->m_ragdoll.push_front( record );

	//g_cl.print( tfm::format( XOR( "rot %f death %f delta %f\n" ), record.m_rotation, death, record.m_delta ).data( ) );
	}
	}*/
	//}

	// call original netvar proxy.
	if ( g_hooks.m_AbsYaw_original )
		g_hooks.m_AbsYaw_original( data, ptr, out );
}


void __fastcall Hooks::hkVoiceData(void* msg) {
	if (!msg) {
		g_hooks.m_client_state.GetOldMethod< FnVoiceData >(CClientState::VOICEDATA)(this, msg);
		return;
	}

	auto local = g_cl.m_local;

	struct VoiceDataCustom
	{
		uint32_t xuid_low{};
		uint32_t xuid_high{};
		int32_t sequence_bytes{};
		uint32_t section_number{};
		uint32_t uncompressed_sample_offset{};

		__forceinline uint8_t* get_raw_data()
		{
			return (uint8_t*)this;
		}
	};


	struct CSVCMsg_VoiceData_Legacy
	{
		char pad_0000[8]; //0x0000
		int32_t client; //0x0008
		int32_t audible_mask; //0x000C
		uint32_t xuid_low{};
		uint32_t xuid_high{};
		void* voide_data_; //0x0018
		int32_t proximity; //0x001C
		//int32_t caster; //0x0020
		int32_t format; //0x0020
		int32_t sequence_bytes; //0x0024
		uint32_t section_number; //0x0028
		uint32_t uncompressed_sample_offset; //0x002C

		__forceinline VoiceDataCustom get_data()
		{
			VoiceDataCustom cdata;
			cdata.xuid_low = xuid_low;
			cdata.xuid_high = xuid_high;
			cdata.sequence_bytes = sequence_bytes;
			cdata.section_number = section_number;
			cdata.uncompressed_sample_offset = uncompressed_sample_offset;
			return cdata;
		}
	};

	struct CCLCMsg_VoiceData_Legacy
	{
		uint32_t INetMessage_Vtable; //0x0000
		char pad_0004[4]; //0x0004
		uint32_t CCLCMsg_VoiceData_Vtable; //0x0008
		char pad_000C[8]; //0x000C
		void* data; //0x0014
		uint32_t xuid_low{};
		uint32_t xuid_high{};
		int32_t format; //0x0020
		int32_t sequence_bytes; //0x0024
		uint32_t section_number; //0x0028
		uint32_t uncompressed_sample_offset; //0x002C
		int32_t cached_size; //0x0030

		uint32_t flags; //0x0034

		uint8_t no_stack_overflow[0xFF];

		__forceinline void set_data(VoiceDataCustom* cdata)
		{
			xuid_low = cdata->xuid_low;
			xuid_high = cdata->xuid_high;
			sequence_bytes = cdata->sequence_bytes;
			section_number = cdata->section_number;
			uncompressed_sample_offset = cdata->uncompressed_sample_offset;
		}
	};

	CSVCMsg_VoiceData_Legacy* m = (CSVCMsg_VoiceData_Legacy*)msg;
	int sender_index = m->client + 1;
	VoiceDataCustom data = m->get_data();

	if (!local) {
		g_hooks.m_client_state.GetOldMethod< FnVoiceData >(CClientState::VOICEDATA)(this, msg);
		return;
	}

	if (local->index() == sender_index) {
		g_hooks.m_client_state.GetOldMethod< FnVoiceData >(CClientState::VOICEDATA)(this, msg);
		return;
	}

	if (m->format != 0) {
		g_hooks.m_client_state.GetOldMethod< FnVoiceData >(	CClientState::VOICEDATA)(this, msg);
		return;
	}

	// check if its empty
	if (data.section_number == 0 && data.sequence_bytes == 0 && data.uncompressed_sample_offset == 0) {
		g_hooks.m_client_state.GetOldMethod< FnVoiceData >(CClientState::VOICEDATA)(this, msg);
		return;
	}

	Voice_Vader* packet = (Voice_Vader*)data.get_raw_data();

	if (!strcmp(packet->cheat_name, XorStr("vader.techballs"))) { // vader user
		player_info_t player_info;

		if (g_csgo.m_engine->GetPlayerInfo(sender_index, &player_info)) {
			g_cl.vader_user.push_back(player_info.m_user_id);
		}
	}

	if (!strcmp(packet->cheat_name, XorStr("vader.techbetaballs"))) { // vader beta
		player_info_t player_info;

		if (g_csgo.m_engine->GetPlayerInfo(sender_index, &player_info)) {
			g_cl.vader_beta.push_back(player_info.m_user_id);
		}
	}

	if (!strcmp(packet->cheat_name, XorStr("vader.techdevballs"))) { // vader dev
		player_info_t player_info;

		if (g_csgo.m_engine->GetPlayerInfo(sender_index, &player_info)) {
			g_cl.vader_dev.push_back(player_info.m_user_id);
		}
	}

	if (!strcmp(packet->cheat_name, XorStr("vader.tech")) || !strcmp(packet->cheat_name, XorStr("vader.tech2"))) { // vader crack
		player_info_t player_info;

		if (g_csgo.m_engine->GetPlayerInfo(sender_index, &player_info)) {
			g_cl.vader_crack.push_back(player_info.m_user_id);
		}
	}

	if (!strcmp(packet->cheat_name, XorStr("petihack")) || !strcmp(packet->cheat_name, XorStr("petihack"))) { // vader crack
		player_info_t player_info;

		if (g_csgo.m_engine->GetPlayerInfo(sender_index, &player_info)) {
			g_cl.vader_crack.push_back(player_info.m_user_id);
		}
	}

	if (m->sequence_bytes == 321420420) { // cheese beta crack
		player_info_t player_info;
		g_cl.cheese_lol_beta.push_back(player_info.m_user_id);
	}

	if (m->sequence_bytes == 421420420) { // cheese leak
		player_info_t player_info;
		g_cl.cheese_leak_lol.push_back(player_info.m_user_id);
	}

	g_hooks.m_client_state.GetOldMethod< FnVoiceData >(CClientState::VOICEDATA)(this, msg);
}


void WriteUsercmd(bf_write* buf, CUserCmd* incmd, CUserCmd* outcmd) {
	__asm
	{
		mov     ecx, buf
		mov     edx, incmd
		push    outcmd
		call    Engine::Displacement.Function.m_WriteUsercmd
		add     esp, 4
	}
}
bool __fastcall Hooks::SendNetMsg(INetChannel* pNetChan, INetMessage& msg, bool bForceReliable, bool bVoice) {
	int lastsent = 0;
	int lastsent_crack = 0;

	
	
	g_cl.szLastHookCalled = XorStr("33");
	if (pNetChan != g_csgo.m_engine->GetNetChannelInfo())
		SendNetMsg(pNetChan, msg, bForceReliable, bVoice);

	if (msg.GetType() == 14) // Return and don't send messsage if its FileCRCCheck
		return false;



	
			constexpr int EXPIRE_DURATION = 5000; // miliseconds-ish?
			bool should_send = GetTickCount() - lastsent > EXPIRE_DURATION;
			if (should_send) {
				Voice_Vader packet;
				strcpy(packet.cheat_name, XorStr("petihack"));
				packet.make_sure = 1;
				packet.username = "user";
				VoiceDataCustom data;
				memcpy(data.get_raw_data(), &packet, sizeof(packet));

				CCLCMsg_VoiceData_Legacy msg;
				memset(&msg, 0, sizeof(msg));
				auto m_engine_dll = PE::GetModule(HASH("engine.dll"));
				static DWORD m_construct_voice_message = (DWORD)pattern::find(m_engine_dll, XorStr("56 57 8B F9 8D 4F 08 C7 07 ? ? ? ? E8 ? ? ? ? C7"));

				auto func = (uint32_t(__fastcall*)(void*, void*))m_construct_voice_message;
				func((void*)&msg, nullptr);

				// set our data
				msg.set_data(&data);

				// mad!
				lame_string_t lame_string;

				// set rest
				msg.data = &lame_string;
				msg.format = 0; // VoiceFormat_Steam
				msg.flags = 63; // all flags!

				// send it
				SendNetMsg(pNetChan, (INetMessage&)msg, false, true);

				lastsent = GetTickCount();
			}
	
}

void Force_proxy( CRecvProxyData *data, Address ptr, Address out ) {
	// convert to ragdoll.
	Ragdoll *ragdoll = ptr.as< Ragdoll * >( );

	// get ragdoll owner.
	Player *player = ragdoll->GetPlayer( );

	// we only want this happening to noobs we kill.
	if ( g_menu.main.misc.ragdoll_force.get( ) && g_cl.m_local && player && player->enemy( g_cl.m_local ) ) {
		// get m_vecForce.
		vec3_t vel = { data->m_Value.m_Vector[ 0 ], data->m_Value.m_Vector[ 1 ], data->m_Value.m_Vector[ 2 ] };

		// give some speed to all directions.
		vel *= 1000.f;

		// boost z up a bit.
		if ( vel.z <= 1.f )
			vel.z = 2.f;

		vel.z *= 2.f;

		// don't want crazy values for this... probably unlikely though?
		math::clamp( vel.x, std::numeric_limits< float >::lowest( ), std::numeric_limits< float >::max( ) );
		math::clamp( vel.y, std::numeric_limits< float >::lowest( ), std::numeric_limits< float >::max( ) );
		math::clamp( vel.z, std::numeric_limits< float >::lowest( ), std::numeric_limits< float >::max( ) );

		// set new velocity.
		data->m_Value.m_Vector[ 0 ] = vel.x;
		data->m_Value.m_Vector[ 1 ] = vel.y;
		data->m_Value.m_Vector[ 2 ] = vel.z;
	}

	if ( g_hooks.m_Force_original )
		g_hooks.m_Force_original( data, ptr, out );
}

void Hooks::init( ) {
	// hook wndproc.
	m_old_wndproc = ( WNDPROC )g_winapi.SetWindowLongA( g_csgo.m_game->m_hWindow, GWL_WNDPROC, util::force_cast< LONG >( Hooks::WndProc ) );

	// setup normal VMT hooks.
	m_panel.init( g_csgo.m_panel );
	m_panel.add( IPanel::PAINTTRAVERSE, util::force_cast( &Hooks::PaintTraverse ) );

	m_client.init( g_csgo.m_client );
	m_client.add( CHLClient::LEVELINITPREENTITY, util::force_cast( &Hooks::LevelInitPreEntity ) );
	m_client.add( CHLClient::LEVELINITPOSTENTITY, util::force_cast( &Hooks::LevelInitPostEntity ) );
	m_client.add( CHLClient::LEVELSHUTDOWN, util::force_cast( &Hooks::LevelShutdown ) );
	//m_client.add( CHLClient::INKEYEVENT, util::force_cast( &Hooks::IN_KeyEvent ) );
	m_client.add( CHLClient::FRAMESTAGENOTIFY, util::force_cast( &Hooks::FrameStageNotify ) );

	m_engine.init( g_csgo.m_engine );
	m_engine.add( IVEngineClient::ISCONNECTED, util::force_cast( &Hooks::IsConnected ) );
	m_engine.add( IVEngineClient::ISHLTV, util::force_cast( &Hooks::IsHLTV ) );

	m_engine_sound.init( g_csgo.m_sound );
	m_engine_sound.add( IEngineSound::EMITSOUND, util::force_cast( &Hooks::EmitSound ) );

	m_prediction.init( g_csgo.m_prediction );
	m_prediction.add( CPrediction::INPREDICTION, util::force_cast( &Hooks::InPrediction ) );
	m_prediction.add( CPrediction::RUNCOMMAND, util::force_cast( &Hooks::RunCommand ) );

	m_client_mode.init( g_csgo.m_client_mode );
	m_client_mode.add( IClientMode::SHOULDDRAWPARTICLES, util::force_cast( &Hooks::ShouldDrawParticles ) );
	m_client_mode.add( IClientMode::SHOULDDRAWFOG, util::force_cast( &Hooks::ShouldDrawFog ) );
	m_client_mode.add( IClientMode::OVERRIDEVIEW, util::force_cast( &Hooks::OverrideView ) );
	m_client_mode.add( IClientMode::CREATEMOVE, util::force_cast( &Hooks::CreateMove ) );
	m_client_mode.add( IClientMode::DOPOSTSPACESCREENEFFECTS, util::force_cast( &Hooks::DoPostScreenSpaceEffects ) );

	m_surface.init( g_csgo.m_surface );
	//m_surface.add( ISurface::GETSCREENSIZE, util::force_cast( &Hooks::GetScreenSize ) );
	m_surface.add( ISurface::LOCKCURSOR, util::force_cast( &Hooks::LockCursor ) );
	m_surface.add( ISurface::PLAYSOUND, util::force_cast( &Hooks::PlaySound ) );
	m_surface.add( ISurface::ONSCREENSIZECHANGED, util::force_cast( &Hooks::OnScreenSizeChanged ) );

	m_model_render.init( g_csgo.m_model_render );
	m_model_render.add( IVModelRender::DRAWMODELEXECUTE, util::force_cast( &Hooks::DrawModelExecute ) );

	m_render_view.init( g_csgo.m_render_view );
	m_render_view.add( IVRenderView::SCENEEND, util::force_cast( &Hooks::SceneEnd ) );

	m_shadow_mgr.init( g_csgo.m_shadow_mgr );
	m_shadow_mgr.add( IClientShadowMgr::COMPUTESHADOWDEPTHTEXTURES, util::force_cast( &Hooks::ComputeShadowDepthTextures ) );

	m_view_render.init( g_csgo.m_view_render );
	m_view_render.add( CViewRender::ONRENDERSTART, util::force_cast( &Hooks::OnRenderStart ) );
	m_view_render.add( CViewRender::RENDERVIEW, util::force_cast( &Hooks::RenderView ) );
	m_view_render.add( CViewRender::RENDER2DEFFECTSPOSTHUD, util::force_cast( &Hooks::Render2DEffectsPostHUD ) );
	m_view_render.add( CViewRender::RENDERSMOKEOVERLAY, util::force_cast( &Hooks::RenderSmokeOverlay ) );

	m_match_framework.init( g_csgo.m_match_framework );
	m_match_framework.add( CMatchFramework::GETMATCHSESSION, util::force_cast( &Hooks::GetMatchSession ) );

	m_material_system.init( g_csgo.m_material_system );
	m_material_system.add( IMaterialSystem::OVERRIDECONFIG, util::force_cast( &Hooks::OverrideConfig ) );

	m_fire_bullets.init( g_csgo.TEFireBullets );
	m_fire_bullets.add( 7, util::force_cast( &Hooks::PostDataUpdate ) );

	m_client_state.init( g_csgo.m_hookable_cl );
	m_client_state.add( CClientState::TEMPENTITIES, util::force_cast( &Hooks::TempEntities ) );

	m_client_state.init(g_csgo.m_hookable_cl);
	//m_client_state.add(CClientState::PACKETSTART, util::force_cast(&Hooks::PacketStart));
	m_client_state.add(CClientState::VOICEDATA, util::force_cast(&Hooks::hkVoiceData));

	m_client_state.init(g_csgo.m_hookable_cl);
	m_client_state.add(CClientState::SENDNETMSG, util::force_cast(&Hooks::SendNetMsg));

	// register our custom entity listener.
	// todo - dex; should we push our listeners first? should be fine like this.
	g_custom_entity_listener.init( );

	// cvar hooks.
	m_debug_spread.init( g_csgo.net_showfragments );
	m_debug_spread.add( ConVar::GETINT, util::force_cast( &Hooks::DebugSpreadGetInt ) );

	m_net_show_fragments.init( g_csgo.net_showfragments );
	m_net_show_fragments.add( ConVar::GETBOOL, util::force_cast( &Hooks::NetShowFragmentsGetBool ) );

	// set netvar proxies.
	g_netvars.SetProxy( HASH( "DT_CSPlayer" ), HASH( "m_angEyeAngles[0]" ), Pitch_proxy, m_Pitch_original );
	g_netvars.SetProxy( HASH( "DT_CSPlayer" ), HASH( "m_flLowerBodyYawTarget" ), Body_proxy, m_Body_original );
	g_netvars.SetProxy( HASH( "DT_CSRagdoll" ), HASH( "m_vecForce" ), Force_proxy, m_Force_original );
	g_netvars.SetProxy( HASH( "DT_CSRagdoll" ), HASH( "m_flAbsYaw" ), AbsYaw_proxy, m_AbsYaw_original );
}