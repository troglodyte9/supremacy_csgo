#include "includes.h"

Visuals g_visuals{ };;

void Visuals::ModulateWorld( ) {
	std::vector< IMaterial* > world, props;

	// iterate material handles.
	for( uint16_t h{ g_csgo.m_material_system->FirstMaterial( ) }; h != g_csgo.m_material_system->InvalidMaterial( ); h = g_csgo.m_material_system->NextMaterial( h ) ) {
		// get material from handle.
		IMaterial* mat = g_csgo.m_material_system->GetMaterial( h );
		if( !mat )
			continue;

		// store world materials.
		if( FNV1a::get( mat->GetTextureGroupName( ) ) == HASH( "World textures" ) )
			world.push_back( mat );

		// store props.
		else if( FNV1a::get( mat->GetTextureGroupName( ) ) == HASH( "StaticProp textures" ) )
			props.push_back( mat );
	}

	// night.
	if( g_menu.main.visuals.world.get( ) == 1 ) {
		for( const auto& w : world )
			w->ColorModulate( 0.17f, 0.16f, 0.18f );

		// IsUsingStaticPropDebugModes my nigga
		if( g_csgo.r_DrawSpecificStaticProp->GetInt( ) != 0 ) {
			g_csgo.r_DrawSpecificStaticProp->SetValue( 0 );
		}

		for( const auto& p : props )
			p->ColorModulate( 0.5f, 0.5f, 0.5f );
	}

	// disable night.
	else {
		for( const auto& w : world )
			w->ColorModulate( 1.f, 1.f, 1.f );

		// restore r_DrawSpecificStaticProp.
		if( g_csgo.r_DrawSpecificStaticProp->GetInt( ) != -1 ) {
			g_csgo.r_DrawSpecificStaticProp->SetValue( -1 );
		}

		for( const auto& p : props )
			p->ColorModulate( 1.f, 1.f, 1.f );
	}

	// transparent props.
	if( g_menu.main.visuals.transparent_props.get( ) ) {

		// IsUsingStaticPropDebugModes my nigga
		if( g_csgo.r_DrawSpecificStaticProp->GetInt( ) != 0 ) {
			g_csgo.r_DrawSpecificStaticProp->SetValue( 0 );
		}

		for( const auto& p : props )
			p->AlphaModulate( 0.85f );
	}

	// disable transparent props.
	else {

		// restore r_DrawSpecificStaticProp.
		if( g_csgo.r_DrawSpecificStaticProp->GetInt( ) != -1 ) {
			g_csgo.r_DrawSpecificStaticProp->SetValue( -1 );
		}

		for( const auto& p : props )
			p->AlphaModulate( 1.0f );
	}
}

void Visuals::ThirdpersonThink( ) {
	ang_t                          offset;
	vec3_t                         origin, forward;
	static CTraceFilterSimple_game filter{ };
	CGameTrace                     tr;

	// for whatever reason overrideview also gets called from the main menu.
	if( !g_csgo.m_engine->IsInGame( ) )
		return;

	// check if we have a local player and he is alive.
	bool alive = g_cl.m_local && g_cl.m_local->alive( );

	// camera should be in thirdperson.
	if( m_thirdperson ) {

		// if alive and not in thirdperson already switch to thirdperson.
		if( alive && !g_csgo.m_input->CAM_IsThirdPerson( ) )
			g_csgo.m_input->CAM_ToThirdPerson( );

		// if dead and spectating in firstperson switch to thirdperson.
		else if( g_cl.m_local->m_iObserverMode( ) == 4 ) {

			// if in thirdperson, switch to firstperson.
			// we need to disable thirdperson to spectate properly.
			if( g_csgo.m_input->CAM_IsThirdPerson( ) ) {
				g_csgo.m_input->CAM_ToFirstPerson( );
				g_csgo.m_input->m_camera_offset.z = 0.f;
			}

			g_cl.m_local->m_iObserverMode( ) = 5;
		}
	}

	// camera should be in firstperson.
	else if( g_csgo.m_input->CAM_IsThirdPerson( ) ) {
		g_csgo.m_input->CAM_ToFirstPerson( );
		g_csgo.m_input->m_camera_offset.z = 0.f;
	}

	// if after all of this we are still in thirdperson.
	if( g_csgo.m_input->CAM_IsThirdPerson( ) ) {
		// get camera angles.
		g_csgo.m_engine->GetViewAngles( offset );

		// get our viewangle's forward directional vector.
		math::AngleVectors( offset, &forward );

		// cam_idealdist convar.
		offset.z = g_csgo.m_cvar->FindVar(HASH("cam_idealdist"))->GetFloat();

		// start pos.
		origin = g_cl.m_shoot_pos;

		// setup trace filter and trace.
		filter.SetPassEntity( g_cl.m_local );

		g_csgo.m_engine_trace->TraceRay(
			Ray( origin, origin - ( forward * offset.z ), { -16.f, -16.f, -16.f }, { 16.f, 16.f, 16.f } ),
			MASK_NPCWORLDSTATIC,
			( ITraceFilter* ) &filter,
			&tr
		);

		// adapt distance to travel time.
		math::clamp( tr.m_fraction, 0.f, 1.f );
		offset.z *= tr.m_fraction;

		// override camera angles.
		g_csgo.m_input->m_camera_offset = { offset.x, offset.y, offset.z };
	}
}

void Visuals::AutopeekIndicator() {
	// dont do if dead.
	if (!g_cl.m_processing)
		return;

	auto weapon = g_cl.m_local->GetActiveWeapon();

	if (!weapon)
		return;

	static auto position = vec3_t(0.f, 0.f, 0.f);

	if (!g_movement.start_position.IsZero())
		position = g_movement.start_position;

	if (position.IsZero())
		return;

	static auto alpha = 0.0f;

	if (g_input.GetKeyState(g_menu.main.movement.autopeek.get()) || alpha) {

		if (g_input.GetKeyState(g_menu.main.movement.autopeek.get()))
			alpha += 85.0f * g_csgo.m_globals->m_frametime;
		else
			alpha -= 85.0f * g_csgo.m_globals->m_frametime;

		alpha = math::dont_break(alpha, 0.0f, 15.0f);
		render::Draw3DFilledCircle(position, alpha, g_menu.main.movement.autopeek_active.get());
		//render::Draw3DCircle(position, 15.0f, outer_color);
	}
}

