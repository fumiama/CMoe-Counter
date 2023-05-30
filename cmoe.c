#include <simple_protobuf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <unistd.h>
#include "cmoe.h"

static uint32_t* items_len;
static counter_t counter;

static char* datfile = "dat.sp";
static char* token = "fumiama";

static FILE* fp;

#define ADD_HEADER_PARAM(buf, offset, h, p) sprintf(buf + offset, (h), (p))
#define HEADER(content_type) HTTP200 SERVER_STRING CACHE_CTRL CONTENT_TYPE(content_type)
#define headers(content_len, content_type) (_headers(content_len, HEADER(content_type), sizeof(HEADER(content_type))-1))
static int _headers(uint32_t content_len, const char* h, size_t hlen) {
    char buf[64];
    size_t offset = ADD_HEADER_PARAM(buf, 0, CONTENT_LEN "\r\n", content_len);
    if(offset <= 0) return -1;
    content_len += offset+hlen;
    struct iovec iov[3] = {{&content_len, sizeof(uint32_t)}, {(void*)h, hlen}, {(void*)buf, offset}};
    return writev(1, (const struct iovec *)&iov, 3) <= 0;
}

static inline void http_error(errcode_enum_t code, char* msg) {
    uint32_t len = strlen(msg) + typel[code];
    char str[len];
    sprintf(str, types[code], msg);
    struct iovec iov[2] = {{(void*)&len, sizeof(uint32_t)}, {(void*)str, len}};
    writev(1, (const struct iovec *)&iov, 2);
}

// get_arg used malloc but has never freed
static char* get_arg(const char* query) {
    int len = 0;
    while(query[len] && query[len] != '&') len++;
    if(len <= 0) return NULL;
    char* name = malloc(len+1);
    memcpy(name, query, len);
    name[len] = 0;
    return name;
}

static int del_user(FILE* fp, SIMPLE_PB* spb) {
    counter_t *d = (counter_t *)spb->target;
    uint32_t next = ftell(fp);
    uint32_t this = next - spb->real_len;
    fseek(fp, 0, SEEK_END);
    uint32_t end = ftell(fp);
    if(next == end) return ftruncate(fileno(fp), end - spb->real_len);
    uint32_t cap = end - next;
    char *data = malloc(cap);
    if(data) {
        fseek(fp, next, SEEK_SET);
        if(fread(data, cap, 1, fp) == 1) {
            if(!ftruncate(fileno(fp), end - spb->real_len)){
                fseek(fp, this, SEEK_SET);
                if(fwrite(data, cap, 1, fp) == 1) {
                    free(data);
                    fflush(fp);
                    return 0;
                }
            }
        }
        free(data);
    }
    return 2;
}

static inline int add_user(char* name, uint32_t count, FILE* fp) {
    counter.count = count;
    strncpy(counter.name, name, COUNTER_NAME_LEN-1);
    fseek(fp, 0, SEEK_END);
    return !set_pb(fp, items_len, sizeof(counter_t), &counter);
}

static inline uint32_t get_content_len(int isbig, uint16_t* len_type, char* cntstr) {
    uint32_t len = sizeof(svg_small) // small & big has the same len
        + sizeof(svg_tail) - 1;
    for(int i = 0; cntstr[i]; i++) {
        len += len_type[cntstr[i] - '0'] + (sizeof(img_slot_front) + sizeof(img_slot_rear) - 2);
        if(i > 0) len++;
        if(i > 2-isbig) len++;
    }
    return len;
}

#define has_next(fp, ch) ((ch=getc(fp)),(feof(fp)?0:(ungetc(ch,fp),1)))
#define set_type(name, t, l) if(!strcmp(theme, name)) {\
                                theme_type = (char**)t;\
                                len_type = (uint16_t*)l;\
                            }
