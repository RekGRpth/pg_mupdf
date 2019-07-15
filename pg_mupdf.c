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

fz_context *context;

//static void *mupdf_malloc(void *arg, size_t size) { return palloc(size); }
//static void *mupdf_realloc(void *arg, void *p, size_t size) { return repalloc(p, size); }
//static void mupdf_free(void *arg, void *p) { if (p) (void)pfree(p); }

void _PG_init(void); void _PG_init(void) {
//    fz_alloc_context alloc_context = {NULL, mupdf_malloc, mupdf_realloc, mupdf_free};
//    if (!(context = fz_new_context(&alloc_context, NULL, FZ_STORE_UNLIMITED))) ereport(ERROR, (errmsg("!fz_new_context")));
    if (!(context = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED))) ereport(ERROR, (errmsg("!fz_new_context")));
    //fz_try(context) 
    (void)fz_register_document_handlers(context);// fz_catch(context) ereport(ERROR, (errmsg("fz_register_document_handlers: %s", fz_caught_message(context))));
    //fz_try(context) 
    (void)fz_set_use_document_css(context, 1);// fz_catch(context) ereport(ERROR, (errmsg("fz_set_use_document_css: %s", fz_caught_message(context))));
}

void _PG_fini(void); void _PG_fini(void) {
    (void)fz_drop_context(context);
}

static void runpage(fz_document *document, int number, fz_document_writer *document_writer) {
    fz_rect mediabox;
    fz_page *page;
    fz_device *dev;
    elog(LOG, "runpage: number=%i", number);
    //fz_try(context) 
    page = fz_load_page(context, document, number - 1);// fz_catch(context) ereport(ERROR, (errmsg("fz_load_page: %s", fz_caught_message(context))));
    //fz_try(context) 
//    (void)fz_var(dev);// fz_catch(context) ereport(ERROR, (errmsg("fz_var: %s", fz_caught_message(context))));
    //fz_try(context) 
    mediabox = fz_bound_page(context, page);// fz_catch(context) ereport(ERROR, (errmsg("fz_bound_page: %s", fz_caught_message(context))));
    //fz_try(context) 
    dev = fz_begin_page(context, document_writer, mediabox);// fz_catch(context) ereport(ERROR, (errmsg("fz_begin_page: %s", fz_caught_message(context))));
    //fz_try(context) 
    (void)fz_run_page(context, page, dev, fz_identity, NULL);// fz_catch(context) ereport(ERROR, (errmsg("fz_run_page: %s", fz_caught_message(context))));
    //fz_try(context) 
    (void)fz_end_page(context, document_writer);// fz_catch(context) ereport(ERROR, (errmsg("fz_end_page: %s", fz_caught_message(context))));
    //fz_try(context) 
    (void)fz_drop_page(context, page);// fz_catch(context) ereport(ERROR, (errmsg("fz_drop_page: %s", fz_caught_message(context))));
}

static void runrange(fz_document *document, const char *range, fz_document_writer *document_writer) {
    int start, end, count;
    elog(LOG, "runrange: range=%s", range);
    fz_try(context) count = fz_count_pages(context, document); fz_catch(context) ereport(ERROR, (errmsg("fz_count_pages: %s", fz_caught_message(context))));
    while ((range = fz_parse_page_range(context, range, &start, &end, count))) if (start < end) for (int i = start; i <= end; i++) runpage(document, i, document_writer); else for (int i = start; i >= end; i--) runpage(document, i, document_writer);
}

EXTENSION(pg_mupdf) {
    text *input_data;
    char *input_type, *output_type, *options = NULL, *range;
    fz_buffer *input_buffer, *output_buffer;
    fz_stream *input_stream;
    fz_document *document;
    fz_output *output;
    fz_document_writer *document_writer;
    unsigned char *output_data = NULL;
    size_t output_len;
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
//    unlink("/tmp/temp.pdf");
//    if (mkfifo("/tmp/temp.pdf", 0600)) ereport(ERROR, (errmsg("mkfifo")));
    //fz_try(context) 
//    input_buffer = fz_new_buffer(context, VARSIZE_ANY_EXHDR(input_data));// fz_catch(context) ereport(ERROR, (errmsg("fz_new_buffer: %s", fz_caught_message(context))));
    //fz_try(context) 
//    (void)fz_append_data(context, input_buffer, VARDATA_ANY(input_data), VARSIZE_ANY_EXHDR(input_data));// fz_catch(context) ereport(ERROR, (errmsg("fz_append_data: %s", fz_caught_message(context))));
    input_buffer = fz_new_buffer_from_data(context, (unsigned char *)VARDATA_ANY(input_data), VARSIZE_ANY_EXHDR(input_data));
    //fz_try(context) 
    input_stream = fz_open_buffer(context, input_buffer);// fz_catch(context) ereport(ERROR, (errmsg("fz_open_buffer: %s", fz_caught_message(context))));
    //fz_try(context) 
    document = fz_open_document_with_stream(context, input_type, input_stream);// fz_catch(context) ereport(ERROR, (errmsg("fz_open_document_with_stream: %s", fz_caught_message(context))));
    output_buffer = fz_new_buffer(context, 0);
    output = fz_new_output_with_buffer(context, output_buffer);
    //fz_try(context) 
    document_writer = fz_new_document_writer(context, (const char *)output, output_type, options);// fz_catch(context) ereport(ERROR, (errmsg("fz_new_document_writer: %s", fz_caught_message(context))));
    (void)runrange(document, range, document_writer);
    output_len = fz_buffer_extract(context, output_buffer, &output_data);
//    output_len = fz_buffer_extract(context, input_buffer, &output_data);
    (void)fz_close_output(context, output);
    (void)fz_drop_output(context, output);
    (void)fz_drop_buffer(context, output_buffer);
    (void)fz_drop_buffer(context, input_buffer);
//    (void)fz_drop_stream(context, input_stream);
    (void)fz_drop_document(context, document);
    (void)fz_close_document_writer(context, document_writer);
    (void)fz_drop_document_writer(context, document_writer);
    (void)pfree(input_data);
    (void)pfree(input_type);
    (void)pfree(output_type);
    if (options) (void)pfree(options);
    (void)pfree(range);
//    PG_RETURN_TEXT_P(cstring_to_text_with_len(VARDATA_ANY(input_data), VARSIZE_ANY_EXHDR(input_data)));
    PG_RETURN_TEXT_P(cstring_to_text_with_len((const char *)output_data, output_len));
}
