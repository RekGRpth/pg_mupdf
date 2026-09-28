-- complain if script is sourced in psql, rather than via ALTER EXTENSION
\echo Use "ALTER EXTENSION pg_mupdf UPDATE TO '2.0'" to load this file. \quit

CREATE FUNCTION mupdf(data BYTEA, input_type TEXT DEFAULT 'html', output_type TEXT DEFAULT 'pdf', options TEXT DEFAULT '', range TEXT DEFAULT '1-N') RETURNS BYTEA AS 'MODULE_PATHNAME', 'pg_mupdf_bytea' LANGUAGE 'c';