void Visuals::ManualAntiAim() {
	int   x, y;

	// dont do if dead.
	if (!g_cl.m_processing)
		return;

	if (!g_menu.main.antiaim.manual_antiaim.get())
		return;

	x = g_cl.m_width / 2;
	y = g_cl.m_height / 2;

	//fade
	int alpha = sin(abs(fmod(-math::pi + (g_csgo.m_globals->m_curtime * (2 / .75)), (math::pi * 2)))) * 225;
	if (alpha < 0)
		alpha = alpha * (-1);

	Color color = g_menu.main.antiaim.color_manual_antiaim.get();


	if (g_hvh.m_left) {
		render::rect_filled(x - 50, y - 7, 1, 1, color);
		render::rect_filled(x - 50, y - 7 + 14, 1, 1, color);
		render::rect_filled(x - 51, y - 7, 1, 3, color);
		render::rect_filled(x - 51, y - 7 + 12, 1, 3, color);
		render::rect_filled(x - 52, y - 7 + 1, 1, 4, color);
		render::rect_filled(x - 52, y - 7 + 1 + 9, 1, 4, color);
		render::rect_filled(x - 53, y - 7 + 1, 1, 5, color);
		render::rect_filled(x - 53, y - 7 + 1 + 8, 1, 5, color);
		render::rect_filled(x - 54, y - 7 + 2, 1, 5, color);
		render::rect_filled(x - 54, y - 7 + 1 + 7, 1, 5, color);
		render::rect_filled(x - 55, y - 7 + 2, 1, 11, color);
		render::rect_filled(x - 56, y - 7 + 3, 1, 9, color);
		render::rect_filled(x - 57, y - 7 + 3, 1, 9, color);
		render::rect_filled(x - 58, y - 7 + 4, 1, 7, color);
		render::rect_filled(x - 59, y - 7 + 4, 1, 7, color);
		render::rect_filled(x - 60, y - 7 + 5, 1, 5, color);
		render::rect_filled(x - 61, y - 7 + 5, 1, 5, color);
		render::rect_filled(x - 62, y - 7 + 6, 1, 3, color);
		render::rect_filled(x - 63, y - 7 + 6, 1, 3, color);
		render::rect_filled(x - 64, y - 7 + 7, 1, 1, color);
	}

	if (g_hvh.m_right) {
		render::rect_filled(x + 50, y - 7, 1, 1, color);
		render::rect_filled(x + 50, y - 7 + 14, 1, 1, color);
		render::rect_filled(x + 51, y - 7, 1, 3, color);
		render::rect_filled(x + 51, y - 7 + 12, 1, 3, color);
		render::rect_filled(x + 52, y - 7 + 1, 1, 4, color);
		render::rect_filled(x + 52, y - 7 + 1 + 9, 1, 4, color);
		render::rect_filled(x + 53, y - 7 + 1, 1, 5, color);
		render::rect_filled(x + 53, y - 7 + 1 + 8, 1, 5, color);
		render::rect_filled(x + 54, y - 7 + 2, 1, 5, color);
		render::rect_filled(x + 54, y - 7 + 1 + 7, 1, 5, color);
		render::rect_filled(x + 55, y - 7 + 2, 1, 11, color);
		render::rect_filled(x + 56, y - 7 + 3, 1, 9, color);
		render::rect_filled(x + 57, y - 7 + 3, 1, 9, color);
		render::rect_filled(x + 58, y - 7 + 4, 1, 7, color);
		render::rect_filled(x + 59, y - 7 + 4, 1, 7, color);
		render::rect_filled(x + 60, y - 7 + 5, 1, 5, color);
		render::rect_filled(x + 61, y - 7 + 5, 1, 5, color);
		render::rect_filled(x + 62, y - 7 + 6, 1, 3, color);
		render::rect_filled(x + 63, y - 7 + 6, 1, 3, color);
		render::rect_filled(x + 64, y - 7 + 7, 1, 1, color);
	}

	if (g_hvh.m_back) {
		render::rect_filled(x - 10 + 10, y + 60, 1, 1, color);
		render::rect_filled(x - 10 + 9, y + 60 - 1, 3, 1, color);
		render::rect_filled(x - 10 + 9, y + 60 - 2, 3, 1, color);
		render::rect_filled(x - 10 + 8, y + 60 - 3, 5, 1, color);
		render::rect_filled(x - 10 + 8, y + 60 - 4, 5, 1, color);
		render::rect_filled(x - 10 + 7, y + 60 - 5, 7, 1, color);
		render::rect_filled(x - 10 + 7, y + 60 - 6, 7, 1, color);
		render::rect_filled(x - 10 + 6, y + 60 - 7, 9, 1, color);
		render::rect_filled(x - 10 + 6, y + 60 - 8, 9, 1, color);
		render::rect_filled(x - 10 + 5, y + 60 - 9, 11, 1, color);
		render::rect_filled(x - 10 + 5, y + 60 - 10, 5, 1, color);
		render::rect_filled(x - 10 + 4, y + 60 - 11, 5, 1, color);
		render::rect_filled(x - 10 + 4, y + 60 - 12, 4, 1, color);
		render::rect_filled(x - 10 + 3, y + 60 - 13, 3, 1, color);
		render::rect_filled(x - 10 + 3, y + 60 - 14, 1, 1, color);
		render::rect_filled(x - 10 + 5 + 6, y + 60 - 10, 5, 1, color);
		render::rect_filled(x - 10 + 5 + 7, y + 60 - 11, 5, 1, color);
		render::rect_filled(x - 10 + 5 + 8, y + 60 - 12, 4, 1, color);
		render::rect_filled(x - 10 + 5 + 10, y + 60 - 13, 3, 1, color);
		render::rect_filled(x - 10 + 5 + 12, y + 60 - 14, 1, 1, color);
	}
}

void Visuals::Hitmarker( ) {

	static auto cross = g_csgo.m_cvar->FindVar(HASH("weapon_debug_spread_show"));
	cross->SetValue(g_menu.main.visuals.force_xhair.get() && !g_cl.m_local->m_bIsScoped() ? 3 : 0); // force crosshair
	if( !g_menu.main.misc.hitmarker.get( ) )
		return;

	if( g_csgo.m_globals->m_curtime > m_hit_end )
		return;

	if( m_hit_duration <= 0.f )
		return;

	float complete = ( g_csgo.m_globals->m_curtime - m_hit_start ) / m_hit_duration;
	int x = g_cl.m_width / 2,
		y = g_cl.m_height / 2,
		alpha = ( 1.f - complete ) * 240;

	constexpr int line{ 6 };

	render::line( x - line, y - line, x - ( line / 4 ), y - ( line / 4 ), { 200, 200, 200, alpha } );
	render::line( x - line, y + line, x - ( line / 4 ), y + ( line / 4 ), { 200, 200, 200, alpha } );
	render::line( x + line, y + line, x + ( line / 4 ), y + ( line / 4 ), { 200, 200, 200, alpha } );
	render::line( x + line, y - line, x + ( line / 4 ), y - ( line / 4 ), { 200, 200, 200, alpha } );
}

void Visuals::NoSmoke( ) {
	if( !smoke1 )
		smoke1 = g_csgo.m_material_system->FindMaterial( XOR( "particle/vistasmokev1/vistasmokev1_fire" ), XOR( "Other textures" ) );

	if( !smoke2 )
		smoke2 = g_csgo.m_material_system->FindMaterial( XOR( "particle/vistasmokev1/vistasmokev1_smokegrenade" ), XOR( "Other textures" ) );

	if( !smoke3 )
		smoke3 = g_csgo.m_material_system->FindMaterial( XOR( "particle/vistasmokev1/vistasmokev1_emods" ), XOR( "Other textures" ) );

	if( !smoke4 )
		smoke4 = g_csgo.m_material_system->FindMaterial( XOR( "particle/vistasmokev1/vistasmokev1_emods_impactdust" ), XOR( "Other textures" ) );

	if( g_menu.main.visuals.nosmoke.get( ) ) {
		if( !smoke1->GetFlag( MATERIAL_VAR_NO_DRAW ) )
			smoke1->SetFlag( MATERIAL_VAR_NO_DRAW, true );

		if( !smoke2->GetFlag( MATERIAL_VAR_NO_DRAW ) )
			smoke2->SetFlag( MATERIAL_VAR_NO_DRAW, true );

		if( !smoke3->GetFlag( MATERIAL_VAR_NO_DRAW ) )
			smoke3->SetFlag( MATERIAL_VAR_NO_DRAW, true );

		if( !smoke4->GetFlag( MATERIAL_VAR_NO_DRAW ) )
			smoke4->SetFlag( MATERIAL_VAR_NO_DRAW, true );
	}

	else {
		if( smoke1->GetFlag( MATERIAL_VAR_NO_DRAW ) )
			smoke1->SetFlag( MATERIAL_VAR_NO_DRAW, false );

		if( smoke2->GetFlag( MATERIAL_VAR_NO_DRAW ) )
			smoke2->SetFlag( MATERIAL_VAR_NO_DRAW, false );

		if( smoke3->GetFlag( MATERIAL_VAR_NO_DRAW ) )
			smoke3->SetFlag( MATERIAL_VAR_NO_DRAW, false );

		if( smoke4->GetFlag( MATERIAL_VAR_NO_DRAW ) )
			smoke4->SetFlag( MATERIAL_VAR_NO_DRAW, false );
	}
}

void Visuals::think( ) {
	// don't run anything if our local player isn't valid.
	if( !g_cl.m_local )
		return;

	if( g_menu.main.visuals.noscope.get( )
		&& g_cl.m_local->alive( )
		&& g_cl.m_local->GetActiveWeapon( )
		&& g_cl.m_local->GetActiveWeapon( )->GetWpnData( )->m_weapon_type == CSWeaponType::WEAPONTYPE_SNIPER_RIFLE
		&& g_cl.m_local->m_bIsScoped( ) ) {

		// rebuild the original scope lines.
		int w = g_cl.m_width,
			h = g_cl.m_height,
			x = w / 2,
			y = h / 2,
			size = g_csgo.cl_crosshair_sniper_width->GetInt( );

		// Here We Use The Euclidean distance To Get The Polar-Rectangular Conversion Formula.
		if( size > 1 ) {
			x -= ( size / 2 );
			y -= ( size / 2 );
		}

		// draw our lines.
		render::rect_filled( 0, y, w, size, colors::black );
		render::rect_filled( x, 0, size, h, colors::black );
	}

	// draw esp on ents.
	for( int i{ 1 }; i <= g_csgo.m_entlist->GetHighestEntityIndex( ); ++i ) {
		Entity* ent = g_csgo.m_entlist->GetClientEntity( i );
		if( !ent )
			continue;

		draw( ent );
	}

	// draw everything else.
	SpreadCrosshair( );
	StatusIndicators( );
	Spectators( );
	PenetrationCrosshair( );
	Hitmarker( );
	DrawPlantedC4( );
	AutopeekIndicator();
	ManualAntiAim();
}

