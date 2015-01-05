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

#ifndef _GC_H_
#define _GC_H_

#include <stdio.h>


/* Object Item Type */
enum gc_object_table_item_type
{
    GC_OBJECT_TABLE_ITEM_TYPE_UNKNOWN = 0,
    GC_OBJECT_TABLE_ITEM_TYPE_EDEN,
    GC_OBJECT_TABLE_ITEM_TYPE_SURVIVOR,
    GC_OBJECT_TABLE_ITEM_TYPE_PERMANENT,
};
typedef enum gc_object_table_item_type gc_object_table_item_type_t;

#define GC_GARBAGE_COLLECT_CONFIRM (0)
#define GC_GARBAGE_COLLECT_POSTPOND (1)


/* Object Table */

struct gc_object_table_item
{
	/* Visited */
    int mark;
	
	/* Not yet been used */
	gc_object_table_item_type_t type;
	size_t mark_count;

	void *internal_object_ptr;

	int (*internal_marker)(void *internal_object_ptr);
	int (*internal_collector)(void *internal_object_ptr, int *confirm);

    struct gc_object_table_item *next;
    struct gc_object_table_item *prev;
};
typedef struct gc_object_table_item gc_object_table_item_t;

struct gc_object_table_item_list
{
    gc_object_table_item_t *begin;
    gc_object_table_item_t *end;
    size_t size;
};
typedef struct gc_object_table_item_list gc_object_table_item_list_t;

struct gc_object_table
{
    /* New allocated */
    gc_object_table_item_list_t *eden;
    /* Experienced (1) collection but still alive */
    gc_object_table_item_list_t *survivor;
    /* Stay in pool for long time and will not be easily collected */
    gc_object_table_item_list_t *permanent;
};
typedef struct gc_object_table gc_object_table_t;


/* Data structure that maintains things been used on GC */
struct gc_stub
{
    gc_object_table_t *obj_tbl;
};
typedef struct gc_stub gc_stub_t;

/* Create and destrot GC Stub */
gc_stub_t *gc_stub_new(void);
int gc_stub_destroy(gc_stub_t *gc_stub);

/* Register new allocated reference object */
int gc_reference_register( \
        gc_stub_t *gc_stub, \
        void *object_internal_ptr, \
        int (*internal_marker)(void *internal_object_ptr), \
        int (*internal_collector)(void *internal_object_ptr, int *confirm), \
        gc_object_table_item_t **target_object_table_item);
/* Unregister useless object */
int gc_resource_reference_unregister( \
        gc_stub_t *gc_stub, \
        void *object_internal_ptr);

/* Clean Marks */
int gc_begin(gc_stub_t *gc_stub);

/* Mark an item */
int gc_mark_major(gc_object_table_item_t *item);
int gc_mark_minor(gc_object_table_item_t *item);

/* Transport */
int gc_transport_to_survivor(gc_stub_t *gc_stub, \
        struct gc_object_table_item *object_table_item);

/* Collect */
int gc_collect(gc_stub_t *gc_stub);
/*int gc_perform_major(gc_stub_t *gc_stub);*/
/*int gc_perform_major_feedback(gc_stub_t *gc_stub);*/
/*int gc_perform_minor(gc_stub_t *gc_stub);*/

#endif

