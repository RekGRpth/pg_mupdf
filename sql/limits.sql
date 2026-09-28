CREATE EXTENSION IF NOT EXISTS pg_mupdf;
SELECT length(mupdf('', 'html', 'txt')); -- load the module
SHOW pg_mupdf.memory_limit;

-- allocation sizes in MuPDF messages depend on the build: show SQLSTATE and DETAIL only
CREATE FUNCTION mupdf_try(data text, output_type text, options text) RETURNS text LANGUAGE plpgsql AS $$
DECLARE
    detail text;
BEGIN
    PERFORM mupdf(data, 'html', output_type, options);
    RETURN 'ok';
EXCEPTION WHEN OTHERS THEN
    GET STACKED DIAGNOSTICS detail = PG_EXCEPTION_DETAIL;
    RETURN SQLSTATE || ' ' || coalesce(nullif(detail, ''), '-');
END $$;

SET pg_mupdf.memory_limit = '20MB';
SELECT mupdf_try('<p>x</p>', 'png', 'resolution=300');
SELECT mupdf_try('<p>x</p>', 'txt', '');
SET pg_mupdf.memory_limit = '64kB';
SELECT mupdf_try('<p>x</p>', 'txt', '');
SET pg_mupdf.memory_limit = 0;
SELECT mupdf_try('<p>x</p>', 'png', 'resolution=300');
RESET pg_mupdf.memory_limit;
-- a single allocation over 1 GB is refused without the limit DETAIL
SELECT mupdf_try('<p>x</p>', 'png', 'resolution=3000');

-- only superusers can change it
CREATE ROLE regress_mupdf_user;
SET ROLE regress_mupdf_user;
SET pg_mupdf.memory_limit = 0;
RESET ROLE;
DROP ROLE regress_mupdf_user;

-- a long render can be canceled
SET statement_timeout = '500ms';
SELECT length(mupdf(repeat('<p>hello world</p>', 100000)));
RESET statement_timeout;
SELECT convert_from(mupdf('<p>alive</p>', 'html', 'txt'), 'utf8');

DROP FUNCTION mupdf_try(text, text, text);
