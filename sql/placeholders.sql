CREATE EXTENSION IF NOT EXISTS pg_mupdf;
SELECT current_user AS orig_user \gset
CREATE ROLE regress_mupdf_login LOGIN;
CREATE TABLE mupdf_placeholder AS SELECT mupdf('<p>inside pdf</p>') AS pdf;
GRANT SELECT ON mupdf_placeholder TO regress_mupdf_login;

\c - regress_mupdf_login
-- the module is not loaded in this session yet: these only create placeholders
SET pg_mupdf.document_handlers = 'html,pdf';
SET pg_mupdf.memory_limit = 0;
-- loading the module rejects them
SELECT mupdf(pdf, 'pdf', 'txt') FROM mupdf_placeholder;
SHOW pg_mupdf.document_handlers;
SHOW pg_mupdf.memory_limit;

\c - :orig_user
DROP TABLE mupdf_placeholder;
DROP ROLE regress_mupdf_login;
