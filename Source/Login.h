// =============================================================================
//  LEVEL-X by Piradex — ecrã de login (fábrica leve)
//  A implementação vive em Login.cpp, para não inflar a TU do editor/processor
//  (o MSVC explode no tempo de compilação se os componentes JUCE do login
//   forem instanciados dentro de PluginEditor.cpp / PluginProcessor.cpp).
// =============================================================================
#pragma once
#include <memory>
#include <functional>

namespace juce { class Component; }

namespace lvlx
{
// Cria o overlay de login; onUnlock é chamado quando a senha é aceite.
std::unique_ptr<juce::Component> makeLoginOverlay (std::function<void()> onUnlock);
}
