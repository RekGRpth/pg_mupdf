EXTENSION = pg_mupdf
MODULE_big = $(EXTENSION)

PG_CONFIG = pg_config
OBJS = $(EXTENSION).o
DATA = pg_mupdf--1.0.sql

LIBS += -lmupdf -lmupdf-third -lfreetype -lharfbuzz -ljpeg -ljbig2dec -lopenjp2
SHLIB_LINK := $(LIBS)

PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

$(OBJS): Makefile