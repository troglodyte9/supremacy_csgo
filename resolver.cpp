#include "includes.h"
#include "animations.h"

Resolver g_resolver{};;

LagRecord* Resolver::FindIdealRecord(AimPlayer* data) {
	LagRecord* first_valid, * current;

	if (data->m_records.empty())
		return nullptr;

	first_valid = nullptr;

	// iterate records.
	for (const auto& it : data->m_records) {
		if (it->dormant() || it->immune() || !it->valid() || it->broke_lc())
			continue;

		// get current record.
		current = it.get();

		// first record that was valid, store it for later.
		if (!first_valid)
			first_valid = current;

		// try to find a record with a shot, lby update, walking or no anti-aim.
		//if (it->m_shot || it->m_mode == Modes::RESOLVE_LAST_LBY || it->m_mode == Modes::RESOLVE_WALK || it->m_mode == Modes::RESOLVE_NONE || it->m_mode == Modes::RESOLVE_LBY_UPDATE)
			//return current;
		if (it->m_shot || it->m_mode == Modes::RESOLVE_BODY || it->m_mode == Modes::RESOLVE_WALK || it->m_mode == Modes::RESOLVE_NONE)
			return current;
	}

	// none found above, return the first valid record if possible.
	return (first_valid) ? first_valid : nullptr;
}

LagRecord* Resolver::FindLastRecord(AimPlayer* data) {
	LagRecord* current;

	if (data->m_records.empty())
		return nullptr;

	// iterate records in reverse.
	for (auto it = data->m_records.crbegin(); it != data->m_records.crend(); ++it) {
		current = it->get();

		// if this record is valid.
		// we are done since we iterated in reverse.
		if (current->valid() && !current->immune() && !current->dormant())
			return current;
	}

	return nullptr;
}

void Resolver::OnBodyUpdate(Player* player, float value) {
	AimPlayer* data = &g_aimbot.m_players[player->index() - 1];

	// set data.
	data->m_old_body = data->m_body;
	data->m_body = value;
}

float Resolver::GetAwayAngle(LagRecord* record) {
	float  delta{ std::numeric_limits< float >::max() };
	vec3_t pos;
	ang_t  away;

	if (g_cl.m_net_pos.empty()) {
		math::VectorAngles(g_cl.m_local->m_vecOrigin() - record->m_pred_origin, away);
		return away.y;
	}

	float owd = (g_cl.m_latency / 2.f);

	float target = record->m_pred_time;

	// iterate all.
	for (const auto& net : g_cl.m_net_pos) {
		float dt = std::abs(target - net.m_time);

		// the best origin.
		if (dt < delta) {
			delta = dt;
			pos = net.m_pos;
		}
	}

	math::VectorAngles(pos - record->m_pred_origin, away);
	return away.y;
}

void Resolver::MatchShot(AimPlayer* data, LagRecord* record) {
	// do not attempt to do this in nospread mode.
	if (g_menu.main.config.mode.get() == 1)
		return;

	float shoot_time = -1.f;

	Weapon* weapon = data->m_player->GetActiveWeapon();
	if (weapon) {
		// with logging this time was always one tick behind.
		// so add one tick to the last shoot time.
		shoot_time = weapon->m_fLastShotTime() + g_csgo.m_globals->m_interval;
	}

	if (game::TIME_TO_TICKS(shoot_time) == game::TIME_TO_TICKS(record->m_sim_time)) {
		if (record->m_lag <= 1)
			record->m_shot = true;

		// more then 1 choke, cant hit pitch, apply prev pitch.
		else if (data->m_records.size() >= 2) {
			LagRecord* previous = data->m_records[1].get();

			if (previous && !previous->dormant())
				record->m_eye_angles.x = previous->m_eye_angles.x;
			resolver_state[record->m_player->index()] = "EXPERIMENTAL";
			return;
		}
		resolver_state[record->m_player->index()] = "RECORDING ENEMY SHOT";
	}

}

void Resolver::SetMode(LagRecord* record) {

	float speed = record->m_velocity.length_2d();

	if ((record->m_flags & FL_ONGROUND) && speed > 0.1f && !record->m_fake_walk && !g_input.GetKeyState(g_menu.main.aimbot.override.get()))
		record->m_mode = Modes::RESOLVE_WALK;

	// if we are overriding
	if (g_input.GetKeyState(g_menu.main.aimbot.override.get()) && record->m_flags & FL_ONGROUND && (speed <= 0.1f) && !g_input.GetKeyState(g_menu.main.aimbot.override.get()))
		record->m_mode = Modes::RESOLVE_OVERRIDE;

	// if on ground, not moving or fakewalking.
	else if ((record->m_flags & FL_ONGROUND) && ((speed < 1.f && !(speed > 2.f) || record->m_fake_walk) && !g_input.GetKeyState(g_menu.main.aimbot.override.get())))
		record->m_mode = Modes::RESOLVE_STAND;

	// if not on ground.
	else if (!(record->m_flags & FL_ONGROUND) && !g_input.GetKeyState(g_menu.main.aimbot.override.get()))
		record->m_mode = Modes::RESOLVE_AIR;

	else if (record->m_mode != Modes::RESOLVE_AIR && record->m_mode != Modes::RESOLVE_STAND && record->m_mode != Modes::RESOLVE_WALK && (speed < 1.f && !(speed > 2.f) && !g_input.GetKeyState(g_menu.main.aimbot.override.get())))
		record->m_mode = Modes::RESOLVE_UNKNOWM;
}

