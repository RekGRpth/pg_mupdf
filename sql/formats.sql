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

-- every output format (svg used to crash); one this build cannot write counts as ok
SELECT f, coalesce(length(mupdf_opt('<p>hello</p>', f)) > 0, true) AS ok
FROM unnest(ARRAY['cbz', 'csv', 'docx', 'html', 'jpeg', 'odt', 'pam', 'pbm', 'pcl', 'pclm', 'pdf', 'pgm', 'pkm', 'png', 'pnm', 'ppm', 'ps', 'pwg', 'stext', 'stext.json', 'svg', 'text', 'txt', 'xhtml']) f;

-- text formats keep the text
SELECT f, position('hello'::bytea in mupdf('<p>hello</p>', 'html', f)) > 0 AS has_text
FROM unnest(ARRAY['html', 'stext.json', 'text', 'txt', 'xhtml']) f;
-- stext has the text of a line only from 1.27, its characters in every version
SELECT position('c="h"'::bytea in mupdf('<p>hello</p>', 'html', 'stext')) > 0 AS stext_has_chars;
DROP FUNCTION mupdf_opt(text, text);
