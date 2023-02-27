#include "includes.h"
#include <Windows.h>
#include <mmsystem.h>
#include <Lmcons.h>

Shots g_shots{ };

void Shots::OnShotFire(Player* target, float damage, int bullets, LagRecord* record) {
	// setup new shot data.
	ShotRecord shot;
	shot.m_target = target;
	shot.m_record = record;
	shot.m_time = g_csgo.m_globals->m_curtime;
	shot.m_lat = g_cl.m_latency;
	shot.m_damage = damage;
	shot.m_pos = g_cl.m_shoot_pos;
	shot.m_impacted = false;
	shot.m_confirmed = false;
	shot.m_hurt = false;
	shot.m_range = g_cl.m_weapon_info->m_range;

	// we are not shooting manually.
	// and this is the first bullet, only do this once.
	if (target && record) {
		// increment total shots on this player.
		AimPlayer* data = &g_aimbot.m_players[target->index() - 1];
		if (data)
			++data->m_shots;
	}

	// add to tracks.
	m_shots.push_front(shot);

	// no need to keep an insane amount of shots.
	while (m_shots.size() > 128)
		m_shots.pop_back();
}

void Shots::OnImpact(IGameEvent* evt) {
	int        attacker;
	vec3_t     pos, dir, start, end;
	float      time;

	// screw this.
	if (!evt || !g_cl.m_local)
		return;

	// get attacker, if its not us, screw it.
	attacker = g_csgo.m_engine->GetPlayerForUserID(evt->m_keys->FindKey(HASH("userid"))->GetInt());
	if (attacker != g_csgo.m_engine->GetLocalPlayer())
		return;

	// decode impact coordinates and convert to vec3.
	pos = {
		evt->m_keys->FindKey(HASH("x"))->GetFloat(),
		evt->m_keys->FindKey(HASH("y"))->GetFloat(),
		evt->m_keys->FindKey(HASH("z"))->GetFloat()
	};

	// get prediction time at this point.
	time = g_csgo.m_globals->m_curtime;

	// add to visual impacts if we have features that rely on it enabled.
	// todo - dex; need to match shots for this to have proper GetShootPosition, don't really care to do it anymore.
	if (g_menu.main.visuals.impact_beams.get())
		m_vis_impacts.push_back({ pos, g_cl.m_local->GetShootPosition(), g_cl.m_local->m_nTickBase() });

	if (g_menu.main.misc.bullet_impacts.get())
		g_csgo.m_debug_overlay->AddBoxOverlay(pos, vec3_t(-2, -2, -2), vec3_t(2, 2, 2), ang_t(0, 0, 0), 0, 0, 255, 127,
			g_csgo.m_cvar->FindVar(HASH("sv_showimpacts_time"))->GetFloat());

	// we did not take a shot yet.
	if (m_shots.empty())
		return;

	struct ShotMatch_t { float delta; ShotRecord* shot; };
	ShotMatch_t match;
	match.delta = std::numeric_limits< float >::max();
	match.shot = nullptr;

	// iterate all shots.
	for (auto& s : m_shots) {

		// this shot was already matched
		// with a 'bullet_impact' event.
		if (s.m_impacted)
			continue;

		// add the latency to the time when we shot.
		// to predict when we would receive this event.
		float predicted = s.m_time + s.m_lat;

		// get the delta between the current time
		// and the predicted arrival time of the shot.
		float delta = std::abs(time - predicted);

		// fuck this.
		if (delta > 1.f)
			continue;

		// store this shot as being the best for now.
		if (delta < match.delta) {
			match.delta = delta;
			match.shot = &s;
		}
	}

	// no valid shotrecord was found.
	ShotRecord* shot = match.shot;
	if (!shot)
		return;

	// this shot was matched.
	shot->m_impacted = true;
	shot->m_impact_pos = pos;
}

