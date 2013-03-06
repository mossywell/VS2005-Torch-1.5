<html>
<body>
<pre>
<h1>Build Log</h1>
<h3>
--------------------Configuration: Torch - Win32 (WCE ARMV4) Release--------------------
</h3>
<h3>Command Lines</h3>
Creating temporary file "C:\TEMP\RSP369.tmp" with contents
[
/nologo /W3 /D _WIN32_WCE=420 /D "WIN32_PLATFORM_WFSP=200" /D "ARM" /D "_ARM_" /D "ARMV4" /D UNDER_CE=420 /D "UNICODE" /D "_UNICODE" /D "NDEBUG" /Fp"ARMV4Rel/Torch.pch" /YX /Fo"ARMV4Rel/" /O2 /MC /c 
"C:\Torch 1.4\Torch.cpp"
]
Creating command line "clarm.exe @C:\TEMP\RSP369.tmp" 
Creating temporary file "C:\TEMP\RSP36A.tmp" with contents
[
commctrl.lib coredll.lib aygshell.lib /nologo /base:"0x00010000" /stack:0x10000,0x1000 /entry:"WinMainCRTStartup" /incremental:no /pdb:"ARMV4Rel/Torch.pdb" /nodefaultlib:"libc.lib /nodefaultlib:libcd.lib /nodefaultlib:libcmt.lib /nodefaultlib:libcmtd.lib /nodefaultlib:msvcrt.lib /nodefaultlib:msvcrtd.lib" /out:"ARMV4Rel/Torch.exe" /subsystem:windowsce,4.20 /align:"4096" /MACHINE:ARM 
".\ARMV4Rel\Torch.obj"
".\ARMV4Rel\Torch.res"
]
Creating command line "link.exe @C:\TEMP\RSP36A.tmp"
<h3>Output Window</h3>
Compiling...
Torch.cpp
Linking...
Signing C:\Torch 1.4\ARMV4Rel\Torch.exe
Error: Unable to open a CSP provider with the correct private key
Error: Signing Failed.  Result = 80092006, (-2146885626)




<h3>Results</h3>
Torch.exe - 0 error(s), 0 warning(s)
</pre>
</body>
</html>
