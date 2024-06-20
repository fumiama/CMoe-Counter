#include <fcntl.h>
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
static FILE* fp;

static char* datfile = "dat.sp";
static char* token = "fumiama";

#define ADD_HEADER_PARAM(buf, offset, h, p) sprintf(buf + offset, (h), (p))
#define HEADER(content_type) HTTP200 SERVER_STRING CACHE_CTRL CONTENT_TYPE(content_type)
#define headers(content_len, content_type) (_headers(content_len, HEADER(content_type), sizeof(HEADER(content_type))-1))
static int _headers(uint32_t content_len, const char* h, size_t hlen) {
    char buf[64];
    size_t offset = ADD_HEADER_PARAM(buf, 0, CONTENT_LEN "\r\n", content_len);
    if (offset <= 0) return -1;
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
    while (query[len] && query[len] != '&') len++;
    if (len <= 0) return NULL;
    char* name = malloc(len+1);
    memcpy(name, query, len);
    name[len] = 0;
    return name;
}

static int del_user(FILE* fp, simple_pb_t* spb) {
    counter_t *d = (counter_t *)spb->target;
    uint32_t next = ftell(fp);
    uint32_t this = next - spb->real_len;
    if (fseek(fp, 0, SEEK_END) < 0) return -2;
    uint32_t end = ftell(fp);
    if (next == end) return ftruncate(fileno(fp), end - spb->real_len);
    uint32_t cap = end - next;
    char *data = malloc(cap);
    if (data) {
        if (fseek(fp, next, SEEK_SET)) return -3;
        if (fread(data, cap, 1, fp) == 1) {
            if (!ftruncate(fileno(fp), end - spb->real_len)){
                if (fseek(fp, this, SEEK_SET)) return -4;
                if (fwrite(data, cap, 1, fp) == 1) {
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
    if (fseek(fp, 0, SEEK_END) < 0) return -2;
    return !set_pb(fp, items_len, sizeof(counter_t), &counter);
}

static inline uint32_t get_content_len(int isbigtiny, uint16_t* len_type, char* cntstr) {
    static int lendiff[] = {
        0|(0<<8)|(-1u<<16), 1|(1<<8)|(0<<16), 1|(2<<8)|(1<<16), 2|(2<<8)|(1<<16), 2|(2<<8)|(1<<16),
        2|(2<<8)|(1<<16),  2|(2<<8)|(1<<16), 2|(2<<8)|(1<<16), 2|(2<<8)|(1<<16), 2|(2<<8)|(1<<16)
    };
    uint32_t len = sizeof(svg_small) // small & big has the same len
        + sizeof(svg_tail) - 1 + 1; // +1 for no \n
    if (isbigtiny&FLAG_TINY) len--; // is tiny
    uint32_t frontsz = ((isbigtiny&FLAG_SVG)?sizeof(svg_slot_front):sizeof(img_slot_front)) - 1;
    uint32_t rearsz = ((isbigtiny&FLAG_SVG)?(sizeof(svg_slot_rear)-3+8):sizeof(img_slot_rear)) - 1 + 1; // +1 for no \n
    int i = 0;
    for (; cntstr[i]; i++) {
        len += (uint32_t)len_type[cntstr[i] - '0'] + frontsz + rearsz;
        len += (uint32_t)((int)((int8_t)((lendiff[i]>>(isbigtiny*8))&0xff)));
    }
    return len;
}

#define cmp_and_set_type(t) if (!strcmp(theme, #t)) { theme_type = (char**)t; len_type = (uint16_t*)t##l; }
static void draw(int count, char* theme, uint32_t color) {
    char cntstrbuf[11];
    sprintf(cntstrbuf, "%010u", count);
    char* cntstr = cntstrbuf;
    int i = 0;
    for (; i < 10; i++) if (cntstrbuf[i] != '0') {
        if (i > 2) cntstr = cntstrbuf+i-2;
        break;
    }
    int isbigtiny = 0;
    char** theme_type = (char**)mb;
    uint16_t* len_type = (uint16_t*)mbl;
    if (theme) {
        cmp_and_set_type(mbh) else
        cmp_and_set_type(r34) else
        cmp_and_set_type(gb) else
        cmp_and_set_type(gbh) else
        cmp_and_set_type(asl) else
        cmp_and_set_type(nix) else
        cmp_and_set_type(mbs)
        isbigtiny = (
            (theme_type == (char**)gb || theme_type == (char**)gbh) |
            ((theme_type == (char**)nix || theme_type == (char**)mbs) << 1) |
            ((theme_type == (char**)mbs) << 2)
        );
    }
    int w, h;
    char *head;
    if (isbigtiny&FLAG_BIG) {
        w = W_BIG;
        h = H_BIG;
        head = (char*)svg_big;
    }
    else if (isbigtiny&FLAG_TINY) {
        w = W_TINY;
        h = H_TINY;
        head = (char*)svg_tiny;
    }
    else {
        w = W_SMALL;
        h = H_SMALL;
        head = (char*)svg_small;
    }
    if (headers(get_content_len(isbigtiny, len_type, cntstr), "image/svg+xml")) {
        write(1, "\0\0\0\0", 4);
        return;
    }

    const char* slot_front = (isbigtiny&FLAG_SVG)?svg_slot_front:img_slot_front;
    const char* slot_rear;
    if (isbigtiny&FLAG_SVG) {
        char rearbuf[sizeof(svg_slot_rear)-3+8];
        snprintf(rearbuf, sizeof(rearbuf), svg_slot_rear, color?color:SVG_DEFAULT_COLOR);
        slot_rear = rearbuf;
    } else slot_rear = img_slot_rear;

    printf(head, w*(10+cntstrbuf-cntstr));
    for (i = 0; cntstr[i]; i++) {
        printf(slot_front, w * i, w, h);
        int n = cntstr[i] - '0';
        fwrite(theme_type[n], len_type[n], 1, stdout);
        puts(slot_rear); // +1 \n for each turn
    }
    puts(svg_tail); // +1 \n
}

#define has_next(fp, ch) ((ch=getc(fp)),(feof(fp)?0:(ungetc(ch,fp),1)))
static void return_count(FILE* fp, char* name, char* theme, uint32_t color) {
    if (!strcmp(name, "demo")) {
        draw(123456789, theme, color);
        return;
    }
    int ch, exist = 0, user_exist = 0;
    char buf[sizeof(simple_pb_t)+sizeof(counter_t)];
    while (has_next(fp, ch)) {
        simple_pb_t *spb = read_pb_into(fp, (simple_pb_t*)buf);
        counter_t *d = (counter_t *)spb->target;
        if (strcmp(name, d->name)) continue;
        if (del_user(fp, spb)) {
            http_error(HTTP500, "Unable to Delete Old Data.");
            return;
        }
        if (add_user(d->name, d->count + 1, fp)) {
            http_error(HTTP500, "Add User Error.");
            return;
        }
        draw(d->count, theme, color);
        return;
    }
    http_error(HTTP404, "No Such User.");
}

static int name_exist(FILE* fp, char* name) {
    if (!strcmp(name, "demo")) {
        return 1;
    }
    int ch, exist = 0;
    char buf[sizeof(simple_pb_t)+sizeof(counter_t)];
    while (has_next(fp, ch)) {
        simple_pb_t *spb = read_pb_into(fp, (simple_pb_t*)buf);
        counter_t *d = (counter_t *)spb->target;
        if (!strcmp(name, d->name)) return 1;
    }
    return 0;
}

#define create_or_open(filename) (open(filename, O_CREAT|O_RDWR, 0600))

#define QS (argv[2])
// Usage: cmoe method query_string
int main(int argc, char **argv) {
    if (argc != 3) {
        http_error(HTTP500, "Argument Count Error.");
        return -1;
    }
    char* str = getenv("DATFILE");
    if (str != NULL) datfile = str;
    str = getenv("TOKEN");
    if (str != NULL) token = str;
    char* name = strstr(QS, "name=");
    items_len = align_struct(sizeof(counter_t), 2, &counter.name, &counter.count);
    if (!items_len) {
        http_error(HTTP500, "Align Struct Error.");
        return 2;
    }
    if (!name) {
        http_error(HTTP400, "Name Argument Notfound.");
        return 3;
    }
    name = get_arg(name + 5);
    if (!name) {
        http_error(HTTP400, "Null Name Argument.");
        return 4;
    }
    char* theme = strstr(QS, "theme=");
    if (theme) {
        theme = get_arg(theme + 6);
    }
    char* colors = strstr(QS, "color=");
    uint32_t color = 0;
    if (colors) {
        colors = get_arg(colors + 6);
        sscanf(colors, "%x", &color);
    }
    char* reg = strstr(QS, "reg=");
    int fd;
    if (!reg) {
        if ((fd=create_or_open(datfile)) < 0) {
            http_error(HTTP500, "Open File Error.");
            return -2;
        }
        if (flock(fd, LOCK_EX)) {
            http_error(HTTP500, "Lock File Error.");
            return -3;
        }
        fp = fdopen(fd, "rb+");
        return_count(fp, name, theme, color);
        return 0;
    }
    reg = get_arg(reg + 4);
    if (!reg) {
        http_error(HTTP400, "Null Register Token.");
        return 5;
    }
    if (strcmp(reg, token)) {
        http_error(HTTP400, "Token Error.");
        return 6;
    }
    if ((fd=create_or_open(datfile)) < 0) {
        http_error(HTTP500, "Open File Error.");
        return -4;
    }
    if (flock(fd, LOCK_EX)) {
        http_error(HTTP500, "Lock File Error.");
        return -5;
    }
    fp = fdopen(fd, "rb+");
    if (name_exist(fp, name)) {
        http_error(HTTP400, "Name Exist.");
        return 7;
    }
    fseek(fp, 0, SEEK_END);
    add_user(name, 0, fp);
    char* msg = "<P>Success.\r\n";
    if (headers(strlen(msg), "text/html")) {
        write(1, "\0\0\0\0", 4);
        return 8;
    }
    return write(1, msg, strlen(msg)) <= 0;
}

static void __attribute__((destructor)) defer_close_fp() {
    if (fp) fclose(fp);
}
