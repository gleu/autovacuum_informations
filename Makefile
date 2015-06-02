# contrib/autovacuum_informations/Makefile

MODULE_big = autovacuum_informations
OBJS = autovacuum_informations.o $(WIN32RES)
PG_CPPFLAGS = -I$(libpq_srcdir)

EXTENSION = autovacuum_informations
DATA = autovacuum_informations--1.0.sql
PGFILEDESC = "autovacuum_informations - more informations on autovacuum work"

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