static void return_count(FILE* fp, char* name, char* theme) {
    int ch, exist = 0, user_exist = 0;
    char buf[sizeof(SIMPLE_PB)+sizeof(counter_t)];
    while(has_next(fp, ch)) {
        SIMPLE_PB *spb = read_pb_into(fp, (SIMPLE_PB*)buf);
        counter_t *d = (counter_t *)spb->target;
        if (strcmp(name, d->name)) continue;
        if(del_user(fp, spb)) {
            http_error(HTTP500, "Unable to Delete Old Data.");
            return;
        }
        if (add_user(d->name, d->count + 1, fp)) {
            http_error(HTTP500, "Add User Error.");
            return;
        }
        char cntstrbuf[11];
        sprintf(cntstrbuf, "%010u", d->count);
        char* cntstr = cntstrbuf;
        for(int i = 0; i < 10; i++) if(cntstrbuf[i] != '0') {
            if(i > 2) cntstr = cntstrbuf+i-2;
            break;
        }
        int isbig = 0;
        char** theme_type = (char**)mb;
        uint16_t* len_type = (uint16_t*)mbl;
        if(theme) {
            set_type("mbh", mbh, mbhl) else
            set_type("r34", r34, r34l) else
            set_type("gb", gb, gbl) else
            set_type("gbh", gbh, gbhl)
            isbig = (theme_type == (char**)gb || theme_type == (char**)gbh);
        }
        int w, h;
        char *head;
        if(isbig) {
            w = W_BIG;
            h = H_BIG;
            head = (char*)svg_big;
        }
        else {
            w = W_SMALL;
            h = H_SMALL;
            head = (char*)svg_small;
        }
        if(headers(get_content_len(isbig, len_type, cntstr), "image/svg+xml")) {
            write(1, "\0\0\0\0", 4);
            return;
        }
        printf(head, w*(10+cntstrbuf-cntstr));
        for(int i = 0; cntstr[i]; i++) {
            printf(img_slot_front, w * i, w, h);
            int n = cntstr[i] - '0';
            fwrite(theme_type[n], len_type[n], 1, stdout);
            printf(img_slot_rear);
        }
        printf(svg_tail);
        return;
    }
    http_error(HTTP404, "No Such User.");
}

static int name_exist(FILE* fp, char* name) {
    int ch, exist = 0;
    char buf[sizeof(SIMPLE_PB)+sizeof(counter_t)];
    while(has_next(fp, ch)) {
        SIMPLE_PB *spb = read_pb_into(fp, (SIMPLE_PB*)buf);
        counter_t *d = (counter_t *)spb->target;
        if (!strcmp(name, d->name)) return 1;
    }
    return 0;
}

#define create_or_open(fp, filename) ((fp = fopen(filename, "rb+"))?(fp):(fp = fopen(filename, "wb+")))

#define flease(fp) { \
    fflush(fp); \
    flock(fileno(fp), LOCK_UN); \
    fclose(fp); fp = NULL; \
}

#define QS (argv[2])
// Usage: cmoe method query_string
int main(int argc, char **argv) {
    if(argc != 3) {
        http_error(HTTP500, "Argument Count Error.");
        return -1;
    }
    char* str = getenv("DATFILE");
    if(str != NULL) datfile = str;
    str = getenv("TOKEN");
    if(str != NULL) token = str;
    char* name = strstr(QS, "name=");
    items_len = align_struct(sizeof(counter_t), 2, &counter.name, &counter.count);
    if(!items_len) {
        http_error(HTTP500, "Align Struct Error.");
        return 2;
    }
    if(!name) {
        http_error(HTTP400, "Name Argument Notfound.");
        return 3;
    }
    name = get_arg(name + 5);
    if(!name) {
        http_error(HTTP400, "Null Name Argument.");
        return 4;
    }
    char* theme = strstr(QS, "theme=");
    if(theme) {
        theme = get_arg(theme + 6);
    }
    char* reg = strstr(QS, "reg=");
    if (!reg) {
        if(!create_or_open(fp, datfile)) {
            http_error(HTTP500, "Open File Error.");
            return -2;
        }
        if(flock(fileno(fp), LOCK_EX)) {
            http_error(HTTP500, "Lock File Error.");
            return -3;
        }
        return_count(fp, name, theme);
        flease(fp);
        return 0;
    }
    reg = get_arg(reg + 4);
    if (!reg) {
        http_error(HTTP400, "Null Register Token.");
        return 5;
    }
    if(strcmp(reg, token)) {
        http_error(HTTP400, "Token Error.");
        return 6;
    }
    if(!create_or_open(fp, datfile)) {
        http_error(HTTP500, "Open File Error.");
        return -4;
    }
    if(flock(fileno(fp), LOCK_EX)) {
        http_error(HTTP500, "Lock File Error.");
        return -5;
    }
    if(name_exist(fp, name)) {
        flease(fp);
        http_error(HTTP400, "Name Exist.");
        return 7;
    }
    flease(fp);
    fp = fopen(datfile, "ab+");
    if (!fp) {
        http_error(HTTP500, "Open File Error.");
        return -6;
    }
    if(flock(fileno(fp), LOCK_EX)) {
        http_error(HTTP500, "Lock File Error.");
        return -7;
    }
    add_user(name, 0, fp);
    flease(fp);
    char* msg = "<P>Success.\r\n";
    if(headers(strlen(msg), "text/html")) {
        write(1, "\0\0\0\0", 4);
        return 8;
    }
    return write(1, msg, strlen(msg)) <= 0;
}

static void __attribute__((destructor)) defer_close_fp() {
    if(fp) flease(fp);
}