void Visuals::Spectators( ) {
	if( !g_menu.main.visuals.spectators.get( ) )
		return;

	std::vector< std::string > spectators{ XOR( "spectators" ) };
	int h = render::menu_shade.m_size.m_height;

	for( int i{ 1 }; i <= g_csgo.m_globals->m_max_clients; ++i ) {
		Player* player = g_csgo.m_entlist->GetClientEntity< Player* >( i );
		if( !player )
			continue;

		if( player->m_bIsLocalPlayer( ) )
			continue;

		if( player->dormant( ) )
			continue;

		if( player->m_lifeState( ) == LIFE_ALIVE )
			continue;

		if( player->GetObserverTarget( ) != g_cl.m_local )
			continue;

		player_info_t info;
		if( !g_csgo.m_engine->GetPlayerInfo( i, &info ) )
			continue;

		spectators.push_back( std::string( info.m_name ).substr( 0, 24 ) );
	}

	size_t total_size = spectators.size( ) * ( h - 1 );

	for( size_t i{ }; i < spectators.size( ); ++i ) {
		const std::string& name = spectators[ i ];

		render::menu_shade.string( g_cl.m_width - 20, ( g_cl.m_height / 2 ) - ( total_size / 2 ) + ( i * ( h - 1 ) ),
			{ 255, 255, 255, 179 }, name, render::ALIGN_RIGHT );
	}
}


Color LerpRGB(Color a, Color b, float t)
{
	return Color
	(
		a.r() + (b.r() - a.r()) * t,
		a.g() + (b.g() - a.g()) * t,
		a.b() + (b.b() - a.b()) * t,
		a.a() + (b.a() - a.a()) * t
	);
}
void Visuals::StatusIndicators( ) {
	// dont do if dead.
	if (!g_cl.m_processing)
		return;

	// compute hud size.
	// int size = ( int )std::round( ( g_cl.m_height / 17.5f ) * g_csgo.hud_scaling->GetFloat( ) );

	struct Indicator_t { Color color; std::string text; };
	std::vector< Indicator_t > indicators{ };


	if (g_menu.main.visuals.indicators.get(3) && g_input.GetKeyState(g_menu.main.aimbot.override.get())) {
		Indicator_t ind{ };

		ind.color = 0xff0000ff;
		ind.text = XOR("OVERRIDE");

		indicators.push_back(ind);
	}

	// PING
	if (g_menu.main.visuals.indicators.get(2) and g_aimbot.m_fake_latency) {
		Indicator_t ind{ };
		ind.color = LerpRGB(0xff0000ff, 0xffffffff, std::clamp((g_csgo.m_cl->m_net_channel->GetLatency(INetChannel::FLOW_INCOMING) * 1000.f) / g_menu.main.misc.fake_latency_amt.get(), 0.f, 1.f));
		ind.text = XOR("PING");

		indicators.push_back(ind);
	}

	// LC
	if (g_menu.main.visuals.indicators.get(1)) {
		if (g_cl.m_local->m_vecVelocity().length_2d() > 270.f || g_cl.m_lagcomp) {
			Indicator_t ind{ };
			ind.color = g_cl.m_lagcomp ? 0xff15c27b : 0xff0000ff;
			ind.text = XOR("LC");

			indicators.push_back(ind);
		}
	}

	// LBY
	if (g_menu.main.visuals.indicators.get(0)) {
		// get the absolute change between current lby and animated angle.
		float change = std::abs(math::NormalizedAngle(g_cl.m_body - g_cl.m_angle.y));

		Indicator_t ind{ };
		ind.color = change > 35.f ? 0xff15c27b : 0xff0000ff;
		ind.text = XOR("LBY");
		indicators.push_back(ind);
	}

	// DMG
	if (g_menu.main.visuals.indicators.get(4) && g_aimbot.m_damage_toggle) {
		int nigger = g_menu.main.aimbot.override_dmg_value.get();
		Indicator_t ind{ };
		ind.color = 0xffffffff;
		ind.text = tfm::format(XOR("%s"), nigger);
		indicators.push_back(ind);
	}

	if (indicators.empty())
		return;

	// iterate and draw indicators.
	for (size_t i{ }; i < indicators.size(); ++i) {
		auto& indicator = indicators[i];

		render::indicator.string(15, g_cl.m_height - 80 - (30 * i), indicator.color, indicator.text);
	}
}

void Visuals::SpreadCrosshair( ) {
	// dont do if dead.
	if( !g_cl.m_processing )
		return;

	if( !g_menu.main.visuals.spread_xhair.get( ) )
		return;

	// get active weapon.
	Weapon* weapon = g_cl.m_local->GetActiveWeapon( );
	if( !weapon )
		return;

	WeaponInfo* data = weapon->GetWpnData( );
	if( !data )
		return;

	// do not do this on: bomb, knife and nades.
	CSWeaponType type = data->m_weapon_type;
	if( type == WEAPONTYPE_KNIFE || type == WEAPONTYPE_C4 || type == WEAPONTYPE_GRENADE )
		return;

	// calc radius.
	float radius = ( ( weapon->GetInaccuracy( ) + weapon->GetSpread( ) ) * 320.f ) / ( std::tan( math::deg_to_rad( g_cl.m_local->GetFOV( ) ) * 0.5f ) + FLT_EPSILON );

	// scale by screen size.
	radius *= g_cl.m_height * ( 1.f / 480.f );

	// get color.
	Color col = g_menu.main.visuals.spread_xhair_col.get( );

	// modify alpha channel.
	col.a( ) = 200 * ( g_menu.main.visuals.spread_xhair_blend.get( ) / 100.f );

	int segements = std::max( 16, ( int ) std::round( radius * 0.75f ) );
	render::circle( g_cl.m_width / 2, g_cl.m_height / 2, radius, segements, col );
}

void Visuals::PenetrationCrosshair( ) {
	int   x, y;
	bool  valid_player_hit;
	Color final_color;

	if( !g_menu.main.visuals.pen_crosshair.get( ) || !g_cl.m_processing )
		return;

	x = g_cl.m_width / 2;
	y = g_cl.m_height / 2;


	valid_player_hit = ( g_cl.m_pen_data.m_target && g_cl.m_pen_data.m_target->enemy( g_cl.m_local ) );
	if( valid_player_hit )
		final_color = colors::transparent_yellow;

	else if( g_cl.m_pen_data.m_pen )
		final_color = colors::transparent_green;

	else
		final_color = colors::transparent_red;

	// todo - dex; use fmt library to get damage string here?
	//             draw damage string?

	// draw small square in center of screen.
	render::rect_filled( x - 1, y - 1, 3, 3, final_color );
}

void Visuals::draw( Entity* ent ) {
	if( ent->IsPlayer( ) ) {
		Player* player = ent->as< Player* >( );

		// dont draw dead players.
		if( !player->alive( ) )
			return;

		if( player->m_bIsLocalPlayer( ) )
			return;

		// draw player esp.
		if (!player->dormant()) g_sound.m_cSoundPlayers[player->index()].reset(true, player->GetAbsOrigin(), player->m_fFlags());
		else g_sound.AdjustPlayerBegin(player);
		DrawPlayer( player );
		g_sound.Finish();
	}

	else if( g_menu.main.visuals.items.get( ) && ent->IsBaseCombatWeapon( ) && !ent->dormant( ) )
		DrawItem( ent->as< Weapon* >( ) );

	else if( g_menu.main.visuals.proj.get( ) )
		DrawProjectile( ent->as< Weapon* >( ) );
}

void Visuals::DrawProjectile( Weapon* ent ) {
	vec2_t screen;
	vec3_t origin = ent->GetAbsOrigin( );
	if( !render::WorldToScreen( origin, screen ) )
		return;

	Color col = g_menu.main.visuals.proj_color.get( );
	col.a( ) = 0xb4;

	// draw decoy.
	if( ent->is( HASH( "CDecoyProjectile" ) ) )
		render::esp_small.string( screen.x, screen.y, col, XOR( "DECOY" ), render::ALIGN_CENTER );

	// draw molotov.
	else if( ent->is( HASH( "CMolotovProjectile" ) ) )
		render::esp_small.string( screen.x, screen.y, col, XOR( "MOLLY" ), render::ALIGN_CENTER );

	else if( ent->is( HASH( "CBaseCSGrenadeProjectile" ) ) ) {
		const model_t* model = ent->GetModel( );

		if( model ) {
			// grab modelname.
			std::string name{ ent->GetModel( )->m_name };

			if( name.find( XOR( "flashbang" ) ) != std::string::npos )
				render::esp_small.string( screen.x, screen.y, col, XOR( "FLASH" ), render::ALIGN_CENTER );

			else if( name.find( XOR( "fraggrenade" ) ) != std::string::npos ) {

				// grenade range.
				if( g_menu.main.visuals.proj_range.get( 0 ) )
					render::sphere( origin, 350.f, 5.f, 1.f, g_menu.main.visuals.proj_range_color.get( ) );

				render::esp_small.string( screen.x, screen.y, col, XOR( "FRAG" ), render::ALIGN_CENTER );
			}
		}
	}

	// find classes.
	else if( ent->is( HASH( "CInferno" ) ) ) {
		// fire range.
		if( g_menu.main.visuals.proj_range.get( 1 ) )
			render::sphere( origin, 150.f, 5.f, 1.f, g_menu.main.visuals.proj_range_color.get( ) );

		render::esp_small.string( screen.x, screen.y, col, XOR( "FIRE" ), render::ALIGN_CENTER );
	}

	else if( ent->is( HASH( "CSmokeGrenadeProjectile" ) ) )
		render::esp_small.string( screen.x, screen.y, col, XOR( "SMOKE" ), render::ALIGN_CENTER );
}