void Resolver::ResolveAngles(Player* player, LagRecord* record) {
	AimPlayer* data = &g_aimbot.m_players[player->index() - 1];

	// mark this record if it contains a shot.
	MatchShot(data, record);

	// next up mark this record with a resolver mode that will be used.
	SetMode(record);

	// run antifreestand to save myself some time
	FindBestAngle(record, data);

	if (g_menu.main.config.mode.get() == 1)
		record->m_eye_angles.x = 90.f;



	// we arrived here we can do the acutal resolve.
	if (record->m_mode == Modes::RESOLVE_WALK && !g_input.GetKeyState(g_menu.main.aimbot.override.get()))
		ResolveWalk(data, record, player);

	// override mode ran, run the correct resolver
	else if (record->m_mode == Modes::RESOLVE_OVERRIDE || g_input.GetKeyState(g_menu.main.aimbot.override.get()))
		ResolveOverride(player, record, data);

	else if (record->m_mode == Modes::RESOLVE_STAND && !g_input.GetKeyState(g_menu.main.aimbot.override.get()))
		ResolveStand(data, record, player);

	else if (record->m_mode == Modes::RESOLVE_AIR && !g_input.GetKeyState(g_menu.main.aimbot.override.get()))
		ResolveAir(data, record, player);

	else if (record->m_mode == Modes::RESOLVE_UNKNOWM && !g_input.GetKeyState(g_menu.main.aimbot.override.get()))
		GetDirectionAngle(player->index(), player, record);

	//PredictBodyUpdates(player, record);


	static bool m_iFirstCheck = true;
	static bool m_iRestartDistortCheck = true;
	static int m_iDistortCheck = 0;
	static int m_iResetCheck = 0;
	static float m_flLastDistortTime = 0.f;
	static float m_flMaxDistortTime = g_csgo.m_globals->m_curtime + 0.10f;
	static float m_flLastResetTime = 0.f;
	static float m_flMaxResetTime = g_csgo.m_globals->m_curtime + 0.85f;
	static bool sugma = false;
	static bool balls = false;

	if ((m_iRestartDistortCheck || m_iFirstCheck) && g_cl.m_local->alive()) {
		record->m_iDistortTiming = g_csgo.m_globals->m_curtime + 0.15f;
		m_iRestartDistortCheck = false;
		m_iFirstCheck = false;
	}

	if ((player->m_AnimOverlay()[3].m_weight == 0.f && player->m_AnimOverlay()[3].m_cycle == 0.f) &&
		player->m_vecVelocity().length() < 0.1f && !m_iRestartDistortCheck) {
		if (!sugma) {
			m_flMaxDistortTime = g_csgo.m_globals->m_curtime + 0.10f;
			sugma = true;
		}

		m_flLastDistortTime = g_csgo.m_globals->m_curtime;

		if (m_flLastDistortTime >= m_flMaxDistortTime) {
			m_iDistortCheck++;
			m_iResetCheck = 0;
			sugma = false;
		}
	}
	else {
		sugma = false;
	}

	if (player->m_AnimOverlay()[3].m_cycle >= 0.1f && player->m_AnimOverlay()[3].m_cycle <= 0.99999f && player->m_vecVelocity().length_2d() < 0.01f && record->m_velocity.length_2d() < 0.1f) {
		if (!balls) {
			m_flMaxResetTime = g_csgo.m_globals->m_curtime + 0.85f;
			balls = true;
		}

		m_flLastResetTime = g_csgo.m_globals->m_curtime;

		if (m_flLastResetTime >= m_flMaxResetTime) {
			m_iResetCheck++;
			balls = false;
		}
	}
	else {
		balls = false;
	}


	if (m_iResetCheck >= 3) {
		m_iRestartDistortCheck = true;
		m_iDistortCheck = 0;
		record->m_iDistorting[player->index()] = false;
	}

	if (m_iDistortCheck >= 2) {
		record->m_iDistorting[player->index()] = true;
	}


//data->resolver_mode = XOR("ANTIFREESTAND");  // was walking antifreestand
// resolver_state[record->m_player->index()] = "FS";
   // normalize the eye angles, doesn't really matter but its clean.
	math::NormalizeAngle(record->m_eye_angles.y);
}

void Resolver::ResolveWalk(AimPlayer* data, LagRecord* record, Player* player) {
	record->m_eye_angles.y = record->m_body;
	record->m_mode = RESOLVE_WALK;
	resolver_state[record->m_player->index()] = "MOVING";

	// delay body update.
	data->m_body_update = record->m_anim_time + 0.22;  // 0.22

	// reset stand and body index.
	data->m_stand_index = 0;
	data->m_stand_index2 = 0;
	data->m_body_index = 0;
	data->m_last_move = 0;
	data->m_lby_delta_index = 0;
	data->m_unknown_move = 0;
	data->m_moving_index = 0;
	data->m_freestanding_index = 0;

	data->m_backwards_idx = 0;
	data->m_freestand_idx = 0;
	data->m_lby_idx = 0;
	data->m_lastmove_idx = 0;

	float speed = record->m_anim_velocity.length_2d();
	Weapon* weapon = data->m_player->GetActiveWeapon();

	if (weapon) {


		WeaponInfo* weapon_data = weapon->GetWpnData();

		if (weapon_data) {

			float max_speed = record->m_player->m_bIsScoped() ? weapon_data->m_max_player_speed_alt : weapon_data->m_max_player_speed;

			if (speed > max_speed * 0.34f && (record->m_flags & 1) != 0 && speed > 1.0 && !record->m_fake_walk && !record->m_dormant)
				data->m_correct_move = true;
		}
	}


	// copy the last record that this player was walking
	// we need it later on because it gives us crucial data.
	std::memcpy(&data->m_walk_record, record, sizeof(LagRecord));
}

