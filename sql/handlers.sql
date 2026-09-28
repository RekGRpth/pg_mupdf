CREATE EXTENSION IF NOT EXISTS pg_mupdf;
SELECT length(mupdf('', 'html', 'txt')); -- load the module
SHOW pg_mupdf.document_handlers;

CREATE TABLE mupdf_doc AS SELECT mupdf('<p>inside pdf</p>') AS pdf;

-- MuPDF picks the parser by content, not by input_type: with the default
-- handlers the PDF parser is unreachable whatever input_type says
SELECT mupdf(pdf, 'pdf', 'txt') FROM mupdf_doc;
SELECT position('%PDF'::bytea in mupdf(pdf, 'html', 'txt')) > 0 AS parsed_as_html FROM mupdf_doc;

SET pg_mupdf.document_handlers = html, pdf;
SHOW pg_mupdf.document_handlers;
SELECT convert_from(mupdf(pdf, 'pdf', 'txt'), 'utf8') FROM mupdf_doc;
SELECT convert_from(mupdf(pdf, 'nosuch', 'txt'), 'utf8') FROM mupdf_doc;

-- MuPDF messages come before the error
SELECT mupdf(convert_to('%PDF-1.7 garbage', 'utf8'), 'pdf');

-- a document without pages
SELECT length(mupdf(mupdf('<p>x</p>', 'html', 'pdf', '', ''), 'pdf', 'txt'));

-- invalid values
SET pg_mupdf.document_handlers = 'html,foo';
SET pg_mupdf.document_handlers = 'html,,pdf';
SET pg_mupdf.document_handlers = '"HTML"';
SET pg_mupdf.document_handlers = '';
SELECT mupdf('<p>x</p>');

-- unquoted names are case-insensitive
SET pg_mupdf.document_handlers = HTML, PDF;
SHOW pg_mupdf.document_handlers;
SELECT convert_from(mupdf(pdf, 'pdf', 'txt'), 'utf8') FROM mupdf_doc;
RESET pg_mupdf.document_handlers;

-- only superusers can change it
CREATE ROLE regress_mupdf_user;
GRANT SELECT ON mupdf_doc TO regress_mupdf_user;
SET ROLE regress_mupdf_user;
SET pg_mupdf.document_handlers = 'html,pdf';
SELECT set_config('pg_mupdf.document_handlers', 'pdf', false);
SELECT mupdf(pdf, 'pdf', 'txt') FROM mupdf_doc;
RESET ROLE;

-- the prefix is reserved
SET pg_mupdf.nosuch = 1;

DROP TABLE mupdf_doc;
DROP ROLE regress_mupdf_user;
