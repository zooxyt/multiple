/* Virtual Machine : Resource Management
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

#include "selfcheck.h"

#include <stdlib.h>

#include "vm_res.h"
#include "vm_res_pool_plain.h"
#include "vm_res_pool_page.h"
#include "vm_res_pool_linked.h"
#include "vm_err.h"
#include "multiple_err.h"


/* Write Barriers */
struct virtual_machine_write_barrier *virtual_machine_write_barrier_new(void)
{
    struct virtual_machine_write_barrier *new_write_barrier = NULL;

    new_write_barrier = (struct virtual_machine_write_barrier *)malloc(sizeof(struct virtual_machine_write_barrier));
    if (new_write_barrier == NULL) { goto fail; }
    new_write_barrier->eden = NULL;
    new_write_barrier->survivor = NULL;
    new_write_barrier->next = NULL;
fail:
    return new_write_barrier;
}

int virtual_machine_write_barrier_destroy(struct virtual_machine_write_barrier *write_barrier)
{
    free(write_barrier);

    return 0;
}

struct virtual_machine_write_barrier_list *virtual_machine_write_barrier_list_new(void)
{
    struct virtual_machine_write_barrier_list *new_write_barrier_list = NULL;

    new_write_barrier_list = (struct virtual_machine_write_barrier_list *)malloc(sizeof(struct virtual_machine_write_barrier_list));
    if (new_write_barrier_list == NULL) { goto fail; }

    new_write_barrier_list->begin = new_write_barrier_list->end = NULL;

fail:
    return new_write_barrier_list;
}

int virtual_machine_write_barrier_list_destroy(struct virtual_machine_write_barrier_list *write_barrier_list)
{
    struct virtual_machine_write_barrier *write_barrier_cur, *write_barrier_next;

    write_barrier_cur = write_barrier_list->begin;
    while (write_barrier_cur != NULL)
    {
        write_barrier_next = write_barrier_cur->next;
        virtual_machine_write_barrier_destroy(write_barrier_cur);
        write_barrier_cur = write_barrier_next;
    }
    free(write_barrier_list);

    return 0;
}

int virtual_machine_write_barrier_list_append(struct virtual_machine_write_barrier_list *write_barrier_list, \
        struct virtual_machine_write_barrier *new_write_barrier)
{
    if (write_barrier_list->begin == NULL)
    {
        write_barrier_list->begin = write_barrier_list->end = new_write_barrier;
    }
    else
    {
        write_barrier_list->end->next = new_write_barrier;
        write_barrier_list->end = new_write_barrier;
    }

    return 0;
}

int virtual_machine_write_barrier_list_update(struct virtual_machine_write_barrier_list *write_barrier_list, \
    struct virtual_machine_object_table_item *eden, \
    struct virtual_machine_object_table_item *survivor)
{
    int ret = 0;
    struct virtual_machine_write_barrier *write_barrier_cur;
    struct virtual_machine_write_barrier *new_write_barrier = NULL;

    write_barrier_cur = write_barrier_list->begin;
    while (write_barrier_cur != NULL)
    {
        if ((write_barrier_cur->eden == eden) && \
                (write_barrier_cur->survivor == survivor))
        { return 0; }
        write_barrier_cur = write_barrier_cur->next;
    }

    new_write_barrier = virtual_machine_write_barrier_new();
    if (new_write_barrier == NULL)
    {
        ret = -MULTIPLE_ERR_MALLOC;
        goto fail;
    }
    new_write_barrier->eden = eden;
    new_write_barrier->survivor = survivor;
    virtual_machine_write_barrier_list_append(write_barrier_list, new_write_barrier);
    new_write_barrier = NULL;

    goto done;
fail:
    if (new_write_barrier != NULL)
    {
        virtual_machine_write_barrier_destroy(new_write_barrier);
    }
done:
    return ret;
}

int virtual_machine_write_barrier_list_eden_is_ref_by_survivor(struct virtual_machine_write_barrier_list *write_barrier_list, \
    struct virtual_machine_object_table_item *eden)
{
    struct virtual_machine_write_barrier *write_barrier_cur;