bool CheckSequence(Player* player, int sequence, bool checklayers = false, int layerstocheck = 15) //decent check could be improved - stolen from cthulhu (sopmk)
{
	// sanity pls
	if (!player || !player->alive())
		return false;

	for (int i = 0; i < checklayers ? layerstocheck : 15; i++) //Check layers
	{
		auto layer = player->m_AnimOverlay()[i];

		if (player->GetSequenceActivity(layer.m_sequence) == sequence) //Check for sequence 
			return true;
	}

	return false; // well this uh 
}
//This are good for eachother like a married couple 
bool CheckLBY(Player* player, LagRecord* record, LagRecord* prev_record) // I would recommend a sequence check -LoOsE (note from sopmk: sequence checks arent always effective, as some lby breakers suppress 979, hence why i'm checking animation values too)
{
	if (player->m_vecVelocity().length_2d() > 1.1f)
		return false; // cant break here

	bool choking = fabs(player->m_flSimulationTime() - player->m_flOldSimulationTime()) > g_csgo.m_globals->m_interval;

	if (int i = 0; i < 13, i++)
	{
		auto layer = record->m_layers[i];
		auto prev_layer = prev_record->m_layers[i];

		// make sure that the animation happened
		if (layer.m_cycle != prev_layer.m_cycle)
		{
			if (layer.m_cycle > 0.9 || layer.m_weight == 1.f) // triggered layer
			{
				if (i == 3) // adjust layer sanity check. If it is the adjust layer, they are most likely breaking LBY
					return true;

				// lby flick lol!
				if (choking && fabs(math::NormalizedAngle(record->m_body - prev_record->m_body)) > 5.f)
					return true;
			}
			else if (choking) // for improper LBY breakers
			{
				if (player->GetSequenceActivity(layer.m_sequence) == 979)
				{
					if (player->GetSequenceActivity(prev_layer.m_sequence) == 979)
					{
						return true; // we can be pretty sure that they are breaking LBY
					}
				}
			}
			return false;
		}
		return false;
	}
	return false;
}

bool Resolver::Spin_Detection(AimPlayer* data) {

	if (data->m_records.empty())
		return false;

	spin_step = 0;

	size_t size{};

	// iterate records.
	for (const auto& it : data->m_records) {
		if (it->dormant())
			break;

		// increment total amount of data.
		++size;
	}

	if (size > 2) {
		LagRecord* record = data->m_records[0].get();

		spindelta = (record->m_body - data->m_records[1].get()->m_body) / data->m_records[1].get()->m_lag;
		spinbody = record->m_body;
		float delta2 = (data->m_records[1].get()->m_body - data->m_records[2].get()->m_body) / data->m_records[2].get()->m_lag;

		return spindelta == delta2 && spindelta > 0.5f;
	}
	else
		return false;
}

Resolver::Directions Resolver::HandleDirections(AimPlayer* data, int ticks, float threshold) {
	CGameTrace tr;
	CTraceFilterSimple filter{ };

	if (!g_cl.m_processing)
		return Directions::YAW_NONE;

	// best target.
	struct AutoTarget_t { float fov; Player* player; };
	AutoTarget_t target{ 180.f + 1.f, nullptr };

	// get best target based on fov.
	auto origin = data->m_player->m_vecOrigin();
	ang_t view;
	float fov = math::GetFOV(g_cl.m_cmd->m_view_angles, g_cl.m_local->GetShootPosition(), data->m_player->WorldSpaceCenter());

	// set best fov.
	if (fov < target.fov) {
		target.fov = fov;
		target.player = data->m_player;
	}

	// get best player.
	const auto player = target.player;
	if (!player)
		return Directions::YAW_NONE;

	auto& bestOrigin = player->m_vecOrigin();

	// skip this player in our traces.
	filter.SetPassEntity(g_cl.m_local);

	// calculate angle direction from thier best origin to our origin
	ang_t angDirectionAngle;
	math::VectorAngles(g_cl.m_local->m_vecOrigin() - bestOrigin, angDirectionAngle);

	vec3_t forward, right, up;
	math::AngleVectors(angDirectionAngle, &forward, &right, &up);

	auto vecStart = g_cl.m_local->GetShootPosition();
	auto vecEnd = vecStart + forward * 100.0f;

	Ray rightRay(vecStart + right * 35.0f, vecEnd + right * 35.0f), leftRay(vecStart - right * 35.0f, vecEnd - right * 35.0f);

	g_csgo.m_engine_trace->TraceRay(rightRay, MASK_SOLID, &filter, &tr);
	float rightLength = (tr.m_endpos - tr.m_startpos).length();

	g_csgo.m_engine_trace->TraceRay(leftRay, MASK_SOLID, &filter, &tr);
	float leftLength = (tr.m_endpos - tr.m_startpos).length();

	static auto leftTicks = 0;
	static auto rightTicks = 0;
	static auto backTicks = 0;

	if (rightLength - leftLength > threshold)
		leftTicks++;
	else
		leftTicks = 0;

	if (leftLength - rightLength > threshold)
		rightTicks++;
	else
		rightTicks = 0;

	if (fabs(rightLength - leftLength) <= threshold)
		backTicks++;
	else
		backTicks = 0;

	Directions direction = Directions::YAW_NONE;

	if (rightTicks > ticks) {
		direction = Directions::YAW_RIGHT;
	}
	else {
		if (leftTicks > ticks) {
			direction = Directions::YAW_LEFT;
		}
		else {
			if (backTicks > ticks)
				direction = Directions::YAW_BACK;
		}
	}

	return direction;
}

