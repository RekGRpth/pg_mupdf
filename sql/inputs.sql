CREATE EXTENSION IF NOT EXISTS pg_mupdf;

CREATE FUNCTION mupdf_render(data bytea, input_type text) RETURNS text LANGUAGE plpgsql AS $$
BEGIN
    RETURN CASE WHEN substr(mupdf(data, input_type, 'pdf'), 1, 5) = '%PDF-'::bytea THEN 'rendered' ELSE 'not a pdf' END;
EXCEPTION WHEN OTHERS THEN
    RETURN SQLERRM;
END $$;
CREATE TABLE mupdf_inputs AS
SELECT f, t, mupdf('<p>hello</p>', 'html', f) AS data
FROM (VALUES ('png', 'png'), ('jpeg', 'jpg'), ('pnm', 'pnm'), ('svg', 'svg'), ('cbz', 'cbz'), ('docx', 'docx')) v(f, t)
UNION ALL SELECT 'txt', 'txt', convert_to('plain text', 'utf8')
UNION ALL SELECT 'fb2', 'fb2', convert_to('<?xml version="1.0"?><FictionBook xmlns="http://www.gribuv.net/FictionBook2/2.0"><body><section><p>fb2 text</p></section></body></FictionBook>', 'utf8');

-- none of these parsers is reachable with the default handlers
SELECT f, mupdf_render(data, t) FROM mupdf_inputs;

SET pg_mupdf.document_handlers = 'html,img,svg,cbz,office,txt,fb2';
SELECT f, mupdf_render(data, t) FROM mupdf_inputs;
SELECT f, convert_from(mupdf(data, t, 'txt'), 'utf8') FROM mupdf_inputs WHERE f IN ('docx', 'txt', 'fb2');

DROP TABLE mupdf_inputs;
DROP FUNCTION mupdf_render;
