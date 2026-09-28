-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pg_mupdf" to load this file. \quit

CREATE FUNCTION mupdf(data TEXT, input_type TEXT DEFAULT 'html', output_type TEXT DEFAULT 'pdf', options TEXT DEFAULT '', range TEXT DEFAULT '1-N') RETURNS BYTEA AS 'MODULE_PATHNAME', 'pg_mupdf_text' LANGUAGE 'c';
CREATE FUNCTION mupdf(data BYTEA, input_type TEXT DEFAULT 'html', output_type TEXT DEFAULT 'pdf', options TEXT DEFAULT '', range TEXT DEFAULT '1-N') RETURNS BYTEA AS 'MODULE_PATHNAME', 'pg_mupdf_bytea' LANGUAGE 'c';
