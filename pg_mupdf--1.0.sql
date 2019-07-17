-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pg_mupdf" to load this file. \quit

CREATE OR REPLACE FUNCTION mupdf(data TEXT, input_type TEXT DEFAULT 'html', output_type TEXT DEFAULT 'pdf', options TEXT DEFAULT '', range TEXT DEFAULT '1-N') RETURNS TEXT AS 'MODULE_PATHNAME', 'pg_mupdf' LANGUAGE 'c';