void Resolver::FindBestAngle(LagRecord* record, AimPlayer* data) {
	// constants
	constexpr float STEP{ 4.f };
	constexpr float RANGE{ 28.f };

	// best target.
	vec3_t enemypos = record->m_player->GetShootPosition();
	float away = GetAwayAngle(record);

	// construct vector of angles to test.
	std::vector< AdaptiveAngle > angles{ };
	angles.emplace_back(away - 180.f);
	angles.emplace_back(away + 90.f);
	angles.emplace_back(away - 90.f);


	// start the trace at the your shoot pos.
	vec3_t start = g_cl.m_local->GetShootPosition();

	// see if we got any valid result.
	// if this is false the path was not obstructed with anything.
	bool valid{ false };

	// iterate vector of angles.
	for (auto it = angles.begin(); it != angles.end(); ++it) {

		// compute the 'rough' estimation of where our head will be.
		vec3_t end{ enemypos.x + std::cos(math::deg_to_rad(it->m_yaw)) * RANGE,
			enemypos.y + std::sin(math::deg_to_rad(it->m_yaw)) * RANGE,
			enemypos.z };

		// draw a line for debugging purposes.
		// g_csgo.m_debug_overlay->AddLineOverlay( start, end, 255, 0, 0, true, 0.1f );

		// compute the direction.
		vec3_t dir = end - start;
		float len = dir.normalize();

		// should never happen.
		if (len <= 0.f)
			continue;

		// step thru the total distance, 4 units per step.
		for (float i{ 0.f }; i < len; i += STEP) {
			// get the current step position.
			vec3_t point = start + (dir * i);

			// get the contents at this point.
			int contents = g_csgo.m_engine_trace->GetPointContents(point, MASK_SHOT_HULL);

			// contains nothing that can stop a bullet.
			if (!(contents & MASK_SHOT_HULL))
				continue;

			float mult = 1.f;

			// over 50% of the total length, prioritize this shit.
			if (i > (len * 0.5f))
				mult = 1.25f;

			// over 90% of the total length, prioritize this shit.
			if (i > (len * 0.75f))
				mult = 1.25f;

			// over 90% of the total length, prioritize this shit.
			if (i > (len * 0.9f))
				mult = 2.f;

			// append 'penetrated distance'.
			it->m_dist += (STEP * mult);

			// mark that we found anything.
			valid = true;
		}
	}

	if (!valid) {
		data->m_has_freestand = false;
		return;
	}

	// put the most distance at the front of the container.
	std::sort(angles.begin(), angles.end(),
		[](const AdaptiveAngle& a, const AdaptiveAngle& b) {
			return a.m_dist > b.m_dist;
		});

	// the best angle should be at the front now.
	AdaptiveAngle* best = &angles.front();

	data->m_has_freestand = true;
	data->m_best_angle = best->m_yaw;
}


void Resolver::PredictBodyUpdates(Player* player, LagRecord* record) {

	if (!g_cl.m_processing)
		return;

	AimPlayer* data = &g_aimbot.m_players[player->index() - 1];

	if (!data)
		return;

	if (data->m_records.empty())
		return;

	// inform esp that we're about to be the prediction process
	data->m_predict = true;

	//check if the player is walking
	// no need for the extra fakewalk check since we null velocity when they're fakewalking anyway
	if (record->m_anim_velocity.length_2d() > 0.1f && !record->m_fake_walk) {
		// predict the first flick they have to do after they stop moving
		data->m_body_update = record->m_anim_time + 0.22f;

		// since we are still not in the prediction process, inform the cheat that we arent predicting yet
		// this is only really used to determine if we should draw the lby timer bar on esp, no other real purpose
		data->m_predict = false;

		// stop here
		return;
	}

	if (data->m_body_index > 0)
		return;

	if (data->m_records.size() >= 2) {
		LagRecord* previous = data->m_records[1].get();

		if (previous && previous->valid()) { //&& !record->m_retard && !previous->m_retard) {
			// lby updated on this tick
			if (previous->m_body != record->m_body || record->m_anim_time >= data->m_body_update) {

				// for backtrack
				// record->m_resolved = true;

				// inform the cheat of the resolver method
				record->m_mode = RESOLVE_BODY;

				// predict the next body update
				data->m_body_update = record->m_anim_time + 1.1f;

				// set eyeangles to lby
				record->m_eye_angles.y = record->m_body;

				resolver_state[player->index()] = "LBYUPD";

				return;
			}
		}
	}
}

bool IsYawSideways(Player* entity, float yaw)
{
	const float delta =
		fabs(math::NormalizedAngle((math::CalcAngle(g_cl.m_local->m_vecOrigin(),
			entity->m_vecOrigin()).y)
			- yaw));

	return delta > 44.f && delta < 135.f;
}

bool IsYawSidewaysHigh(Player* entity, float yaw)
{
	const float delta =
		fabs(math::NormalizedAngle((math::CalcAngle(g_cl.m_local->m_vecOrigin(),
			entity->m_vecOrigin()).y)
			- yaw));

	return delta > 25.f && delta < 165.f;
}


