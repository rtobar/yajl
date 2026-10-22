/* a string split across yajl_parse() calls, including a zero-length call
 * while an escape or utf8 sequence is pending, must decode as if it had
 * been fed whole, and an invalid escape after such a split must still be
 * rejected. The parsing test driver never makes zero-length calls, so this
 * cannot be a case file. */

#include <yajl/yajl_parse.h>
#include <stdio.h>
#include <string.h>

static unsigned char got[32];
static size_t gotLen;

static int on_string(void *ctx, const unsigned char *s, size_t len)
{
    (void) ctx;
    if (len > sizeof(got)) return 0;
    memcpy(got, s, len);
    gotLen = len;
    return 1;
}

static yajl_callbacks cb = {
    NULL, NULL, NULL, NULL, NULL, on_string, NULL, NULL, NULL, NULL, NULL
};

static yajl_status feed(const char **chunks, size_t n)
{
    yajl_handle h = yajl_alloc(&cb, NULL, NULL);
    yajl_status st = yajl_status_ok;
    size_t i;

    gotLen = 0;
    for (i = 0; i < n && st == yajl_status_ok; i++) {
        st = yajl_parse(h, (const unsigned char *) chunks[i],
                        strlen(chunks[i]));
    }
    if (st == yajl_status_ok) st = yajl_complete_parse(h);
    yajl_free(h);
    return st;
}

int main(void)
{
    const char *escape[] = { "[\"ab\\u00", "", "1b\"]" };
    const char *utf8[]   = { "[\"a\xe2", "", "\x82\xac\"]" };
    const char *badHex[] = { "[\"ab\\u00", "", "G0\"]" };

    if (feed(escape, 3) != yajl_status_ok || gotLen != 3 ||
        memcmp(got, "ab\x1b", 3) != 0) {
        printf("split \\u escape not decoded\n");
        return 1;
    }
    if (feed(utf8, 3) != yajl_status_ok || gotLen != 4 ||
        memcmp(got, "a\xe2\x82\xac", 4) != 0) {
        printf("split utf8 sequence not accepted\n");
        return 1;
    }
    if (feed(badHex, 3) != yajl_status_error) {
        printf("invalid hex after a split \\u escape not rejected\n");
        return 1;
    }
    return 0;
}
