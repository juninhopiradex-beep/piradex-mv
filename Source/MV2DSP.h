#pragma once
#include <JuceHeader.h>
#include <cmath>

//==============================================================================
//  PiradexMV — motor de dinâmica de dois níveis (estilo Waves MV2)
//
//  LOW  -> upward compression: levanta o que está ABAIXO do threshold
//          (traz corpo/detalhe/room sem tocar nos picos).
//  HIGH -> downward limiting: controla os picos ACIMA do threshold.
//
//  Response  -> attack/release do detector (Smooth / Balanced / Fast).
//  OutputMode-> Classic (colorido, saturação suave) ou Modern (limpo).
//==============================================================================

class LevelDetector
{
public:
    void prepare (double sampleRate) { sr = sampleRate; recompute(); }

    void setTimes (float attackMs_, float releaseMs_)
    {
        attackMs = attackMs_;
        releaseMs = releaseMs_;
        recompute();
    }

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
    float attackMs = 10.0f, releaseMs = 120.0f;
    float aCoef = 0.0f, rCoef = 0.0f, env = 0.0f;
};

//==============================================================================
class MV2Engine
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        lowDet.prepare (sampleRate);
        highDet.prepare (sampleRate);
        applyResponse (response);
    }

    void reset()
    {
        lowDet.reset();
        highDet.reset();
    }

    // 0..1
    void setLow  (float v) { lowAmt  = juce::jlimit (0.0f, 1.0f, v); }
    void setHigh (float v) { highAmt = juce::jlimit (0.0f, 1.0f, v); }

    // 0 Smooth, 1 Balanced, 2 Fast
    void setResponse (int r) { response = r; applyResponse (r); }

    // 0 Classic, 1 Modern
    void setMode (int m) { mode = m; }

    inline float processSample (float x)
    {
        // ---------------- LOW: upward compression ----------------
        float lEnv = lowDet.process (x);
        float lDb  = juce::Decibels::gainToDecibels (lEnv + 1.0e-9f);

        float lThresh  = -12.0f - lowAmt * 26.0f;   // -12 .. -38 dB
        float lMaxGain =  lowAmt * 20.0f;           // até +20 dB
        float lRatio   = 2.5f;

        float lGainDb = 0.0f;
        if (lDb < lThresh)
        {
            float below = lThresh - lDb;
            lGainDb = juce::jmin (below * (1.0f - 1.0f / lRatio), lMaxGain);
        }

        // ---------------- HIGH: downward limiting ----------------
        float hEnv = highDet.process (x);
        float hDb  = juce::Decibels::gainToDecibels (hEnv + 1.0e-9f);

        float hThresh = 0.0f - highAmt * 22.0f;     // 0 .. -22 dB
        float hRatio  = 1.0f + highAmt * 24.0f;     // 1:1 .. 25:1  (limiter)

        float hGainDb = 0.0f;
        if (hDb > hThresh)
        {
            float over = hDb - hThresh;
            hGainDb = -(over - over / hRatio);
        }

        float g = juce::Decibels::decibelsToGain (lGainDb + hGainDb);
        float y = x * g;

        // ---------------- carácter (Classic = saturação suave) ----
        if (mode == 0)
        {
            const float drive = 1.0f + highAmt * 0.6f;
            y = std::tanh (y * drive) / std::tanh (drive);
        }

        lastGrLow  = lGainDb;
        lastGrHigh = hGainDb;
        return y;
    }

    float lastGrLow = 0.0f, lastGrHigh = 0.0f;

private:
    void applyResponse (int r)
    {
        switch (r)
        {
            case 0: lowDet.setTimes (20.0f, 250.0f); highDet.setTimes (3.0f, 120.0f); break; // Smooth
            case 2: lowDet.setTimes (3.0f,  80.0f);  highDet.setTimes (0.5f, 50.0f);  break; // Fast
            default:lowDet.setTimes (8.0f,  140.0f); highDet.setTimes (1.0f, 80.0f);  break; // Balanced
        }
    }

    double sr = 44100.0;
    LevelDetector lowDet, highDet;
    float lowAmt = 0.0f, highAmt = 0.0f;
    int response = 1, mode = 1;
};
