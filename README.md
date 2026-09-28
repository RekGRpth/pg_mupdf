PostgreSQL implementation of Convert HTML to PDF using MuPDF.

### Requirements

- PostgreSQL 9.4 or later with the `pgxs` build infrastructure (`pg_config` on `PATH`).
- [MuPDF](https://mupdf.com/) 1.24 or later installed as a shared library (`libmupdf`), headers included. Earlier versions are refused at build time: their HTML parser reads stylesheets from the server's working directory (the data directory) and above it, any input it does not recognize goes to the PDF parser whatever `pg_mupdf.document_handlers` says, and an invalid page range renders the same page until memory runs out.
- When building MuPDF from source, use the system HarfBuzz (`USE_SYSTEM_HARFBUZZ=yes` or `USE_SYSTEM_LIBS=yes`), as distribution packages do: the bundled copy keeps static data in memory that pg_mupdf frees after each call, and the next call in the session crashes the server process.

Tested with PostgreSQL 9.4 to 19 and MuPDF 1.24, 1.25, 1.27 and 1.28.

### Installation

```sh
git clone https://github.com/RekGRpth/pg_mupdf.git
cd pg_mupdf
make
make install
```

Then, in the target database:

```sql
create extension pg_mupdf;
```

A database with version 1.0 is updated with `alter extension pg_mupdf update;`, which adds the `bytea` variant of `mupdf()`. Version 2.0 also changes behavior: by default only HTML and XHTML are parsed (see `pg_mupdf.document_handlers`), and `text` input is converted to UTF-8 (see [Input encoding](#input-encoding)).

Run the regression tests with `make installcheck` (requires a running server and superuser access; the tests create three temporary databases and connect as a temporary role).

### Use of the extension

```sql
select curl_easy_setopt_followlocation(1);
select curl_easy_setopt_url('https://github.com');
select curl_easy_perform();
copy (
    select mupdf(curl_easy_getinfo_data_in(), options:='compress')
) to '/var/lib/postgresql/mupdf.pdf' WITH (FORMAT binary, HEADER false)
```

```sql
select mupdf('<h1>Report</h1><p>Hello</p>');                         -- PDF
select mupdf('<p>Hello</p>', 'html', 'png', 'resolution=150');       -- PNG
select convert_from(mupdf('<p>Hello</p>', 'html', 'txt'), 'utf8');   -- plain text
```

### Functions

| Function | Returns |
| --- | --- |
| `mupdf(data text, input_type text default 'html', output_type text default 'pdf', options text default '', range text default '1-N')` | `bytea` |
| `mupdf(data bytea, input_type text default 'html', output_type text default 'pdf', options text default '', range text default '1-N')` | `bytea` |

- `data` — the document. `text` is converted to UTF-8, `bytea` is passed as is (see [Input encoding](#input-encoding)).
- `input_type` — MuPDF document type, such as `html`, `xhtml` or `text/html`. MuPDF chooses the parser by the content of the input when it recognizes it, and only the parsers listed in `pg_mupdf.document_handlers` are available.
- `output_type` — output format: `cbz`, `csv`, `docx`, `html`, `jpeg`, `odt`, `pam`, `pbm`, `pcl`, `pclm`, `pdf`, `pgm`, `pkm`, `png`, `pnm`, `ppm`, `ps`, `pwg`, `stext`, `stext.json`, `svg`, `text`/`txt`, `xhtml`. `svg` holds a single page only.
- `options` — comma-separated MuPDF writer options, such as `compress` for PDF or `resolution=300` for raster formats.
- `range` — pages to render in MuPDF syntax: `1-N` (all), `N-1` (all, reversed), `1,3,5-7`. Page numbers beyond the document are clamped to it; an invalid range is skipped with a warning; an empty range, like a document without pages, gives an output with no pages.

Every argument must be non-`NULL`. Problems MuPDF recovers from (a broken image, a repaired PDF) are reported as `WARNING` after the call, at most 100 per call.

### Input encoding

MuPDF reads HTML as UTF-8 unless the document declares its encoding. It honors `<meta http-equiv="Content-Type" content="text/html; charset=...">` and the XML `encoding` declaration, but not `<meta charset="...">`.

- `text` is converted from the database encoding to UTF-8 (in `SQL_ASCII` databases it is passed as is). A `text` document should therefore declare no encoding or UTF-8.
- `bytea` is passed unchanged: use it for documents in their own encoding and for binary input such as PDF or images.

### Settings

`pg_mupdf.document_handlers` — comma-separated list of MuPDF document handlers allowed to parse input; only superusers can change it. Default: `html,xhtml` (those of them available in the MuPDF build). MuPDF chooses the parser by the content of the input, not by `input_type`, so this list is what decides which parsers are reachable. Available: `cbz`, `epub`, `fb2`, `gz`, `html`, `img`, `mobi`, `office`, `pdf`, `svg`, `txt`, `xhtml`, `xps`.

`pg_mupdf.memory_limit` — maximum memory MuPDF may allocate in one call; only superusers can change it. Default: `1GB`, `0` means no limit. A single allocation is also limited to 1 GB. A running call can be canceled (`statement_timeout`, `pg_cancel_backend`).

### Security

- The document is opened from memory: MuPDF does not read local files or fetch URLs, so external resources must be embedded (`data:` URIs) or fetched beforehand, e.g. with [pg_curl](https://github.com/RekGRpth/pg_curl).
- Only the parsers in `pg_mupdf.document_handlers` are reachable, and memory use per call is bounded by `pg_mupdf.memory_limit`. The HTML parser still decodes images embedded in the document.
- `mupdf()` is executable by `PUBLIC` by default. To restrict it:

  ```sql
  revoke execute on function mupdf(text, text, text, text, text), mupdf(bytea, text, text, text, text) from public;
  ```

- MuPDF runs inside the PostgreSQL backend process, so a memory-safety bug in MuPDF itself can still crash the backend.
