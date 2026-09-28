DROP EXTENSION IF EXISTS pg_mupdf;

CREATE EXTENSION pg_mupdf VERSION '1.0';
SELECT oid::regprocedure, prosrc FROM pg_proc WHERE proname = 'mupdf' ORDER BY 1::text;
SELECT convert_from(mupdf('<p>1.0</p>', 'html', 'txt'), 'utf8');

ALTER EXTENSION pg_mupdf UPDATE;
SELECT extversion FROM pg_extension WHERE extname = 'pg_mupdf';
SELECT oid::regprocedure, prosrc FROM pg_proc WHERE proname = 'mupdf' ORDER BY 1::text;
SELECT convert_from(mupdf('<p>updated text</p>', 'html', 'txt'), 'utf8');
SELECT convert_from(mupdf(convert_to('<p>updated bytea</p>', 'utf8'), 'html', 'txt'), 'utf8');

DROP EXTENSION pg_mupdf;
CREATE EXTENSION pg_mupdf;
SELECT extversion FROM pg_extension WHERE extname = 'pg_mupdf';
SELECT oid::regprocedure, prosrc FROM pg_proc WHERE proname = 'mupdf' ORDER BY 1::text;
