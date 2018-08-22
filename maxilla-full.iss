
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING .ISS SCRIPT FILES!

[Setup]
AppName=Maxilla
AppVerName=Maxilla
DefaultDirName={pf}\Maxilla
DefaultGroupName=Maxilla
UninstallDisplayIcon={app}\maxilla.exe
Compression=lzma
SolidCompression=yes
OutputDir=.
OutputBaseFilename=Maxilla-0.175-Full
ChangesAssociations=yes

[InstallDelete]
Type: files; Name: "{app}/maxilla.exe";
Type: files; Name: "{app}/maxilla.exe.manifest";

[Files]
Source: "Release/maxilla.exe"; DestDir: "{app}"
Source: "Release/maxilla.exe.manifest"; DestDir: "{app}"
Source: "glut-3.7.6/lib/glut/Release/glut32.dll"; DestDir: "{app}"
Source: "imagemagik/convert.exe"; DestDir: "{app}"
Source: "imagemagik/vcomp100.dll"; DestDir: "{app}"
Source: "./distributable_upper.wrl"; DestDir: "{app}"
Source: "./distributable_lower.wrl"; DestDir: "{app}"
Source: "./distributable_combined.dst"; DestDir: "{app}"
Source: "c:/progra~2/Microsoft Visual Studio 9.0/VC/redist/x86/Microsoft.VC90.CRT/Microsoft.VC90.CRT.manifest"; DestDir: "{app}"
Source: "c:/progra~2/Microsoft Visual Studio 9.0/VC/redist/x86/Microsoft.VC90.CRT/msvcr90.dll"; DestDir: "{app}"
Source: "c:/progra~2/Microsoft Visual Studio 9.0/VC/redist/x86/Microsoft.VC90.CRT/msvcp90.dll"; DestDir: "{app}"
Source: "c:/progra~2/Microsoft Visual Studio 9.0/VC/redist/x86/Microsoft.VC90.CRT/msvcm90.dll"; DestDir: "{app}"
Source: "ghostscript-win32/gs909w32.exe"; DestDir: "{tmp}"

[Run]
Filename: "{tmp}\gs909w32.exe"; Parameters: "/S"; Flags: waituntilterminated skipifdoesntexist; StatusMsg: "Installing GScript...";

[Registry]
Root: HKCR; Subkey: ".wrl"; ValueType: string; ValueName: ""; ValueData: "VRML"; Flags: uninsdeletevalue
Root: HKCR; Subkey: "VRML"; ValueType: string; ValueName: ""; ValueData: "VRML"; Flags: uninsdeletekey
Root: HKCR; Subkey: "VRML\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\maxilla.exe,0"
Root: HKCR; Subkey: "VRML\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\maxilla.exe"" ""%1"""
Root: HKCR; Subkey: ".dst"; ValueType: string; ValueName: ""; ValueData: "DST"; Flags: uninsdeletevalue
Root: HKCR; Subkey: "DST"; ValueType: string; ValueName: ""; ValueData: "DST"; Flags: uninsdeletekey
Root: HKCR; Subkey: "DST\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\maxilla.exe,0"
Root: HKCR; Subkey: "DST\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\maxilla.exe"" ""%1"""

[Icons]
Name: "{group}\Maxilla"; Filename: "{app}\maxilla.exe"; WorkingDir: {app}
Name: "{userdesktop}\Maxilla"; Filename: "{app}\maxilla.exe"; WorkingDir: "{app}"

