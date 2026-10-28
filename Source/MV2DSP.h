#pragma once
#include <JuceHeader.h>
#include <cmath>

//==============================================================================
//  LEVEL-X — motor de dinâmica de dois níveis (estilo Waves MV2)
//
//  LOW  -> upward compression: levanta o que está ABAIXO do threshold.
//  HIGH -> downward limiting: controla os picos ACIMA do threshold.
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

    void setLow  (float v) { lowAmt  = juce::jlimit (0.0f, 1.0f, v); }
    void setHigh (float v) { highAmt = juce::jlimit (0.0f, 1.0f, v); }
    void setResponse (int r) { if (r != response) { response = r; applyResponse (r); } }

    // devolve o ganho linear para a amostra 'x' (detetor alimentado por 'x')
    inline float computeGain (float x)
    {
        float e  = det.process (x);
        float db = juce::Decibels::gainToDecibels (e + 1.0e-9f);

        // LOW: upward
        float lThresh  = -12.0f - lowAmt * 26.0f;
        float lMaxGain =  lowAmt * 20.0f;
        float lGainDb  = 0.0f;
        if (db < lThresh)
            lGainDb = juce::jmin ((lThresh - db) * (1.0f - 1.0f / 2.5f), lMaxGain);

        // HIGH: downward (limiter)
        float hThresh = 0.0f - highAmt * 22.0f;
        float hRatio  = 1.0f + highAmt * 24.0f;
        float hGainDb = 0.0f;
        if (db > hThresh)
        {
            float over = db - hThresh;
            hGainDb = -(over - over / hRatio);
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
    float lowAmt = 0.0f, highAmt = 0.0f;
    int response = 1;
};