void Resolver::ResolveStand(AimPlayer* data, LagRecord* record, Player* player) {

	data->m_moved = false;

	// for no-spread call a seperate resolver.
	if (g_menu.main.config.mode.get() == 1) {
		StandNS(data, record);
		return;
	}
	float time_since_moving = FLT_MAX;
	float last_move_delta = FLT_MAX;

	// get predicted away angle for the player.
	float away = GetAwayAngle(record);

	// pointer for easy access.
	LagRecord* move = &data->m_walk_record;

	C_AnimationLayer* curr = &record->m_layers[3];
	int act = data->m_player->GetSequenceActivity(curr->m_sequence);
	auto at_target_yaw = math::CalcAngle(g_cl.m_local->m_vecOrigin(), player->m_vecOrigin()).y;
	float m_time_since_moving = FLT_MAX;

	// we have a valid moving record.
	///if (move->m_sim_time > 0.f && !move->dormant() && !record->dormant() && data->m_last_move < 1) { // move->m_sim_time > 0.f && !move->dormant() && !record->dormant() && data->m_last_move < 1
	if (move->m_sim_time > 0.f) {
		vec3_t delta = move->m_origin - record->m_origin;

		// check if moving record is close.
		if (delta.length() <= 128.f) {
			// indicate that we are using the moving lby.
			data->m_moved = true;
			m_time_since_moving = record->m_anim_time - move->m_anim_time;
		}
	}

	LagRecord* previous_record = data->m_records.size() > 1 ? data->m_records[1].get() : nullptr;



	bool lby_change = false;

	if (data->m_records.size() >= 2) {
		LagRecord* previous = data->m_records[1].get();

		if (previous && !previous->dormant() && previous->valid()) {
			lby_change = previous->m_body != record->m_body;
		}
	}

	ang_t ang;
	bool breaking = CheckLBY(data->m_player, record, FindLastRecord(data));

	// we have no known lastmove, let's just bruteforce
	if (!data->m_moved) {
		record->m_mode = Modes::RESOLVE_STAND3;
		resolver_state[player->index()] = "BRUTEFORCE";
		switch (data->m_stand_index3 % 5) {
		case 0:
			record->m_eye_angles.y = data->m_best_angle;
			break;
		case 1:
			record->m_eye_angles.y = away + 180.f;
			break;
		case 2:
			record->m_eye_angles.y = move->m_body + 110.f;
			break;
		case 3:
			record->m_eye_angles.y = move->m_body - 110.f;
			break;
		case 4:
			record->m_eye_angles.y = away + 180.f;
			break;
		default:
			break;
		}
	} else if (data->m_moved) {
		float diff = math::NormalizedAngle(record->m_body - move->m_body);
		float delta = record->m_anim_time - move->m_anim_time;
		const Directions direction = HandleDirections(data);

		if (!record->m_dormant
			&& previous_record->m_dormant
			&& (record->m_origin - previous_record->m_origin).length_2d() > 16.0f)
		{
			data->m_correct_move = false;
		}

		bool can_last_move = data->m_correct_move && data->m_lastmove_idx < 1;

		if (record->m_fake_walk) {
			{
				record->m_mode = Modes::RESOLVE_FAKEWALK;
				record->m_eye_angles.y = record->m_body;
				resolver_state[record->m_player->index()] = XOR("FAKEWALK");
				return;
			}
		}



		
		if (fabsf(last_move_delta) < 12.f
			&& can_last_move)
		{
			// normal lastmove
			record->m_mode = Modes::RESOLVE_LASTMOVE;
			record->m_eye_angles.y = move->m_body;
			resolver_state[record->m_player->index()] = XOR("LASTMOVINGLBY");
		}
		else if (IsYawSideways(player, move->m_body) && fabsf(last_move_delta) <= 15.f && can_last_move) {
			record->m_mode = Modes::RESOLVE_LASTMOVE;
			/// sideways lastmove
			record->m_eye_angles.y = move->m_body;
			resolver_state[record->m_player->index()] = XOR("SIDE-LASTMOVINGLBY");
		}
		else if (data->m_stand_index == 0 && g_hvh.DoEdgeAntiAim(data->m_player, ang)) 
		{
			//enemy is edging
			record->m_mode = Modes::RESOLVE_EDGE;
			record->m_eye_angles.y = ang.y;
			resolver_state[record->m_player->index()] = "EDGE";
		}
		else if ((data->m_reverse_fs < 1 && IsYawSidewaysHigh(player, move->m_body)))
		{
			//reversefs
			record->m_mode = Modes::RESOLVE_REVERSEFS;
			resolver_state[player->index()] = "REVERSEFS";
			record->m_eye_angles.y = data->m_best_angle;
		}
		else if ((data->m_reverse_fs > 1 && IsYawSidewaysHigh(player, move->m_body)))
		{
			//set resolve mode
			record->m_mode = Modes::RESOLVE_STAND2;
			switch (data->m_stand_index % 3) {
			case 0:
				record->m_eye_angles.y = data->m_best_angle;
				resolver_state[player->index()] = "AFS[0]";
				break;
			case 1:
				record->m_eye_angles.y = away + 180.f;//yet
				resolver_state[player->index()] = "AFS[1]";
				break;
			case 2:
				record->m_eye_angles.y = away;//yet
				resolver_state[player->index()] = "AFS[2]";
				break;
			}

		}
		else {
			if (!data->m_has_freestand) {
				record->m_eye_angles.y = away + 180.f; // backward
				resolver_state[player->index()] = "BACKWARD";
			}
		}

		if (!record->m_fake_walk && data->m_body_index <= 2) {
			record->m_mode = Modes::RESOLVE_BODY;

			// body updated.
			if ((math::AngleDiff(previous_record->m_body, record->m_body) > 30.f)) {

				// set the resolve mode.
				resolver_state[player->index()] = "LBYUPD";

				// update their old body.
				data->m_old_body = record->m_body;

				// set angles to current LBY.
				record->m_eye_angles.y = record->m_body;
				iPlayers[player->index()] = false;

				// delay body update.
				data->m_body_update = record->m_anim_time + 1.1f;
				data->m_has_body_updated = true;

				// we've seen them update.
				// exit out of the resolver, thats it.
				return;
			}

			// lby should have updated here.
			if (data->m_has_body_updated && (record->m_anim_time >= data->m_body_update)) {
				resolver_state[player->index()] = "LBYPRED";

				// set angles to current LBY.
				record->m_eye_angles.y = record->m_body;

				// predict next body update.
				data->m_body_update = record->m_anim_time + 1.1f;

				iPlayers[player->index()] = false;

				// exit out of the resolver, thats it.
				return;
			}
		}
	}

	

	/*switch (data->m_missed_shots % 4)
	{
	case 1:
		record->m_eye_angles.y = at_target_yaw + 180.f;
		break;
	case 2:
		record->m_eye_angles.y = at_target_yaw - 75.f;
		break;
	case 3:
		record->m_eye_angles.y = at_target_yaw + 75.f;
		break;
	*/
}

