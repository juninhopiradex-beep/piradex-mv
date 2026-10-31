// =============================================================================
//  LEVEL-X by Piradex — interface fina para a licença
//
//  As TUs pesadas (PluginProcessor.cpp / PluginEditor.cpp) NÃO devem chamar
//  nenhuma função lic:: diretamente: qualquer chamada lic:: dentro dessas
//  unidades (DSP/GUI enormes) faz o otimizador do MSVC explodir no tempo de
//  compilação. Em vez disso usam o Watcher, que mantém o estado numa cache
//  atómica (atualizada fora da thread de áudio) e só é lido inline.
//  A implementação está em LicenseGate.cpp / License.cpp.
// =============================================================================
#pragma once
#include <atomic>
#include <memory>

namespace lic
{
    void initGate();
    bool isUnlockedGate();
    void checkExpiryGate();

    // Vigia o estado da licença fora da thread de áudio; expõe-o numa cache atómica.
    class Watcher
    {
    public:
        Watcher();
        ~Watcher();
        bool unlocked() const noexcept { return cache_.load (std::memory_order_relaxed); }
    private:
        std::atomic<bool> cache_ { true };
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}