void Visuals::DrawItem( Weapon* item ) {
	// we only want to draw shit without owner.
	Entity* owner = g_csgo.m_entlist->GetClientEntityFromHandle( item->m_hOwnerEntity( ) );
	if( owner )
		return;

	// is the fucker even on the screen?
	vec2_t screen;
	vec3_t origin = item->GetAbsOrigin( );
	if( !render::WorldToScreen( origin, screen ) )
		return;

	WeaponInfo* data = item->GetWpnData( );
	if( !data )
		return;

	Color col = g_menu.main.visuals.item_color.get( );
	col.a( ) = 0xb4;

	// render bomb in green.
	if( item->is( HASH( "CC4" ) ) )
		render::esp_small.string( screen.x, screen.y, { 150, 200, 60, 0xb4 }, XOR( "BOMB" ), render::ALIGN_CENTER );

	// if not bomb
	// normal item, get its name.
	else {
		std::string name{ item->GetLocalizedName( ) };

		// smallfonts needs uppercase.
		std::transform( name.begin( ), name.end( ), name.begin( ), ::toupper );

		render::esp_small.string( screen.x, screen.y, col, name, render::ALIGN_CENTER );
	}

	if( !g_menu.main.visuals.ammo.get( ) )
		return;

	// nades do not have ammo.
	if( data->m_weapon_type == WEAPONTYPE_GRENADE || data->m_weapon_type == WEAPONTYPE_KNIFE )
		return;

	if( item->m_iItemDefinitionIndex( ) == 0 || item->m_iItemDefinitionIndex( ) == C4 )
		return;

	std::string ammo = tfm::format( XOR( "(%i/%i)" ), item->m_iClip1( ), item->m_iPrimaryReserveAmmoCount( ) );
	render::esp_small.string( screen.x, screen.y - render::esp_small.m_size.m_height - 1, col, ammo, render::ALIGN_CENTER );
}

void Visuals::OffScreen( Player* player, int alpha ) {
	vec3_t view_origin, target_pos, delta;
	vec2_t screen_pos, offscreen_pos;
	float  leeway_x, leeway_y, radius, offscreen_rotation;
	bool   is_on_screen;
	Vertex verts[ 3 ], verts_outline[ 3 ];
	Color  color;

	// todo - dex; move this?
	static auto get_offscreen_data = [ ] ( const vec3_t& delta, float radius, vec2_t& out_offscreen_pos, float& out_rotation ) {
		ang_t  view_angles( g_csgo.m_view_render->m_view.m_angles );
		vec3_t fwd, right, up( 0.f, 0.f, 1.f );
		float  front, side, yaw_rad, sa, ca;

		// get viewport angles forward directional vector.
		math::AngleVectors( view_angles, &fwd );

		// convert viewangles forward directional vector to a unit vector.
		fwd.z = 0.f;
		fwd.normalize( );

		// calculate front / side positions.
		right = up.cross( fwd );
		front = delta.dot( fwd );
		side = delta.dot( right );

		// setup offscreen position.
		out_offscreen_pos.x = radius * -side;
		out_offscreen_pos.y = radius * -front;

		// get the rotation ( yaw, 0 - 360 ).
		out_rotation = math::rad_to_deg( std::atan2( out_offscreen_pos.x, out_offscreen_pos.y ) + math::pi );

		// get needed sine / cosine values.
		yaw_rad = math::deg_to_rad( -out_rotation );
		sa = std::sin( yaw_rad );
		ca = std::cos( yaw_rad );

		// rotate offscreen position around.
		out_offscreen_pos.x = ( int ) ( ( g_cl.m_width / 2.f ) + ( radius * sa ) );
		out_offscreen_pos.y = ( int ) ( ( g_cl.m_height / 2.f ) - ( radius * ca ) );
	};

	/*if (!g_menu.main.players.offscreen.get())
		return*/

	if( !g_cl.m_processing || !g_cl.m_local->enemy( player ) )
		return;

	// get the player's center screen position.
	target_pos = player->WorldSpaceCenter( );
	is_on_screen = render::WorldToScreen( target_pos, screen_pos );

	// give some extra room for screen position to be off screen.
	leeway_x = g_cl.m_width / 18.f;
	leeway_y = g_cl.m_height / 18.f;

	// origin is not on the screen at all, get offscreen position data and start rendering.
	if( !is_on_screen
		|| screen_pos.x < -leeway_x
		|| screen_pos.x >( g_cl.m_width + leeway_x )
		|| screen_pos.y < -leeway_y
		|| screen_pos.y >( g_cl.m_height + leeway_y ) ) {

		// get viewport origin.
		view_origin = g_csgo.m_view_render->m_view.m_origin;

		// get direction to target.
		delta = ( target_pos - view_origin ).normalized( );

		// note - dex; this is the 'YRES' macro from the source sdk.
		radius = 200.f * ( g_cl.m_height / 480.f );

		// get the data we need for rendering.
		get_offscreen_data( delta, radius, offscreen_pos, offscreen_rotation );

		// bring rotation back into range... before rotating verts, sine and cosine needs this value inverted.
		// note - dex; reference: 
		// https://github.com/VSES/SourceEngine2007/blob/43a5c90a5ada1e69ca044595383be67f40b33c61/src_main/game/client/tf/tf_hud_damageindicator.cpp#L182
		offscreen_rotation = -offscreen_rotation;

		// setup vertices for the triangle.
		verts[ 0 ] = { offscreen_pos.x, offscreen_pos.y };        // 0,  0
		verts[ 1 ] = { offscreen_pos.x - 12.f, offscreen_pos.y + 24.f }; // -1, 1
		verts[ 2 ] = { offscreen_pos.x + 12.f, offscreen_pos.y + 24.f }; // 1,  1

		// setup verts for the triangle's outline.
		verts_outline[ 0 ] = { verts[ 0 ].m_pos.x - 1.f, verts[ 0 ].m_pos.y - 1.f };
		verts_outline[ 1 ] = { verts[ 1 ].m_pos.x - 1.f, verts[ 1 ].m_pos.y + 1.f };
		verts_outline[ 2 ] = { verts[ 2 ].m_pos.x + 1.f, verts[ 2 ].m_pos.y + 1.f };

		// rotate all vertices to point towards our target.
		verts[ 0 ] = render::RotateVertex( offscreen_pos, verts[ 0 ], offscreen_rotation );
		verts[ 1 ] = render::RotateVertex( offscreen_pos, verts[ 1 ], offscreen_rotation );
		verts[ 2 ] = render::RotateVertex( offscreen_pos, verts[ 2 ], offscreen_rotation );
		// verts_outline[ 0 ] = render::RotateVertex( offscreen_pos, verts_outline[ 0 ], offscreen_rotation );
		// verts_outline[ 1 ] = render::RotateVertex( offscreen_pos, verts_outline[ 1 ], offscreen_rotation );
		// verts_outline[ 2 ] = render::RotateVertex( offscreen_pos, verts_outline[ 2 ], offscreen_rotation );

		// todo - dex; finish this, i want it.
		// auto &damage_data = m_offscreen_damage[ player->index( ) ];
		// 
		// // the local player was damaged by another player recently.
		// if( damage_data.m_time > 0.f ) {
		//     // // only a small amount of time left, start fading into white again.
		//     // if( damage_data.m_time < 1.f ) {
		//     //     // calculate step needed to reach 255 in 1 second.
		//     //     // float step = UINT8_MAX / ( 1000.f * g_csgo.m_globals->m_frametime );
		//     //     float step = ( 1.f / g_csgo.m_globals->m_frametime ) / UINT8_MAX;
		//     //     
		//     //     // increment the new value for the color.
		//     //     // if( damage_data.m_color_step < 255.f )
		//     //         damage_data.m_color_step += step;
		//     // 
		//     //     math::clamp( damage_data.m_color_step, 0.f, 255.f );
		//     // 
		//     //     damage_data.m_color.g( ) = (uint8_t)damage_data.m_color_step;
		//     //     damage_data.m_color.b( ) = (uint8_t)damage_data.m_color_step;
		//     // }
		//     // 
		//     // g_cl.print( "%f %f %u %u %u\n", damage_data.m_time, damage_data.m_color_step, damage_data.m_color.r( ), damage_data.m_color.g( ), damage_data.m_color.b( ) );
		//     
		//     // decrement timer.
		//     damage_data.m_time -= g_csgo.m_globals->m_frametime;
		// }
		// 
		// else
		//     damage_data.m_color = colors::white;

		// render!
		color = Color(255, 255, 255); // damage_data.m_color;
		color.a( ) = ( alpha == 255 ) ? alpha : alpha / 2;

		g_csgo.m_surface->DrawSetColor( color );
		g_csgo.m_surface->DrawTexturedPolygon( 3, verts );

		// g_csgo.m_surface->DrawSetColor( colors::black );
		// g_csgo.m_surface->DrawTexturedPolyLine( 3, verts_outline );
	}
}