void Shots::OnHurt(IGameEvent* evt) {
	int         attacker, victim, group, hp;
	float       time, damage;
	std::string name;

	if (!evt || !g_cl.m_local)
		return;

	attacker = g_csgo.m_engine->GetPlayerForUserID(evt->m_keys->FindKey(HASH("attacker"))->GetInt());
	victim = g_csgo.m_engine->GetPlayerForUserID(evt->m_keys->FindKey(HASH("userid"))->GetInt());

	// skip invalid player indexes.
	// should never happen? world entity could be attacker, or a nade that hits you.
	if (attacker < 1 || attacker > 64 || victim < 1 || victim > 64)
		return;

	if (g_menu.main.misc.notifications.get(0) && attacker != g_csgo.m_engine->GetLocalPlayer() && victim == g_csgo.m_engine->GetLocalPlayer()) {
		group = evt->m_keys->FindKey(HASH("hitgroup"))->GetInt();

		if (group == HITGROUP_GEAR)
			return;

		// get the player that was hurt.
		Player* target = g_csgo.m_entlist->GetClientEntity< Player* >(victim);
		if (!target)
			return;

		// get player info.
		player_info_t info;
		if (!g_csgo.m_engine->GetPlayerInfo(attacker, &info))
			return;

		// get player name;
		name = std::string(info.m_name).substr(0, 24);

		// get damage reported by the server.
		damage = (float)evt->m_keys->FindKey(HASH("dmg_health"))->GetInt();

		// get remaining hp.
		hp = evt->m_keys->FindKey(HASH("health"))->GetInt();

		std::string out = tfm::format(XOR("harmed by %s in the %s for %i\n"), name, m_groups[group], (int)damage);
		g_notify.add(out, colors::white);
	}

	// we were not the attacker or we hurt ourselves.
	if (attacker != g_csgo.m_engine->GetLocalPlayer() || victim == g_csgo.m_engine->GetLocalPlayer())
		return;

	// get hitgroup.
	// players that get naded ( DMG_BLAST ) or stabbed seem to be put as HITGROUP_GENERIC.
	group = evt->m_keys->FindKey(HASH("hitgroup"))->GetInt();

	// invalid hitgroups ( note - dex; HITGROUP_GEAR isn't really invalid, seems to be set for hands and stuff? ).
	if (group == HITGROUP_GEAR)
		return;

	if (group == HITGROUP_HEAD)
		iHeadshot = true;
	else
		iHeadshot = false;

	// get the player that was hurt.
	Player* target = g_csgo.m_entlist->GetClientEntity< Player* >(victim);
	if (!target)
		return;

	// get player info.
	player_info_t info;
	if (!g_csgo.m_engine->GetPlayerInfo(victim, &info))
		return;

	// get player name;
	name = std::string(info.m_name).substr(0, 24);

	// get damage reported by the server.
	damage = (float)evt->m_keys->FindKey(HASH("dmg_health"))->GetInt();

	// get remaining hp.
	hp = evt->m_keys->FindKey(HASH("health"))->GetInt();

	// get prediction time at this point.
	time = game::TICKS_TO_TIME(g_cl.m_local->m_nTickBase());

	// hitmarker.
	if (g_menu.main.misc.hitmarker.get()) {
		g_visuals.m_hit_duration = 2.f;
		g_visuals.m_hit_start = g_csgo.m_globals->m_curtime;
		g_visuals.m_hit_end = g_visuals.m_hit_start + g_visuals.m_hit_duration;

		// bind to draw
		iHitDmg = damage;

		// get interpolated origin.
		iPlayerOrigin = target->GetAbsOrigin();

		// get hitbox bounds.
		target->ComputeHitboxSurroundingBox(&iPlayermins, &iPlayermaxs);

		// correct x and y coordinates.
		iPlayermins = { iPlayerOrigin.x, iPlayerOrigin.y, iPlayermins.z };
		iPlayermaxs = { iPlayerOrigin.x, iPlayerOrigin.y, iPlayermaxs.z + 8.f };
	}

	if (g_menu.main.misc.hitmarker.get())
		g_csgo.m_sound->EmitAmbientSound(XOR("buttons/arena_switch_press_02.wav"), 1.f);


	

	// print this shit.
	if (g_menu.main.misc.notifications.get(1)) {
		std::string out = tfm::format(XOR("hurt %s in the %s for %i (%i hp remaining).\n"), name, m_groups[group], (int)damage, std::clamp(hp, 0, 100));
		g_notify.add(out, colors::white);
	}

	if (group == HITGROUP_GENERIC)
		return;

	// if we hit a player, mark vis impacts.
	if (!m_vis_impacts.empty()) {
		for (auto& i : m_vis_impacts) {
			if (i.m_tickbase == g_cl.m_local->m_nTickBase())
				i.m_hit_player = true;
		}
	}

	struct ShotMatch_t { float delta; ShotRecord* shot; };
	ShotMatch_t match;
	match.delta = std::numeric_limits< float >::max();
	match.shot = nullptr;

	if (m_shots.empty())
		return;

	m_shots.front().m_hurt = true;

	// iterate all shots.
	for (auto& s : m_shots) {

		// this shot was already matched
		// with a 'player_hurt' event.
		if (s.m_hurt)
			continue;

		// add the latency to the time when we shot.
		// to predict when we would receive this event.
		float predicted = s.m_time + s.m_lat;

		// get the delta between the current time
		// and the predicted arrival time of the shot.
		float delta = std::abs(time - predicted);

		// fuck this.
		if (delta > 1.f)
			continue;

		// store this shot as being the best for now.
		if (delta < match.delta) {
			match.delta = delta;
			match.shot = &s;
		}
	}

	// no valid shotrecord was found.
	ShotRecord* shot = match.shot;
	if (!shot)
		return;

	// this shot was matched.
	shot->m_hurt = true;
}

