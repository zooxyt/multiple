/* Garbage Collection Infrastructure
 * Copyright(C) 2013-2014 Cheryl Natsu

 * This file is part of multiple - Multiple Paradigm Language Emulator

 * multiple is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * multiple is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. 
 */


#include <stdlib.h>
#include <stdio.h>

#include "gc.h"


/* Declarations */

static struct gc_object_table_item *gc_object_table_item_new(void);
static int gc_object_table_item_destroy(struct gc_object_table_item *object_table_item);

static struct gc_object_table_item_list *gc_object_table_item_list_new(void);
static int gc_object_table_item_list_destroy(struct gc_object_table_item_list *list);
static int gc_object_table_item_list_append(struct gc_object_table_item_list *list, \
        struct gc_object_table_item *new_object_table_item);
static int gc_object_table_item_list_marks_clear(struct gc_object_table_item_list *list);
static int gc_object_table_item_list_remove_item(struct gc_object_table_item_list *list, \
        void *internal_object_ptr);

static struct gc_object_table *gc_object_table_new(void);
static int gc_object_table_destroy(struct gc_object_table *table);
static int gc_object_table_append(struct gc_object_table *table, \
        struct gc_object_table_item *new_object);
static int gc_object_table_marks_clear(struct gc_object_table *object_table);
static int gc_object_table_remove_item(struct gc_object_table *object_table, void *internal_object_ptr);
int gc_object_table_transport_to_survivor(struct gc_object_table *table, \
        struct gc_object_table_item *object_table_item);


/* Object Table */

static struct gc_object_table_item *gc_object_table_item_new(void)
{
    struct gc_object_table_item *new_object_table_item = NULL;

    new_object_table_item = (struct gc_object_table_item *)malloc(sizeof(struct gc_object_table_item));
    if (new_object_table_item == NULL) 
    { goto fail; }
    new_object_table_item->mark = 0;
    new_object_table_item->mark_count = 0;
    new_object_table_item->type = GC_OBJECT_TABLE_ITEM_TYPE_EDEN;
    new_object_table_item->prev = new_object_table_item->next = NULL;
    new_object_table_item->internal_object_ptr = NULL;
    new_object_table_item->internal_marker = NULL;
    new_object_table_item->internal_collector = NULL;
    goto done;
fail:
    if (new_object_table_item != NULL)
    {
        free(new_object_table_item);
        new_object_table_item = NULL;
    }
done:
    return new_object_table_item;
}

static int gc_object_table_item_destroy(struct gc_object_table_item *object_table_item)
{
    free(object_table_item);

    return 0;
}

static struct gc_object_table_item_list *gc_object_table_item_list_new(void)
{
    struct gc_object_table_item_list *new_list = NULL;

    new_list = (struct gc_object_table_item_list *)malloc(sizeof(struct gc_object_table_item_list));
    if (new_list == NULL) return NULL;
    new_list->begin = new_list->end = NULL;
    new_list->size = 0;

    return new_list;
}

static int gc_object_table_item_list_destroy(struct gc_object_table_item_list *list)
{
    struct gc_object_table_item *object_table_item_cur, *object_table_item_next;
    int confirm;

    object_table_item_cur = list->begin;
    while (object_table_item_cur != NULL)
    {
        object_table_item_next = object_table_item_cur->next; 

        object_table_item_cur->internal_collector(object_table_item_cur->internal_object_ptr, &confirm);

        gc_object_table_item_destroy(object_table_item_cur);
        object_table_item_cur = object_table_item_next; 
    }
    free(list);

    return 0;
}

static int gc_object_table_item_list_append(struct gc_object_table_item_list *list, \
        struct gc_object_table_item *new_object_table_item)
{
    if (list->begin == NULL)
    {
        list->begin = list->end = new_object_table_item;
    }
    else
    {
        list->end->next = new_object_table_item;
        new_object_table_item->prev = list->end;
        list->end = new_object_table_item;
    }
    list->size += 1;

    return 0;
}

static int gc_object_table_item_list_marks_clear(struct gc_object_table_item_list *list)
{
    struct gc_object_table_item *table_item_cur;

    table_item_cur = list->begin;
    while (table_item_cur != NULL)
    {
        table_item_cur->mark = 0;

        table_item_cur = table_item_cur->next;
    }

    return 0;
}

