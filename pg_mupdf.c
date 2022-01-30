#include <postgres.h>
#include <fmgr.h>

#include <utils/builtins.h>
#include <mupdf/fitz.h>

#define EXTENSION(function) Datum (function)(PG_FUNCTION_ARGS); PG_FUNCTION_INFO_V1(function); Datum (function)(PG_FUNCTION_ARGS)

PG_MODULE_MAGIC;

static void pg_mupdf_error_callback(void *user, const char *message) {
    ereport(ERROR, (errmsg("%s", message)));
}

static void pg_mupdf_warning_callback(void *user, const char *message) {
    ereport(WARNING, (errmsg("%s", message)));
}

static void runpage(fz_context *ctx, fz_document *doc, fz_document_writer *wri, int number) {
    fz_page *page = fz_load_page(ctx, doc, number - 1);
    fz_rect mediabox = fz_bound_page(ctx, page);
    fz_device *dev = fz_begin_page(ctx, wri, mediabox);
    fz_run_page(ctx, page, dev, fz_identity, NULL);
    fz_end_page(ctx, wri);
    fz_drop_page(ctx, page);
}

static void runrange(fz_context *ctx, fz_document *doc, fz_document_writer *wri, const char *range) {
    int start, end, count = fz_count_pages(ctx, doc);
    while ((range = fz_parse_page_range(ctx, range, &start, &end, count))) {
        if (start < end) for (int i = start; i <= end; ++i) runpage(ctx, doc, wri, i);
        else for (int i = start; i >= end; --i) runpage(ctx, doc, wri, i);
    }
}

EXTENSION(pg_mupdf) {
    bytea *pdf;
    char *input_type, *output_type, *options, *range;
    fz_buffer *buf;
    fz_context *ctx;
    fz_document *doc;
    fz_document_writer *wri;
    fz_output *out;
    fz_stream *stm;
    size_t output_len;
    text *input_data;
    unsigned char *output_data = NULL;
    if (PG_ARGISNULL(0)) ereport(ERROR, (errmsg("input_data is null!")));
    if (PG_ARGISNULL(1)) ereport(ERROR, (errmsg("input_type is null!")));
    if (PG_ARGISNULL(2)) ereport(ERROR, (errmsg("output_type is null!")));
    if (PG_ARGISNULL(3)) ereport(ERROR, (errmsg("options is null!")));
    if (PG_ARGISNULL(4)) ereport(ERROR, (errmsg("range is null!")));
    input_data = DatumGetTextP(PG_GETARG_DATUM(0));
    input_type = TextDatumGetCString(PG_GETARG_DATUM(1));
    output_type = TextDatumGetCString(PG_GETARG_DATUM(2));
    options = TextDatumGetCString(PG_GETARG_DATUM(3));
    range = TextDatumGetCString(PG_GETARG_DATUM(4));
    if (!(ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED))) ereport(ERROR, (errmsg("!fz_new_context")));
    fz_set_error_callback(ctx, pg_mupdf_error_callback, NULL);
    fz_set_warning_callback(ctx, pg_mupdf_warning_callback, NULL);
    fz_try(ctx) {
        fz_register_document_handlers(ctx);
        fz_set_use_document_css(ctx, 1);
        stm = fz_open_memory(ctx, (unsigned char *)VARDATA_ANY(input_data), VARSIZE_ANY_EXHDR(input_data));
        doc = fz_open_document_with_stream(ctx, input_type, stm);
        buf = fz_new_buffer(ctx, 0);
        out = fz_new_output_with_buffer(ctx, buf);
        wri = fz_new_document_writer_with_output(ctx, out, output_type, options);
        runrange(ctx, doc, wri, range);
        output_len = fz_buffer_storage(ctx, buf, &output_data);
        pdf = cstring_to_text_with_len((const char *)output_data, output_len);
        fz_drop_buffer(ctx, buf);
    } fz_always(ctx) {
        fz_close_document_writer(ctx, wri);
        fz_drop_document_writer(ctx, wri);
        fz_drop_document(ctx, doc);
        fz_drop_stream(ctx, stm);
    } fz_catch(ctx) {
        fz_rethrow(ctx);
    }
    fz_drop_context(ctx);
    pfree(input_type);
    pfree(output_type);
    pfree(options);
    pfree(range);
    PG_RETURN_BYTEA_P(pdf);
}
