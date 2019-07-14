#include <postgres.h>
#include <fmgr.h>

#include <utils/builtins.h>
#include <mupdf/fitz.h>

#define EXTENSION(function) Datum (function)(PG_FUNCTION_ARGS); PG_FUNCTION_INFO_V1(function); Datum (function)(PG_FUNCTION_ARGS)

PG_MODULE_MAGIC;

fz_context *ctx;

//static void *mupdf_malloc(void *arg, size_t size) { return palloc(size); }
//static void *mupdf_realloc(void *arg, void *p, size_t size) { return repalloc(p, size); }
//static void mupdf_free(void *arg, void *p) { if (p) (void)pfree(p); }

void _PG_init(void); void _PG_init(void) {
//    fz_alloc_context alloc_context = {NULL, mupdf_malloc, mupdf_realloc, mupdf_free};
//    if (!(ctx = fz_new_context(&alloc_context, NULL, FZ_STORE_UNLIMITED))) ereport(ERROR, (errmsg("!fz_new_context")));
    if (!(ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED))) ereport(ERROR, (errmsg("!fz_new_context")));
    //fz_try(ctx) 
    (void)fz_register_document_handlers(ctx);// fz_catch(ctx) ereport(ERROR, (errmsg("fz_register_document_handlers: %s", fz_caught_message(ctx))));
    //fz_try(ctx) 
    (void)fz_set_use_document_css(ctx, 1);// fz_catch(ctx) ereport(ERROR, (errmsg("fz_set_use_document_css: %s", fz_caught_message(ctx))));
}

void _PG_fini(void); void _PG_fini(void) {
    (void)fz_drop_context(ctx);
}

static void runpage(fz_document *doc, int number, fz_document_writer *out) {
    fz_rect mediabox;
    fz_page *page;
    fz_device *dev = NULL;
    //fz_try(ctx) 
    page = fz_load_page(ctx, doc, number - 1);// fz_catch(ctx) ereport(ERROR, (errmsg("fz_load_page: %s", fz_caught_message(ctx))));
    //fz_try(ctx) 
    (void)fz_var(dev);// fz_catch(ctx) ereport(ERROR, (errmsg("fz_var: %s", fz_caught_message(ctx))));
    //fz_try(ctx) 
    mediabox = fz_bound_page(ctx, page);// fz_catch(ctx) ereport(ERROR, (errmsg("fz_bound_page: %s", fz_caught_message(ctx))));
    //fz_try(ctx) 
    dev = fz_begin_page(ctx, out, mediabox);// fz_catch(ctx) ereport(ERROR, (errmsg("fz_begin_page: %s", fz_caught_message(ctx))));
    //fz_try(ctx) 
    (void)fz_run_page(ctx, page, dev, fz_identity, NULL);// fz_catch(ctx) ereport(ERROR, (errmsg("fz_run_page: %s", fz_caught_message(ctx))));
    //fz_try(ctx) 
    (void)fz_end_page(ctx, out);// fz_catch(ctx) ereport(ERROR, (errmsg("fz_end_page: %s", fz_caught_message(ctx))));
    //fz_try(ctx) 
    (void)fz_drop_page(ctx, page);// fz_catch(ctx) ereport(ERROR, (errmsg("fz_drop_page: %s", fz_caught_message(ctx))));
}

static void runrange(fz_document *doc, const char *range, fz_document_writer *out) {
    int start, end, count;
    fz_try(ctx) count = fz_count_pages(ctx, doc); fz_catch(ctx) ereport(ERROR, (errmsg("fz_count_pages: %s", fz_caught_message(ctx))));
    while ((range = fz_parse_page_range(ctx, range, &start, &end, count))) if (start < end) for (int i = start; i <= end; i++) runpage(doc, i, out); else for (int i = start; i >= end; i--) runpage(doc, i, out);
}

EXTENSION(pg_mupdf) {
    text *data;
    char *input, *output, *options = NULL, *page;
    fz_buffer *buf;
    fz_stream *stream;
    fz_document *doc;
    fz_document_writer *out;
    if (PG_ARGISNULL(0)) ereport(ERROR, (errmsg("data is null!")));
    data = DatumGetTextP(PG_GETARG_DATUM(0));
    if (PG_ARGISNULL(1)) ereport(ERROR, (errmsg("input is null!")));
    input = TextDatumGetCString(PG_GETARG_DATUM(1));
    if (PG_ARGISNULL(2)) ereport(ERROR, (errmsg("output is null!")));
    output = TextDatumGetCString(PG_GETARG_DATUM(2));
    if (!PG_ARGISNULL(3)) options = TextDatumGetCString(PG_GETARG_DATUM(3));
    if (PG_ARGISNULL(4)) ereport(ERROR, (errmsg("page is null!")));
    page = TextDatumGetCString(PG_GETARG_DATUM(4));
    elog(LOG, "pg_mupdf: data=%s, input=%s, output=%s, options=%s, page=%s", VARDATA_ANY(data), input, output, options, page);
    //fz_try(ctx) 
    buf = fz_new_buffer(ctx, VARSIZE_ANY_EXHDR(data));// fz_catch(ctx) ereport(ERROR, (errmsg("fz_new_buffer: %s", fz_caught_message(ctx))));
    //fz_try(ctx) 
    (void)fz_append_data(ctx, buf, VARDATA_ANY(data), VARSIZE_ANY_EXHDR(data));// fz_catch(ctx) ereport(ERROR, (errmsg("fz_append_data: %s", fz_caught_message(ctx))));
    //fz_try(ctx) 
    stream = fz_open_buffer(ctx, buf);// fz_catch(ctx) ereport(ERROR, (errmsg("fz_open_buffer: %s", fz_caught_message(ctx))));
    //fz_try(ctx) 
    doc = fz_open_document_with_stream(ctx, input, stream);// fz_catch(ctx) ereport(ERROR, (errmsg("fz_open_document_with_stream: %s", fz_caught_message(ctx))));
    //fz_try(ctx) 
    out = fz_new_document_writer(ctx, NULL, output, options);// fz_catch(ctx) ereport(ERROR, (errmsg("fz_new_document_writer: %s", fz_caught_message(ctx))));
    (void)runrange(doc, page, out);
    (void)fz_drop_document(ctx, doc);
    (void)fz_close_document_writer(ctx, out);
    (void)fz_drop_document_writer(ctx, out);
    (void)pfree(data);
    (void)pfree(input);
    (void)pfree(output);
    if (options) (void)pfree(options);
    (void)pfree(page);
    PG_RETURN_TEXT_P(cstring_to_text_with_len(VARDATA_ANY(data), VARSIZE_ANY_EXHDR(data)));
}