static int gc_object_table_item_list_remove_item(struct gc_object_table_item_list *list, \
        void *internal_object_ptr)
{
    struct gc_object_table_item *table_item_cur, *table_item_target;

    table_item_cur = list->begin;
    while (table_item_cur != NULL)
    {
        if (table_item_cur->internal_object_ptr == internal_object_ptr)
        {
            /* Found, remove it */
            table_item_target = table_item_cur;

            /* When at first or last */
            if (table_item_target->prev == NULL) { list->begin = table_item_target->next; }
            if (table_item_target->next == NULL) { list->end = table_item_target->prev; }

            if (table_item_target->next != NULL) table_item_target->next->prev = table_item_target->prev;
            if (table_item_target->prev != NULL) table_item_target->prev->next = table_item_target->next;

            gc_object_table_item_destroy(table_item_target);

            list->size -= 1;

            /* Did removing */
            return 1;
        }
        table_item_cur = table_item_cur->next;
    }

    return 0;
}


int gc_transport_to_survivor(gc_stub_t *gc_stub, \
        struct gc_object_table_item *object_table_item)
{
    int ret = 0;

    ret = gc_object_table_item_list_remove_item(gc_stub->obj_tbl->eden, object_table_item->internal_object_ptr);
    if (ret != 1) /* Did removing */
    { return -1; }
    gc_object_table_item_list_append(gc_stub->obj_tbl->survivor, object_table_item);
    object_table_item->type = GC_OBJECT_TABLE_ITEM_TYPE_SURVIVOR;

    return 0;
}


static struct gc_object_table *gc_object_table_new(void)
{
    struct gc_object_table *new_object_table = NULL;

    new_object_table = (struct gc_object_table *)malloc(sizeof(struct gc_object_table));
    if (new_object_table == NULL) goto fail;
    new_object_table->eden = NULL;
    new_object_table->survivor = NULL;
    new_object_table->permanent = NULL;

    new_object_table->eden = gc_object_table_item_list_new();
    if (new_object_table->eden == NULL) goto fail;

    new_object_table->survivor = gc_object_table_item_list_new();
    if (new_object_table->survivor == NULL) goto fail;

    new_object_table->permanent = gc_object_table_item_list_new();
    if (new_object_table->permanent == NULL) goto fail;

    goto done;
fail:
    if (new_object_table != NULL)
    {
        if (new_object_table->eden != NULL)
        { gc_object_table_item_list_destroy(new_object_table->eden); }
        if (new_object_table->survivor != NULL)
        { gc_object_table_item_list_destroy(new_object_table->survivor); }
        if (new_object_table->permanent != NULL)
        { gc_object_table_item_list_destroy(new_object_table->permanent); }
        free(new_object_table);
        new_object_table = NULL;
    }
done:
    return new_object_table;
}

static int gc_object_table_destroy(struct gc_object_table *object_table)
{
    if (object_table->eden != NULL)
    { gc_object_table_item_list_destroy(object_table->eden); }
    if (object_table->survivor != NULL)
    { gc_object_table_item_list_destroy(object_table->survivor); }
    if (object_table->permanent != NULL)
    { gc_object_table_item_list_destroy(object_table->permanent); }
    free(object_table);

    return 0;
}

static int gc_object_table_append(struct gc_object_table *table, \
        struct gc_object_table_item *object)
{
    return gc_object_table_item_list_append(table->eden, object);
}

static int gc_object_table_marks_clear(struct gc_object_table *object_table)
{
    gc_object_table_item_list_marks_clear(object_table->eden);
    gc_object_table_item_list_marks_clear(object_table->survivor);
    gc_object_table_item_list_marks_clear(object_table->permanent);

    return 0;
}

static int gc_object_table_remove_item(struct gc_object_table *object_table, void *internal_object_ptr)
{
    if (gc_object_table_item_list_remove_item(object_table->eden, internal_object_ptr) != 0) return 0;
    if (gc_object_table_item_list_remove_item(object_table->survivor, internal_object_ptr) != 0) return 0;
    if (gc_object_table_item_list_remove_item(object_table->permanent, internal_object_ptr) != 0) return 0;

    return 0;
}



/* Interface */

/* Create and destrot GC Stub */

gc_stub_t *gc_stub_new(void)
{
    gc_stub_t *new_stub = NULL;

    new_stub = (gc_stub_t *)malloc(sizeof(gc_stub_t));
    if (new_stub == NULL) { goto fail; }
    new_stub->obj_tbl = gc_object_table_new();
    if (new_stub->obj_tbl == NULL) { goto fail; }

    goto done;
fail:
    if (new_stub != NULL)
    {
        gc_stub_destroy(new_stub);
        new_stub = NULL;
    }
done:
    return new_stub;
}

