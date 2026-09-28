CREATE EXTENSION IF NOT EXISTS pg_mupdf;

-- MuPDF writes svg to memory from 1.25, csv from 1.26, raster images from 1.27: NULL for a format this build cannot write
CREATE FUNCTION mupdf_opt(data text, output_type text) RETURNS bytea LANGUAGE plpgsql AS $$
BEGIN
    RETURN mupdf(data, 'html', output_type);
EXCEPTION WHEN OTHERS THEN
    IF SQLERRM = 'unknown output document format: ' || output_type THEN
        RETURN NULL;
    END IF;
    RAISE;
END $$;

-- text output
SELECT convert_from(mupdf('<p>hello <b>world</b></p>', 'html', 'txt'), 'utf8');

-- binary formats: only signatures, sizes depend on the MuPDF build
SELECT substr(mupdf('<p>hello</p>'), 1, 5) = '%PDF-'::bytea AS pdf;
SELECT substr(mupdf('<p>hello</p>', 'html', 'pdf', 'compress'), 1, 5) = '%PDF-'::bytea AS pdf_options;
SELECT coalesce(substr(mupdf_opt('<p>hello</p>', 'png'), 1, 8) = '\x89504e470d0a1a0a'::bytea, true) AS png;
SELECT coalesce(position('<svg'::bytea in mupdf_opt('<p>hello</p>', 'svg')) > 0, true) AS svg;

-- xhtml and bytea input
SELECT convert_from(mupdf('<?xml version="1.0"?><html xmlns="http://www.w3.org/1999/xhtml"><body><p>xhtml</p></body></html>', 'xhtml', 'txt'), 'utf8');
SELECT convert_from(mupdf(convert_to('<p>bytea</p>', 'utf8'), 'html', 'txt'), 'utf8');

-- empty input
SELECT length(mupdf('', 'html', 'txt'));

-- page ranges: one letter per page
SELECT r, replace(convert_from(mupdf('<p style="font-size:400pt;margin:0">a</p><p style="font-size:400pt;margin:0">b</p><p style="font-size:400pt;margin:0">c</p>', 'html', 'txt', '', r), 'utf8'), E'\n', '') AS pages
FROM unnest(ARRAY['1-N', '1', '2', 'N', 'N-1', '2-3', '3-1', '1,1', '0', '5', '']) r;
SELECT length(mupdf('<p>x</p>', 'html', 'txt', '', 'garbage'));

-- compressed (TOAST) input used to be freed before MuPDF read it
CREATE TABLE mupdf_toast (h text);
INSERT INTO mupdf_toast VALUES (repeat('<p>hello world</p>', 1000)), (repeat('<p>hello world</p>', 20000));
SELECT pg_column_size(h) < octet_length(h) AS compressed, mupdf(h, 'html', 'txt') = mupdf(h || '', 'html', 'txt') AS same FROM mupdf_toast;
DROP TABLE mupdf_toast;
DROP FUNCTION mupdf_opt(text, text);