    write_barrier_cur = write_barrier_list->begin;
    while (write_barrier_cur != NULL)
    {
        if (write_barrier_cur->eden == eden)
        { return 1; }
        write_barrier_cur = write_barrier_cur->next;
    }

    return 0;
}

int virtual_machine_write_barrier_list_remove_item(struct virtual_machine_write_barrier_list *write_barrier_list, \
    struct virtual_machine_object_table_item *item)
{
    struct virtual_machine_write_barrier *write_barrier_cur, *write_barrier_next;
    struct virtual_machine_write_barrier *write_barrier_prev = NULL;

    write_barrier_cur = write_barrier_list->begin;
    while (write_barrier_cur != NULL)
    {
        write_barrier_next = write_barrier_cur->next; 
        if ((write_barrier_cur->eden == item) || \
                (write_barrier_cur->survivor == item))
        {
            virtual_machine_write_barrier_destroy(write_barrier_cur);
            if (write_barrier_prev != NULL)
            {
                write_barrier_prev->next = write_barrier_next;
            }
            else
            {
                write_barrier_list->begin = write_barrier_next;
            }
        }
        else
        {
            write_barrier_prev = write_barrier_cur;
        }
        write_barrier_cur = write_barrier_next;
    }

    return 0;
}


/* Resource */
struct virtual_machine_resource_source *virtual_machine_resource_source_new(\
        void *(*init)(size_t size), \
        int (*uninit)(void *pool_ptr), \
        void *(*pool_malloc)(void *pool_ptr, size_t size), \
        int (*pool_free)(void *pool_ptr, void *ptr),
        int (*lack)(void *pool_ptr),
        int (*feedback)(void *pool_ptr),
        size_t size)
{
    struct virtual_machine_resource_source *new_resource_source = NULL;

    new_resource_source = (struct virtual_machine_resource_source *)malloc(sizeof(struct virtual_machine_resource_source));
    if (new_resource_source == NULL) goto fail;
    new_resource_source->init = init;
    new_resource_source->uninit = uninit;
    new_resource_source->malloc = pool_malloc;
    new_resource_source->free = pool_free;
    new_resource_source->lack = lack;
    new_resource_source->feedback = feedback;
    new_resource_source->pool = (new_resource_source->init)(size);
    goto done;
fail:
    if (new_resource_source != NULL)
    {
        free(new_resource_source);
        new_resource_source = NULL;
    }
done:
    return new_resource_source;
}

