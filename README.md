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
