#include "includes.h"
#include "animations.h"
class Animations g_animations;

bool Animations::IsPlaying979(LagRecord* record, LagRecord* previous_record, Player* entity) {

	if (!record || !previous_record)
		return false;

	if (record->m_velocity.length_2d() > 0.1f) {
		if (entity->m_vecVelocity().length_2d() > 0.1f)
			return false;
	}

	C_AnimationLayer* m_adjustment_layer = &record->m_layers[3];
	C_AnimationLayer* m_previous_adjustment_layer = &previous_record->m_layers[3];

	int m_activity = entity->GetSequenceActivity(m_adjustment_layer->m_sequence);
	int m_previous_acitvity = entity->GetSequenceActivity(m_previous_adjustment_layer->m_sequence);

	if (m_activity == 979 && m_activity != 980) {
		if (m_previous_acitvity == 979 && m_previous_acitvity != 980) {
			if (m_adjustment_layer->m_cycle != m_previous_adjustment_layer->m_cycle
				&& m_adjustment_layer->m_cycle <= 1.0f && m_adjustment_layer->m_weight > 0.8) {
				m_animation_type = AnimatingType::BREAKING979;
				return true;
			}
		}
		else if (m_previous_acitvity == 980) {
			if (m_adjustment_layer->m_weight == 0.0 && m_adjustment_layer->m_cycle == 0.0) {
				m_animation_type = AnimatingType::BREAKING979;
				return true;
			}
		}
	}
	else
		return false;
}
bool Animations::IsYawDistorting(AimPlayer* data, LagRecord* record, LagRecord* previous_record) {

	if (data->m_records.size() < 3)
		return false;

	LagRecord* pre_previous = data->m_records[2].get();

	static float m_last_move_time = FLT_MAX;
	static float m_last_move_yaw = FLT_MAX;

	bool m_can_distort = true;

	if (record->m_player->m_PlayerAnimState()->m_velocity_length_xy > 0.1f && record->m_player->m_PlayerAnimState()->m_on_ground) {
		m_last_move_time = g_csgo.m_globals->m_realtime;
		m_last_move_yaw = data->m_player->m_flLowerBodyYawTarget();
		m_can_distort = false;
	}

	if (m_last_move_time == FLT_MAX)
		return false;

	if (m_last_move_yaw == FLT_MAX)
		return false;

	if (record == previous_record)
		return false;

	if (m_can_distort) {

		if (fabsf(g_csgo.m_globals->m_realtime - m_last_move_time) > g_csgo.m_globals->m_curtime && (g_csgo.m_globals->m_curtime >
			record->m_sim_time || g_csgo.m_globals->m_curtime == record->m_sim_time)) {
			return false;
		}

		if (record->m_sim_time != record->m_old_sim_time) {
			if (g_csgo.m_globals->m_curtime != record->m_old_sim_time
				&& record->m_body != data->m_old_body)
				return false;
		}

		if (record->m_body == m_last_move_yaw) {
			if (record->m_body == data->m_old_body) {
				m_animation_type = AnimatingType::DISTORTION;
				return true;
			}
			return false;
		}

		float m_difference_delta = remainderf(record->m_body - previous_record->m_body, 360.f);
		bool m_has_lby = fabsf(m_difference_delta) > 35.f;

		auto m_spin_delta = (record->m_body - previous_record->m_body) / previous_record->m_lag;
		const auto m_prev_spin_delta = (previous_record->m_body - pre_previous->m_body) / pre_previous->m_lag;

		auto sin_cos_delta = fabsf(math::NormalizedAngle(sin(record->m_eye_angles.y) - cos(record->m_eye_angles.y)));

		if (m_spin_delta > 0.5f) {
			if (fabsf(previous_record->m_body - record->m_body) > 35.f && m_has_lby) {
				if (m_spin_delta == m_prev_spin_delta) {
					if (sin_cos_delta > 120.f && sin_cos_delta < 145.f) {
						m_animation_type = AnimatingType::DISTORTION;
						return true;
					}
				}
			}
		}
	}

	return false;
}	