-- text is converted to UTF-8 from the database encoding, except in SQL_ASCII
SELECT current_database() AS orig_db \gset
SET client_min_messages = warning;
DROP DATABASE IF EXISTS regression_mupdf_win1251;
DROP DATABASE IF EXISTS regression_mupdf_sql_ascii;
RESET client_min_messages;
CREATE DATABASE regression_mupdf_win1251 ENCODING 'WIN1251' LOCALE 'C' TEMPLATE template0;
CREATE DATABASE regression_mupdf_sql_ascii ENCODING 'SQL_ASCII' LOCALE 'C' TEMPLATE template0;

\c regression_mupdf_win1251
SET client_encoding = 'UTF8';
CREATE EXTENSION pg_mupdf;
SELECT convert_from(mupdf('<p>привет</p>', 'html', 'txt'), 'utf8') = E'привет\n\n' AS text;
CREATE TABLE mupdf_toast (h text);
INSERT INTO mupdf_toast VALUES (repeat('<p>привет мир</p>', 20000));
SELECT pg_column_compression(h) IS NOT NULL AS compressed, convert_from(mupdf(h, 'html', 'txt'), 'utf8') = repeat(E'привет мир\n\n', 20000) AS toast FROM mupdf_toast;
-- a text declaring the database encoding is now read wrong: pass such documents as bytea
SELECT convert_from(mupdf('<meta http-equiv="Content-Type" content="text/html; charset=windows-1251"><p>привет</p>', 'html', 'txt'), 'utf8') = E'привет\n\n' AS text_declared;
SELECT convert_from(mupdf(convert_to('<meta http-equiv="Content-Type" content="text/html; charset=windows-1251"><p>привет</p>', 'win1251'), 'html', 'txt'), 'utf8') = E'привет\n\n' AS bytea_declared;

\c regression_mupdf_sql_ascii
SET client_encoding = 'UTF8';
CREATE EXTENSION pg_mupdf;
-- SQL_ASCII text is passed as is
SELECT encode(mupdf('<p>привет</p>', 'html', 'txt'), 'hex') = encode(convert_to(E'привет\n\n', 'utf8'), 'hex') AS utf8_bytes;
SELECT encode(mupdf(convert_from('\x3c6d65746120687474702d65717569763d22436f6e74656e742d547970652220636f6e74656e743d22746578742f68746d6c3b20636861727365743d77696e646f77732d31323531223e3c703eeff0e8e2e5f23c2f703e'::bytea, 'sql_ascii'), 'html', 'txt'), 'hex') = encode(convert_to(E'привет\n\n', 'utf8'), 'hex') AS cp1251_bytes_declared;

\c :orig_db
DROP DATABASE regression_mupdf_win1251;
DROP DATABASE regression_mupdf_sql_ascii;
