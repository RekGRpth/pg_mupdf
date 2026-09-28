#include <postgres.h>

#include <fmgr.h>
#include <limits.h>
#include <miscadmin.h>
#include <utils/builtins.h>
#include <utils/guc.h>
#include <utils/memutils.h>
#include <utils/varlena.h>
#if PG_VERSION_NUM >= 160000
#include <varatt.h>
#endif

#include <mupdf/fitz.h>

#define EXTENSION(function) Datum (function)(PG_FUNCTION_ARGS); PG_FUNCTION_INFO_V1(function); Datum (function)(PG_FUNCTION_ARGS)

PG_MODULE_MAGIC;

/* not in public headers; weak, so a MuPDF build without some handler still loads */
extern fz_document_handler cbz_document_handler __attribute__((weak));
extern fz_document_handler epub_document_handler __attribute__((weak));
extern fz_document_handler fb2_document_handler __attribute__((weak));
extern fz_document_handler gz_document_handler __attribute__((weak));
extern fz_document_handler html_document_handler __attribute__((weak));
extern fz_document_handler img_document_handler __attribute__((weak));
extern fz_document_handler mobi_document_handler __attribute__((weak));
extern fz_document_handler office_document_handler __attribute__((weak));
extern fz_document_handler pdf_document_handler __attribute__((weak));
extern fz_document_handler svg_document_handler __attribute__((weak));
extern fz_document_handler txt_document_handler __attribute__((weak));
extern fz_document_handler xhtml_document_handler __attribute__((weak));
extern fz_document_handler xps_document_handler __attribute__((weak));

static const struct {
    const char *name;
    fz_document_handler *handler;
} pg_mupdf_handlers[] = {
    {"cbz", &cbz_document_handler},
    {"epub", &epub_document_handler},
    {"fb2", &fb2_document_handler},
    {"gz", &gz_document_handler},
    {"html", &html_document_handler},
    {"img", &img_document_handler},
    {"mobi", &mobi_document_handler},
    {"office", &office_document_handler},
    {"pdf", &pdf_document_handler},
    {"svg", &svg_document_handler},
    {"txt", &txt_document_handler},
    {"xhtml", &xhtml_document_handler},
    {"xps", &xps_document_handler},
};

static bool memory_limit_exceeded;
static char *document_handlers;
static char messages[100][256];
static int memory_limit;
static int messages_count, messages_skipped;

static fz_document_handler *pg_mupdf_handler(const char *name) {
    for (size_t i = 0; i < lengthof(pg_mupdf_handlers); i++) if (!strcmp(pg_mupdf_handlers[i].name, name)) return pg_mupdf_handlers[i].handler;
    return NULL;
}

static bool check_document_handlers(char **newval, void **extra, GucSource source) {
    char *rawstring = pstrdup(*newval);
    List *elemlist;
    ListCell *l;
    if (!SplitIdentifierString(rawstring, ',', &elemlist)) {
        GUC_check_errdetail("List syntax is invalid.");
        pfree(rawstring);
        list_free(elemlist);
        return false;
    }
    foreach(l, elemlist) if (!pg_mupdf_handler(lfirst(l))) {
        GUC_check_errdetail("Document handler \"%s\" is unknown or not available in this MuPDF build.", (char *)lfirst(l));
        pfree(rawstring);
        list_free(elemlist);
        return false;
    }
    pfree(rawstring);
    list_free(elemlist);
    return true;
}

PGDLLEXPORT void _PG_init(void);
void _PG_init(void) {
    DefineCustomIntVariable("pg_mupdf.memory_limit", "Maximum memory MuPDF may allocate in one call.", "0 means no limit.", &memory_limit, 1024 * 1024, 0, MAX_KILOBYTES, PGC_SUSET, GUC_UNIT_KB, NULL, NULL, NULL);
    DefineCustomStringVariable("pg_mupdf.document_handlers", "MuPDF document handlers allowed to parse input.", "Comma-separated list of: cbz, epub, fb2, gz, html, img, mobi, office, pdf, svg, txt, xhtml, xps.", &document_handlers, "html,xhtml", PGC_SUSET, GUC_LIST_INPUT, check_document_handlers, NULL, NULL);
#if PG_VERSION_NUM >= 150000
    MarkGUCPrefixReserved("pg_mupdf");
#else
    EmitWarningsOnPlaceholders("pg_mupdf");
#endif
}

