@echo off
SET EMXOMFLD_TYPE=WLINK
SET EMXOMFLD_LINKER=wl.exe
SET EMXOMFLD_PRELINK=0
make -f Makefile.os2 clean
make -f Makefile.os2 %1 2>&1 | tee compile.log
