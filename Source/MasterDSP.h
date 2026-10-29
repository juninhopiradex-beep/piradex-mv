#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>
#include <cmath>

//==============================================================================
//  Utilitários de masterização: K-weighting + LUFS (M/S/I), True Peak,
//  limiter com lookahead (Ceiling) e dither TPDF.
//==============================================================================

struct Biquad
{
    float b0=1,b1=0,b2=0,a1=0,a2=0, z1=0,z2=0;
    inline float process (float x) noexcept
    {
        float y = b0*x + z1;
        z1 = b1*x - a1*y + z2;
        z2 = b2*x - a2*y;
        return y;
    }
    void reset() noexcept { z1 = z2 = 0.0f; }
};

//-----------------------------------------------------------------------------
// K-weighting (ITU-R BS.1770): shelving + high-pass
struct KWeighting
{
    Biquad s1, s2;
    void prepare (double sr)
    {
        {   // high-shelf
            double f0=1681.974450955533, G=3.999843853973347, Q=0.7071752369554196;
            double K=std::tan (juce::MathConstants<double>::pi*f0/sr);
            double Vh=std::pow (10.0, G/20.0), Vb=std::pow (Vh, 0.4996667741545416);
            double a0=1.0 + K/Q + K*K;
            s1.b0=(float)((Vh + Vb*K/Q + K*K)/a0);
            s1.b1=(float)(2.0*(K*K - Vh)/a0);
            s1.b2=(float)((Vh - Vb*K/Q + K*K)/a0);
            s1.a1=(float)(2.0*(K*K - 1.0)/a0);
            s1.a2=(float)((1.0 - K/Q + K*K)/a0);
        }
        {   // high-pass
            double f0=38.13547087602444, Q=0.5003270373238773;
            double K=std::tan (juce::MathConstants<double>::pi*f0/sr);
            double a0=1.0 + K/Q + K*K;
            s2.b0=(float)(1.0/a0);
            s2.b1=(float)(-2.0/a0);
            s2.b2=(float)(1.0/a0);
            s2.a1=(float)(2.0*(K*K - 1.0)/a0);
            s2.a2=(float)((1.0 - K/Q + K*K)/a0);
        }
        s1.reset(); s2.reset();
    }
    inline float process (float x) noexcept { return s2.process (s1.process (x)); }
    void reset() noexcept { s1.reset(); s2.reset(); }
};

//-----------------------------------------------------------------------------
// Medidor de loudness BS.1770 (Momentary 400ms, Short 3s, Integrated gated)
class LoudnessMeter
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        kw[0].prepare (sr); kw[1].prepare (sr);
        n400 = (int) (0.400 * sr);
        n3s  = (int) (3.000 * sr);
        hop  = (int) (0.100 * sr);
        buf400.assign ((size_t) n400, 0.0f);
        buf3s .assign ((size_t) n3s,  0.0f);
        i400 = i3s = 0; sum400 = sum3s = 0.0; hopCount = 0;
        blocks.clear(); blocks.reserve (40000);
        reset();
    }
    void reset()
    {
        std::fill (buf400.begin(), buf400.end(), 0.0f);
        std::fill (buf3s.begin(),  buf3s.end(),  0.0f);
        i400=i3s=0; sum400=sum3s=0.0; hopCount=0;
        kw[0].reset(); kw[1].reset();
        blocks.clear();
        lufsM.store(-100.0f); lufsS.store(-100.0f); lufsI.store(-100.0f);
    }

    // processa um bloco (não-destrutivo)
    void process (const float* L, const float* R, int ns)
    {
        for (int i=0;i<ns;++i)
        {
            float wl = kw[0].process (L[i]);
            float wr = R ? kw[1].process (R[i]) : wl;
            float z = wl*wl + wr*wr;                 // soma dos canais (pesos 1.0)

            sum400 -= buf400[(size_t)i400]; buf400[(size_t)i400]=z; sum400 += z;
            if (++i400>=n400) i400=0;
            sum3s  -= buf3s [(size_t)i3s ]; buf3s [(size_t)i3s ]=z; sum3s  += z;
            if (++i3s >=n3s ) i3s =0;

            if (++hopCount>=hop)
            {
                hopCount=0;
                double m400 = sum400/(double)n400;
                float lm = m400>1e-12 ? (float)(-0.691 + 10.0*std::log10 (m400)) : -100.0f;
                lufsM.store (lm);
                double m3 = sum3s/(double)n3s;
                lufsS.store (m3>1e-12 ? (float)(-0.691 + 10.0*std::log10 (m3)) : -100.0f);
                // bloco para integrado (usa janela 400ms)
                if (m400>1e-12 && blocks.size() < blocks.capacity())
                    blocks.push_back ((float) m400);
                computeIntegrated();
            }
        }
    }

    float getM() const { return lufsM.load(); }
    float getS() const { return lufsS.load(); }
    float getI() const { return lufsI.load(); }