void Visuals::DrawPlayer( Player* player ) {
	constexpr float MAX_DORMANT_TIME = 10.f;
	constexpr float DORMANT_FADE_TIME = MAX_DORMANT_TIME / 2.f;

	Rect		  box;
	player_info_t info;
	Color		  color;

	// get player index.
	int index = player->index( );

	// get reference to array variable.
	float& opacity = m_opacities[ index - 1 ];
	bool& draw = m_draw[ index - 1 ];

	// opacity should reach 1 in 300 milliseconds.
	constexpr int frequency = 1.f / 0.3f;

	// the increment / decrement per frame.
	float step = frequency * g_csgo.m_globals->m_frametime;

	// is player enemy.
	bool enemy = player->enemy( g_cl.m_local );
	bool dormant = player->dormant( );

	if( g_menu.main.visuals.enemy_radar.get( ) && enemy && !dormant )
		player->m_bSpotted( ) = true;

	// we can draw this player again.
	if( !dormant )
		draw = true;

	if( !draw )
		return;

	// if non-dormant	-> increment
	// if dormant		-> decrement
	dormant ? opacity -= step : opacity += step;

	// is dormant esp enabled for this player.
	bool dormant_esp = enemy;

	// clamp the opacity.
	math::clamp( opacity, 0.f, 1.f );
	if( !opacity && !dormant_esp )
		return;

	// stay for x seconds max.
	float dt = g_csgo.m_globals->m_curtime - player->m_flSimulationTime( );
	if( dormant && dt > MAX_DORMANT_TIME )
		return;

	// calculate alpha channels.
	int alpha = ( int ) ( 255.f * opacity );
	int low_alpha = ( int ) ( 179.f * opacity );

	// get color based on enemy or not.
	color = enemy ? g_menu.main.players.box_enemy.get( ) : g_menu.main.players.box_friendly.get( );

	if( dormant && dormant_esp ) {
		alpha = 112;
		low_alpha = 80;

		// fade.
		if( dt > DORMANT_FADE_TIME ) {
			// for how long have we been fading?
			float faded = ( dt - DORMANT_FADE_TIME );
			float scale = 1.f - ( faded / DORMANT_FADE_TIME );

			alpha *= scale;
			low_alpha *= scale;
		}

		// override color.
		color = { 112, 112, 112 };
	}

	// override alpha.
	color.a( ) = alpha;

	// get player info.
	if( !g_csgo.m_engine->GetPlayerInfo( index, &info ) )
		return;

	// run offscreen ESP.
	OffScreen( player, alpha );

	// attempt to get player box.
	if( !GetPlayerBoxRect( player, box ) ) {
		// OffScreen( player );
		return;
	}

	// DebugAimbotPoints( player );

	bool bone_esp = ( enemy && g_menu.main.players.skeleton.get( 0 ) ) || ( !enemy && g_menu.main.players.skeleton.get( 1 ) );
	if( bone_esp )
		DrawSkeleton( player, alpha );

	// is box esp enabled for this player.
	bool box_esp = ( enemy && g_menu.main.players.box.get( 0 ) ) || ( !enemy && g_menu.main.players.box.get( 1 ) );

	// render box if specified.
	if( box_esp )
		render::rect_outlined( box.x, box.y, box.w, box.h, color, { 10, 10, 10, low_alpha } );

	// is name esp enabled for this player.
	bool name_esp = ( enemy && g_menu.main.players.name.get( 0 ) ) || ( !enemy && g_menu.main.players.name.get( 1 ) );

	// draw name.
	if( name_esp ) {
		// fix retards with their namechange meme 
		// the point of this is overflowing unicode compares with hardcoded buffers, good hvh strat
		std::string name{ std::string( info.m_name ).substr( 0, 24 ) };

		Color clr = g_menu.main.players.name_color.get( );
		// override alpha.
		clr.a( ) = low_alpha;

		render::esp.string( box.x + box.w / 2, box.y - render::esp.m_size.m_height, clr, name, render::ALIGN_CENTER );
	}

	// is health esp enabled for this player.
	bool health_esp = ( enemy && g_menu.main.players.health.get( 0 ) ) || ( !enemy && g_menu.main.players.health.get( 1 ) );
	AimPlayer* data = &g_aimbot.m_players[player->index() - 1];
	if( health_esp ) {
		int y = box.y + 1;
		int h = box.h - 2;

		// retarded servers that go above 100 hp..
		int hp = std::min( 100, player->m_iHealth( ) );

		// calculate hp bar color.
		int r = std::min( ( 510 * ( 100 - hp ) ) / 100, 255 );
		int g = std::min( ( 510 * hp ) / 100, 255 );

		// get hp bar height.
		int fill = ( int ) std::round( hp * h / 100.f );

		// render background.
		render::rect_filled( box.x - 6, y - 1, 4, h + 2, { 10, 10, 10, low_alpha } );

		// render actual bar.
		render::rect( box.x - 5, y + h - fill, 2, fill, { r, g, 0, alpha } );

		// if hp is below max, draw a string.
		if( hp < 100 )
			render::esp_small.string( box.x - 5, y + ( h - fill ) - 5, { 255, 255, 255, low_alpha }, std::to_string( hp ), render::ALIGN_CENTER );
	}

	// draw flags.
	{
		std::vector< std::pair< std::string, Color > > flags;

		auto items = enemy ? g_menu.main.players.flags_enemy.GetActiveIndices( ) : g_menu.main.players.flags_friendly.GetActiveIndices( );
		auto rage_items = g_menu.main.aimbot.rage_flags.GetActiveIndices();
		
		// NOTE FROM NITRO TO DEX -> stop removing my iterator loops, i do it so i dont have to check the size of the vector
		// with range loops u do that to do that.
		for( auto it = items.begin( ); it != items.end( ); ++it ) {

			// money.
			if( *it == 0 )
				flags.push_back( { tfm::format( XOR( "$%i" ), player->m_iAccount( ) ), { 150, 200, 60, low_alpha } } );

			// armor.
			if( *it == 1 ) {
				// helmet and kevlar.
				if( player->m_bHasHelmet( ) && player->m_ArmorValue( ) > 0 )
					flags.push_back( { XOR( "HK" ), { 255, 255, 255, low_alpha } } );

				// only helmet.
				else if( player->m_bHasHelmet( ) )
					flags.push_back( { XOR( "H" ), { 255, 255, 255, low_alpha } } );

				// only kevlar.
				else if( player->m_ArmorValue( ) > 0 )
					flags.push_back( { XOR( "K" ), { 255, 255, 255, low_alpha } } );
			}

			// scoped.
			if( *it == 2 && player->m_bIsScoped( ) )
				flags.push_back( { XOR( "ZOOM" ), { 60, 180, 225, low_alpha } } );

			// flashed.
			if( *it == 3 && player->m_flFlashBangTime( ) > 0.f )
				flags.push_back( { XOR( "FLASHED" ), { 255, 255, 0, low_alpha } } );

			// reload.
			if( *it == 4 ) {
				// get ptr to layer 1.
				C_AnimationLayer* layer1 = &player->m_AnimOverlay( )[ 1 ];

				// check if reload animation is going on.
				if( layer1->m_weight != 0.f && player->GetSequenceActivity( layer1->m_sequence ) == 967 /* ACT_CSGO_RELOAD */ )
					flags.push_back( { XOR( "RELOAD" ), { 60, 180, 225, low_alpha } } );
			}

			// bomb.
			if( *it == 5 && player->HasC4( ) )
				flags.push_back( { XOR( "BOMB" ), { 255, 0, 0, low_alpha } } );

			

		
		}

		for (auto rage_items = items.begin(); rage_items != items.end(); ++rage_items) {

	

			if (*rage_items == 0) {
				{
					auto m_weapon = g_cl.m_local->GetActiveWeapon();
					if (m_weapon && !m_weapon->IsKnife())
					{
						auto data = m_weapon->GetWpnData();

						if (data->m_damage >= (int)std::round(player->m_iHealth()))
							flags.push_back({ XOR("LETHAL"), { 163, 169, 111, low_alpha } });
					}
				}
			}

			if (*rage_items == 1) {
				if (dormant) {
					flags.push_back({ g_resolver.resolver_state[player->index()], { 130,130,130, low_alpha } });
				}
				else {
					flags.push_back({ g_resolver.resolver_state[player->index()], { 255,255,255, low_alpha} });
				}
			}

			if (*rage_items == 2 && data) {
				LagRecord* current = !data->m_records.empty() ? data->m_records.front().get() : nullptr;
				if (current && !current->dormant()) {
					if (current->m_shift < -1 && g_menu.main.aimbot.rage_flags.get(2))
						flags.push_back({ XOR("DT"), {255, 255, 255, low_alpha} });
				}
			}

			if (*rage_items == 3) {
				if (!g_cl.vader_user.empty()) {
					if (std::find(g_cl.vader_user.begin(), g_cl.vader_user.end(), info.m_user_id) != g_cl.vader_user.end()) {

						flags.push_back({ "VADER", dormant ? Color(220, 220, 220, low_alpha) : Color(255, 0, 0, low_alpha) });
					}
				}

				if (!g_cl.vader_beta.empty()) {
					if (std::find(g_cl.vader_beta.begin(), g_cl.vader_beta.end(), info.m_user_id) != g_cl.vader_beta.end()) {

						flags.push_back({ "VADER", dormant ? Color(220, 220, 220, low_alpha) : Color(255, 0, 0, low_alpha) });
					}
				}

				if (!g_cl.vader_dev.empty()) {
					if (std::find(g_cl.vader_dev.begin(), g_cl.vader_dev.end(), info.m_user_id) != g_cl.vader_dev.end()) {

						flags.push_back({ "VADER", dormant ? Color(220, 220, 220, low_alpha) : Color(255, 0, 0, low_alpha) });
					}
				}

				if (!g_cl.vader_crack.empty()) {
					if (std::find(g_cl.vader_crack.begin(), g_cl.vader_crack.end(), info.m_user_id) != g_cl.vader_crack.end()) {

						flags.push_back({ "VADER", dormant ? Color(220, 220, 220, low_alpha) : Color(255, 0, 0, low_alpha) });
					}
				}

				if (!g_cl.cheese_leak_lol.empty()) {
					if (std::find(g_cl.cheese_leak_lol.begin(), g_cl.cheese_leak_lol.end(), info.m_user_id) != g_cl.cheese_leak_lol.end()) {

						flags.push_back({ "CHEESE", dormant ? Color(220, 220, 220, low_alpha) : Color(255, 0, 0, low_alpha) });
					}
				}

				if (!g_cl.cheese_lol_beta.empty()) {
					if (std::find(g_cl.cheese_lol_beta.begin(), g_cl.cheese_lol_beta.end(), info.m_user_id) != g_cl.cheese_lol_beta.end()) {

						flags.push_back({ "CHEESE", dormant ? Color(220, 220, 220, low_alpha) : Color(255, 0, 0, low_alpha) });
					}
				}
			}
		}
		// iterate flags.
		for( size_t i{ }; i < flags.size( ); ++i ) {
			// get flag job (pair).
			const auto& f = flags[ i ];

			int offset = i * ( render::esp_small.m_size.m_height );

			// draw flag.
			render::esp_small.string( box.x + box.w + 2, box.y + offset, f.second, f.first );
		}
	}

	// draw bottom bars.
	{
		int  offset{ 0 };

		// draw lby update bar.
		if( enemy && g_menu.main.players.lby_update.get( ) ) {


			// make sure everything is valid.
			if( data && data->m_moved &&data->m_records.size( ) ) {
				// grab lag record.
				LagRecord* current = data->m_records.front( ).get( );

				if( current ) {
					if( !( current->m_velocity.length_2d( ) > 0.1 && !current->m_fake_walk ) && data->m_body_index <= 3 ) {
						// calculate box width
						float cycle = std::clamp<float>( data->m_body_update - current->m_anim_time, 0.f, 1.0f );
						float width = ( box.w * cycle ) / 1.1f;

						if( width > 0.f ) {
							// draw.
							render::rect_filled( box.x, box.y + box.h + 2, box.w, 4, { 10, 10, 10, low_alpha } );

							Color clr = g_menu.main.players.lby_update_color.get( );
							clr.a( ) = alpha;
							render::rect( box.x + 1, box.y + box.h + 3, width, 2, clr );

							// move down the offset to make room for the next bar.
							offset += 5;
						}
					}
				}
			}
		}

		// draw weapon.
		if( ( enemy && g_menu.main.players.weapon.get( 0 ) ) || ( !enemy && g_menu.main.players.weapon.get( 1 ) ) ) {
			Weapon* weapon = player->GetActiveWeapon( );
			if( weapon ) {
				WeaponInfo* data = weapon->GetWpnData( );
				if( data ) {
					int bar;
					float scale;

					// the maxclip1 in the weaponinfo
					int max = data->m_max_clip1;
					int current = weapon->m_iClip1( );

					C_AnimationLayer* layer1 = &player->m_AnimOverlay( )[ 1 ];

					// set reload state.
					bool reload = ( layer1->m_weight != 0.f ) && ( player->GetSequenceActivity( layer1->m_sequence ) == 967 );

					// ammo bar.
					if( max != -1 && g_menu.main.players.ammo.get( ) ) {
						// check for reload.
						if( reload )
							scale = layer1->m_cycle;

						// not reloading.
						// make the division of 2 ints produce a float instead of another int.
						else
							scale = ( float ) current / max;

						// relative to bar.
						bar = ( int ) std::round( ( box.w - 2 ) * scale );

						// draw.
						render::rect_filled( box.x, box.y + box.h + 2 + offset, box.w, 4, { 10, 10, 10, low_alpha } );

						Color clr = g_menu.main.players.ammo_color.get( );
						clr.a( ) = alpha;
						render::rect( box.x + 1, box.y + box.h + 3 + offset, bar, 2, clr );

						// less then a 5th of the bullets left.
						if( current <= ( int ) std::round( max / 5 ) && !reload )
							render::esp_small.string( box.x + bar, box.y + box.h + offset, { 255, 255, 255, low_alpha }, std::to_string( current ), render::ALIGN_CENTER );

						offset += 6;
					}

					// text.
					if( g_menu.main.players.weapon_mode.get( ) == 0 ) {
						// construct std::string instance of localized weapon name.
						std::string name{ weapon->GetLocalizedName( ) };

						// smallfonts needs upper case.
						std::transform( name.begin( ), name.end( ), name.begin( ), ::toupper );

						render::esp_small.string( box.x + box.w / 2, box.y + box.h + offset, { 255, 255, 255, low_alpha }, name, render::ALIGN_CENTER );
					}

					// icons.
					else if( g_menu.main.players.weapon_mode.get( ) == 1 ) {
						// icons are super fat..
						// move them back up.
						offset -= 5;

						std::string icon = tfm::format( XOR( "%c" ), m_weapon_icons[ weapon->m_iItemDefinitionIndex( ) ] );
						render::cs.string( box.x + box.w / 2, box.y + box.h + offset, { 255, 255, 255, low_alpha }, icon, render::ALIGN_CENTER );
					}
				}
			}
		}
	}
}

