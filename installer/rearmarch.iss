; SPDX-License-Identifier: GPL-3.0-or-later
;
; Inno Setup script for RearMarch.
;
; Per user install, no elevation: everything the application writes lives in
; %LOCALAPPDATA%, so there is nothing to elevate for.
;
; Build with:  iscc installer\rearmarch.iss
; Expects a Release build first:  cmake --build build --config Release
;
; Keep AppVersion in sync with project(VERSION ...) in CMakeLists.txt. The
; version in the executable itself is stamped at compile time from CMake, so
; only this line and CHANGELOG.md are manual.

#define AppName "RearMarch"
#define AppVersion "0.1.0"
#define AppId "{{EDCC69F6-21F9-472A-ABF7-97540B78512F}"
#define AppUrl "https://github.com/thewoldaa/rear-march"
#define AppSource "..\build\Release"

[Setup]
AppId={#AppId}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher=thewoldaa <thewoldaa@users.noreply.github.com>
AppPublisherURL={#AppUrl}
AppSupportURL={#AppUrl}/issues
AppUpdatesURL={#AppUrl}/releases
DefaultDirName={localappdata}\Programs\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
AllowNoIcons=yes
LicenseFile=..\LICENSE
OutputDir=Output
OutputBaseFilename={#AppName}-{#AppVersion}-setup
SetupIconFile=..\resources\rearmarch.ico
UninstallDisplayIcon={app}\RearMarch.exe
UninstallDisplayName={#AppName} {#AppVersion}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
VersionInfoVersion={#AppVersion}
VersionInfoCompany=thewoldaa
VersionInfoDescription={#AppName} Setup
VersionInfoProductName={#AppName}
VersionInfoProductVersion={#AppVersion}
VersionInfoCopyright=GPL-3.0-or-later

; The application holds this mutex for its whole lifetime. Without it Setup
; would overwrite a running copy and leave the user with two hooks installed.
AppMutex=Local\RearMarch.Instance.v1
CloseApplications=yes
; The postinstall [Run] entry already offers to start the application, so a
; copy that Setup closed for an upgrade is not restarted a second time.
RestartApplications=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
; Inno Setup does not ship Indonesian, so the translation is committed under
; installer\languages\. It is the official unofficial one from
; https://jrsoftware.org/files/istrans/ (maintained by Edward Tie).
Name: "indonesian"; MessagesFile: "languages\Indonesian.isl"

[CustomMessages]
english.DeleteSettings=Also delete your settings?
indonesian.DeleteSettings=Hapus juga pengaturan Anda?

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
; The application asks to run at login on its own first start. Creating the
; entry here keeps the "Programs and Features" story tidy (the uninstaller
; removes it), and declining it is honoured: the post-install launch passes
; --no-autostart so the application does not re-create what was refused.
Name: "autostart"; Description: "{cm:AutoStartProgram,{#AppName}}"; GroupDescription: "{cm:AutoStartProgramGroupDescription}"

[Files]
Source: "{#AppSource}\RearMarch.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#AppSource}\locales\*"; DestDir: "{app}\locales"; Flags: ignoreversion
Source: "..\docs\*"; DestDir: "{app}\docs"; Flags: ignoreversion
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\CHANGELOG.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion

; Upgrading must not leave a translation or document from the previous version
; behind. Both directories are rewritten completely just below.
[InstallDelete]
Type: filesandordirs; Name: "{app}\locales"
Type: filesandordirs; Name: "{app}\docs"

[Icons]
Name: "{autoprograms}\{#AppName}"; Filename: "{app}\RearMarch.exe"; Comment: "Turn the game camera with one key"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\RearMarch.exe"; Comment: "Turn the game camera with one key"; Tasks: desktopicon

[Registry]
; Matches the value the application writes itself, so both agree on what
; "start with Windows" means. Removed on uninstall even if it was disabled in
; the meantime (delete is harmless when the value is already gone).
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "{#AppName}"; ValueData: """{app}\RearMarch.exe"" --minimized"; Flags: uninsdeletevalue; Tasks: autostart
; Clearing the task on a reinstall has to remove the entry the previous install
; left behind, otherwise the choice would be quietly ignored while the old entry
; keeps launching the application.
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "{#AppName}"; Flags: deletevalue; Tasks: not autostart

[Run]
Filename: "{app}\RearMarch.exe"; Parameters: "--locale {code:GetLocaleCode}"; Description: "{cm:LaunchProgram,{#AppName}}"; Flags: nowait postinstall skipifsilent; Tasks: autostart
Filename: "{app}\RearMarch.exe"; Parameters: "--locale {code:GetLocaleCode} --no-autostart"; Description: "{cm:LaunchProgram,{#AppName}}"; Flags: nowait postinstall skipifsilent; Tasks: not autostart

[Code]

{ Language code for the application's --locale option, taken from the language
  the wizard was run in. The application only uses it on a first run, so it
  seeds the dashboard language without ever overriding a later choice. }
function GetLocaleCode(Param: String): String;
begin
  case ActiveLanguage of
    'indonesian': Result := 'id';
  else
    Result := 'en';
  end;
end;

{ Settings live in the user's profile and survive an upgrade on purpose. Ask
  before removing them, and only at uninstall. SuppressibleMsgBox rather than
  MsgBox so an unattended /VERYSILENT uninstall does not wait for an answer;
  the default there is to keep the settings, which is the safe direction. }
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  DataDir: String;
begin
  if CurUninstallStep <> usPostUninstall then
    Exit;

  DataDir := ExpandConstant('{localappdata}\RearMarch');
  if not DirExists(DataDir) then
    Exit;

  if SuppressibleMsgBox(ExpandConstant('{cm:DeleteSettings}'), mbConfirmation,
                        MB_YESNO, IDNO) = IDYES then
    DelTree(DataDir, True, True, True);
end;
