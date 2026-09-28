PostgreSQL implementation of Convert HTML to PDF using MuPDF.

### [Use of the extension](#use-of-the-extension)

```sql
select curl_easy_setopt_followlocation(1);
select curl_easy_setopt_url('https://github.com');
select curl_easy_perform();
copy (
select mupdf(convert_from(curl_easy_getinfo_data_in(), 'utf-8'), options:='compress')
) to '/var/lib/postgresql/mupdf.pdf' WITH (FORMAT binary, HEADER false)
```

`data` may be `text` or `bytea`. `text` is passed to MuPDF as UTF-8 (converted from the database encoding, except in `SQL_ASCII` databases). Use `bytea` for documents in another encoding — MuPDF honors `<meta http-equiv="Content-Type" content="...; charset=...">` and the XML `encoding` declaration, but not `<meta charset>` — and for binary input such as PDF (see `pg_mupdf.document_handlers`).

### Settings

`pg_mupdf.document_handlers` — comma-separated list of MuPDF document handlers allowed to parse input; only superusers can change it. Default: `html,xhtml` (those of them available in the MuPDF build). MuPDF chooses the parser by the content of the input, not by `input_type`, so this list is what decides which parsers are reachable. Available: `cbz`, `epub`, `fb2`, `gz`, `html`, `img`, `mobi`, `office`, `pdf`, `svg`, `txt`, `xhtml`, `xps`.

`pg_mupdf.memory_limit` — maximum memory MuPDF may allocate in one call; only superusers can change it. Default: `1GB`, `0` means no limit. A single allocation is also limited to 1 GB. A running call can be canceled (`statement_timeout`, `pg_cancel_backend`).
