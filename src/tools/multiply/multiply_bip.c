/* Multiply -- Multiple IR Generator
 * Built-in Procedure
   Copyright(C) 2013-2014 Cheryl Natsu

   This file is part of multiple - Multiple Paradigm Language Emulator

   multiple is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   multiple is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>. 
   */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "multiple_err.h"
#include "multiple_ir.h"

#include "multiply_bip.h"


struct multiply_customizable_built_in_procedure_write_back *multiply_customizable_built_in_procedure_write_back_new(uint32_t instrument_number)
{
    struct multiply_customizable_built_in_procedure_write_back *new_write_back = NULL;

    new_write_back = (struct multiply_customizable_built_in_procedure_write_back *)malloc(sizeof(struct multiply_customizable_built_in_procedure_write_back));
    if (new_write_back == NULL) { goto fail; }
    new_write_back->instrument_number = instrument_number;

fail:
    return new_write_back;
}

int multiply_customizable_built_in_procedure_write_back_destroy(struct multiply_customizable_built_in_procedure_write_back *write_back)
{
    free(write_back);
    return 0;
}

struct multiply_customizable_built_in_procedure_write_back_list *multiply_customizable_built_in_procedure_write_back_list_new(void)
{
    struct multiply_customizable_built_in_procedure_write_back_list *new_write_back_list = NULL;

    new_write_back_list = (struct multiply_customizable_built_in_procedure_write_back_list *)malloc(sizeof(struct multiply_customizable_built_in_procedure_write_back_list));
    if (new_write_back_list == NULL) { goto fail; }
    new_write_back_list->begin = new_write_back_list->end = NULL;
    new_write_back_list->size = 0;

fail:
    return new_write_back_list;
}

int multiply_customizable_built_in_procedure_write_back_list_destroy(struct multiply_customizable_built_in_procedure_write_back_list *write_back_list)
{
    struct multiply_customizable_built_in_procedure_write_back *write_back_cur, *write_back_next;

    write_back_cur = write_back_list->begin;
    while (write_back_cur != NULL)
    {
        write_back_next = write_back_cur->next;
        multiply_customizable_built_in_procedure_write_back_destroy(write_back_cur);
        write_back_cur = write_back_next;
    }
    free(write_back_list);

    return 0;
}

int multiply_customizable_built_in_procedure_write_back_list_append_with_configure(struct multiply_customizable_built_in_procedure_write_back_list *write_back_list, uint32_t instrument_number)
{
    int ret = 0;
    struct multiply_customizable_built_in_procedure_write_back *new_write_back = NULL;

    new_write_back = multiply_customizable_built_in_procedure_write_back_new(instrument_number);
    if (new_write_back == NULL) 
    { 
        ret = -MULTIPLE_ERR_MALLOC;
        goto fail; 
    }

    if (write_back_list->begin == NULL)
    {
        write_back_list->begin = write_back_list->end = new_write_back;
    }
    else
    {
        write_back_list->end->next = new_write_back;
        write_back_list->end = new_write_back;
    }
    write_back_list->size += 1;

fail:
    return ret;
}

struct multiply_customizable_built_in_procedure *multiply_customizable_built_in_procedure_new(const char *name, const size_t name_len)
{
    struct multiply_customizable_built_in_procedure *new_customizable_built_in_procedure = NULL;

    new_customizable_built_in_procedure = (struct multiply_customizable_built_in_procedure *)malloc(sizeof(struct multiply_customizable_built_in_procedure));
    if (new_customizable_built_in_procedure == NULL) { goto fail; }
    new_customizable_built_in_procedure->write_backs = NULL;
    new_customizable_built_in_procedure->name = NULL;
    new_customizable_built_in_procedure->name = (char *)malloc(sizeof(char) * (name_len + 1));
    if (new_customizable_built_in_procedure->name == NULL) { goto fail; };
    memcpy(new_customizable_built_in_procedure->name, name, name_len);
    new_customizable_built_in_procedure->name[name_len] = '\0';
    new_customizable_built_in_procedure->name_len = name_len;
    new_customizable_built_in_procedure->called = 0;
    new_customizable_built_in_procedure->instrument_number_icode = 0;
    if ((new_customizable_built_in_procedure->write_backs = multiply_customizable_built_in_procedure_write_back_list_new()) == NULL)
    { goto fail; }
    new_customizable_built_in_procedure->next = NULL;

