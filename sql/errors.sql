CREATE EXTENSION IF NOT EXISTS pg_mupdf;

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
SELECT position('<svg'::bytea in mupdf('<p>x</p>', 'html', 'svg')) > 0 AS svg;
SELECT mupdf(repeat('<p>page</p>', 60), 'html', 'svg');

-- the session survives all of the above
SELECT convert_from(mupdf('<p>alive</p>', 'html', 'txt'), 'utf8');
