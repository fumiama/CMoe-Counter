#include <simple_protobuf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <unistd.h>
#include "cmoe.h"

static uint32_t* items_len;
static COUNTER counter;

#define ADD_HEADER(h) {\
    strcpy(buf + offset, (h));\
    offset += sizeof((h)) - 1;\
}
#define ADD_HEADER_PARAM(h, p) {\
    sprintf(buf + offset, (h), (p));\
    offset += strlen(buf + offset);\
}

static void headers(uint32_t content_len, const char* content_type) {
    char buf[1024];
    uint32_t offset = 0;

    ADD_HEADER(HTTP200 SERVER_STRING CACHE_CTRL);
    ADD_HEADER_PARAM(CONTENT_TYPE, content_type);
    ADD_HEADER_PARAM(CONTENT_LEN "\r\n", content_len);
    content_len += offset;
    struct iovec iov[2] = {{&content_len, sizeof(uint32_t)}, {buf, offset}};
    writev(1, &iov, 2);
}

static void http_error(ERRCODE code, char* msg) {
    uint32_t len = strlen(msg) + typel[code];
    char* str = malloc(len);
    sprintf(str, types[code], msg);
    struct iovec iov[2] = {{&len, sizeof(uint32_t)}, {str, len}};
    writev(1, &iov, 2);
    free(str);
    exit(EXIT_FAILURE);
}

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
    COUNTER *d = (COUNTER *)spb->target;
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

static int add_user(char* name, uint32_t count, FILE* fp) {
    counter.count = count;
    strncpy(counter.name, name, COUNTER_NAME_LEN-1);
    fseek(fp, 0, SEEK_END);
    return !set_pb(fp, items_len, sizeof(COUNTER), &counter);
}

static uint32_t get_content_len(int isbig, uint16_t* len_type, char* cntstr) {
    uint32_t len = sizeof(svg_small)
        + 16 + isbig
        + sizeof(svg_tail) - 1;
    for(int i = 0; cntstr[i]; i++) {
        len += len_type[cntstr[i] - '0'] + (sizeof(img_slot_front) + sizeof(img_slot_rear) - 2);
    }
    return len;
}

#define has_next(fp, ch) ((ch=getc(fp)),(feof(fp)?0:(ungetc(ch,fp),1)))
#define set_type(name, t, l) if(!strcmp(theme, name)) {\
                                theme_type = t;\
                                len_type = l;\
                            }
static void return_count(char* name, char* theme) {
    FILE* fp = fopen(DATFILE, "rb+");
    if(!fp) fp = fopen(DATFILE, "wb+");
    if(fp) {
        flock(fileno(fp), LOCK_EX);
        int ch, exist = 0, user_exist = 0;
        while(has_next(fp, ch)) {
            SIMPLE_PB *spb = get_pb(fp);
            COUNTER *d = (COUNTER *)spb->target;
            if (!strcmp(name, d->name)) {
                if(del_user(fp, spb)) http_error(HTTP500, "Unable to Delete Old Data.");
                else {
                    if (add_user(d->name, d->count + 1, fp)) http_error(HTTP500, "Add User Error.");
                    else {
                        fclose(fp);
                        char cntstrbuf[11];
                        sprintf(cntstrbuf, "%010u", d->count);
                        free(spb);
                        char* cntstr;
                        for(cntstr = cntstrbuf+9; cntstr > cntstrbuf; cntstr--) if(*cntstr == '0') break;
                        if(cntstr - cntstrbuf > 2) cntstr -= 2; // 保留 3 位 0
                        else cntstr = cntstrbuf; // 保留所有 0
                        int isbig = 0;
                        char** theme_type = mb;
                        uint16_t* len_type = mbl;
                        if(theme) {
                            set_type("mbh", mbh, mbhl) else
                            set_type("r34", r34, r34l) else
                            set_type("gb", gb, gbl) else
                            set_type("gbh", gbh, gbhl)
                            isbig = (theme_type == gb || theme_type == gbh);
                        }
                        int w, h;
                        char *head;
                        if(isbig) {
                            w = W_BIG;
                            h = H_BIG;
                            head = svg_big;
                        }
                        else {
                            w = W_SMALL;
                            h = H_SMALL;
                            head = svg_small;
                        }
                        headers(get_content_len(isbig, len_type, cntstr), "image/svg+xml");
                        printf(head, w*(10+cntstrbuf-cntstr));
                        for(int i = 0; cntstr[i]; i++) {
                            printf(img_slot_front, w * i, w, h);
                            int n = cntstr[i] - '0';
                            fwrite(theme_type[n], len_type[n], 1, stdout);
                            printf(img_slot_rear);
                        }
                        fflush(stdout);
                        write(1, svg_tail, sizeof(svg_tail)-1);
                    }
                    return;
                }
            } else free(spb);
        }
        fclose(fp);
        http_error(HTTP404, "No Such User.");
    } else http_error(HTTP500, "Open File Error.");
}

static int name_exist(char* name) {
    FILE* fp = fopen(DATFILE, "rb+");
    if(!fp) fp = fopen(DATFILE, "wb+");
    if(fp) {
        int ch, exist = 0;
        flock(fileno(fp), LOCK_EX);
        while(has_next(fp, ch)) {
            SIMPLE_PB *spb = get_pb(fp);
            COUNTER *d = (COUNTER *)spb->target;
            if (!strcmp(name, d->name)) {
                free(spb);
                fclose(fp);
                return 1;
            }
            else free(spb);
        }
        fclose(fp);
        return 0;
    } else http_error(HTTP500, "Open File Error.");
}

#define QS (argv[2])
// Usage: cmoe method query_string
int main(int argc, char **argv) {
    if(argc == 3) {
        char* name = strstr(QS, "name=");
        items_len = align_struct(sizeof(COUNTER), 2, &counter.name, &counter.count);
        if(!items_len) http_error(HTTP500, "Align Struct Error.");
        else if(name) {
            name = get_arg(name + 5);
            if(name) {
                char* theme = strstr(QS, "theme=");
                if(theme) {
                    theme = get_arg(theme + 6);
                }
                char* reg = strstr(QS, "reg=");
                if(reg) {
                    reg = get_arg(reg + 4);
                    if(reg) {
                        if(strcmp(reg, TOKEN)) http_error(HTTP400, "Token Error.");
                        else if(!name_exist(name)) {
                            FILE* fp = fopen(DATFILE, "ab+");
                            if(fp) {
                                flock(fileno(fp), LOCK_EX);
                                add_user(name, 0, fp);
                                fclose(fp);
                                char* msg = "<P>Success.\r\n";
                                headers(strlen(msg), "text/html");
                                write(1, msg, strlen(msg));
                            } else http_error(HTTP500, "Open File Error.");
                        } else http_error(HTTP400, "Name Exist.");
                    } else http_error(HTTP400, "Null Register Token.");
                } else return_count(name, theme);
            } else http_error(HTTP400, "Null Name Argument.");
        } else http_error(HTTP400, "Name Argument Notfound.");
    } else http_error(HTTP500, "Argument Count Error.");
    return 0;
}