static struct virtual_machine_resource_source *virtual_machine_resource_source_new_preset(int type, size_t size)
{
    switch (type)
    {
        case VIRTUAL_MACHINE_STARTUP_MEM_TYPE_LIB_C:
            return virtual_machine_resource_source_new(\
                    &virtual_machine_resource_pool_plain_init, 
                    &virtual_machine_resource_pool_plain_uninit, 
                    &virtual_machine_resource_pool_plain_malloc, 
                    &virtual_machine_resource_pool_plain_free, 
                    &virtual_machine_resource_pool_plain_lack, 
                    NULL, 
                    size);
        case VIRTUAL_MACHINE_STARTUP_MEM_TYPE_PAGED_FALLBACK:
            return virtual_machine_resource_source_new(\
                    &virtual_machine_resource_pool_page_init, 
                    &virtual_machine_resource_pool_page_uninit, 
                    &virtual_machine_resource_pool_page_malloc, 
                    &virtual_machine_resource_pool_page_free, 
                    &virtual_machine_resource_pool_page_lack, 
                    NULL, 
                    size);
        case VIRTUAL_MACHINE_STARTUP_MEM_TYPE_PAGED_64B_FALLBACK:
            return virtual_machine_resource_source_new(\
                    &virtual_machine_resource_pool_page_64b_init, 
                    &virtual_machine_resource_pool_page_uninit, 
                    &virtual_machine_resource_pool_page_malloc, 
                    &virtual_machine_resource_pool_page_free, 
                    &virtual_machine_resource_pool_page_lack, 
                    NULL, 
                    size);
        case VIRTUAL_MACHINE_STARTUP_MEM_TYPE_PAGED_128B_FALLBACK:
            return virtual_machine_resource_source_new(\
                    &virtual_machine_resource_pool_page_128b_init, 
                    &virtual_machine_resource_pool_page_uninit, 
                    &virtual_machine_resource_pool_page_malloc, 
                    &virtual_machine_resource_pool_page_free, 
                    &virtual_machine_resource_pool_page_lack, 
                    NULL,
                    size);
        case VIRTUAL_MACHINE_STARTUP_MEM_TYPE_LINKED_FALLBACK:
            return virtual_machine_resource_source_new(\
                    &virtual_machine_resource_pool_linked_init, 
                    &virtual_machine_resource_pool_linked_uninit, 
                    &virtual_machine_resource_pool_linked_malloc, 
                    &virtual_machine_resource_pool_linked_free, 
                    &virtual_machine_resource_pool_linked_lack, 
                    &virtual_machine_resource_pool_linked_feedback, 
                    size);
        case VIRTUAL_MACHINE_STARTUP_MEM_TYPE_LINKED_64B_FALLBACK:
            return virtual_machine_resource_source_new(\
                    &virtual_machine_resource_pool_linked_64b_init, 
                    &virtual_machine_resource_pool_linked_uninit, 
                    &virtual_machine_resource_pool_linked_malloc, 
                    &virtual_machine_resource_pool_linked_free, 
                    &virtual_machine_resource_pool_linked_lack, 
                    &virtual_machine_resource_pool_linked_feedback, 
                    size);
        case VIRTUAL_MACHINE_STARTUP_MEM_TYPE_LINKED_128B_FALLBACK:
            return virtual_machine_resource_source_new(\
                    &virtual_machine_resource_pool_linked_128b_init, 
                    &virtual_machine_resource_pool_linked_uninit, 
                    &virtual_machine_resource_pool_linked_malloc, 
                    &virtual_machine_resource_pool_linked_free, 
                    &virtual_machine_resource_pool_linked_lack, 
                    &virtual_machine_resource_pool_linked_feedback, 
                    size);
        default:
            return NULL;
    }
}

int virtual_machine_resource_source_destroy(struct virtual_machine_resource_source *source)
{
    if (source == NULL) return -MULTIPLE_ERR_NULL_PTR;

    source->uninit(source->pool);
    free(source);

    return 0;
}

struct virtual_machine_resource *virtual_machine_resource_new(struct virtual_machine_startup *startup)
{
    int idx;
    struct virtual_machine_resource *new_resource = NULL;

    new_resource = (struct virtual_machine_resource *)malloc(sizeof(struct virtual_machine_resource));
    if (new_resource == NULL) goto fail;

    for (idx = 0; idx != VIRTUAL_MACHINE_RESOURCE_SOURCE_COUNT; idx++)
    { new_resource->sources[idx] = NULL; }

    if (startup == NULL)
    {
        /* No settings */
        new_resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_INFRASTRUCTURE] = virtual_machine_resource_source_new(\
                &virtual_machine_resource_pool_plain_init, 
                &virtual_machine_resource_pool_plain_uninit, 
                &virtual_machine_resource_pool_plain_malloc, 
                &virtual_machine_resource_pool_plain_free, 
                &virtual_machine_resource_pool_plain_lack, 
                NULL, 
                0);
        new_resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_PRIMITIVE] = virtual_machine_resource_source_new(\
                &virtual_machine_resource_pool_plain_init, 
                &virtual_machine_resource_pool_plain_uninit, 
                &virtual_machine_resource_pool_plain_malloc, 
                &virtual_machine_resource_pool_plain_free, 
                &virtual_machine_resource_pool_plain_lack, 
                NULL,
                0);
        new_resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_REFERENCE] = virtual_machine_resource_source_new(\
                &virtual_machine_resource_pool_plain_init, 
                &virtual_machine_resource_pool_plain_uninit, 
                &virtual_machine_resource_pool_plain_malloc, 
                &virtual_machine_resource_pool_plain_free, 
                &virtual_machine_resource_pool_plain_lack, 
                NULL,
                0);
    }
    else
    {
        new_resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_INFRASTRUCTURE] = virtual_machine_resource_source_new_preset( \
                startup->items[VIRTUAL_MACHINE_STARTUP_MEM_ITEM_INFRASTRUCTURE].type, \
                startup->items[VIRTUAL_MACHINE_STARTUP_MEM_ITEM_INFRASTRUCTURE].size);
        new_resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_PRIMITIVE] = virtual_machine_resource_source_new_preset( \
                startup->items[VIRTUAL_MACHINE_STARTUP_MEM_ITEM_PRIMITIVE].type, \
                startup->items[VIRTUAL_MACHINE_STARTUP_MEM_ITEM_PRIMITIVE].size);
        new_resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_REFERENCE] = virtual_machine_resource_source_new_preset( \
                startup->items[VIRTUAL_MACHINE_STARTUP_MEM_ITEM_REFERENCE].type, \
                startup->items[VIRTUAL_MACHINE_STARTUP_MEM_ITEM_REFERENCE].size);
        if ((new_resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_INFRASTRUCTURE] == NULL) || \
                (new_resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_PRIMITIVE] == NULL) || \
                (new_resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_REFERENCE] == NULL))
        {
            goto fail;
        }
    }

    goto done;
