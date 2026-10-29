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
        if (! manualTimes) applyResponse (response);
        updateHP();
    }
    void reset() { det.reset(); hpX = hpY = 0.0f; }

    // LOW (upward)
    void setLowThresh  (float dB) { lowThresh  = dB; }
    void setLowMaxGain (float dB) { lowMaxGain = dB; }
    void setLowAmount  (float a)  { lowAmt  = juce::jlimit (0.0f, 1.0f, a); }
    // HIGH (downward / limiter)
    void setHighThresh (float dB) { highThresh = dB; }
    void setHighRatio  (float r)  { highRatio  = juce::jmax (1.0f, r); }
    void setHighAmount (float a)  { highAmt = juce::jlimit (0.0f, 1.0f, a); }

    void setResponse (int r) { manualTimes = false; if (r != response) { response = r; applyResponse (r); } }
    void setTimes (float aMs, float rMs) { manualTimes = true; det.setTimes (aMs, rMs); }
    void setKnee  (float dB) { knee = juce::jmax (0.1f, dB); }
    void setDetectorHP (float freq)
    {
        hpOn = (freq >= 20.0f);
        hpFreq = freq; updateHP();
    }

    // devolve o ganho linear para a amostra 'x'
    inline float computeGain (float x)
    {
        float key = hpOn ? hp (x) : x;
        float e  = det.process (key);
        float db = juce::Decibels::gainToDecibels (e + 1.0e-9f);

        // LOW: upward (soft-knee)
        float upGR = upwardGR (db);
        float lGainDb = juce::jmin (upGR, lowMaxGain) * lowAmt;

        // HIGH: downward (soft-knee)
        float downGR = downwardGR (db);
        float hGainDb = -downGR * highAmt;

        // makeup automático
        float hMakeupDb = juce::jmin ((-highThresh) * (1.0f - 1.0f / highRatio) * highAmt * 0.6f, 12.0f);

        grLow  = lGainDb;
        grHigh = hGainDb;
        return juce::Decibels::decibelsToGain (lGainDb + hGainDb + hMakeupDb);
    }

    float grLow = 0.0f, grHigh = 0.0f;

private:
    inline float upwardGR (float db) const
    {
        const float upRatio = 2.5f, slope = 1.0f - 1.0f/upRatio;
        float d = lowThresh - db;                 // >0 abaixo do threshold
        if (d <= -knee*0.5f) return 0.0f;
        if (d >=  knee*0.5f) return d * slope;
        float t = d + knee*0.5f;                  // 0..knee
        return slope * t*t / (2.0f*knee);
    }
    inline float downwardGR (float db) const
    {
        const float slope = 1.0f - 1.0f/highRatio;
        float d = db - highThresh;                // >0 acima do threshold
        if (d <= -knee*0.5f) return 0.0f;
        if (d >=  knee*0.5f) return d * slope;
        float t = d + knee*0.5f;
        return slope * t*t / (2.0f*knee);
    }
    inline float hp (float in) noexcept
    {
        float out = hpA * (hpY + in - hpX);
        hpX = in; hpY = out; return out;
    }
    void updateHP()
    {
        hpA = std::exp (-2.0f * juce::MathConstants<float>::pi * juce::jmax (1.0f, hpFreq) / (float) sr);
    }
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
    float knee = 6.0f;
    int response = 1;
    bool manualTimes = false;
    bool hpOn = false; float hpFreq = 20.0f, hpA = 0.0f, hpX = 0.0f, hpY = 0.0f;
};
