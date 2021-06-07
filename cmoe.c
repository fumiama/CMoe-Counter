#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <simple_protobuf.h>
#include "cmoe.h"

static uint32_t* items_len;
static COUNTER counter;

#define ADD_HERDER(h)\
    strcpy(buf + offset, (h));\
    offset += sizeof((h)) - 1;
#define ADD_HERDER_PARAM(h, p)\
    sprintf(buf + offset, (h), (p));\
    offset += strlen(buf + offset);
#define EXTNM_IS_NOT(name) (strcmp(filepath+extpos, name))

static void headers(uint32_t content_len, const char* content_type, int no_cache) {
    char buf[1024];
    uint32_t offset = 0;

    ADD_HERDER(HTTP200 SERVER_STRING);
    if(no_cache) ADD_HERDER(CACHE_CTRL);
    ADD_HERDER_PARAM(CONTENT_TYPE, content_type);
    ADD_HERDER_PARAM(CONTENT_LEN "\r\n", content_len);
    fwrite(buf, offset, 1, stdout);
}

static void http_error(char* type, char* msg) {
    fprintf(stdout, type, msg);
    exit(EXIT_FAILURE);
}

static char* get_arg(char* query) {
    int len = 0;
    while(query[len] && query[len] != '&') len++;
    if(len > 0) {
        char* name = malloc(len+1);
        memcpy(name, query, len);
        name[len] = 0;
        return name;
    } else return NULL;
}

static int del_user(FILE* fp, SIMPLE_PB* spb) {
    COUNTER *d = (COUNTER *)spb->target;
    uint32_t next = ftell(fp);
    uint32_t this = next - spb->real_len;
    fseek(fp, 0, SEEK_END);
    uint32_t end = ftell(fp);
    if(next == end) {
        return ftruncate(fileno(fp), end - spb->real_len);
    } else {
        uint32_t cap = end - next;
        char *data = malloc(cap);
        if(data) {
            fseek(fp, next, SEEK_SET);
            if(fread(data, cap, 1, fp) == 1) {
                if(!ftruncate(fileno(fp), end - spb->real_len)){
                    fseek(fp, this, SEEK_SET);
                    if(fwrite(data, cap, 1, fp) == 1) {
                        free(data);
                        return 0;
                    }
                }
            }
            free(data);
        }
        return 2;
    }
}

static int add_user(char* name, uint32_t count, FILE* fp) {
    counter.count = count;
    strncpy(counter.name, name, COUNTER_NAME_LEN-1);
    fseek(fp, 0, SEEK_END);
    return !set_pb(fp, items_len, sizeof(COUNTER), &counter);
}

static uint32_t get_content_len(int isbig, uint16_t* len_type, char* cntstr) {
    uint32_t len = sizeof(svg_small) - 1
        + (sizeof(img_slot_front) + sizeof(img_slot_rear) - 2) * 10
        + 16 + isbig
        + sizeof(svg_tail) - 1;
    for(int i = 0; i < 10; i++) {
        len += len_type[cntstr[i] - '0'];
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
        int ch, exist = 0;
        while(has_next(fp, ch)) {
            SIMPLE_PB *spb = get_pb(fp);
            COUNTER *d = (COUNTER *)spb->target;
            if (!strcmp(name, d->name)) {
                if(del_user(fp, spb)) http_error(HTTP500, "Unable to Delete Old Data.");
                else {
                    if (add_user(d->name, d->count + 1, fp)) http_error(HTTP500, "Add User Error.");
                    else {
                        fflush(fp);
                        char cntstr[11];
                        sprintf(cntstr, "%010u", d->count);
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
                        headers(get_content_len(isbig, len_type, cntstr), "image/svg+xml", 1);
                        fwrite(head, sizeof(svg_small)-1, 1, stdout);
                        for(int i = 0; i < 10; i++) {
                            printf(img_slot_front, w * i, w, h);
                            int n = cntstr[i] - '0';
                            fwrite(theme_type[n], len_type[n], 1, stdout);
                            printf(img_slot_rear);
                        }
                        fwrite(svg_tail, sizeof(svg_tail)-1, 1, stdout);
                    }
                    free(spb);
                    return;
                }
            }
            else free(spb);
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
//Usage: cmoe method query_string
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
                                add_user(name, 0, fp);
                                char* msg = "<P>Success.\r\n";
                                headers(strlen(msg), "text/html", 1);
                                fwrite(msg, strlen(msg), 1, stdout);
                                fclose(fp);
                            } else http_error(HTTP500, "Open File Error.");
                        } else http_error(HTTP400, "Name Exist.");
                    } else http_error(HTTP400, "Null Register Token.");
                } else return_count(name, theme);
            } else http_error(HTTP400, "Null Name Argument.");
        } else http_error(HTTP400, "Name Argument Notfound.");
    } else http_error(HTTP500, "Argument Count Error.");
    return 0;
}