void Resolver::ResolveYawBruteforce(LagRecord* record, Player* player, AimPlayer* data)
{
	auto local_player = g_cl.m_local;
	if (!local_player)
		return;

	const float at_target_yaw = math::CalcAngle(player->m_vecOrigin(), local_player->m_vecOrigin()).y;

	switch (data->m_missed_shots % 4)
	{
	case 1:
		record->m_eye_angles.y = at_target_yaw + 180.f;
		break;
	case 2:
		record->m_eye_angles.y = at_target_yaw - 75.f;
		break;
	case 3:
		record->m_eye_angles.y = at_target_yaw + 75.f;
		break;
	}

}

void Resolver::ResolveAir(AimPlayer* data, LagRecord* record, Player* player) {


	//record->m_eye_angles.y = record->m_body;
	float  delta{ std::numeric_limits< float >::max() };
	vec3_t pos;
	ang_t  away;
	resolver_state[player->index()] = "AIR";

	// other cheats predict you by their own latency.
	// they do this because, then they can put their away angle to exactly
	// where you are on the server at that moment in time.

	// the idea is that you would need to know where they 'saw' you when they created their user-command.
	// lets say you move on your client right now, this would take half of our latency to arrive at the server.
	// the delay between the server and the target client is compensated by themselves already, that is fortunate for us.

	// we have no historical origins.
	// no choice but to use the most recent one.
	//if( g_cl.m_net_pos.empty( ) ) {
	math::VectorAngles(g_cl.m_local->m_vecOrigin() - data->m_player->m_vecOrigin(), away);

	switch (data->m_missed_shots % 4)
	{
	case 0:
		record->m_eye_angles.y = away.y + 180.f;
		break;
	case 1:
		record->m_eye_angles.y = away.y + 135.f;
		break;
	case 2:
		record->m_eye_angles.y = away.y - 175.f;
		break;
	case 3:
		FindBestAngle(record, data);
		break;

	}

}

void Resolver::StandNS(AimPlayer* data, LagRecord* record) {
	// get away angles.
	float away = GetAwayAngle(record);

	switch (data->m_shots % 8) {
	case 0:
		record->m_eye_angles.y = away + 180.f; // + 180.f      //previous resolver angles vvvvvv (dont change them unless you make a better resolver, or add another //)
		break;

	case 1:
		record->m_eye_angles.y = away + 90.f; // + 90.f
		break;

	case 2:
		record->m_eye_angles.y = away - 90.f; // - 90.f
		break;

	case 3:
		record->m_eye_angles.y = away + 45.f; // + 45.f
		break;
	case 4:
		record->m_eye_angles.y = away - 45.f; // - 45.f
		break;

	case 5:
		record->m_eye_angles.y = away + 135.f; // + 135.f
		break;

	case 6:
		record->m_eye_angles.y = away - 135.f; // - 135.f
		break;

	case 7:
		record->m_eye_angles.y = away + 0.f; // + 0.f
		break;

	default:
		break;
	}

	// force LBY to not fuck any pose and do a true bruteforce.
	record->m_body = record->m_eye_angles.y;
}

float Resolver::GetDirectionAngle(int index, Player* player, LagRecord* record) {
	const auto left_thickness = g_cl.m_left_thickness[index];
	const auto right_thickness = g_cl.m_right_thickness[index];
	const auto at_target_yaw = math::CalcAngle(g_cl.m_local->m_vecOrigin(), player->m_vecOrigin()).y;// was g_cl.m_at_target_angle[index];
	const auto at_target_angle = g_cl.m_at_target_angle[index];
	AimPlayer* data;
	data = &g_aimbot.m_players[index - 1];
	auto yessir = player->m_flLowerBodyYawTarget();
	auto sds = player->m_AnimOverlay()[3].m_prev_cycle;
	record->m_mode = Modes::RESOLVE_UNKNOWM;

	auto angle = 0.f;
	if ((left_thickness >= 350 && right_thickness >= 350) || (left_thickness <= 50 && right_thickness <= 50) || (std::fabs(left_thickness - right_thickness) <= 7)) { // was 7
		record->m_mode = Modes::RESOLVE_UNKNOWM;
		switch (data->m_unknown_move % 7) {
		case 0:
			angle = math::normalize_float(at_target_yaw);  // -180  // at_target_yaw + 0
			record->m_eye_angles.y = angle;
			resolver_state[record->m_player->index()] = "FS-1.1";
			break;
		case 1:
			angle = math::normalize_float(at_target_yaw + 180.f);  // + 0.f  // at_target_yaw - 180
			record->m_eye_angles.y = angle;
			resolver_state[record->m_player->index()] = "FS-1.2";
			break;
		case 2:
			angle = math::normalize_float(at_target_yaw + 10.f); // + 170.f
			record->m_eye_angles.y = angle;
			resolver_state[record->m_player->index()] = "FS-1.3";
			break;
		case 3:
			angle = math::normalize_float(at_target_yaw + 20.f); // + 20.f
			record->m_eye_angles.y = angle;
			resolver_state[record->m_player->index()] = "FS-1.4";
			break;
		case 4:
			angle = math::normalize_float(at_target_yaw - 170.f); // - 170.f
			record->m_eye_angles.y = angle;
			resolver_state[record->m_player->index()] = "FS-1.5";
			break;
		case 5:
			angle = math::normalize_float(at_target_yaw - 20.f); // - 20.f
			record->m_eye_angles.y = angle;
			resolver_state[record->m_player->index()] = "FS-1.7";
			break;
		case 6:
			angle = math::normalize_float(at_target_yaw + 180.f); // + 180.f
			record->m_eye_angles.y = angle;
			resolver_state[record->m_player->index()] = "FS-1.8";
			break;
		default:
			break;

		}
	}
	else {
		if (left_thickness > right_thickness) {
			record->m_mode = Modes::RESOLVE_UNKNOWM;
			switch (data->m_unknown_move % 2) {
			case 0:
				angle = math::normalize_float(at_target_angle - 90.f); // - 90.f
				record->m_eye_angles.y = angle;
				resolver_state[record->m_player->index()] = "FS-2.1";
				break;
			case 1:
				angle = math::normalize_float(at_target_angle - 70.f); // -70.f
				record->m_eye_angles.y = angle;
				resolver_state[record->m_player->index()] = "FS-2.2";
				break;
			default:
				break;
			}
		}
		else if (left_thickness == right_thickness) {
			record->m_mode = Modes::RESOLVE_UNKNOWM;
			switch (data->m_unknown_move % 2) {
			case 0:
				angle = math::normalize_float(at_target_angle + 180.f);  // was -180.f (+ 180.f)
				record->m_eye_angles.y = angle;
				resolver_state[record->m_player->index()] = "FS-3.1";
				break;
			case 1:
				angle = math::normalize_float(at_target_angle + 175.f); // + 175.f
				record->m_eye_angles.y = angle;
				resolver_state[record->m_player->index()] = "FS-3.2";
				break;
			default:
				break;
			}
		}
		else {
			record->m_mode = Modes::RESOLVE_UNKNOWM;
			switch (data->m_unknown_move % 2) {
			case 0:
				angle = math::normalize_float(at_target_angle + 90.f);
				record->m_eye_angles.y = angle;
				resolver_state[record->m_player->index()] = "FS-4.1";
				break;
			case 1:
				angle = math::normalize_float(at_target_angle + 70.f);
				record->m_eye_angles.y = angle;
				resolver_state[record->m_player->index()] = "FS-4.2";
				break;
			default:
				break;

			}
		}
	}
	return angle;

}

