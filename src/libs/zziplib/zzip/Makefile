
top_srcdir = ../../../../src/libs/zziplib
srcdir = $(top_srcdir)/zzip

CC = gcc
AR = ar cru
RANLIB = ranlib

OBJECTS =dir.o err.o fetch.o file.o info.o memdisk.o mmapped.o \
         plugin.o stat.o write.o zip.o

_CFLAGS = -DHAVE_CONFIG_H -I.. -I../../../../src/libs/zziplib -I../../../../src/libs/zlib -Wall

XCFLAGS = -fomit-frame-pointer $(_CFLAGS)

all: $(OBJECTS)
	$(AR) libzzip.a $(OBJECTS)

%.o: $(srcdir)/%.c 
	$(CC) $(CFLAGS) $(XCFLAGS) -c $< -o $@

