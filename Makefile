$(OBJS): Makefile
DATA = pg_mupdf--1.0.sql
EXTENSION = pg_mupdf
MODULE_big = $(EXTENSION)
OBJS = $(EXTENSION).o
PG_CONFIG = pg_config
PGXS = $(shell $(PG_CONFIG) --pgxs)
SHLIB_LINK = -lmupdf -lfreetype -lharfbuzz -ljpeg -ljbig2dec -lopenjp2 -lmujs
include $(PGXS)
