#pragma once

class ShotRecord;

struct AntiFreestandingRecord
{
	int right_damage = 0, left_damage = 0, back_damage = 0;
	float right_fraction = 0.f, left_fraction = 0.f, back_fraction = 0.f;
};


class Resolver {
public:
	enum Modes : size_t {
		RESOLVE_NONE = 0,
		RESOLVE_WALK,
		RESOLVE_STAND,
		RESOLVE_AIR,
		RESOLVE_LBY_UPDATE,
		RESOLVE_OVERRIDE,
		RESOLVE_LAST_LBY,
		RESOLVE_BRUTEFORCE,
		RESOLVE_FREESTAND,
		RESOLVE_DELTA,
		RESOLVE_BODY,
		RESOLVE_STOPPED_MOVING,
		RESOLVE_UNKNOWM,
		RESOLVE_LASTMOVE,
		RESOLVE_STAND2,
		RESOLVE_STAND1,
		RESOLVE_BACKWARDS,
		RESOLVE_LBY,
		RESOLVE_DISTORT,
		RESOLVE_FAKEWALK,
	};

	enum class Directions : int {
		YAW_RIGHT = -1,
		YAW_BACK,
		YAW_LEFT,
		YAW_NONE,
	};

	enum class fs_dir : int {
		FS_LEFT = 0,
		FS_RIGHT,
		FS_BACK
	};

	struct fs_record {
		float fraction[3];
		bool  hit[3];

		__forceinline float get_yaw(LagRecord* record, bool r) {

			float fs_yaw[3] = {
				r ? 90.f : -90.f,
				r ? -90.f : 90.f,
				180.f,
			};

			if (hit[int(fs_dir::FS_LEFT)] || hit[int(fs_dir::FS_RIGHT)]) {

				auto sort = (fraction[int(fs_dir::FS_LEFT)] > fraction[int(fs_dir::FS_RIGHT)]) ? int(fs_dir::FS_LEFT) : int(fs_dir::FS_RIGHT);
				return fs_yaw[sort];

			}

			return fs_yaw[int(fs_dir::FS_BACK)];
		}

		__forceinline void update(CGameTrace t, fs_dir d) {
			fraction[int(d)] = t.m_fraction;
			hit[int(d)] = t.hit();
		}
	};
public:
	LagRecord* FindIdealRecord(AimPlayer* data);
	LagRecord* FindLastRecord(AimPlayer* data);
	void OnBodyUpdate(Player* player, float value);
	float GetAwayAngle(LagRecord* record);
	void MatchShot(AimPlayer* data, LagRecord* record);
	void Override(LagRecord* record);
	void SetMode(LagRecord* record);
	void ResolveYawBruteforce(LagRecord* record, Player* player, AimPlayer* data);
	bool Spin_Detection(AimPlayer* data);
	void ResolveStand(AimPlayer* data, LagRecord* record, Player* player);
	void ResolveAir(AimPlayer* data, LagRecord* record, Player* player);
	void ResolveAngles(Player* player, LagRecord* record);
	void ResolveWalk(AimPlayer* data, LagRecord* record, Player* player);
	//void ResolveWalk(AimPlayer* data, LagRecord* record);
	//void ResolveStand(AimPlayer* data, LagRecord* record);
	void StandNS(AimPlayer* data, LagRecord* record);
	//void ResolveAir(AimPlayer* data, LagRecord* record);
	void AntiFreestanding(AimPlayer* data, LagRecord* record);
	void FindBestAngle(LagRecord* record, AimPlayer* data);
	Directions HandleDirections(AimPlayer* data, int ticks = 10, float threshold = 20.f);
	void AirNS(AimPlayer* data, LagRecord* record);
	//void AntiFreestand(LagRecord* record);
	float GetLBYRotatedYaw(float lby, float yaw);
	bool AntiFreestanding(Player* entity, float& yaw, int damage_tolerance);
	float GetDirectionAngle(int index, Player* player, LagRecord* record);
	void ResolvePoses(Player* player, LagRecord* record);
	void PredictBodyUpdates(Player* player, LagRecord* record);
	void AntiFreestand(LagRecord* record);
	bool IsSideway(LagRecord* record, float m_yaw);
	void ResolveOverride(Player* player, LagRecord* record, AimPlayer* data);


	float spindelta;
	float spinbody;
	int spin_step;

public:
	std::array< vec3_t, 64 > m_impacts;
	int	   iPlayers[64];
	std::array< std::string, 64 > resolver_state;
	fs_record m_fsrecord[64];
	AntiFreestandingRecord anti_freestanding_record;
	// check if the players yaw is sideways.
	__forceinline bool IsLastMoveValid(LagRecord* record, float m_yaw) {
		const auto away = GetAwayAngle(record);
		const float delta = fabs(math::NormalizedAngle(away - m_yaw));
		return delta > 20.f && delta < 160.f;
	}
	class PlayerResolveRecord
	{
	public:
		struct AntiFreestandingRecord
		{
			int right_damage = 0, left_damage = 0;
			float right_fraction = 0.f, left_fraction = 0.f;
		};

	public:
		AntiFreestandingRecord m_sAntiEdge;
	};

	vec3_t last_eye;

	bool using_anti_freestand;

	float left_damage[64];
	float right_damage[64];
	float back_damage[64];

	std::vector<vec3_t> last_eye_positions;
};

extern Resolver g_resolver;