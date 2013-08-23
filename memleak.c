/*
 * Copyright 2013 Jung-Sang Ahn <jungsang.ahn@gmail.com>.
 * All Rights Reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define _MALLOC_OVERRIDE
//#define _PRINT_DBG

#ifdef _PRINT_DBG
    #define DBG(args...) fprintf(stderr, args)
#else
    #define DBG(args...)
#endif

#include "rbwrap.h"

struct memleak_item {
    uint64_t addr;
    char *file;
    size_t size;
    size_t line;
    struct rb_node rb;
};

static struct rb_root rbtree;
static uint8_t start_sw = 0;

int memleak_cmp(struct rb_node *a, struct rb_node *b, void *aux)
{
    struct memleak_item *aa, *bb;
    aa = _get_entry(a, struct memleak_item, rb);
    bb = _get_entry(b, struct memleak_item, rb);
    if (aa->addr < bb->addr) return -1;
    else if (aa->addr > bb->addr) return 1;
    else return 0;
}

void memleak_start()
{
    rbwrap_init(&rbtree, NULL);
    start_sw = 1;
}

void memleak_end()
{
    struct rb_node *r;
    struct memleak_item *item;
    
    start_sw = 0;

    r = rb_first(&rbtree);
    while(r){
        item = _get_entry(r, struct memleak_item, rb);
        r = rb_next(r);
        rb_erase(&item->rb, &rbtree);

        fprintf(stderr, "address 0x%016lx (allocated at %s:%ld, size %ld) is not freed\n", 
            item->addr, item->file, item->line, item->size);
        free(item);
    }
}

void * memleak_alloc(size_t size, char *file, size_t line)
{
    void *addr = malloc(size);
    if (addr && start_sw) {
        DBG("malloc at %s:%ld, size %ld\n", file, line, size);
        struct memleak_item *item = (struct memleak_item *)malloc(sizeof(struct memleak_item));
        item->addr = (uint64_t)addr;
        item->file = file;
        item->line = line;
        item->size = size;
        rbwrap_insert(&rbtree, &item->rb, memleak_cmp);
    }

    return addr;
}

void memleak_free(void *addr)
{
    free(addr);
    if (start_sw) {
        struct rb_node *r;
        struct memleak_item *item, query;

        query.addr = (uint64_t)addr;
        r = rbwrap_search(&rbtree, &query.rb, memleak_cmp);
        if (!r) return;

        item = _get_entry(r, struct memleak_item, rb);
        DBG("free address 0x%016lx (allocated at %s:%ld, size %ld)\n", 
            item->addr, item->file, item->line, item->size);

        rb_erase(r, &rbtree);
    }
}