    goto done;
fail:
    if (new_customizable_built_in_procedure != NULL)
    {
        if (new_customizable_built_in_procedure->write_backs != NULL) multiply_customizable_built_in_procedure_write_back_list_destroy(new_customizable_built_in_procedure->write_backs);
        if (new_customizable_built_in_procedure->name != NULL) free(new_customizable_built_in_procedure->name);
        free(new_customizable_built_in_procedure);
        new_customizable_built_in_procedure = NULL;
    }
done:
    return new_customizable_built_in_procedure; 
}

int multiply_customizable_built_in_procedure_destroy(struct multiply_customizable_built_in_procedure *customizable_built_in_procedure)
{
    if (customizable_built_in_procedure == NULL) return -MULTIPLE_ERR_NULL_PTR;

	if (customizable_built_in_procedure->write_backs != NULL) multiply_customizable_built_in_procedure_write_back_list_destroy(customizable_built_in_procedure->write_backs);
    if (customizable_built_in_procedure->name != NULL) free(customizable_built_in_procedure->name);
    free(customizable_built_in_procedure);

    return 0;
}

struct multiply_customizable_built_in_procedure_list *multiply_customizable_built_in_procedure_list_new(void)
{
    struct multiply_customizable_built_in_procedure_list *new_customizable_built_in_procedure_list = NULL;

    new_customizable_built_in_procedure_list = (struct multiply_customizable_built_in_procedure_list *)malloc(sizeof(struct multiply_customizable_built_in_procedure_list));
    if (new_customizable_built_in_procedure_list == NULL) { goto fail; }
    new_customizable_built_in_procedure_list->begin = new_customizable_built_in_procedure_list->end = NULL;
    new_customizable_built_in_procedure_list->customizable_built_in_procedures = NULL;

    goto done;
fail:
    if (new_customizable_built_in_procedure_list != NULL)
    {
        free(new_customizable_built_in_procedure_list);
        new_customizable_built_in_procedure_list = NULL;
    }
done:
    return new_customizable_built_in_procedure_list;
}

int multiply_customizable_built_in_procedure_list_destroy(struct multiply_customizable_built_in_procedure_list *customizable_built_in_procedure_list)
{
    struct multiply_customizable_built_in_procedure *customizable_built_in_procedure_cur, *customizable_built_in_procedure_next;

    if (customizable_built_in_procedure_list == NULL) return -MULTIPLE_ERR_NULL_PTR;

    customizable_built_in_procedure_cur = customizable_built_in_procedure_list->begin;
    while (customizable_built_in_procedure_cur != NULL)
    {
        customizable_built_in_procedure_next = customizable_built_in_procedure_cur->next;
        multiply_customizable_built_in_procedure_destroy( customizable_built_in_procedure_cur);
        customizable_built_in_procedure_cur = customizable_built_in_procedure_next;
    }

    free(customizable_built_in_procedure_list);

    return 0;
}

int multiply_customizable_built_in_procedure_list_register(struct multiply_customizable_built_in_procedure_list *list, \
        char **customizable_built_in_procedures)
{
    list->customizable_built_in_procedures = customizable_built_in_procedures;
    return 0;
}


static int multiply_customizable_built_in_procedure_list_append(struct multiply_customizable_built_in_procedure_list *customizable_built_in_procedure_list, \
        struct multiply_customizable_built_in_procedure *new_customizable_built_in_procedure)
{
    int ret = 0;

