/* contrib/autovacuum_informations/autovacuum_informations--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION autovacuum_informations" to load this file. \quit

CREATE OR REPLACE FUNCTION get_autovacuum_launcher_pid()
RETURNS bigint
AS '$libdir/autovacuum_informations', 'get_autovacuum_launcher_pid'
LANGUAGE C VOLATILE STRICT;

CREATE OR REPLACE FUNCTION get_autovacuum_workers_infos()
RETURNS setof record
AS '$libdir/autovacuum_informations', 'get_autovacuum_workers_infos'
LANGUAGE C VOLATILE STRICT;
