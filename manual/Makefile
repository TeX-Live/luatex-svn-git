# $Id$

all:	luatexref-t.pdf functionref.pdf

functionref.pdf:	functionref.tex fdata.lua

%.pdf:	%.tex
	context $<

clean:
	- rm -f *.log
	- rm -f *.tuc
