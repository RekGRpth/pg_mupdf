PostgreSQL implementation of Convert HTML to PDF using MuPDF.

### [Use of the extension](#use-of-the-extension)

```sql
copy (
select mupdf($$<html>
    <head>
    </head>
    <body>
        Hello, World!
    </body>
</html>$$)
) to '/var/lib/postgresql/mupdf.pdf' WITH (FORMAT binary, HEADER false)
```
