#!/bin/bash

# [dev] Normal build
CFLAGS="-mips32 -O2 -pipe"
LDFLAGS="-L/dingux/mipsel-linux-uclibc/lib -s"

# [pre-release] Profiling
#CFLAGS="-mips32 -O2 -pipe -fprofile-generate"
#LDFLAGS="-L/dingux/mipsel-linux-uclibc/lib -fprofile-generate"

# [release] Profiled build (15% performance increase, not bad for a freebie)
#CFLAGS="-mips32 -O2 -pipe -fprofile-use -fmove-loop-invariants"
#LDFLAGS="-L/dingux/mipsel-linux-uclibc/lib -s -fprofile-use -fmove-loop-invariants"

# [dev] Debug build for use with gdb
#CFLAGS="-g"
#LDFLAGS="-L/dingux/mipsel-linux-uclibc/lib"

# [dev] OProfile build (debug symbols), you can leave out O2 for better info, but that will produce a VERY slow binary with inaccurate measurements.
#CFLAGS="-mips32 -O2 -pipe -g"
#LDFLAGS="-L/dingux/mipsel-linux-uclibc/lib"

PREFIX=/dingux/mipsel-linux-uclibc
HOST=mipsel-linux-uclibc
TARGET=mipsel-linux-uclibc

OPTIONS="--disable-opengl --disable-sndfile --disable-lua --enable-dingoo --disable-networking"

../configure --prefix=$PREFIX --with-sdl-exec-prefix="$PREFIX" --host="$HOST" --target="$TARGET" CFLAGS="$CFLAGS" CXXFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" $OPTIONS


# END of script. Random comments follow.


# Vorbis support is required, without it'll output noise or downright crash (2010 source).
# M1A1 crashes with sndfile support enabled. Curious. The others have no trouble with it.
#
# For compiling the last subversion A1 source or newer stable 2010 sources:
# -L/dingux/mipsel-linux-uclibc/lib
# installing pkg-config for dingux: don't. These are the flags needed:
# -lpng12 -lvorbis -lm -logg
# init loop in shell.cpp causes crash without vorbis?. The 10-2009 source doesn't have this flaw.
# Nothing can be really disabled even though the flags exist.. perhaps just to harass you like
# "lol you think you can compile without OpenGL? THINK AGAIN :D <insert gazillion compiler errors or runtime segfault/freezes here>"
#
# Latest A1 SVN doesn't improve anything for sw rendering on the Dingoo, only adds more hassle. 2009 is what I stuck with.
# Liquid clipping bug still exists too. This is not caused by dingoo modifications. NOTE: Liquid clipping bug found and fixed :)
#
# Optimization flags used are quite optimal. -O3 increases size and launch time, and appears to be slightly slower. About the other flags:
# Already defined by O2+:
# -fthread-jumps -fexpensive-optimizations -fstrength-reduce
# Defined O1+:
# -fomit-frame-pointer
# Does not appear to do much for this game:
# -frename-registers
# Not compliant. Known to cause problems. Doesn't do much for this game:
# -ffast-math
# Will force software floating point operations, but MIPS-like CPUs support them in hardware I'd say, these are RISC CPUs (with FPU?)
# -msoft-float
#
# -s is used in ldflags now to strip the symbols from the binary, objcopy doesn't provide more so this saves you a step.
#
# - Nigel