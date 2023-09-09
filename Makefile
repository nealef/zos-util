DEBUG=-DNDEBUG
DEFINES=-D_XOPEN_SOURCE_EXTENDED -D_UNIX03_THREADS -D_POSIX_THREADS -D_OPEN_SYS_FILE_EXT
INCLUDES=-I. -I/usr/local/include/python3.10 -I/opt/cpython/Include/internal
CFLAGS=$(DEBUG) -O3 -qarch=10 -qlanglvl=extc99 -q32 -Wc,DLL $(DEFINES) -qexportall -qascii -qstrict -qnocsect -qxplink -qgonumber -qenum=int

all: zos_util.cpython-310.so
	@cp zos_util.cpython-310.* /opt/cpython/build/lib.zvm-3.10/

zos_util.o : zos_util.c
	xlc $(CFLAGS) $(INCLUDES) -c $< -o $@

zos_util.cpython-310.so : zos_util.o
	xlc -Wl,dll -q32 -qxplink -O -qdll -L/usr/lcl/lib /usr/lcl/lib/libncurses.x /opt/cpython/libpython3.10.x $< -Wl,dll -o $@
