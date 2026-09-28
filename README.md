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

### Settings

`pg_mupdf.document_handlers` — comma-separated list of MuPDF document handlers allowed to parse input; only superusers can change it. Default: `html,xhtml`. MuPDF chooses the parser by the content of the input, not by `input_type`, so this list is what decides which parsers are reachable. Available: `cbz`, `epub`, `fb2`, `gz`, `html`, `img`, `mobi`, `office`, `pdf`, `svg`, `txt`, `xhtml`, `xps`.