    if (customizable_built_in_procedure_list == NULL) return -MULTIPLE_ERR_NULL_PTR;
    if (new_customizable_built_in_procedure == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (customizable_built_in_procedure_list->begin == NULL)
    {
        customizable_built_in_procedure_list->begin = customizable_built_in_procedure_list->end = new_customizable_built_in_procedure;
    }
    else
    {
        customizable_built_in_procedure_list->end->next = new_customizable_built_in_procedure;
        customizable_built_in_procedure_list->end = new_customizable_built_in_procedure;
    }

    return ret;
}

struct multiply_customizable_built_in_procedure *multiply_customizable_built_in_procedure_list_lookup(struct multiply_customizable_built_in_procedure_list *customizable_built_in_procedure_list, \
        const char *name, const size_t name_len)
{
    struct multiply_customizable_built_in_procedure *customizable_built_in_procedure_cur;

    customizable_built_in_procedure_cur = customizable_built_in_procedure_list->begin;
    while (customizable_built_in_procedure_cur != NULL)
    {
        if ((customizable_built_in_procedure_cur->name_len == name_len) && \
                (strncmp(customizable_built_in_procedure_cur->name, name, name_len) == 0))
        {
            return customizable_built_in_procedure_cur;
        }
        customizable_built_in_procedure_cur = customizable_built_in_procedure_cur->next; 
    }

    return NULL;
}

static int is_customizable_built_in_procedure( \
        struct multiply_customizable_built_in_procedure_list *list, \
        const char *name, const size_t name_len)
{
    char **cur = NULL;
    char *p;

    cur = list->customizable_built_in_procedures;
    if (cur == NULL) return 0;
    while (*cur != NULL)
    {
        p = *cur;
        if ((name_len == strlen(p)) && \
                (strncmp(name, p, name_len) == 0))
        { return 1; }

        cur++;
    }

    return 0;
}

int multiply_customizable_built_in_procedure_list_called( \
        struct multiply_customizable_built_in_procedure_list *customizable_built_in_procedure_list, \
        const char *name, const size_t name_len)
{
    int ret = 0;
    struct multiply_customizable_built_in_procedure *customizable_built_in_procedure_target = NULL;
    struct multiply_customizable_built_in_procedure *new_customizable_built_in_procedure = NULL;

    if (customizable_built_in_procedure_list == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (is_customizable_built_in_procedure( \
                customizable_built_in_procedure_list, \
                name, name_len) == 0)
    { return 0; }

    customizable_built_in_procedure_target = multiply_customizable_built_in_procedure_list_lookup(customizable_built_in_procedure_list, name, name_len);
    if (customizable_built_in_procedure_target == NULL)
    {
        /* Not exists, create one */
        new_customizable_built_in_procedure = multiply_customizable_built_in_procedure_new(name, name_len);
        if (new_customizable_built_in_procedure == NULL)
        {
            ret = -MULTIPLE_ERR_MALLOC;
            goto fail;
        }
        if ((ret = multiply_customizable_built_in_procedure_list_append(customizable_built_in_procedure_list, \
                        new_customizable_built_in_procedure)) != 0)
        { goto fail; }
        customizable_built_in_procedure_target = new_customizable_built_in_procedure;
        new_customizable_built_in_procedure = NULL;
    }
    customizable_built_in_procedure_target->called = 1;
    goto done;
fail:
    if (new_customizable_built_in_procedure != NULL)
    {
        multiply_customizable_built_in_procedure_destroy(new_customizable_built_in_procedure);
        new_customizable_built_in_procedure = NULL;
    }
done:
    return ret;
}

int multiply_customizable_built_in_procedure_list_add_writeback( \
        struct multiply_customizable_built_in_procedure_list *customizable_built_in_procedure_list, \
        const char *name, const size_t name_len, uint32_t instrument_number)
{
    int ret = 0;
    struct multiply_customizable_built_in_procedure *customizable_built_in_procedure_target = NULL;
    struct multiply_customizable_built_in_procedure *new_customizable_built_in_procedure = NULL;

    if (customizable_built_in_procedure_list == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (is_customizable_built_in_procedure( \
                customizable_built_in_procedure_list, \
                name, name_len) == 0) 
    { return 0; }

    customizable_built_in_procedure_target = multiply_customizable_built_in_procedure_list_lookup(customizable_built_in_procedure_list, name, name_len);
    if (customizable_built_in_procedure_target == NULL)
    {
        /* Not exists, create one */
        new_customizable_built_in_procedure = multiply_customizable_built_in_procedure_new(name, name_len);
        if (new_customizable_built_in_procedure == NULL)
        {
            ret = -MULTIPLE_ERR_MALLOC;
            goto fail;
        }
        if ((ret = multiply_customizable_built_in_procedure_list_append(customizable_built_in_procedure_list, \
                        new_customizable_built_in_procedure)) != 0)
        { goto fail; }
        customizable_built_in_procedure_target = new_customizable_built_in_procedure;
        new_customizable_built_in_procedure = NULL;
    }
    if ((ret = multiply_customizable_built_in_procedure_write_back_list_append_with_configure( \
                    customizable_built_in_procedure_target->write_backs, instrument_number)) != 0) { goto fail; }
    goto done;
fail:
    if (new_customizable_built_in_procedure != NULL)
    {
        multiply_customizable_built_in_procedure_destroy(new_customizable_built_in_procedure);
        new_customizable_built_in_procedure = NULL;
    }
done:
    return ret;
}

