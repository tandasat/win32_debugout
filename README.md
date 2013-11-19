win32_debugout
==============

This DLL shows debug strings on DebubView from an attached process by
win32_remote.exe. It solves the problem that you cannot see debug strings from
a debugee process on DebugView when you are remotely debugging it with IDA Pro.


Usage
-----------------

1. Launch DebugView and enable Capture Win32,
- Inject this DLL to win32_remote.exe with your favorite tool.
- Confirm the string "DLL installation succeeded." on DebugView.

Then, DebugView will show debug strings as well as an Output Window on IDA Pro.

Note
-----------------
This DLL can work only when:

- win32_remote.exe is not being affected by ASLR (usually on XP)
- win32_remote.exe is one that is included in IDA Pro 6.4