void Visuals::DrawPlantedC4( ) {
	bool        mode_2d, mode_3d, is_visible;
	float       explode_time_diff, dist, range_damage;
	vec3_t      dst, to_target;
	int         final_damage;
	std::string time_str, damage_str;
	Color       damage_color;
	vec2_t      screen_pos;

	static auto scale_damage = [ ] ( float damage, int armor_value ) {
		float new_damage, armor;

		if( armor_value > 0 ) {
			new_damage = damage * 0.5f;
			armor = ( damage - new_damage ) * 0.5f;

			if( armor > ( float ) armor_value ) {
				armor = ( float ) armor_value * 2.f;
				new_damage = damage - armor;
			}

			damage = new_damage;
		}

		return std::max( 0, ( int ) std::floor( damage ) );
	};

	// store menu vars for later.
	mode_2d = g_menu.main.visuals.planted_c4.get( 0 );
	mode_3d = g_menu.main.visuals.planted_c4.get( 1 );
	if( !mode_2d && !mode_3d )
		return;

	// bomb not currently active, do nothing.
	if( !m_c4_planted )
		return;

	// calculate bomb damage.
	// references:
	//     https://github.com/VSES/SourceEngine2007/blob/43a5c90a5ada1e69ca044595383be67f40b33c61/se2007/game/shared/cstrike/weapon_c4.cpp#L271
	//     https://github.com/VSES/SourceEngine2007/blob/43a5c90a5ada1e69ca044595383be67f40b33c61/se2007/game/shared/cstrike/weapon_c4.cpp#L437
	//     https://github.com/ValveSoftware/source-sdk-2013/blob/master/sp/src/game/shared/sdk/sdk_gamerules.cpp#L173
	{
		// get our distance to the bomb.
		// todo - dex; is dst right? might need to reverse CBasePlayer::BodyTarget...
		dst = g_cl.m_local->WorldSpaceCenter( );
		to_target = m_planted_c4_explosion_origin - dst;
		dist = to_target.length( );

		// calculate the bomb damage based on our distance to the C4's explosion.
		range_damage = m_planted_c4_damage * std::exp( ( dist * dist ) / ( ( m_planted_c4_radius_scaled * -2.f ) * m_planted_c4_radius_scaled ) );

		// now finally, scale the damage based on our armor (if we have any).
		final_damage = scale_damage( range_damage, g_cl.m_local->m_ArmorValue( ) );
	}

	// m_flC4Blow is set to gpGlobals->curtime + m_flTimerLength inside CPlantedC4.
	explode_time_diff = m_planted_c4_explode_time - g_csgo.m_globals->m_curtime;

	// get formatted strings for bomb.
	time_str = tfm::format( XOR( "%.2f" ), explode_time_diff );
	damage_str = tfm::format( XOR( "%i" ), final_damage );

	// get damage color.
	damage_color = ( final_damage < g_cl.m_local->m_iHealth( ) ) ? colors::white : colors::red;

	// finally do all of our rendering.
	is_visible = render::WorldToScreen( m_planted_c4_explosion_origin, screen_pos );

	// 'on screen (2D)'.
	if( mode_2d ) {
		// g_cl.m_height - 80 - ( 30 * i )
		// 80 - ( 30 * 2 ) = 20

		// render::menu_shade.string( 60, g_cl.m_height - 20 - ( render::hud_size.m_height / 2 ), 0xff0000ff, "", render::hud );

		// todo - dex; move this next to indicators?

		if( explode_time_diff > 0.f )
			render::esp.string( 2, 65, colors::white, time_str, render::ALIGN_LEFT );

		if( g_cl.m_local->alive( ) )
			render::esp.string( 2, 65 + render::esp.m_size.m_height, damage_color, damage_str, render::ALIGN_LEFT );
	}

	// 'on bomb (3D)'.
	if( mode_3d && is_visible ) {
		if( explode_time_diff > 0.f )
			render::esp_small.string( screen_pos.x, screen_pos.y, colors::white, time_str, render::ALIGN_CENTER );

		// only render damage string if we're alive.
		if( g_cl.m_local->alive( ) )
			render::esp_small.string( screen_pos.x, ( int ) screen_pos.y + render::esp_small.m_size.m_height, damage_color, damage_str, render::ALIGN_CENTER );
	}
}

