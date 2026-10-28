# Assinatura + Notarização Apple (LEVEL-X)

O workflow já tem os passos de **codesign** (dos bundles), **assinatura do .pkg**
e **notarização**. Ficam *inativos* até adicionares os segredos no GitHub.
Enquanto não os adicionares, o build gera binários **não assinados** (precisam
de `xattr -cr`, como agora).

## 1. Pré-requisitos
- Conta **Apple Developer** (99 USD/ano).
- Dois certificados (cria-os no Keychain/developer.apple.com → Certificates):
  - **Developer ID Application** — assina o .vst3 / .component / .app
  - **Developer ID Installer** — assina o .pkg
- Exporta cada um como **.p12** (com palavra-passe) a partir do Keychain Access.
- Uma **app-specific password** da tua Apple ID (appleid.apple.com → Segurança).
- O teu **Team ID** (developer.apple.com → Membership).

## 2. Converter os .p12 para base64
```bash
base64 -i DeveloperID_Application.p12 | pbcopy   # cola em MACOS_CERTIFICATE
base64 -i DeveloperID_Installer.p12  | pbcopy    # cola em MACOS_INSTALLER_CERTIFICATE
```

## 3. Adicionar os segredos no GitHub
Repositório → **Settings → Secrets and variables → Actions → New repository secret**:

| Secret | Valor |
|---|---|
| `MACOS_CERTIFICATE` | base64 do .p12 Developer ID **Application** |
| `MACOS_CERTIFICATE_PWD` | palavra-passe desse .p12 |
| `MACOS_INSTALLER_CERTIFICATE` | base64 do .p12 Developer ID **Installer** |
| `MACOS_INSTALLER_CERTIFICATE_PWD` | palavra-passe desse .p12 |
| `MACOS_SIGN_IDENTITY` | ex.: `Developer ID Application: Ricardo Amorim (TEAMID)` |
| `MACOS_INSTALLER_IDENTITY` | ex.: `Developer ID Installer: Ricardo Amorim (TEAMID)` |
| `APPLE_ID` | o teu email Apple ID |
| `APPLE_TEAM_ID` | o Team ID (10 caracteres) |
| `APPLE_APP_PASSWORD` | app-specific password |
| `KEYCHAIN_PASSWORD` | qualquer palavra-passe (temporária, para o keychain do runner) |

O nome exato das identidades obténs com:
```bash
security find-identity -v -p codesigning
```

## 4. Pronto
No próximo push, o workflow assina os bundles, assina o `.pkg` e **notariza**
(`notarytool --wait`) + faz `stapler staple`. O instalador passa a abrir no
macOS **sem avisos** do Gatekeeper, incluindo no Catalina.
