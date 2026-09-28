CREATE EXTENSION IF NOT EXISTS pg_mupdf;

-- every output format (svg used to crash)
SELECT f, mupdf('<p>hello</p>', 'html', f) IS NOT NULL AS ok
FROM unnest(ARRAY['cbz', 'csv', 'docx', 'html', 'jpeg', 'odt', 'pam', 'pbm', 'pcl', 'pclm', 'pdf', 'pgm', 'pkm', 'png', 'pnm', 'ppm', 'ps', 'pwg', 'stext', 'stext.json', 'svg', 'text', 'txt', 'xhtml']) f;

-- text formats keep the text
SELECT f, position('hello'::bytea in mupdf('<p>hello</p>', 'html', f)) > 0 AS has_text
FROM unnest(ARRAY['html', 'stext', 'stext.json', 'text', 'txt', 'xhtml']) f;
