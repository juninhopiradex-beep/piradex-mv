// =============================================================================
//  LEVEL-X by Piradex — Watcher da licença (TU própria, pequena)
//  Mantém o estado numa cache atómica, atualizada por um Timer na thread de
//  mensagens. Assim as TUs pesadas (processor/editor) nunca chamam lic::.
// =============================================================================
#include "LicenseGate.h"
#include "License.h"
#include <juce_events/juce_events.h>

namespace lic
{
struct Watcher::Impl : private juce::Timer
{
    std::atomic<bool>* cache = nullptr;

    explicit Impl (std::atomic<bool>* c) : cache (c)
    {
        License::get();                                       // inicializa na thread de mensagens
        cache->store (License::get().isUnlocked(), std::memory_order_relaxed);
        startTimer (1000);
    }

    void timerCallback() override
    {
        License::get().checkExpiry();
        cache->store (License::get().isUnlocked(), std::memory_order_relaxed);
    }
};

Watcher::Watcher() : impl_ (std::make_unique<Impl> (&cache_)) {}
Watcher::~Watcher() = default;
}
