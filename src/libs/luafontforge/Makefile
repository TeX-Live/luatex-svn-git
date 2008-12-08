# $Id$

srcdir=../../../src/libs/luafontforge

FF_LIB=ffdummies.o luafflib.o

_CFLAGS=-D_NO_LIBSPIRO -DFONTFORGE_CONFIG_NO_WINDOWING_UI=1 -DLUA_FF_LIB=1 $(XCFLAGS)

.PHONY: newfile

fontforge: $(FF_LIB) newfile
	rm -f libff.a
	cd fontforge && $(MAKE) libs
	cd fontforge/fontforge && $(MAKE) ../libfontforge.la
	sh -c "ar cru libff.a fontforge/fontforge/*.o fontforge/gutils/*.o fontforge/Unicode/*.o $(FF_LIB)"
	ranlib libff.a

%.o: $(srcdir)/src/%.c
	$(CC) $(_CFLAGS) $(CFLAGS) -I$(srcdir)/../lua51 -I$(srcdir)/fontforge/inc -I$(srcdir)/fontforge/fontforge -c $< -o $@

clean:
	rm  -f libff.a $(FF_LIB)
	cd fontforge; $(MAKE) clean
