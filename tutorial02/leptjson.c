#include "leptjson.h"
#include <assert.h> /* assert() */
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h> /* NULL, strtod() */

#define EXPECT(c, ch)             \
    do {                          \
        assert(*c->json == (ch)); \
        c->json++;                \
    } while (0)

typedef struct {
    const char* json;
} lept_context;

static void lept_parse_whitespace(lept_context* c) {
    const char* p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

static int lept_parse_all(lept_context* c, lept_value* v, const char* ctype, lept_type returnType) {
    /*注意在 C 语言中，数组长度、索引值最好使用 size_t 类型，而不是 int 或 unsigned。*/
    size_t i;
    EXPECT(c, ctype[0]);
    for (i = 0; ctype[i + 1]; ++i) {
        if (ctype[i + 1] != c->json[i])
            return LEPT_PARSE_INVALID_VALUE;
    }
    c->json += i;
    v->type = returnType;
    return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, lept_value* v) {
    /*使用一个指针 p 来表示当前的解析字符位置。这样做有两个好处，一是代码更简单，二是在某些编译器下性能更好（因为不能确定 c 会否被改变，从而每次更改 c->json 都要做一次间接访问）*/
    const char* p = c->json;
    if (*p == '-')
        p++;
    /*针对小数点前的数字进行处理*/
    if (*p == '0')
        p++;
    else {
        if (!((*p) >= '1' && (*p) <= '9'))
            return LEPT_PARSE_INVALID_VALUE;
        for (p++; (*p) >= '0' && (*p) <= '9'; p++)
            ;
    }
    /*检查小数点及后续处理*/
    if (*p == '.') {
        p++;
        if (!((*p) >= '0' && (*p) <= '9'))
            return LEPT_PARSE_INVALID_VALUE;
        for (p++; (*p) >= '0' && (*p) <= '9'; p++)
            ;
    }
    /*检查指数*/
    if (*p == 'e' || *p == 'E') {
        p++;
        if (*p == '+' || *p == '-')
            p++;
        if (!((*p) >= '0' && (*p) <= '9'))
            return LEPT_PARSE_INVALID_VALUE;
        for (p++; (*p) >= '0' && (*p) <= '9'; p++)
            ;
    }
    errno = 0;
    /* \TODO validate number */
    v->n = strtod(c->json, NULL);
    if (errno == ERANGE && (v->n == HUGE_VAL || v->n == -HUGE_VAL))
        return LEPT_PARSE_NUMBER_TOO_BIG;

    c->json = p;
    v->type = LEPT_NUMBER;
    return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 't':
            return lept_parse_all(c, v, "true", LEPT_TRUE);
        case 'f':
            return lept_parse_all(c, v, "false", LEPT_FALSE);
        case 'n':
            return lept_parse_all(c, v, "null", LEPT_NULL);
        default:
            return lept_parse_number(c, v);
        case '\0':
            return LEPT_PARSE_EXPECT_VALUE;
    }
}

int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c);
    if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = LEPT_NULL;
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->n;
}