private:
    void computeIntegrated()
    {
        // gate absoluto -70 LUFS
        double sum=0.0; int cnt=0;
        for (float m : blocks) { float l=-0.691f+10.0f*std::log10 (m);
            if (l > -70.0f) { sum+=m; ++cnt; } }
        if (cnt==0) { lufsI.store(-100.0f); return; }
        double mean = sum/cnt;
        float relGate = (float)(-0.691 + 10.0*std::log10 (mean)) - 10.0f;
        sum=0.0; cnt=0;
        for (float m : blocks) { float l=-0.691f+10.0f*std::log10 (m);
            if (l > -70.0f && l > relGate) { sum+=m; ++cnt; } }
        if (cnt==0) { lufsI.store(-100.0f); return; }
        lufsI.store ((float)(-0.691 + 10.0*std::log10 (sum/cnt)));
    }

    double sr=48000;
    KWeighting kw[2];
    std::vector<float> buf400, buf3s, blocks;
    int n400=0,n3s=0,hop=0,i400=0,i3s=0,hopCount=0;
    double sum400=0.0, sum3s=0.0;
    std::atomic<float> lufsM{-100.0f}, lufsS{-100.0f}, lufsI{-100.0f};
};

//-----------------------------------------------------------------------------
// True Peak (oversampling 4x por FIR polifásico simples) — medição dBTP
class TruePeakMeter
{
public:
    void prepare (double) { reset(); }
    void reset() { z.fill (0.0f); peak.store (0.0f); }

    void process (const float* d, int ns)
    {
        // 4x com FIR curto (kernel sinc janelado), estima inter-sample peaks
        static const float h[4][4] = {
            {1.0f, 0.0f, 0.0f, 0.0f},
            {-0.0645f, 0.8755f, 0.2026f, -0.0136f},
            {-0.0947f, 0.5946f, 0.5946f, -0.0947f},
            {-0.0136f, 0.2026f, 0.8755f, -0.0645f}
        };
        float localMax = peak.load();
        for (int i=0;i<ns;++i)
        {
            z[0]=z[1]; z[1]=z[2]; z[2]=z[3]; z[3]=d[i];
            for (int p=0;p<4;++p)
            {
                float y = h[p][0]*z[0]+h[p][1]*z[1]+h[p][2]*z[2]+h[p][3]*z[3];
                localMax = juce::jmax (localMax, std::abs (y));
            }
        }
        peak.store (localMax);
    }
    float getPeak() { float v=peak.load(); return v; }   // linear
    void  decay (float factor) { peak.store (peak.load()*factor); }

private:
    std::array<float,4> z { {0,0,0,0} };
    std::atomic<float> peak { 0.0f };
};

//-----------------------------------------------------------------------------
// Limiter brickwall com lookahead (Ceiling)
class LookaheadLimiter
{
public:
    void prepare (double sampleRate, int maxBlock)
    {
        sr = sampleRate;
        maxLook = (int) (0.01 * sr) + 8;              // até 10ms
        dl[0].assign ((size_t) maxLook, 0.0f);
        dl[1].assign ((size_t) maxLook, 0.0f);
        gBuf.assign ((size_t) maxLook, 1.0f);
        reset();
        juce::ignoreUnused (maxBlock);
    }
    void reset()
    {
        for (auto& d : dl) std::fill (d.begin(), d.end(), 0.0f);
        std::fill (gBuf.begin(), gBuf.end(), 1.0f);
        wpos=0; gain=1.0f;
    }
    void setParams (float ceilingDb, float lookMs, float releaseMs)
    {
        ceiling = juce::Decibels::decibelsToGain (ceilingDb);
        look = juce::jlimit (1, maxLook-1, (int) (lookMs*0.001*sr));
        relCoef = std::exp (-1.0f / (float)(sr * juce::jmax (1.0f, releaseMs) / 1000.0f));
    }

    // processa in-place (estéreo ou mono)
    void process (float* L, float* R, int ns)
    {
        for (int i=0;i<ns;++i)
        {
            float l = L[i]; float r = R ? R[i] : L[i];
            float peak = juce::jmax (std::abs (l), std::abs (r));
            float target = (peak > ceiling) ? ceiling/peak : 1.0f;

            // attack instantâneo (temos lookahead), release suave
            if (target < gain) gain = target;
            else               gain += (target - gain) * (1.0f - relCoef);

            // escreve no delay
            int rp = wpos - look; if (rp < 0) rp += maxLook;
            dl[0][(size_t)wpos] = l;
            dl[1][(size_t)wpos] = R ? r : l;
            gBuf[(size_t)wpos] = gain;

            // lê a amostra atrasada e aplica o ganho mínimo da janela de lookahead
            float gmin = 1.0f;
            for (int k=0;k<look;++k){ int idx=wpos-k; if (idx<0) idx+=maxLook; gmin=juce::jmin(gmin,gBuf[(size_t)idx]); }
            float outL = dl[0][(size_t)rp] * gmin;
            float outR = dl[1][(size_t)rp] * gmin;

            L[i] = outL; if (R) R[i] = outR;
            if (++wpos>=maxLook) wpos=0;
        }
    }
    int latency() const { return look; }

private:
    double sr=48000; int maxLook=512, look=64, wpos=0;
    float ceiling=1.0f, relCoef=0.999f, gain=1.0f;
    std::vector<float> dl[2], gBuf;
};

//-----------------------------------------------------------------------------
// Dither TPDF (1 LSB a 16-bit)
class Dither
{
public:
    void setEnabled (bool e) { on = e; }
    inline float process (float x) noexcept
    {
        if (!on) return x;
        float d = (rng.nextFloat() - rng.nextFloat()) * q;   // TPDF
        return x + d;
    }
private:
    bool on=false;
    juce::Random rng;
    static constexpr float q = 1.0f / 32768.0f;
};
