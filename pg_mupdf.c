#include <postgres.h>
#include <fmgr.h>

#include <utils/builtins.h>
#include <mupdf/fitz.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <fcntl.h>
//#include <unistd.h>

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
    fz_try(ctx) {
        fz_register_document_handlers(ctx);
        fz_set_use_document_css(ctx, 1);
    } fz_catch(ctx) {
        ereport(ERROR, (errmsg("fz_caught_message: %s", fz_caught_message(ctx))));
    }
}

void _PG_fini(void); void _PG_fini(void) {
    (void)fz_drop_context(ctx);
}

static void runpage(fz_document *doc, int number, fz_document_writer *wri) {
    fz_page *page = fz_load_page(ctx, doc, number - 1);
    elog(LOG, "runpage: number=%i", number);
    fz_try(ctx) {
        fz_rect mediabox = fz_bound_page(ctx, page);
        fz_device *dev = fz_begin_page(ctx, wri, mediabox);
        fz_run_page(ctx, page, dev, fz_identity, NULL);
        fz_end_page(ctx, wri);
    } fz_always(ctx) {
        fz_drop_page(ctx, page);
    } fz_catch(ctx) {
        ereport(WARNING, (errmsg("fz_caught_message: %s", fz_caught_message(ctx))));
        fz_rethrow(ctx);
    }
}

static void runrange(fz_document *doc, const char *range, fz_document_writer *wri) {
    int start, end, count;
    elog(LOG, "runrange: range=%s", range);
    fz_try(ctx) count = fz_count_pages(ctx, doc); fz_catch(ctx) ereport(ERROR, (errmsg("fz_count_pages: %s", fz_caught_message(ctx))));
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
    char *input_type, *output_type, *options = NULL, *range;
    fz_buffer *input_buffer;
    fz_buffer *output_buffer;
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
    if (!PG_ARGISNULL(3)) options = TextDatumGetCString(PG_GETARG_DATUM(3));
    if (PG_ARGISNULL(4)) ereport(ERROR, (errmsg("range is null!")));
    range = TextDatumGetCString(PG_GETARG_DATUM(4));
    elog(LOG, "pg_mupdf: input_data=%s, input_type=%s, output_type=%s, options=%s, range=%s", VARDATA_ANY(input_data), input_type, output_type, options, range);
    fz_try(ctx) input_buffer = fz_new_buffer_from_data(ctx, (unsigned char *)VARDATA_ANY(input_data), VARSIZE_ANY_EXHDR(input_data)); fz_catch(ctx) ereport(ERROR, (errmsg("fz_new_buffer_from_data: %s", fz_caught_message(ctx))));
    fz_try(ctx) input_stream = fz_open_buffer(ctx, input_buffer); fz_catch(ctx) ereport(ERROR, (errmsg("fz_open_buffer: %s", fz_caught_message(ctx))));
    fz_try(ctx) doc = fz_open_document_with_stream(ctx, input_type, input_stream); fz_catch(ctx) ereport(ERROR, (errmsg("fz_open_document_with_stream: %s", fz_caught_message(ctx))));
    fz_try(ctx) output_buffer = fz_new_buffer(ctx, 0); fz_catch(ctx) ereport(ERROR, (errmsg("fz_new_buffer: %s", fz_caught_message(ctx))));
    ctx->user = output_buffer;
    fz_try(ctx) wri = fz_new_document_writer(ctx, "buf:", output_type, options); fz_catch(ctx) ereport(ERROR, (errmsg("fz_new_document_writer: %s", fz_caught_message(ctx))));
    (void)runrange(doc, range, wri);
    (void)fz_drop_buffer(ctx, input_buffer);
    (void)fz_drop_document(ctx, doc);
    (void)fz_close_document_writer(ctx, wri);
    (void)fz_drop_document_writer(ctx, wri);
    fz_try(ctx) output_len = fz_buffer_storage(ctx, output_buffer, &output_data); fz_catch(ctx) ereport(ERROR, (errmsg("fz_buffer_storage: %s", fz_caught_message(ctx))));
    (void)pfree(input_data);
    (void)pfree(input_type);
    (void)pfree(output_type);
    if (options) (void)pfree(options);
    (void)pfree(range);
    PG_RETURN_TEXT_P(cstring_to_text_with_len((const char *)output_data, output_len));
}
