CREATE EXTENSION IF NOT EXISTS pg_mupdf;

-- MuPDF warnings are reported after MuPDF returns
SELECT mupdf('<p>x</p>', 'html', 'txt', '', 'garbage') IS NOT NULL AS ok;

-- at most 100 per call
SELECT mupdf(string_agg('<img src="x' || i || '">', ''), 'html', 'txt') IS NOT NULL AS ok FROM generate_series(1, 150) i;

-- no cleanup noise on error paths
SELECT mupdf('<p>x</p>', 'html', 'nosuchout');

-- local files are not read (relative paths would resolve against the data directory)
SELECT mupdf('<link rel="stylesheet" href="postgresql.conf"><p>x</p>', 'html', 'txt') IS NOT NULL AS ok;
SELECT mupdf('<img src="file:///etc/passwd"><p>x</p>', 'html', 'txt') IS NOT NULL AS ok;
