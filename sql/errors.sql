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
CREATE FUNCTION mupdf_opt_error(data text, output_type text) RETURNS text LANGUAGE plpgsql AS $$
BEGIN
    RETURN CASE WHEN mupdf_opt(data, output_type) IS NULL THEN NULL ELSE 'ok' END;
EXCEPTION WHEN OTHERS THEN
    RETURN SQLERRM;
END $$;

-- NULL arguments
SELECT mupdf(NULL);
SELECT mupdf(NULL::bytea);
SELECT mupdf('<p>x</p>', NULL);
SELECT mupdf('<p>x</p>', 'html', NULL);
SELECT mupdf('<p>x</p>', 'html', 'pdf', NULL);
SELECT mupdf('<p>x</p>', 'html', 'pdf', '', NULL);

-- unknown types used to crash the backend
SELECT mupdf('<p>x</p>', 'nosuchtype');
SELECT mupdf('<p>x</p>', 'html', 'nosuchout');
SELECT mupdf('<p>x</p>', '');
SELECT mupdf('<p>x</p>', 'html', '');

-- the output type is a format name, never a file
SELECT mupdf('<p>x</p>', 'html', '/tmp/pg_mupdf_regress.pdf');

-- svg output used to crash; svg holds one page only
SELECT coalesce(position('<svg'::bytea in mupdf_opt('<p>x</p>', 'svg')) > 0, true) AS svg;
SELECT coalesce(mupdf_opt_error(repeat('<p>page</p>', 60), 'svg') = 'cannot write multiple pages to a single SVG output', true) AS svg_one_page;

-- the session survives all of the above
SELECT convert_from(mupdf('<p>alive</p>', 'html', 'txt'), 'utf8');
DROP FUNCTION mupdf_opt_error(text, text);
DROP FUNCTION mupdf_opt(text, text);
