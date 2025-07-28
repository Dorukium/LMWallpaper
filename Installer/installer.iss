#define MyAppName "LMWallpaper"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "LMWallpaper Team"
#define MyAppURL "https://github.com/lmwallpaper/lmwallpaper"
#define MyAppExeName "LMWallpaper.exe"

[Setup]
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
OutputDir=output
OutputBaseFilename=lmwallpaper_setup
Compression=lzma
SolidCompression=yes
WizardStyle=modern
DisableWelcomePage=no
DisableDirPage=no
DisableProgramGroupPage=yes
UninstallDisplayIcon={app}\{#MyAppExeName}
UninstallDisplayName={#MyAppName}
DefaultDirName={pf}\LMWallpaper  // Bu satır eklenmeli

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "turkish"; MessagesFile: "compiler:Languages\Turkish.isl"

[Files]
Source: "..\LMWallpaper\Release\LMWallpaper.exe"; DestDir: "{app}"
Source: "..\LMWallpaper\Release\*"; DestDir: "{app}"
Source: "..\External\opencv\*"; DestDir: "{app}\opencv"
Source: "..\External\nlohmann\*"; DestDir: "{app}\nlohmann"
Source: "..\External\ffmpeg\*"; DestDir: "{app}\ffmpeg"

[Dirs]
Name: "{app}\cache"
Name: "{app}\settings"

[Icons]
Name: "{group}\LMWallpaper"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\Uninstall LMWallpaper"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\{#MyAppExeName}"; Parameters: "--install"; WorkingDir: "{app}"; Flags: nowait

[UninstallRun]
Filename: "{app}\{#MyAppExeName}"; Parameters: "--uninstall"; WorkingDir: "{app}"; Flags: nowait

[CustomMessages]
english.StartMenuEntry="LMWallpaper"
turkish.StartMenuEntry="LMWallpaper"

[Code]
function InitializeSetup(): Boolean;
begin
    // Türkçe dil kontrolü
    if LanguageIs('tr') then begin
        WizardTitle('LMWallpaper Kurulumu');
        WizardWelcomeMsg('LMWallpaper\'i bilgisayarınıza kurmak üzeresiniz.');
        WizardPreparingMsg('Kurulum için gerekli dosyalar hazırlanıyor...');
        WizardInstallingMsg('LMWallpaper kurulumu yapılıyor...');
        WizardFinalMsg('Kurulum başarıyla tamamlandı!');
    end;
    
    Result := True;
end;

function UpdateRegistry(): Boolean;
var
    ErrorCode: Integer;
begin
    // Otomatik başlatma kayıt defteri girişi
    RegWriteValue(HKCU, 'Software\Microsoft\Windows\CurrentVersion\Run', 
                 'LMWallpaper', REG_SZ, 
                 ExpandConstant('{app}\{#MyAppExeName} --autostart'));
    
    Result := True;
end;

function Uninstall(): Boolean;
begin
    // Otomatik başlatma kayıt defteri girişini kaldır
    RegDeleteValue(HKCU, 'Software\Microsoft\Windows\CurrentVersion\Run', 'LMWallpaper');
    
    Result := True;
end;