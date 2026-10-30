// =============================================================================
//  LEVEL-X by Piradex — interface fina para a licença
//
//  NÃO inclui <juce_cryptography>: é incluída pelas TUs pesadas
//  (PluginProcessor.cpp / PluginEditor.cpp) para que o MSVC não tenha de
//  compilar o módulo de criptografia dentro dessas unidades já pesadas
//  (DSP/GUI), o que fazia o tempo de compilação explodir.
//  A implementação está em License.cpp.
// =============================================================================
#pragma once

namespace lic
{
    bool isUnlockedGate();    // true se desbloqueado (ou dentro do período aberto)
    void initGate();          // cria o singleton na thread de mensagens
    void checkExpiryGate();   // bloqueia se a senha beta expirar durante o uso
}
