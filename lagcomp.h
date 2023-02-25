#pragma once

class AimPlayer;

struct SequenceObject_t
{
    SequenceObject_t(int iInReliableState, int iOutReliableState, int iSequenceNr, float flCurrentTime)
        : iInReliableState(iInReliableState), iOutReliableState(iOutReliableState), iSequenceNr(iSequenceNr), flCurrentTime(flCurrentTime) { }

    int iInReliableState;
    int iOutReliableState;
    int iSequenceNr;
    float flCurrentTime;
};

class LagCompensation {
public:
    enum LagType : size_t {
        INVALID = 0,
        CONSTANT,
        ADAPTIVE,
        RANDOM,
    };

    // Values
    std::deque<SequenceObject_t> vecSequences = { };
    /* our real incoming sequences count */
    int nRealIncomingSequence = 0;
    /* count of incoming sequences what we can spike */
    int nLastIncomingSequence = 0;

public:
    bool StartPrediction(AimPlayer* player);
    void Extrapolate(Player* player, vec3_t& origin, vec3_t& velocity, int& flags, bool wasonground);
    void PlayerMove(LagRecord* record);
    void AirAccelerate(LagRecord* record, ang_t angle, float fmove, float smove);
    void PredictAnimations(CCSGOPlayerAnimState* state, LagRecord* record, LagRecord* previous);
    void UpdateIncomingSequences(INetChannel* pNetChannel);
    void ClearIncomingSequences();
    void AddLatencyToNetChannel(INetChannel* pNetChannel, float flLatency);
    void update_lerp();
};

extern LagCompensation g_lagcomp;