void Resolver::AirNS(AimPlayer* data, LagRecord* record) {
	// get away angles.
	float away = GetAwayAngle(record);

	switch (data->m_missed_shots % 9) {
	case 0:
		record->m_eye_angles.y = away + 180.f;
		break;

	case 1:
		record->m_eye_angles.y = away + 150.f;
		break;
	case 2:
		record->m_eye_angles.y = away - 150.f;
		break;

	case 3:
		record->m_eye_angles.y = away + 165.f;
		break;

	case 4:
		record->m_eye_angles.y = away - 165.f;
		break;

	case 5:
		record->m_eye_angles.y = away + 135.f;
		break;

	case 6:
		record->m_eye_angles.y = away - 135.f;
		break;

	case 7:
		record->m_eye_angles.y = away + 90.f;
		break;

	case 8:
		record->m_eye_angles.y = away - 90.f;
		break;

	default:
		break;
	}
}

void Resolver::ResolvePoses(Player* player, LagRecord* record) {
	AimPlayer* data = &g_aimbot.m_players[player->index() - 1];

	// only do this bs when in air.
	if (record->m_mode == Modes::RESOLVE_AIR) {
		// ang = pose min + pose val x ( pose range )

		// lean_yaw
		player->m_flPoseParameter()[2] = g_csgo.RandomInt(0, 4) * 0.25f;

		// body_yaw
		player->m_flPoseParameter()[11] = g_csgo.RandomInt(1, 3) * 0.25f;
	}
}

bool Resolver::IsSideway(LagRecord* record, float m_yaw) {

	const float delta = fabsf(math::AngleDiff(m_yaw, g_resolver.GetAwayAngle(record)));
	return delta > 35.f && delta < 145.f;
}

bool hitPlayer[64];

float Resolver::GetLBYRotatedYaw(float lby, float yaw)
{
	float delta = math::NormalizedAngle(yaw - lby);
	if (fabs(delta) < 25.f)
		return lby;

	if (delta > 0.f)
		return yaw + 25.f;

	return yaw;
}


void Resolver::AntiFreestanding(AimPlayer* data, LagRecord* record)
{
	resolver_state[record->m_player->index()] = "ANTI-FS";

	float away = GetAwayAngle(record);


	if ((data->tr_right.m_fraction > 0.97f && data->tr_left.m_fraction > 0.97f) || (data->tr_left.m_fraction == data->tr_right.m_fraction) || std::abs(data->tr_left.m_fraction - data->tr_right.m_fraction) <= 0.1f) {
		record->m_eye_angles.y = away + 180.f;
	}
	else {
		record->m_eye_angles.y = (data->tr_left.m_fraction < data->tr_right.m_fraction) ? away - 90.f : away + 90.f;
	}
}