void Shots::OnWeaponFire(IGameEvent* evt) {
	int        attacker;

	// screw this.
	if (!evt || !g_cl.m_local)
		return;

	// get attacker, if its not us, screw it.
	attacker = g_csgo.m_engine->GetPlayerForUserID(evt->m_keys->FindKey(HASH("userid"))->GetInt());
	if (attacker != g_csgo.m_engine->GetLocalPlayer())
		return;

	struct ShotMatch_t { float delta; ShotRecord* shot; };
	ShotMatch_t match;
	match.delta = std::numeric_limits< float >::max();
	match.shot = nullptr;

	// iterate all shots.
	for (auto& s : m_shots) {

		// this shot was already matched
		// with a 'weapon_fire' event.
		if (s.m_confirmed)
			continue;

		// add the latency to the time when we shot.
		// to predict when we would receive this event.
		float predicted = s.m_time + s.m_lat;

		// get the delta between the current time
		// and the predicted arrival time of the shot.
		float delta = std::abs(g_csgo.m_globals->m_curtime - predicted);

		// fuck this.
		if (delta > 1.f)
			continue;

		// store this shot as being the best for now.
		if (delta < match.delta) {
			match.delta = delta;
			match.shot = &s;
		}
	}

	// no valid shotrecord was found.
	ShotRecord* shot = match.shot;
	if (!shot)
		return;

	// this shot was matched.
	shot->m_confirmed = true;
}