/* failing the allocation is the only safe way to stop MuPDF: it unwinds to our fz_catch */
static bool alloc_allowed(MemoryContext mcxt, size_t size) {
    if (QueryCancelPending || ProcDiePending) return false;
    if (memory_limit && MemoryContextMemAllocated(mcxt, false) + size > (Size)memory_limit * 1024) {
        memory_limit_exceeded = true;
        return false;
    }
    return true;
}

static void *fz_malloc_default_my(void *opaque, size_t size) {
    if (!size || !AllocSizeIsValid(size) || !alloc_allowed(opaque, size)) return NULL;
    return MemoryContextAllocExtended(opaque, size, MCXT_ALLOC_NO_OOM);
}

static void *fz_realloc_default_my(void *opaque, void *old, size_t size) {
    if (!old) return fz_malloc_default_my(opaque, size);
    if (!size) return old;
    if (!AllocSizeIsValid(size) || !alloc_allowed(opaque, size)) return NULL;
#if PG_VERSION_NUM >= 160000
    return repalloc_extended(old, size, MCXT_ALLOC_NO_OOM);
#else
    return repalloc(old, size);
#endif
}

static void fz_free_default_my(void *opaque, void *ptr) {
    if (ptr) pfree(ptr);
}

/* called from inside MuPDF, where ereport must not be used: only remember the message */
static void pg_mupdf_message_callback(void *user, const char *message) {
    if (messages_count < lengthof(messages)) strlcpy(messages[messages_count++], message, sizeof(messages[0]));
    else messages_skipped++;
}

static void pg_mupdf_report_messages(void) {
    int count = messages_count, skipped = messages_skipped;
    messages_count = messages_skipped = 0;
    for (int i = 0; i < count; i++) ereport(WARNING, (errmsg("%s", messages[i])));
    if (skipped) ereport(WARNING, (errmsg("%d more MuPDF messages skipped", skipped)));
}

