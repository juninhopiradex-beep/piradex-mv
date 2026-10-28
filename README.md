# LEVEL-X

Processador de dinâmica de **dois níveis** inspirado no conceito do Waves MV2 — não é um compressor normal: trata as duas pontas da dinâmica ao mesmo tempo.

**Criador e Desenvolvedor: Ricardo Amorim / Juninho Piradex**
Formatos: **VST3 / AU / Standalone** · macOS (Intel + Apple Silicon) · v1.0.0

---

## O conceito

| Controlo | Tipo | O que faz |
|---|---|---|
| **LOW** | *Upward compression* | Levanta o que está **abaixo** do threshold — traz o room de uma bateria, foca vozes mixadas baixas, sem comprometer os picos. |
| **HIGH** | *Downward limiting* | Controla os **picos** altos. |
| **OUTPUT** | Ganho | Nível de saída. |

Extras do painel: **Trim** (ganho de entrada), **Mix** (dry/wet paralelo), **Mute / Dim / Mono / Bypass**, **Oversampling** (1x–8x), **Response** (Fast / Medium / Slow — attack/release do detector) e **Output Mode** (Stereo linkado / Dual Mono / Mid-Side). Meters de saída **L/R** arco-íris com leitura de pico.

> Excecional em vozes — controla picos e amplifica os níveis baixos. Realça o room de instrumentos acústicos mantendo a dinâmica. Funciona como leveling amplifier para dinâmicas muito variáveis. Tudo com três controlos principais.

## Cadeia de sinal

`Trim → [oversampling ↑] → LOW (upward) + HIGH (downward) → [↓] → Output → Mix → Mono → Dim/Mute`

O detetor segue o nível com attack/release próprios; o LOW soma ganho positivo abaixo do threshold (limitado a um máximo), o HIGH aplica redução acima do threshold com ratio crescente até funcionar como limiter. O **Output Mode** decide como o ganho é aplicado ao estéreo: *Stereo* usa o mesmo ganho nos dois canais (imagem estável), *Dual Mono* trata L e R independentes, *Mid/Side* processa M e S em separado.

## Compilar (GitHub Actions — recomendado)

1. Faz push destes ficheiros para o GitHub.
2. O workflow `.github/workflows/build.yml` compila sozinho para macOS (Intel + Apple Silicon).
3. Em **Actions → build concluído → Artifacts**, descarrega o pacote com `LEVEL-X.component` (AU), `LEVEL-X.vst3` e `LEVEL-X.app`.

## Instalar no Mac

```bash
cp -R "LEVEL-X.component" ~/Library/Audio/Plug-Ins/Components/
cp -R "LEVEL-X.vst3"      ~/Library/Audio/Plug-Ins/VST3/
# remover quarentena (plugin não assinado)
xattr -cr ~/Library/Audio/Plug-Ins/Components/"LEVEL-X.component"
xattr -cr ~/Library/Audio/Plug-Ins/VST3/"LEVEL-X.vst3"
```

## Compilar localmente

```bash
cmake -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
cmake --build build --config Release
```

## Afinar o carácter

Em `Source/MV2DSP.h`: `lThresh`/`lMaxGain` (upward), `hThresh`/`hRatio` (limiter) e os tempos em `applyResponse()`.

---
BeatFreak · Piradex — 2026
