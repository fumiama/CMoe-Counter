#include <simple_protobuf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cmoe.h"

static uint32_t* items_len;
static counter_t counter;

static FILE* fp;

static char buf[sizeof(simple_pb_t)+sizeof(counter_t)];

static int del_user(FILE* fp, simple_pb_t* spb) {
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

#define has_next(fp, ch) ((ch=getc(fp)),(feof(fp)?0:(ungetc(ch,fp),1)))

int main(int argc, char** argv) {
    if (argc != 2) {
        puts("Usage: editor dat.sp");
        return 0;
    }
    fp = fopen(argv[1], "rb+");
    if (!fp) {
        perror("fopen");
        return 1;
    }
    items_len = align_struct(sizeof(counter_t), 2, &counter.name, &counter.count);
    if(!items_len) {
        perror("align_struct");
        return 2;
    }
    int go;
    lop: do {
        go = 1;
        puts("\ncommands:");
        puts("\ta string uint: add user with count.");
        puts("\td string: delete user.");
        puts("\tD string: delete users match the keyword (empty is not allowed).");
        puts("\tg string: get user count.");
        puts("\tm uint: print users < count.");
        puts("\tM uint: delete users < count.");
        puts("\tp string: print users match the keyword (empty for all).");
        puts("\tq: quit.");
        putchar('>');putchar(' ');
        char c = getchar();
        switch (c) {
            case 'a':
                while ((c = getchar()) == ' '); // skip space
                memset(counter.name, 0, sizeof(counter.name));
                counter.name[0] = c;
                while ((c = getchar()) != ' ' && go < sizeof(counter.name)-1) counter.name[go++] = c;
                while ((c = getchar()) == ' '); // skip space
                ungetc(c, stdin);
                scanf("%u", &counter.count);
                while (getchar() != '\n'); // skip to endl
                printf("set user '%s' to %u\n", counter.name, counter.count);
                if (fseek(fp, 0, SEEK_END)) {
                    perror("fseek");
                    return 3;
                }
                if (set_pb(fp, items_len, sizeof(counter_t), &counter) != 2) {
                    puts("set pb error.");
                    return 4;
                }
            break;
            case 'd':
                while ((c = getchar()) == ' '); // skip space
                memset(counter.name, 0, sizeof(counter.name));
                counter.name[0] = c;
                while ((c = getchar()) != '\n' && go < sizeof(counter.name)-1) counter.name[go++] = c;
                if (fseek(fp, 0, SEEK_SET)) {
                    perror("fseek");
                    return 5;
                }
                while(has_next(fp, c)) {
                    simple_pb_t *spb = read_pb_into(fp, (simple_pb_t*)buf);
                    counter_t *d = (counter_t *)spb->target;
                    if (strcmp(counter.name, d->name)) continue;
                    if(del_user(fp, spb)) {
                        perror("del_user");
                        return 6;
                    }
                    printf("del user '%s'.\n", d->name);
                    goto lop;
                }
                printf("no such user '%s'.\n", counter.name);
            break;
            case 'D':
                if (getchar() == '\n') {
                    puts("empty D is not allowed.");
                    break;
                }
                while ((c = getchar()) == ' '); // skip space
                memset(counter.name, 0, sizeof(counter.name));
                counter.name[0] = c;
                while ((c = getchar()) != '\n' && go < sizeof(counter.name)-1) counter.name[go++] = c;
                go = 1;
            scanlopD:
                if (fseek(fp, 0, SEEK_SET)) {
                    perror("fseek");
                    return 7;
                }
                while(has_next(fp, c)) {
                    simple_pb_t *spb = read_pb_into(fp, (simple_pb_t*)buf);
                    counter_t *d = (counter_t *)spb->target;
                    if (strstr(d->name, counter.name)) {
                        if(del_user(fp, spb)) {
                            perror("del_user");
                            return 8;
                        }
                        printf("del user '%s'.\n", d->name);
                        go++;
                        goto scanlopD;
                    }
                }
                if (go == 1) puts("no result.");
            break;
            case 'g':
                while ((c = getchar()) == ' '); // skip space
                memset(counter.name, 0, sizeof(counter.name));
                counter.name[0] = c;
                while ((c = getchar()) != '\n' && go < sizeof(counter.name)-1) counter.name[go++] = c;
                if (fseek(fp, 0, SEEK_SET)) {
                    perror("fseek");
                    return 5;
                }
                while(has_next(fp, c)) {
                    simple_pb_t *spb = read_pb_into(fp, (simple_pb_t*)buf);
                    counter_t *d = (counter_t *)spb->target;
                    if (strcmp(counter.name, d->name)) continue;
                    printf("user '%s' = %u.\n", d->name, d->count);
                    goto lop;
                }
                printf("no such user '%s'.\n", counter.name);
            break;
            case 'm':
                while ((c = getchar()) == ' '); // skip space
                ungetc(c, stdin);
                scanf("%u", &counter.count);
                while (getchar() != '\n'); // skip to endl
                go = 1;
                if (fseek(fp, 0, SEEK_SET)) {
                    perror("fseek");
                    return 7;
                }
                while(has_next(fp, c)) {
                    simple_pb_t *spb = read_pb_into(fp, (simple_pb_t*)buf);
                    counter_t *d = (counter_t *)spb->target;
                    if (d->count < counter.count) {
                        printf("'%s' = %u\n", d->name, d->count);
                        go++;
                    }
                }
                if (go == 1) puts("no result.");
            break;
            case 'M':
                if (getchar() == '\n') {
                    puts("empty M is not allowed.");
                    break;
                }
                while ((c = getchar()) == ' '); // skip space
                ungetc(c, stdin);
                scanf("%u", &counter.count);
                while (getchar() != '\n'); // skip to endl
                go = 1;
            scanlopM:
                if (fseek(fp, 0, SEEK_SET)) {
                    perror("fseek");
                    return 7;
                }
                while(has_next(fp, c)) {
                    simple_pb_t *spb = read_pb_into(fp, (simple_pb_t*)buf);
                    counter_t *d = (counter_t *)spb->target;
                    if (d->count < counter.count) {
                        if(del_user(fp, spb)) {
                            perror("del_user");
                            return 8;
                        }
                        printf("del user '%s'.\n", d->name);
                        go++;
                        goto scanlopM;
                    }
                }
                if (go == 1) puts("no result.");
            break;
            case 'p':
                if ((c = getchar()) != '\n') {
                    while ((c = getchar()) == ' '); // skip space
                    memset(counter.name, 0, sizeof(counter.name));
                    counter.name[0] = c;
                    while ((c = getchar()) != '\n' && go < sizeof(counter.name)-1) counter.name[go++] = c;
                } else counter.name[0] = 0;
                go = 1;
                if (fseek(fp, 0, SEEK_SET)) {
                    perror("fseek");
                    return 7;
                }
                while(has_next(fp, c)) {
                    simple_pb_t *spb = read_pb_into(fp, (simple_pb_t*)buf);
                    counter_t *d = (counter_t *)spb->target;
                    if (!counter.name[0] || strstr(d->name, counter.name)) {
                        printf("'%s' = %u\n", d->name, d->count);
                        go++;
                    }
                }
                if (go == 1) puts("no result.");
            break;
            case 'q':
                go = 0;
            break;
            default:
                printf("invalid operator '%c'.\n", c);
                while (getchar() != '\n'); // skip to endl
            break;
        }
    } while (go);
}

static void __attribute__((destructor)) defer_close_fp() {
    if(fp) {
        if (fclose(fp)) perror("fclose");
        else puts("file closed.");
    }
}
