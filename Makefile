$(OBJS): Makefile
DATA = pg_mupdf--1.0.sql pg_mupdf--1.0--2.0.sql pg_mupdf--2.0.sql
EXTENSION = pg_mupdf
MODULE_big = $(EXTENSION)
OBJS = $(EXTENSION).o
PG_CONFIG = pg_config
REGRESS = $(patsubst sql/%.sql,%,$(TESTS))
TESTS = $(wildcard sql/*.sql)
PGXS = $(shell $(PG_CONFIG) --pgxs)
SHLIB_LINK = -lmupdf
include $(PGXS)
