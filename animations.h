#pragma once


enum AnimatingType : size_t {
    BREAKING979,
    DISTORTION,
};

class Animations {
public:
    AnimatingType m_animation_type;
public:
    bool IsPlaying979(LagRecord* record, LagRecord* previous_record, Player* entity);
    bool IsYawDistorting(AimPlayer* data, LagRecord* record, LagRecord* previous_record);
};

extern Animations g_animations;