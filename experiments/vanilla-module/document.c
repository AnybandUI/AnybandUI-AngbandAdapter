#include "angband.h"
#include "z-textblock.h"
#include <windows.h>
/* Metadata belongs to the frontend. Engine textblocks retain their stock ABI. */
struct document {
    const textblock *text;
    struct textblock_span *spans;
    size_t count, start;
    struct document *next;
};
static struct document *documents;
static struct document *document(const textblock *tb)
{
    struct document *d;
    for (d = documents; d; d = d->next) if (d->text == tb) return d;
    d = mem_zalloc(sizeof(*d));
    d->text = tb;
    d->next = documents;
    documents = d;
    return d;
}
static void add_span(struct document *d, const struct textblock_span *span)
{
    struct textblock_span s = *span;
    s.section = string_make(s.section);
    s.label = s.label ? string_make(s.label) : NULL;
    s.value = s.value ? string_make(s.value) : NULL;
    d->spans = mem_realloc(d->spans, (d->count + 1) * sizeof(s));
    d->spans[d->count++] = s;
}
void textblock_end_section(textblock *tb, const char *section)
{
    struct document *d = document(tb);
    size_t end = wcslen(textblock_text(tb));
    struct textblock_span s = {section, NULL, NULL, d->start, end - d->start, 0};
    add_span(d, &s);
    d->start = end;
}
void textblock_append_field(textblock *tb, const char *section,
    const char *label, const char *value, uint8_t attr)
{
    struct document *d = document(tb);
    struct textblock_span s = {section, label, value, wcslen(textblock_text(tb)), 0, attr};
    textblock_append_c(tb, attr, "%s%s%s\n", label ? label : "", label ? ": " : "", value ? value : "");
    d->start = wcslen(textblock_text(tb));
    s.length = d->start - s.start;
    add_span(d, &s);
}
const struct textblock_span *textblock_spans(const textblock *tb, size_t *count)
{
    struct document *d = document(tb);
    *count = d->count;
    return d->spans;
}
void textblock_free(textblock *tb)
{
    struct document **link = &documents;
    void (*release)(textblock *) = (void (*)(textblock *))GetProcAddress(GetModuleHandleW(NULL), "textblock_free");
    assert(release);
    while (*link) {
        struct document *d = *link;
        if (d->text == tb) {
            size_t i;
            for (i = 0; i < d->count; ++i) {
                string_free((char *)d->spans[i].section);
                string_free((char *)d->spans[i].label);
                string_free((char *)d->spans[i].value);
            }
            *link = d->next;
            mem_free(d->spans);
            mem_free(d);
            break;
        }
        link = &d->next;
    }
    release(tb);
}
void textblock_append_textblock(textblock *tb, const textblock *from)
{
    const wchar_t *text = textblock_text(from);
    const uint8_t *attrs = textblock_attrs(from);
    struct document *source = document(from), *dest = document(tb);
    size_t i, offset = wcslen(textblock_text(tb));
    assert(tb != from);
    for (i = 0; text[i]; ++i) textblock_append_pict(tb, attrs[i], text[i]);
    textblock_append(tb, "%s", "");
    for (i = 0; i < source->count; ++i) {
        struct textblock_span s = source->spans[i];
        s.start += offset;
        add_span(dest, &s);
    }
    if (source->count) dest->start = offset + source->start;
}