void Resolver::AntiFreestand(LagRecord* record) {
	resolver_state[record->m_player->index()] = "ANTI-FS";
	record->m_mode = RESOLVE_FREESTAND;

	// constants
	constexpr float STEP{ 4.f };
	constexpr float RANGE{ 32.f };

	// best target.
	vec3_t enemypos = record->m_player->GetShootPosition();
	float away = GetAwayAngle(record);

	// construct vector of angles to test.
	std::vector< AdaptiveAngle > angles{ };
	angles.emplace_back(away - 180.f);
	angles.emplace_back(away + 90.f);
	angles.emplace_back(away - 90.f);

	// start the trace at the your shoot pos.
	vec3_t start = g_cl.m_local->GetShootPosition();

	// see if we got any valid result.
	// if this is false the path was not obstructed with anything.
	bool valid{ false };

	// iterate vector of angles.
	for (auto it = angles.begin(); it != angles.end(); ++it) {

		// compute the 'rough' estimation of where our head will be.
		vec3_t end{ enemypos.x + std::cos(math::deg_to_rad(it->m_yaw)) * RANGE,
			enemypos.y + std::sin(math::deg_to_rad(it->m_yaw)) * RANGE,
			enemypos.z };

		// draw a line for debugging purposes.
		// g_csgo.m_debug_overlay->AddLineOverlay( start, end, 255, 0, 0, true, 0.1f );

		// compute the direction.
		vec3_t dir = end - start;
		float len = dir.normalize();

		// should never happen.
		if (len <= 0.f)
			continue;

		// step thru the total distance, 4 units per step.
		for (float i{ 0.f }; i < len; i += STEP) {
			// get the current step position.
			vec3_t point = start + (dir * i);

			// get the contents at this point.
			int contents = g_csgo.m_engine_trace->GetPointContents(point, MASK_SHOT_HULL);

			// contains nothing that can stop a bullet.
			if (!(contents & MASK_SHOT_HULL))
				continue;

			float mult = 1.f;

			// over 50% of the total length, prioritize this shit.
			if (i > (len * 0.5f))
				mult = 1.25f;

			// over 90% of the total length, prioritize this shit.
			if (i > (len * 0.75f))
				mult = 1.25f;

			// over 90% of the total length, prioritize this shit.
			if (i > (len * 0.9f))
				mult = 2.f;

			// append 'penetrated distance'.
			it->m_dist += (STEP * mult);

			// mark that we found anything.
			valid = true;
		}
	}

	if (!valid) {
		return;
	}

	// put the most distance at the front of the container.
	std::sort(angles.begin(), angles.end(),
		[](const AdaptiveAngle& a, const AdaptiveAngle& b) {
			return a.m_dist > b.m_dist;
		});

	// the best angle should be at the front now.
	AdaptiveAngle* best = &angles.front();

	record->m_eye_angles.y = best->m_yaw;
}

void Resolver::ResolveOverride(Player* player, LagRecord* record, AimPlayer* data) {

	// get predicted away angle for the player.
	float away = GetAwayAngle(record);

	// pointer for easy access.
	LagRecord* move = &data->m_walk_record;

	C_AnimationLayer* curr = &record->m_layers[3];
	int act = data->m_player->GetSequenceActivity(curr->m_sequence);

	if (g_input.GetKeyState(g_menu.main.aimbot.override.get())) {
		ang_t                          viewangles;
		g_csgo.m_engine->GetViewAngles(viewangles);

		//auto yaw = math::clamp (g_cl.m_local->GetAbsOrigin(), Player->origin()).y;
		const float at_target_yaw = math::CalcAngle(g_cl.m_local->m_vecOrigin(), player->m_vecOrigin()).y;

		if (fabs(math::NormalizedAngle(viewangles.y - at_target_yaw)) > 30.f)
			return FindBestAngle(record, data);

		record->m_eye_angles.y = (math::NormalizedAngle(viewangles.y - at_target_yaw) > 0) ? at_target_yaw + 90.f : at_target_yaw - 90.f;

		//return UTILS::GetLBYRotatedYaw(entity->m_flLowerBodyYawTarget(), (math::NormalizedAngle(viewangles.y - at_target_yaw) > 0) ? at_target_yaw + 90.f : at_target_yaw - 90.f);

		record->m_mode = Modes::RESOLVE_OVERRIDE;
	}

	bool did_lby_flick{ false };

	if (data->m_body != data->m_old_body)
	{
		record->m_eye_angles.y = record->m_body;

		data->m_body_update = record->m_anim_time + 1.1f;

		iPlayers[record->m_player->index()] = false;
		record->m_mode = Modes::RESOLVE_BODY;
	}
	else
	{
		// LBY SHOULD HAVE UPDATED HERE.
		if (record->m_anim_time >= data->m_body_update) {
			// only shoot the LBY flick 3 times.
			// if we happen to miss then we most likely mispredicted
			if (data->m_body_index < 1) {
				// set angles to current LBY.
				record->m_eye_angles.y = record->m_body;

				data->m_body_update = record->m_anim_time + 1.1f;

				// set the resolve mode.
				iPlayers[record->m_player->index()] = false;
				record->m_mode = Modes::RESOLVE_LBY_UPDATE;
			}
		}
	}

	// we have a valid moving record.
	if (move->m_sim_time > 0.f) {
		vec3_t delta = move->m_origin - record->m_origin;

		// check if moving record is close.
		if (delta.length() <= 128.f) {
			// indicate that we are using the moving lby.
			data->m_moved = true;
		}
	}

	// a valid moving context was found
	if (data->m_moved) {
		float diff = math::NormalizedAngle(record->m_body - move->m_body);
		float delta = record->m_anim_time - move->m_anim_time;

		// it has not been time for this first update yet.
		if (delta < 0.22f) {
			// set angles to current LBY.
			record->m_eye_angles.y = move->m_body;

			// set resolve mode.
			record->m_mode = Modes::RESOLVE_STOPPED_MOVING;

			// exit out of the resolver, thats it.
			return;
		}

		// LBY SHOULD HAVE UPDATED HERE.
		else if (record->m_anim_time >= data->m_body_update) {
			// only shoot the LBY flick 3 times.
			// if we happen to miss then we most likely mispredicted.
			if (data->m_body_index < 1) {
				// set angles to current LBY.
				record->m_eye_angles.y = data->m_body;

				// predict next body update.
				data->m_body_update = record->m_anim_time + 1.1f;

				// set the resolve mode.
				record->m_mode = Modes::RESOLVE_BODY;

				return;
			}
		}
	}
}