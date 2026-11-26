[Setup]
AppId=PulseStudioDAW
AppName=Pulse Studio
AppVersion=0.15.0
DefaultDirName={autopf}\Pulse Studio
DefaultGroupName=Pulse Studio
OutputBaseFilename=PulseStudio-Setup
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
Compression=lzma2
SolidCompression=yes
[Files]
Source: "{#AppExe}"; DestDir: "{app}"; Flags: ignoreversion
[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; Flags: unchecked
[Icons]
Name: "{group}\Pulse Studio"; Filename: "{app}\Pulse Studio.exe"
Name: "{autodesktop}\Pulse Studio"; Filename: "{app}\Pulse Studio.exe"; Tasks: desktopicon
[Run]
Filename: "{app}\Pulse Studio.exe"; Description: "Launch Pulse Studio"; Flags: nowait postinstall skipifsilent
