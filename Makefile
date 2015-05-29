# Sample makefile

!include <win32.mak>

all: midilog.exe

.c.obj:
  $(cc) $(cdebug) $(cflags) $(cvars) $*.c

midilog.exe: midilog.obj
  $(link) $(ldebug) $(conflags) -out:midilog.exe midilog.obj $(conlibs) Winmm.lib User32.lib
