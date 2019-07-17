#include <postgres.h>
#include <fmgr.h>

#include <utils/builtins.h>
#include <mupdf/fitz.h>

#define EXTENSION(function) Datum (function)(PG_FUNCTION_ARGS); PG_FUNCTION_INFO_V1(function); Datum (function)(PG_FUNCTION_ARGS)

PG_MODULE_MAGIC;

fz_context *ctx;

void _PG_init(void); void _PG_init(void) {
    if (!(ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED))) ereport(ERROR, (errmsg("!fz_new_context")));
    fz_try(ctx) {
        fz_register_document_handlers(ctx);
        fz_set_use_document_css(ctx, 1);
    } fz_catch(ctx) {
        ereport(ERROR, (errmsg("fz_caught_message: %s", fz_caught_message(ctx))));
    }
}

void _PG_fini(void); void _PG_fini(void) {
    fz_drop_context(ctx);
}

static void runpage(fz_document *doc, int number, fz_document_writer *wri) {
    fz_page *page = fz_load_page(ctx, doc, number - 1);
//    elog(LOG, "runpage: number=%i", number);
    fz_try(ctx) {
        fz_rect mediabox = fz_bound_page(ctx, page);
        fz_device *dev = fz_begin_page(ctx, wri, mediabox);
        fz_run_page(ctx, page, dev, fz_identity, NULL);
        fz_end_page(ctx, wri);
    } fz_always(ctx) {
        fz_drop_page(ctx, page);
    } fz_catch(ctx) {
        fz_rethrow(ctx);
    }
}

static void runrange(fz_document *doc, const char *range, fz_document_writer *wri) {
    int start, end, count;
//    elog(LOG, "runrange: range=%s", range);
    count = fz_count_pages(ctx, doc);
    while ((range = fz_parse_page_range(ctx, range, &start, &end, count))) {
        if (start < end) {
            for (int i = start; i <= end; i++) {
                runpage(doc, i, wri);
            }
        } else {
            for (int i = start; i >= end; i--) {
                runpage(doc, i, wri);
            }
        }
    }
}

EXTENSION(pg_mupdf) {
    text *input_data;
    char *input_type, *output_type, *options, *range;
    fz_buffer *input_buffer, *output_buffer;
    fz_stream *input_stream;
    fz_document *doc;
    fz_document_writer *wri;
    unsigned char *output_data = NULL;
    size_t output_len = 0;
    if (PG_ARGISNULL(0)) ereport(ERROR, (errmsg("input_data is null!")));
    input_data = DatumGetTextP(PG_GETARG_DATUM(0));
    if (PG_ARGISNULL(1)) ereport(ERROR, (errmsg("input_type is null!")));
    input_type = TextDatumGetCString(PG_GETARG_DATUM(1));
    if (PG_ARGISNULL(2)) ereport(ERROR, (errmsg("output_type is null!")));
    output_type = TextDatumGetCString(PG_GETARG_DATUM(2));
    if (PG_ARGISNULL(3)) ereport(ERROR, (errmsg("options is null!")));
    options = TextDatumGetCString(PG_GETARG_DATUM(3));
    if (PG_ARGISNULL(4)) ereport(ERROR, (errmsg("range is null!")));
    range = TextDatumGetCString(PG_GETARG_DATUM(4));
//    elog(LOG, "pg_mupdf: input_data=%s, input_type=%s, output_type=%s, options=%s, range=%s", VARDATA_ANY(input_data), input_type, output_type, options, range);
    fz_try(ctx) {
        output_buffer = fz_new_buffer(ctx, 0);
        fz_set_user_context(ctx, output_buffer);
        input_buffer = fz_new_buffer_from_data(ctx, (unsigned char *)VARDATA_ANY(input_data), VARSIZE_ANY_EXHDR(input_data));
        input_stream = fz_open_buffer(ctx, input_buffer);
        doc = fz_open_document_with_stream(ctx, input_type, input_stream);
        wri = fz_new_document_writer(ctx, "buf:", output_type, options);
        runrange(doc, range, wri);
    } fz_always(ctx) {
        fz_close_document_writer(ctx, wri);
        fz_drop_document_writer(ctx, wri);
        fz_drop_document(ctx, doc);
        fz_drop_buffer(ctx, input_buffer);
    } fz_catch(ctx) {
        ereport(ERROR, (errmsg("fz_caught_message: %s", fz_caught_message(ctx))));
    }
    output_len = fz_buffer_storage(ctx, output_buffer, &output_data);
    pfree(input_data);
    pfree(input_type);
    pfree(output_type);
    pfree(options);
    pfree(range);
    PG_RETURN_TEXT_P(cstring_to_text_with_len((const char *)output_data, output_len));
}