fail:
    if (new_resource != NULL)
    {
        if (new_resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_INFRASTRUCTURE] != NULL) \
            virtual_machine_resource_source_destroy(new_resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_INFRASTRUCTURE]);
        if (new_resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_PRIMITIVE] != NULL) \
            virtual_machine_resource_source_destroy(new_resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_PRIMITIVE]);
        if (new_resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_REFERENCE] != NULL) \
            virtual_machine_resource_source_destroy(new_resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_REFERENCE]);
        free(new_resource);
        new_resource = NULL;
    }
done:
    return new_resource;
}

int virtual_machine_resource_destroy(struct virtual_machine_resource *resource)
{
    if (resource == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_INFRASTRUCTURE] != NULL) \
        virtual_machine_resource_source_destroy(resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_INFRASTRUCTURE]);
    if (resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_PRIMITIVE] != NULL) \
        virtual_machine_resource_source_destroy(resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_PRIMITIVE]);
    if (resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_REFERENCE] != NULL) \
        virtual_machine_resource_source_destroy(resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_REFERENCE]);
    free(resource);

    return 0;
}

void *virtual_machine_resource_malloc(struct virtual_machine_resource *resource, size_t size)
{
    return resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_INFRASTRUCTURE]->malloc(resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_INFRASTRUCTURE]->pool, size);
}

int virtual_machine_resource_free(struct virtual_machine_resource *resource, void *ptr)
{
    return resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_INFRASTRUCTURE]->free(resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_INFRASTRUCTURE]->pool, ptr);
}

void *virtual_machine_resource_malloc_primitive(struct virtual_machine_resource *resource, size_t size)
{
    return resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_PRIMITIVE]->malloc(resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_PRIMITIVE]->pool, size);
}

int virtual_machine_resource_free_primitive(struct virtual_machine_resource *resource, void *ptr)
{
    return resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_PRIMITIVE]->free(resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_PRIMITIVE]->pool, ptr);
}

void *virtual_machine_resource_malloc_reference(struct virtual_machine_resource *resource, size_t size)
{
    return resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_REFERENCE]->malloc(resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_REFERENCE]->pool, size);
}

int virtual_machine_resource_free_reference(struct virtual_machine_resource *resource, void *ptr)
{
    return resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_REFERENCE]->free(resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_REFERENCE]->pool, ptr);
}


/* GC Interface*/

int virtual_machine_resource_lack(struct virtual_machine_resource *resource)
{
    if (resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_PRIMITIVE]->lack(resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_PRIMITIVE]->pool) != 0)
    { return 1; }
    if (resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_REFERENCE]->lack(resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_REFERENCE]->pool) != 0)
    { return 1; }
    return 0;
}

int virtual_machine_resource_feedback(struct virtual_machine_resource *resource)
{
    if (resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_PRIMITIVE]->feedback != NULL)
    {
        resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_PRIMITIVE]->feedback(resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_PRIMITIVE]->pool);
    }
    if (resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_REFERENCE]->feedback != NULL)
    {
        resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_REFERENCE]->feedback(resource->sources[VIRTUAL_MACHINE_RESOURCE_SOURCE_REFERENCE]->pool);
    }
    return 0;
}