bool Visuals::GetPlayerBoxRect( Player* player, Rect& box ) {
	//matrix3x4_t& tran_frame = player->coord_frame();

	//const vec3_t min = player->m_vecMins();
	//const vec3_t max = player->m_vecMaxs();

	//vec3_t screen_boxes[8];

	//vec3_t points[] = {
	//	vec3_t(min.x, min.y, min.z),
	//	vec3_t(min.x, max.y, min.z),
	//	vec3_t(max.x, max.y, min.z),
	//	vec3_t(max.x, min.y, min.z),
	//	vec3_t(max.x, max.y, max.z),
	//	vec3_t(min.x, max.y, max.z),
	//	vec3_t(min.x, min.y, max.z),
	//	vec3_t(max.x, min.y, max.z)
	//};

	//for (int i = 0; i <= 7; i++)
	//	if (!render::world_to_screen(math::vector_transform(points[i], tran_frame), screen_boxes[i]))
	//		return false;

	//vec3_t box_array[] = {
	//	screen_boxes[3], // fl
	//	screen_boxes[5], // br
	//	screen_boxes[0], // bl
	//	screen_boxes[4], // fr
	//	screen_boxes[2], // fr
	//	screen_boxes[1], // br
	//	screen_boxes[6], // bl
	//	screen_boxes[7] // fl
	//};

	//float left = screen_boxes[3].x, bottom = screen_boxes[3].y, right = screen_boxes[3].x, top = screen_boxes[3].y;

	//for (int i = 0; i <= 7; i++) {
	//	if (left > box_array[i].x)
	//		left = box_array[i].x;

	//	if (bottom < box_array[i].y)
	//		bottom = box_array[i].y;

	//	if (right < box_array[i].x)
	//		right = box_array[i].x;

	//	if (top > box_array[i].y)
	//		top = box_array[i].y;
	//}

	//box.x = static_cast<int>(left);
	//box.y = static_cast<int>(top);
	//box.w = static_cast<int>(right) - static_cast<int>(left);
	//box.h = static_cast<int>(bottom) - static_cast<int>(top);

	//return true;

	vec3_t origin, mins, maxs;
	vec2_t bottom, top;

	// get interpolated origin.
	origin = player->GetAbsOrigin();

	// get hitbox bounds.
	player->ComputeHitboxSurroundingBox(&mins, &maxs);

	// correct x and y coordinates.
	mins = { origin.x, origin.y, mins.z };
	maxs = { origin.x, origin.y, maxs.z + 8.f };

	if (!render::WorldToScreen(mins, bottom) || !render::WorldToScreen(maxs, top))
		return false;

	box.h = bottom.y - top.y;
	box.w = box.h / 2.f;
	box.x = bottom.x - (box.w / 2.f);
	box.y = bottom.y - box.h;

	return true;
}

void Visuals::DrawHistorySkeleton( Player* player, int opacity ) {
	const model_t* model;
	studiohdr_t* hdr;
	mstudiobone_t* bone;
	AimPlayer* data;
	LagRecord* record;
	int           parent;
	vec3_t        bone_pos, parent_pos;
	vec2_t        bone_pos_screen, parent_pos_screen;

	if( !g_menu.main.misc.fake_latency.get( ) )
		return;

	// get player's model.
	model = player->GetModel( );
	if( !model )
		return;

	// get studio model.
	hdr = g_csgo.m_model_info->GetStudioModel( model );
	if( !hdr )
		return;

	data = &g_aimbot.m_players[ player->index( ) - 1 ];
	if( !data )
		return;

	record = g_resolver.FindLastRecord( data );
	if( !record )
		return;

	for( int i{ }; i < hdr->m_num_bones; ++i ) {
		// get bone.
		bone = hdr->GetBone( i );
		if( !bone || !( bone->m_flags & BONE_USED_BY_HITBOX ) )
			continue;

		// get parent bone.
		parent = bone->m_parent;
		if( parent == -1 )
			continue;

		// resolve main bone and parent bone positions.
		record->m_bones->get_bone( bone_pos, i );
		record->m_bones->get_bone( parent_pos, parent );

		Color clr = player->enemy( g_cl.m_local ) ? g_menu.main.players.skeleton_enemy.get( ) : g_menu.main.players.skeleton_friendly.get( );
		clr.a( ) = opacity;

		// world to screen both the bone parent bone then draw.
		if( render::WorldToScreen( bone_pos, bone_pos_screen ) && render::WorldToScreen( parent_pos, parent_pos_screen ) )
			render::line( bone_pos_screen.x, bone_pos_screen.y, parent_pos_screen.x, parent_pos_screen.y, clr );
	}
}

void Visuals::DrawSkeleton( Player* player, int opacity ) {
	const model_t* model;
	studiohdr_t* hdr;
	mstudiobone_t* bone;
	int           parent;
	BoneArray     matrix[ 128 ];
	vec3_t        bone_pos, parent_pos;
	vec2_t        bone_pos_screen, parent_pos_screen;

	// get player's model.
	model = player->GetModel( );
	if( !model )
		return;

	// get studio model.
	hdr = g_csgo.m_model_info->GetStudioModel( model );
	if( !hdr )
		return;

	// get bone matrix.
	if( !player->SetupBones( matrix, 128, BONE_USED_BY_ANYTHING, g_csgo.m_globals->m_curtime ) )
		return;

	for( int i{ }; i < hdr->m_num_bones; ++i ) {
		// get bone.
		bone = hdr->GetBone( i );
		if( !bone || !( bone->m_flags & BONE_USED_BY_HITBOX ) )
			continue;

		// get parent bone.
		parent = bone->m_parent;
		if( parent == -1 )
			continue;

		// resolve main bone and parent bone positions.
		matrix->get_bone( bone_pos, i );
		matrix->get_bone( parent_pos, parent );

		Color clr = player->enemy( g_cl.m_local ) ? g_menu.main.players.skeleton_enemy.get( ) : g_menu.main.players.skeleton_friendly.get( );
		clr.a( ) = opacity;

		// world to screen both the bone parent bone then draw.
		if( render::WorldToScreen( bone_pos, bone_pos_screen ) && render::WorldToScreen( parent_pos, parent_pos_screen ) )
			render::line( bone_pos_screen.x, bone_pos_screen.y, parent_pos_screen.x, parent_pos_screen.y, clr );
	}
}