void Shots::OnShotMiss(ShotRecord& shot) {
	vec3_t     pos, dir, start, end;
	CGameTrace trace;

	// shots we fire manually won't have a record.
	if (!shot.m_record)
		return;

	// not in nospread mode, see if the shot missed due to spread.
	Player* target = shot.m_target;
	if (!target)
		return;

	// not gonna bother anymore.
	if (!target->alive())
		return;

	AimPlayer* data = &g_aimbot.m_players[target->index() - 1];
	if (!data)
		return;

	// this record was deleted already.
	if (!shot.m_record->m_bones)
		return;

	// we are going to alter this player.
	// store all his og data.
	BackupRecord backup;
	backup.store(target);

	// write historical matrix of the time that we shot
	// into the games bone cache, so we can trace against it.
	shot.m_record->cache();

	// start position of trace is where we took the shot.
	start = shot.m_pos;

	// where our shot landed at.
	pos = shot.m_impact_pos;

	// the impact pos contains the spread from the server
	// which is generated with the server seed, so this is where the bullet
	// actually went, compute the direction of this from where the shot landed
	// and from where we actually took the shot.
	dir = (pos - start).normalized();

	// get end pos by extending direction forward.
	end = start + (dir * shot.m_range);

	// intersect our historical matrix with the path the shot took.
	g_csgo.m_engine_trace->ClipRayToEntity(Ray(start, end), MASK_SHOT, target, &trace);

	//if (!data->m_records.empty() && g_cl.m_processing)
	//	g_visuals.DrawHitboxMatrix(data->m_records.front().get(), colors::red, 10.f); /* where we hit */

	// we did not hit jackshit, or someone else.
	if (!trace.m_entity || !trace.m_entity->IsPlayer() || trace.m_entity != target)
		g_notify.add(XOR("missed shot due to spread\n"), colors::red);

	// we should have 100% hit this player..
	// this is a miss due to wrong angles.
	else if (trace.m_entity == target) {
		size_t mode = shot.m_record->m_mode;

		if (mode == Resolver::Modes::RESOLVE_BODY) {
			++data->m_body_index;
		}

		else if (mode == Resolver::Modes::RESOLVE_BRUTEFORCE)
		{
			++data->m_bruteforce_idx;
		}

		else if (mode == Resolver::Modes::RESOLVE_UNKNOWM) {
			++data->m_unknown_move;
		}

		else if (mode == Resolver::Modes::RESOLVE_STAND) {
			++data->m_stand_index;
		}

		else if (mode == Resolver::Modes::RESOLVE_STAND2) {
			++data->m_stand_index2;
		}

		else if (mode == Resolver::Modes::RESOLVE_BACKWARDS)
		{
			++data->m_backwards_idx;
		}

		else if (mode == Resolver::Modes::RESOLVE_STAND3)
		{
			++data->m_stand_index3;
			++data->stand3_missed_shots;
		}

		else if (mode == Resolver::Modes::RESOLVE_LBY)
		{
			++data->m_lby_idx;
		}

		else if (mode == Resolver::Modes::RESOLVE_FREESTAND)
		{
			++data->m_freestand_idx;
		}

		else if (mode == Resolver::Modes::RESOLVE_REVERSEFS){
			++data->m_reverse_fs;
		}

		else if (mode == Resolver::Modes::RESOLVE_LASTMOVE)
		{
			++data->m_lastmove_idx;
		}

		++data->m_missed_shots;

		player_info_t info;
		g_csgo.m_engine->GetPlayerInfo(shot.m_target->index(), &info);

		// print resolver debug.
		if (g_menu.main.misc.notifications.get(5))
			g_notify.add(tfm::format(XOR("missed shot due to resolver | ent: %s, m: %d, lc: %d, fw: %d, l: %d, vel: %.2f, lat: %.2f\n"),
				info.m_name, shot.m_record->m_mode, shot.m_record->m_broke_lc, shot.m_record->m_fake_walk, shot.m_record->m_lag,
				shot.m_record->m_velocity.length_2d(), shot.m_lat), colors::red, 8.f, true);
	}

	// restore player to his original state.
	backup.restore(target);
}

void Shots::Think() {
	if (!g_cl.m_processing || m_shots.empty()) {
		// we're dead, we won't need this data anymore.
		if (!m_shots.empty())
			m_shots.clear();

		// we don't handle shots if we're dead or if there are none to handle.
		return;
	}

	// iterate all shots.
	for (auto it = m_shots.begin(); it != m_shots.end(); ) {
		// too much time has passed, we don't need this anymore.
		if (it->m_time + 1.f < g_csgo.m_globals->m_curtime)
			// remove it.
			it = m_shots.erase(it);
		else
			it = next(it);
	}

	// iterate all shots.
	for (auto it = m_shots.begin(); it != m_shots.end(); ) {
		// our shot impacted, and it was confirmed, but we didn't damage anyone. we missed.
		if (it->m_impacted && it->m_confirmed && !it->m_hurt) {
			// handle the shot.
			OnShotMiss(*it);

			// since we've handled this shot, we won't need it anymore.
			it = m_shots.erase(it);
		}
		else
			it = next(it);
	}
}