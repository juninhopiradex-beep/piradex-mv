; LEVEL-X by Piradex — instalador Windows (Inno Setup)
; Instala o VST3 em C:\Program Files\Common Files\VST3

#define MyName "LEVEL-X"
#define MyVersion "1.0.0"
#define MyPublisher "Ricardo Amorim / Juninho Piradex"

; Caminhos passados pelo workflow (-D). Valores por omissão para build local.
#ifndef VST3SRC
  #define VST3SRC "out\LEVEL-X.vst3"
#endif
#ifndef OUTDIR
  #define OUTDIR "."
#endif

[Setup]
AppName={#MyName}
AppVersion={#MyVersion}
AppPublisher={#MyPublisher}
AppPublisherURL=https://www.beatfreakstudio.com
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
DisableWelcomePage=no
UninstallDisplayName={#MyName} (VST3)
OutputBaseFilename=LEVEL-X-Installer-Windows
OutputDir={#OUTDIR}
Compression=lzma2
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
PrivilegesRequired=admin
WizardStyle=modern

[Messages]
WelcomeLabel2=Este assistente vai instalar o {#MyName} (VST3) no teu sistema.%n%nCriador e Desenvolvedor: {#MyPublisher}%nVersão {#MyVersion}

[Files]
; Copia o bundle VST3 inteiro (pasta) para a pasta VST3 comum
Source: "{#VST3SRC}\*"; DestDir: "{commoncf64}\VST3\LEVEL-X.vst3"; \
    Flags: recursesubdirs createallsubdirs ignoreversion

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\LEVEL-X.vst3"
