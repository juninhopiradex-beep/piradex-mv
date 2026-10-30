// =============================================================================
//  LEVEL-X by Piradex — acesso por senha (offline)
//
//  Criador e Desenvolvedor: Ricardo Amorim / Juninho Piradex
//  O binário NÃO contém as senhas: só um "salt" aleatório e o hash
//  PBKDF2-HMAC-SHA256 (60 000 iterações) de cada uma.
//  Modo aberto (trial) até 31/12/2026: funciona sem senha. A partir de
//  01/01/2027 exige a senha master (#Tecnico01) ou uma licença válida.
// =============================================================================
#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_cryptography/juce_cryptography.h>
#include <atomic>

namespace lic
{
enum class Kind { None, Master, Trial };

struct KeyEntry
{
    const char* id;
    Kind        kind;
    const char* saltHex;
    const char* hashHex;
    int expYear, expMonth, expDay;   // só para Trial (válida até ao fim deste dia)
};

class License
{
public:
    enum class Result { Ok, Wrong, Expired };

    static License& get();

    // Período aberto (beta): o plugin funciona sem pedir senha até 31/12/2026.
    static bool inTrial()
    {
       #if defined (MXS_ALLOW_TEST_UNLOCK) || defined (ANL_ALLOW_TEST_UNLOCK) || defined (EQM_ALLOW_TEST_UNLOCK) \
        || defined (DLX_ALLOW_TEST_UNLOCK) || defined (ROOMX_ALLOW_TEST_UNLOCK) || defined (PIRADEX_NO_TRIAL)
        return false;   // nas builds de teste o trial fica desligado (para testar o bloqueio)
       #else
        return juce::Time::getCurrentTime() < juce::Time (2027, 0, 1, 0, 0, 0); // aberto ate 31/12/2026
       #endif
    }
    bool   isUnlocked() const  { return unlocked.load() || inTrial(); }
    Kind   kind() const        { return currentKind; }
    juce::String statusText() const;

    static int matchPassword (const juce::String& password, bool& expired);   // índice ou -1
    Result applyMatch (int index, bool expired, bool remember);
    Result tryPassword (const juce::String& password, bool remember);
    void   lock();
    void   checkExpiry();

    static juce::MemoryBlock pbkdf2 (const juce::String& password, const juce::MemoryBlock& salt, int iterations, int dkLen);
    static juce::MemoryBlock fromHex (const char* hex);
    static bool isExpired (const KeyEntry& k, juce::Time now = juce::Time::getCurrentTime());

    static constexpr int kIterations = 60000;

private:
    License();
    void tryAutoUnlock();
    void setUnlocked (const KeyEntry& k);
    static juce::String tokenFor (const KeyEntry& k);
    std::unique_ptr<juce::PropertiesFile> props;
    std::atomic<bool> unlocked { false };
    Kind currentKind = Kind::None;
    const KeyEntry* currentKey = nullptr;
};
} // namespace lic