int gc_stub_destroy(gc_stub_t *gc_stub)
{
    if (gc_stub->obj_tbl != NULL)
    { gc_object_table_destroy(gc_stub->obj_tbl); }
    free(gc_stub);

    return 0;
}

/* Register new allocated reference object */
int gc_reference_register( \
        gc_stub_t *gc_stub, \
        void *object_internal_ptr, \
        int (*internal_marker)(void *internal_object_ptr), \
        int (*internal_collector)(void *internal_object_ptr, int *confirm), \
        gc_object_table_item_t **target_object_table_item)
{
    int ret = 0;
    gc_object_table_item_t *new_object_table_item = NULL;

    *target_object_table_item = NULL;

    new_object_table_item = gc_object_table_item_new();
    if (new_object_table_item == NULL) 
    { ret = -1; goto fail; }
    new_object_table_item->internal_object_ptr = object_internal_ptr;
    new_object_table_item->internal_marker = internal_marker;
    new_object_table_item->internal_collector = internal_collector;
    *target_object_table_item = new_object_table_item;

    gc_object_table_append(gc_stub->obj_tbl, new_object_table_item);
    new_object_table_item = NULL;

    goto done;
fail:
    if (new_object_table_item != NULL)
    { gc_object_table_item_destroy(new_object_table_item); }
done:
    return ret;
}

/* Unregister useless object */
int gc_resource_reference_unregister( \
        gc_stub_t *gc_stub, \
        void *object_internal_ptr)
{
    gc_object_table_remove_item(gc_stub->obj_tbl, object_internal_ptr);
    return 0;
}

/* Clean Marks */
int gc_begin(gc_stub_t *gc_stub)
{
    return gc_object_table_marks_clear(gc_stub->obj_tbl);
}

/* Mark an item */
int gc_mark_major(gc_object_table_item_t *item)
{
    if (item == NULL) return 0;

    if (item->mark == 0)
    {
        item->mark = 1;
        item->internal_marker(item->internal_object_ptr);
        item->mark_count++;
    }
    return 0;
}
int gc_mark_minor(gc_object_table_item_t *item)
{
    if (item == NULL) return 0;

    if (item->mark == 0)
    {
        item->mark = 1;
        if (item->type == GC_OBJECT_TABLE_ITEM_TYPE_EDEN)
        { item->internal_marker(item->internal_object_ptr); }
        item->mark_count++;
    }
    return 0;
}


static int gc_object_table_item_list_garbage_collect( \
        gc_object_table_item_list_t *list)
{
    struct gc_object_table_item *table_item_cur, *table_item_target;
    int confirm;

    table_item_cur = list->begin;
    while (table_item_cur != NULL)
    {
        if (table_item_cur->mark == 0)
        {
            /* no marked, collect */
            table_item_cur->internal_collector(table_item_cur->internal_object_ptr, &confirm);

            /* Confirming if to remove the item */
            if (confirm == GC_GARBAGE_COLLECT_CONFIRM)
            {
                table_item_target = table_item_cur;

                /* Remove item */
                if (table_item_cur->prev == NULL) { list->begin = table_item_cur->next; }
                if (table_item_cur->next == NULL) { list->end = table_item_cur->prev; }
                if (table_item_cur->prev != NULL) { table_item_cur->prev->next = table_item_cur->next; }
                if (table_item_cur->next != NULL) { table_item_cur->next->prev = table_item_cur->prev; }

                table_item_cur = table_item_target->next;

                gc_object_table_item_destroy(table_item_target);
                list->size -= 1;
            }
            else
            {
                /* Skip to next item only when current item not been collected */
                table_item_cur = table_item_cur->next;
            }
        }
        else
        {
            /* Skip to next item only when current item not been collected */
            table_item_cur = table_item_cur->next;
        }
    }

    return 0;
}

int gc_collect(gc_stub_t *gc_stub)
{
    gc_object_table_item_list_garbage_collect(gc_stub->obj_tbl->eden);
    gc_object_table_item_list_garbage_collect(gc_stub->obj_tbl->survivor);
    gc_object_table_item_list_garbage_collect(gc_stub->obj_tbl->permanent);

    return 0;
}