void Visuals::RenderGlow( ) {
	Color   color;
	Player* player;

	if( !g_cl.m_local )
		return;

	if( !g_csgo.m_glow->m_object_definitions.Count( ) )
		return;

	float blend = g_menu.main.players.glow_blend.get( ) / 100.f;

	for( int i{ }; i < g_csgo.m_glow->m_object_definitions.Count( ); ++i ) {
		GlowObjectDefinition_t* obj = &g_csgo.m_glow->m_object_definitions[ i ];

		// skip non-players.
		if( !obj->m_entity || !obj->m_entity->IsPlayer( ) )
			continue;

		// get player ptr.
		player = obj->m_entity->as< Player* >( );

		if( player->m_bIsLocalPlayer( ) )
			continue;

		// get reference to array variable.
		float& opacity = m_opacities[ player->index( ) - 1 ];

		bool enemy = player->enemy( g_cl.m_local );

		if( enemy && !g_menu.main.players.glow.get( 0 ) )
			continue;

		if( !enemy && !g_menu.main.players.glow.get( 1 ) )
			continue;

		// enemy color
		if( enemy )
			color = g_menu.main.players.glow_enemy.get( );

		// friendly color
		else
			color = g_menu.main.players.glow_friendly.get( );

		obj->m_render_occluded = true;
		obj->m_render_unoccluded = false;
		obj->m_render_full_bloom = false;
		obj->m_color = { ( float ) color.r( ) / 255.f, ( float ) color.g( ) / 255.f, ( float ) color.b( ) / 255.f };
		obj->m_alpha = opacity * blend;
	}
}

void Visuals::DrawHitboxMatrix( LagRecord* record, Color col, float time ) {
	const model_t* model;
	studiohdr_t* hdr;
	mstudiohitboxset_t* set;
	mstudiobbox_t* bbox;
	vec3_t             mins, maxs, origin;
	ang_t			   angle;

	model = record->m_player->GetModel( );
	if( !model )
		return;

	hdr = g_csgo.m_model_info->GetStudioModel( model );
	if( !hdr )
		return;

	set = hdr->GetHitboxSet( record->m_player->m_nHitboxSet( ) );
	if( !set )
		return;

	for( int i{ }; i < set->m_hitboxes; ++i ) {
		bbox = set->GetHitbox( i );
		if( !bbox )
			continue;

		// bbox.
		if( bbox->m_radius <= 0.f ) {
			// https://developer.valvesoftware.com/wiki/Rotation_Tutorial

			// convert rotation angle to a matrix.
			matrix3x4_t rot_matrix;
			g_csgo.AngleMatrix( bbox->m_angle, rot_matrix );

			// apply the rotation to the entity input space (local).
			matrix3x4_t matrix;
			math::ConcatTransforms( record->m_bones[ bbox->m_bone ], rot_matrix, matrix );

			// extract the compound rotation as an angle.
			ang_t bbox_angle;
			math::MatrixAngles( matrix, bbox_angle );

			// extract hitbox origin.
			vec3_t origin = matrix.GetOrigin( );

			// draw box.
			g_csgo.m_debug_overlay->AddBoxOverlay( origin, bbox->m_mins, bbox->m_maxs, bbox_angle, col.r( ), col.g( ), col.b( ), 0, time );
		}

		// capsule.
		else {
			// NOTE; the angle for capsules is always 0.f, 0.f, 0.f.

			// create a rotation matrix.
			matrix3x4_t matrix;
			g_csgo.AngleMatrix( bbox->m_angle, matrix );

			// apply the rotation matrix to the entity output space (world).
			math::ConcatTransforms( record->m_bones[ bbox->m_bone ], matrix, matrix );

			// get world positions from new matrix.
			math::VectorTransform( bbox->m_mins, matrix, mins );
			math::VectorTransform( bbox->m_maxs, matrix, maxs );

			g_csgo.m_debug_overlay->AddCapsuleOverlay( mins, maxs, bbox->m_radius, col.r( ), col.g( ), col.b( ), col.a( ), time, 0, 0 );
		}
	}
}

void Visuals::DrawBeams() {
	float time = g_csgo.m_globals->m_curtime;
	bool       is_final_impact;

	for (size_t i{ }; i < bulletImpactInfo.size() && !bulletImpactInfo.empty(); ++i) {
		auto& tr = bulletImpactInfo[i];

		// already rendering this impact, skip over it.
		if (tr.ignore)
			continue;

		// is this the final impact?
		// last impact in the vector, it's the final impact.
		if (i == (bulletImpactInfo.size() - 1))
			is_final_impact = true;

		// the current impact's tickbase is different than the next, it's the final impact.
		else if ((i + 1) < bulletImpactInfo.size() && tr.m_nTickbase != bulletImpactInfo.operator[ ](i + 1).m_nTickbase)
			is_final_impact = true;

		else
			is_final_impact = false;

		float delta = time - tr.m_flExpTime;

		// impact is too old, erase it.
		if (delta > 1.0f)
			bulletImpactInfo.erase(bulletImpactInfo.begin() + i);

		Color col = tr.m_cColor;

		if (is_final_impact) {

			BeamInfo_t beam_info;
			beam_info.m_vecStart = tr.m_vecStartPos;
			beam_info.m_vecEnd = tr.m_vecHitPos;
			beam_info.m_nType = 0;
			beam_info.m_pszModelName = ("sprites/purplelaser1.vmt");
			beam_info.m_nModelIndex = g_csgo.m_model_info->GetModelIndex(("sprites/purplelaser1.vmt"));
			beam_info.m_flHaloScale = 0.0f;
			beam_info.m_flLife = 3.f;
			beam_info.m_flWidth = 4.0f;
			beam_info.m_flEndWidth = 4.0f;
			beam_info.m_flFadeLength = 0.0f;
			beam_info.m_flAmplitude = 2.0f;
			beam_info.m_flBrightness = col.a();
			beam_info.m_flSpeed = 0.2f;
			beam_info.m_nStartFrame = 0;
			beam_info.m_flFrameRate = 0.f;
			beam_info.m_flRed = col.r();
			beam_info.m_flGreen = col.g();
			beam_info.m_flBlue = col.b();
			beam_info.m_nSegments = 2;
			beam_info.m_bRenderable = true;
			beam_info.m_nFlags = 0x100 | 0x200 | 0x8000;


			Beam_t* beam = g_csgo.m_beams->CreateBeamPoints(beam_info);
			if (beam) {
				g_csgo.m_beams->DrawBeam(beam);

				tr.ignore = true;
			}
			break;
		}
	}
}

void Visuals::PushBeamInfo(BulletImpactInfo beam_info) {
	bulletImpactInfo.emplace_back(beam_info);
}

void Visuals::DebugAimbotPoints( Player* player ) {
	std::vector< vec3_t > p2{ };

	AimPlayer* data = &g_aimbot.m_players.at( player->index( ) - 1 );
	if( !data || data->m_records.empty( ) )
		return;

	LagRecord* front = data->m_records.front( ).get( );
	if( !front || front->dormant( ) )
		return;

	// get bone matrix.
	BoneArray matrix[ 128 ];
	if( !g_bones.setup( player, matrix, front ) )
		return;

	data->SetupHitboxes( front, false );
	if( data->m_hitboxes.empty( ) )
		return;

	for( const auto& it : data->m_hitboxes ) {
		std::vector< vec3_t > p1{ };

		if( !data->SetupHitboxPoints( front, matrix, it.m_index, p1 ) )
			continue;

		for( auto& p : p1 )
			p2.push_back( p );
	}

	if( p2.empty( ) )
		return;

	for( auto& p : p2 ) {
		vec2_t screen;

		if( render::WorldToScreen( p, screen ) )
			render::rect_filled( screen.x, screen.y, 2, 2, { 0, 255, 255, 255 } );
	}
}

void Visuals::ImpactData()
{
	if (!g_cl.m_processing) return;

	if (!g_menu.main.misc.bullet_impacts.get()) return;

	//call this in fsn or whatever
	static auto last_count = 0;
	auto& client_impact_list = *(CUtlVector< client_hit_verify_t >*)((uintptr_t)g_cl.m_local + 0xBA84);

	Color color = g_menu.main.misc.client_impact.get();
	color.a() = (g_menu.main.misc.impact_alpha.get() * 2.55);

	for (auto i = client_impact_list.Count(); i > last_count; i--) {
		g_csgo.m_debug_overlay->AddBoxOverlay(client_impact_list[i - 1].pos, vec3_t(-2, -2, -2), vec3_t(2, 2, 2), ang_t(0, 0, 0), color.r(), color.g(), color.b(), color.a(), 4.f);
	}

	if (client_impact_list.Count() != last_count)
		last_count = client_impact_list.Count();
}