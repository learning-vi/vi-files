all: exrm exsummary exchanges edittut editvindex viin viapp visummary viapwh
# TROFF will frequently be just troff if you have a typesetter
# but no versatec.
#TROFF=	/usr/ucb/vtroff -x
TROFF=nroff 
exrm:
	${TROFF} -ms ex.rm > ex.rm.man;
exsummary:
	tbl ex.summary | ${TROFF} -ms > ex.summary.man;
exchanges:
	${TROFF} -ms ex1.1-2.0 > ex1.1-2.0.man;
	${TROFF} -ms ex2.0-3.1 > ex2.0-3.1.man;
	${TROFF} -ms ex3.1-3.5 > ex3.1-3.5.man;
edittut:
	${TROFF} -ms edit.tut > edit.tut.man;
# index for versatec is different from the one in edit.tut
# because the fonts are different and entries reference page
# rather than section numbers.  if you have a typesetter
# you should just use the index in edit.tut, and ignore editvindex.
editvindex:
	${TROFF} -ms -n22 edit.vindex > edit.vindex.man;
viin:
	tbl vi.in | ${TROFF} -ms > vi.in.man;
viapp:
# numbers below are guessed at
# if using a typesetter use -n23 instead of -n25.
	tbl vi.chars | ${TROFF} -ms -n23 > vi.chars.man;
visummary:
	tbl vi.summary | ${TROFF} -ms > vi.summary.man;
viapwh:
	${TROFF} -ms vi.apwh.ms > vi.apwh.ms.man; 
