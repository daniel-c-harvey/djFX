#pragma once

class UserParameters
{
    public:
        void setHP(float);
        void setLP(float);

        float getHPCutoff();
        float getHPResonance();
        float getLPCutoff();
        float getLPResonance();

    protected:
        float p_hp_cutoff;
        float p_hp_resonance;

        float p_lp_cutoff;
        float p_lp_resonance;
};