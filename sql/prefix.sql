CREATE EXTENSION IF NOT EXISTS pg_mupdf;
SELECT length(mupdf('', 'html', 'txt')); -- load the module

-- the prefix is reserved since 15 (expected/prefix_1.out covers older versions)
SET pg_mupdf.nosuch = 1;
