#pragma once
#include <JuceHeader.h>
#include <cmath>

//==============================================================================
//  LEVEL-X — motor de dinâmica de dois níveis (estilo Waves MV2)
//
//  LOW  -> UPWARD COMPRESSION: levanta o que está ABAIXO do threshold
//          (Threshold, Max Gain, Amount).
//  HIGH -> DOWNWARD LIMITING : controla os picos ACIMA do threshold
//          (Threshold, Ratio, Amount).
//  Response -> attack/release do detector (Fast / Medium / Slow).
//
//  computeGain(x) devolve o ganho LINEAR a aplicar à amostra, para permitir
//  modos estéreo (Stereo linkado, Dual Mono, Mid/Side) feitos no processor.
//==============================================================================

class LevelDetector
{
public:
    void prepare (double sampleRate) { sr = sampleRate; recompute(); }
    void setTimes (float a, float r) { attackMs = a; releaseMs = r; recompute(); }

    inline float process (float x)
    {
        float rect = std::abs (x);
        float coef = (rect > env) ? aCoef : rCoef;
        env = coef * env + (1.0f - coef) * rect;
        return env;
    }
    void reset() { env = 0.0f; }

private:
    void recompute()
    {
        aCoef = std::exp (-1.0f / (float) (sr * (attackMs  / 1000.0)));
        rCoef = std::exp (-1.0f / (float) (sr * (releaseMs / 1000.0)));
    }
    double sr = 44100.0;
    float attackMs = 8.0f, releaseMs = 120.0f;
    float aCoef = 0.0f, rCoef = 0.0f, env = 0.0f;
};

//==============================================================================
class MV2Engine
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        det.prepare (sampleRate);
        applyResponse (response);
    }
    void reset() { det.reset(); }

    // LOW (upward)
    void setLowThresh  (float dB) { lowThresh  = dB; }
    void setLowMaxGain (float dB) { lowMaxGain = dB; }
    void setLowAmount  (float a)  { lowAmt  = juce::jlimit (0.0f, 1.0f, a); }
    // HIGH (downward / limiter)
    void setHighThresh (float dB) { highThresh = dB; }
    void setHighRatio  (float r)  { highRatio  = juce::jmax (1.0f, r); }
    void setHighAmount (float a)  { highAmt = juce::jlimit (0.0f, 1.0f, a); }

    void setResponse (int r) { if (r != response) { response = r; applyResponse (r); } }

    // devolve o ganho linear para a amostra 'x' (detetor alimentado por 'x')
    inline float computeGain (float x)
    {
        float e  = det.process (x);
        float db = juce::Decibels::gainToDecibels (e + 1.0e-9f);

        // LOW: upward compression — levanta abaixo do threshold, limitado a Max Gain
        float lGainDb = 0.0f;
        if (db < lowThresh)
        {
            const float upRatio = 2.5f;                 // quanto do défice é recuperado
            float lift = (lowThresh - db) * (1.0f - 1.0f / upRatio);
            lGainDb = juce::jmin (lift, lowMaxGain) * lowAmt;
        }

        // HIGH: downward limiting — reduz acima do threshold com ratio
        float hGainDb = 0.0f;
        if (db > highThresh)
        {
            float over = db - highThresh;
            hGainDb = -(over - over / highRatio) * highAmt;
        }

        grLow  = lGainDb;
        grHigh = hGainDb;
        return juce::Decibels::decibelsToGain (lGainDb + hGainDb);
    }

    float grLow = 0.0f, grHigh = 0.0f;

private:
    void applyResponse (int r)
    {
        switch (r)
        {
            case 0: det.setTimes (2.0f,  70.0f);  break;  // Fast
            case 2: det.setTimes (20.0f, 280.0f); break;  // Slow
            default:det.setTimes (8.0f,  140.0f); break;  // Medium
        }
    }
    double sr = 44100.0;
    LevelDetector det;
    float lowThresh = -28.0f, lowMaxGain = 12.0f, lowAmt = 0.5f;
    float highThresh = -6.0f, highRatio = 4.0f,  highAmt = 0.5f;
    int response = 1;
};