static void runpage(fz_context *ctx, fz_document *doc, fz_document_writer *wri, int number) {
    fz_page *page = fz_load_page(ctx, doc, number - 1);
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

static void runrange(fz_context *ctx, fz_document *doc, fz_document_writer *wri, const char *range) {
    int count = fz_count_pages(ctx, doc);
    for (int start, end; (range = fz_parse_page_range(ctx, range, &start, &end, count));) {
        if (start < end) for (int i = start; i <= end; ++i) runpage(ctx, doc, wri, i);
        else for (int i = start; i >= end; --i) runpage(ctx, doc, wri, i);
    }
}

EXTENSION(pg_mupdf) {
    bytea *pdf = NULL;
    char *input_type, *output_type, *options, *range;
    fz_buffer *buf = NULL;
    fz_context *ctx;
    fz_document *doc = NULL;
    fz_document_writer *wri = NULL;
    List *handlers;
    ListCell *l;
    MemoryContext mcxt;
    fz_stream *stm = NULL;
    size_t output_len;
    text *input_data;
    unsigned char *output_data;
    fz_alloc_context fz_alloc_default_my = {
        NULL,
        fz_malloc_default_my,
        fz_realloc_default_my,
        fz_free_default_my
    };
    if (PG_ARGISNULL(0)) ereport(ERROR, (errmsg("input_data is null!")));
    if (PG_ARGISNULL(1)) ereport(ERROR, (errmsg("input_type is null!")));
    if (PG_ARGISNULL(2)) ereport(ERROR, (errmsg("output_type is null!")));
    if (PG_ARGISNULL(3)) ereport(ERROR, (errmsg("options is null!")));
    if (PG_ARGISNULL(4)) ereport(ERROR, (errmsg("range is null!")));
    input_data = PG_GETARG_TEXT_PP(0);
    input_type = TextDatumGetCString(PG_GETARG_DATUM(1));
    output_type = TextDatumGetCString(PG_GETARG_DATUM(2));
    options = TextDatumGetCString(PG_GETARG_DATUM(3));
    range = TextDatumGetCString(PG_GETARG_DATUM(4));
    if (!SplitIdentifierString(pstrdup(document_handlers), ',', &handlers)) ereport(ERROR, (errmsg("invalid pg_mupdf.document_handlers")));
    mcxt = AllocSetContextCreate(CurrentMemoryContext, "pg_mupdf", ALLOCSET_DEFAULT_SIZES);
    fz_alloc_default_my.user = mcxt;
    memory_limit_exceeded = false;
    messages_count = messages_skipped = 0;
    if (!(ctx = fz_new_context(&fz_alloc_default_my, NULL, FZ_STORE_DEFAULT))) {
        MemoryContextDelete(mcxt);
        CHECK_FOR_INTERRUPTS();
        ereport(ERROR, (errmsg("!fz_new_context")));
    }
    fz_set_error_callback(ctx, pg_mupdf_message_callback, NULL);
    fz_set_warning_callback(ctx, pg_mupdf_message_callback, NULL);
    fz_var(buf);
    fz_var(doc);
    fz_var(pdf);
    fz_var(stm);
    fz_var(wri);
    fz_try(ctx) {
        foreach(l, handlers) fz_register_document_handler(ctx, pg_mupdf_handler(lfirst(l)));
        fz_set_use_document_css(ctx, 1);
        buf = fz_new_buffer(ctx, 0);
        stm = fz_open_memory(ctx, (unsigned char *)VARDATA_ANY(input_data), VARSIZE_ANY_EXHDR(input_data));
        doc = fz_open_document_with_stream(ctx, input_type, stm);
        wri = fz_new_document_writer_with_buffer(ctx, buf, output_type, options);
        runrange(ctx, doc, wri, range);
        fz_close_document_writer(ctx, wri);
        output_len = fz_buffer_storage(ctx, buf, &output_data);
        if (!AllocSizeIsValid(output_len + VARHDRSZ) || !(pdf = MemoryContextAllocExtended(CurrentMemoryContext, output_len + VARHDRSZ, MCXT_ALLOC_NO_OOM))) fz_throw(ctx, FZ_ERROR_LIMIT, "cannot allocate result of %zu bytes", output_len);
        SET_VARSIZE(pdf, output_len + VARHDRSZ);
        memcpy(VARDATA(pdf), output_data, output_len);
    } fz_always(ctx) {
        fz_drop_document_writer(ctx, wri);
        fz_drop_document(ctx, doc);
        fz_drop_stream(ctx, stm);
        fz_drop_buffer(ctx, buf);
    } fz_catch(ctx) {
        char *message = pstrdup(fz_convert_error(ctx, NULL));
        fz_drop_context(ctx);
        MemoryContextDelete(mcxt);
        pg_mupdf_report_messages();
        CHECK_FOR_INTERRUPTS();
        if (memory_limit_exceeded) ereport(ERROR, (errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED), errmsg("%s", message), errdetail("pg_mupdf.memory_limit (%d kB) exceeded.", memory_limit)));
        ereport(ERROR, (errmsg("%s", message)));
    }
    fz_drop_context(ctx);
    MemoryContextDelete(mcxt);
    pg_mupdf_report_messages();
    PG_FREE_IF_COPY(input_data, 0);
    pfree(input_type);
    pfree(output_type);
    pfree(options);
    pfree(range);
    PG_RETURN_BYTEA_P(pdf);
}
