/* String Objects
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

/* A String is a chain of characters that represents the literal value
 * of a text */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "crc32.h"

#include "multiple_err.h"

#include "vm_opcode.h"
#include "vm_object_aio.h"
#include "vm_types.h"
#include "vm_gc.h"
#include "vm_err.h"

int virtual_machine_object_str_internal_marker(void *object_internal);
int virtual_machine_object_str_internal_collector(void *object_internal, int *confirm);

static struct virtual_machine_object_str_internal *virtual_machine_object_str_internal_new_with_value(struct virtual_machine *vm, \
        const char *str, const size_t len)
{
    struct virtual_machine_object_str_internal *new_object_str_internal = NULL;

    if ((new_object_str_internal = (struct virtual_machine_object_str_internal *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_object_str_internal))) == NULL)
    {
        goto fail;
    }
    new_object_str_internal->vm = vm;
    new_object_str_internal->str = NULL;

    if ((new_object_str_internal->str = (char *)virtual_machine_resource_malloc_reference(vm->resource, sizeof(char) * (len + 1))) == NULL)
    {
        goto fail;
    }
    memcpy(new_object_str_internal->str, str, len);
    new_object_str_internal->str[len] = '\0';
    new_object_str_internal->len = len;
    crc32_str(&(new_object_str_internal->checksum_crc32), (unsigned char *)new_object_str_internal->str, len);

    goto done;
fail:
    if (new_object_str_internal != NULL)
    {
        if (new_object_str_internal->str != NULL) virtual_machine_resource_free_reference(vm->resource, new_object_str_internal->str);
        virtual_machine_resource_free(vm->resource, new_object_str_internal);
        new_object_str_internal = NULL;
    }
done:
    return new_object_str_internal;
}

static int virtual_machine_object_str_internal_destroy(struct virtual_machine *vm, \
        struct virtual_machine_object_str_internal *object_str_internal)
{
    if (object_str_internal == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (object_str_internal->str != NULL) virtual_machine_resource_free_reference(vm->resource, object_str_internal->str);
    virtual_machine_resource_free(vm->resource, object_str_internal);

    return 0;
}

/*
struct virtual_machine_object_str_internal *virtual_machine_object_str_internal_clone(struct virtual_machine *vm, \
        const struct virtual_machine_object_str_internal *object_str_internal)
{
    struct virtual_machine_object_str_internal *new_object_str_internal = NULL;

    if (object_str_internal == NULL) return NULL;

    new_object_str_internal = virtual_machine_object_str_internal_new_with_value(vm, object_str_internal->str, object_str_internal->len);

    return new_object_str_internal;
}
*/

static int virtual_machine_object_str_internal_print(const struct virtual_machine_object_str_internal *object_str_internal)
{
    if (object_str_internal == NULL) return -MULTIPLE_ERR_NULL_PTR;

    fwrite(object_str_internal->str, object_str_internal->len, 1, stdout);

    return 0;
}

/* Internal Marker */
int virtual_machine_object_str_internal_marker(void *object_internal)
{
    /* There is no internal objects inside a string */
    (void)object_internal;

    return 0;
}

/* Internal Collector */
int virtual_machine_object_str_internal_collector(void *object_internal, int *confirm)
{
    struct virtual_machine_object_str_internal *object_str_internal;

    *confirm = VIRTUAL_MACHINE_GARBAGE_COLLECT_CONFIRM;

    object_str_internal = object_internal;

    virtual_machine_object_str_internal_destroy(object_str_internal->vm, object_str_internal);

    return 0;
}

/* Create a new string object with specified value */
struct virtual_machine_object *virtual_machine_object_str_new_with_value( \
        struct virtual_machine *vm, \
        const char *str, const size_t len)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_str *new_object_str = NULL;

    /* Create list object */
    if ((new_object_str = (struct virtual_machine_object_str *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_object_str))) == NULL)
    { goto fail; }
    new_object_str->ptr_internal = NULL;
    new_object_str->ptr_internal = virtual_machine_object_str_internal_new_with_value(vm, str, len);
    if (new_object_str->ptr_internal == NULL) { goto fail; }

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_STR)) == NULL)
    { goto fail; }
    if (_virtual_machine_object_ptr_set(new_object, new_object_str) != 0)
    { goto fail; }

    if (virtual_machine_resource_reference_register( \
                vm->gc_stub, \
                new_object, \
                new_object_str->ptr_internal, \
                &virtual_machine_object_str_internal_marker, \
                &virtual_machine_object_str_internal_collector) != 0)
    { goto fail; }

    new_object_str = NULL;

    return new_object;
fail:
    if (new_object_str != NULL) 
    {
        if (new_object_str->ptr_internal != NULL) virtual_machine_object_str_internal_destroy(vm, new_object_str->ptr_internal);
        virtual_machine_resource_free(vm->resource, new_object_str);
    }
    if (new_object != NULL) _virtual_machine_object_destroy(vm, new_object);

    return NULL;
}

int virtual_machine_object_str_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object)
{
    struct virtual_machine_object_str *object_str = NULL;

    if (object == NULL) return -MULTIPLE_ERR_NULL_PTR;

    object_str = (struct virtual_machine_object_str *)object->ptr;
    if (object_str != NULL)
    {
        virtual_machine_resource_free(vm->resource, object_str);
    }
    _virtual_machine_object_destroy(vm, object);

    return 0;
}

struct virtual_machine_object *virtual_machine_object_str_clone( \
        struct virtual_machine *vm, \
        const struct virtual_machine_object *object)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_str *object_str = NULL;
    struct virtual_machine_object_str *new_object_str = NULL;

    if ((object == NULL) || (object->ptr == NULL)) return NULL;

    object_str = object->ptr;

    if ((new_object_str = (struct virtual_machine_object_str *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_object_str))) == NULL)
    { goto fail; }
    new_object_str->ptr_internal = NULL;
    new_object_str->ptr_internal = object_str->ptr_internal;

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_STR)) == NULL)
    { goto fail; }
    if (_virtual_machine_object_ptr_set(new_object, new_object_str) != 0)
    { goto fail; }

    new_object_str = NULL;

    return new_object;
fail:
    return NULL;
}

int virtual_machine_object_str_print(const struct virtual_machine_object *object)
{
    struct virtual_machine_object_str *object_str;

    object_str = object->ptr;
    virtual_machine_object_str_internal_print(object_str->ptr_internal);

    return 0;
}

static int convert_str_to_int(struct virtual_machine *vm, int *value_out, char *str, size_t len)
{
    int ret = 0;
    int sign = 0;
    int value = 0;
    int base = 10;
    int digit = 0;
    char *str_p = str, *str_endp = str + len;
    char ch;
    static const int base_shift[17] = 
    { 0, 0, 1, 0, 2, 0, 0, 0, 3, 0,  0,  0,  0,  0,  0,  0,  4, };
    if ((len >= 1) && (*str_p == '-'))
    { sign = 1; str_p++; }
    if (len >= 2)
    {
        if (*str_p == '0' && (*(str_p + 1) == 'x' || *(str_p + 1) == 'X'))
        { base = 16; str_p += 2; }
        else if (*str_p == '0' && (*(str_p + 1) == 'b' || *(str_p + 1) == 'B'))
        { base = 2; str_p += 2; }
        else if (*str_p == '0')
        { base = 8; str_p += 1; }
        else
        { base = 10; }
    }
    else if (len >= 1)
    {
        if (*str_p == '0')
        { base = 8; str_p += 1; }
        else
        { base = 10; }
    }
    else
    {
        /* zero length */
        vm_err_update(vm->r, -VM_ERR_CONVERT_FAILED, \
                "runtime error: failed to convert zero-length string from \'str\' to \'int\'");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    while (str_p != str_endp)
    {
        ch = *str_p;
        switch (base)
        {
            case 2:
            case 8:
                if (ch < '0' && ch >= base)
                {
                    vm_err_update(vm->r, -VM_ERR_CONVERT_FAILED, \
                            "runtime error: failed to convert string \'%s\' from \'str\' to \'int\'\n"
                            "note: string contains invalid character \'%c\'", str, ch);
                    ret = -MULTIPLE_ERR_VM;
                    goto fail;
                }
                else
                { value = (value << base_shift[base]) | (ch - (int)('0')); }
                break;
            case 10:
                if (ch < '0' && ch >= base)
                {
                    vm_err_update(vm->r, -VM_ERR_CONVERT_FAILED, \
                            "runtime error: failed to convert string \'%s\' from \'str\' to \'int\'\n"
                            "note: string contains invalid character \'%c\'", str, ch);
                    ret = -MULTIPLE_ERR_VM;
                    goto fail;
                }
                else
                { value = (value * 10) + (ch - (int)('0')); }
                break;
            case 16:
                if (ch >= '0' && ch <= '9')
                { digit = ch - (int)('0'); }
                else if (ch >= 'a' && ch <= 'f')
                { digit = ch - (int)('a') + 10; }
                else if (ch >= 'A' && ch <= 'F')
                { digit = ch - (int)('A') + 10; }
                else
                {
                    vm_err_update(vm->r, -VM_ERR_CONVERT_FAILED, \
                            "runtime error: failed to convert string \'%s\' from \'str\' to \'int\'\n"
                            "note: string contains invalid character \'%c\'", str, ch);
                    ret = -MULTIPLE_ERR_VM;
                    goto fail;
                }
                value = value << base_shift[base] | digit;
                break;
            default:
                break;
        }
        str_p++;
    }
    if (sign != 0) value = -value;
    *value_out = value;
fail:
    return ret;
}

/* size */
int virtual_machine_object_str_size(struct virtual_machine *vm,  \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src)
{
    int ret = 0;
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_str *object_str = NULL;
    struct virtual_machine_object_str_internal *object_str_internal = NULL;

    object_str = (struct virtual_machine_object_str *)object_src->ptr;
    object_str_internal = object_str->ptr_internal;

    if ((new_object = virtual_machine_object_int_new_with_value(vm, (int)(object_str_internal->len))) == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    *object_out = new_object;
fail:
    return ret;
}

/* convert */
int virtual_machine_object_str_convert(struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        const uint32_t type)
{
    int ret = 0;
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_str *object_str = NULL;
    struct virtual_machine_object_str_internal *object_str_internal = NULL;
    int value;

    (void)vm;

    if (object_src == NULL)
    {
        VM_ERR_NULL_PTR(vm->r);
        return -MULTIPLE_ERR_VM;
    }

    *object_out = NULL;

    switch (type)
    {
        case OBJECT_TYPE_STR:
            new_object = virtual_machine_object_clone(vm, object_src);
            break;
        case OBJECT_TYPE_SYMBOL:
            object_str = ((struct virtual_machine_object_str *)(object_src->ptr));
            object_str_internal = object_str->ptr_internal; 
            new_object = virtual_machine_object_symbol_new_with_value(vm, object_str_internal->str, object_str_internal->len);
            break;
        case OBJECT_TYPE_BOOL:
            /* Anything except none and 0 convert to bool is true */
            new_object = virtual_machine_object_bool_new_with_value(vm, VIRTUAL_MACHINE_OBJECT_BOOL_VALUE_TRUE);
            break;
        case OBJECT_TYPE_INT:
            object_str = ((struct virtual_machine_object_str *)(object_src->ptr));
            object_str_internal = object_str->ptr_internal; 
            ret = convert_str_to_int(vm, &value, object_str_internal->str, object_str_internal->len);
            if (ret != 0) goto fail;
            new_object = virtual_machine_object_int_new_with_value(vm, value);
            break;
    }
    if (new_object == NULL) 
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    *object_out = new_object;
    goto done;
fail:
    if (new_object != NULL) virtual_machine_object_destroy(vm, new_object);
done:
    return ret;
}

/* add */
int virtual_machine_object_str_add(struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_left, const struct virtual_machine_object *object_right, \
        const uint32_t opcode)
{
    int ret = 0;
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_str *object_left_str = NULL, *object_right_str = NULL;
    struct virtual_machine_object_str_internal *object_left_str_internal = NULL, *object_right_str_internal = NULL;
    char *new_str = NULL;
    size_t new_str_len = 0;
    char *bad_type_name;

    (void)vm;

    if ((object_left == NULL) || (object_right == NULL)) return -MULTIPLE_ERR_NULL_PTR;

    *object_out = NULL;

    (void)opcode;

    /* Str + Str = Str */
    if (object_right->type != OBJECT_TYPE_STR)
    {
        ret = virtual_machine_object_id_to_type_name(&bad_type_name, NULL, object_right->type);
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type, "
                "\'str\' + \'%s\'", ret == 0 ? bad_type_name : "undefined type");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    /* Kernel */
    object_left_str = object_left->ptr; object_left_str_internal = object_left_str->ptr_internal;
    object_right_str = object_right->ptr; object_right_str_internal = object_right_str->ptr_internal;

    /* Construct the concatenated string */
    new_str_len = object_left_str_internal->len + object_right_str_internal->len;

    new_str = (char *)virtual_machine_resource_malloc(vm->resource, sizeof(char) * (new_str_len + 1));
    if (new_str == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    memcpy(new_str, object_left_str_internal->str, object_left_str_internal->len);
    memcpy(new_str + object_left_str_internal->len, object_right_str_internal->str, object_right_str_internal->len);
    new_str[new_str_len] = '\0';

    /* Create new object */
    new_object = virtual_machine_object_str_new_with_value(vm, new_str, new_str_len);
    if (new_object == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    *object_out = new_object;
    new_object = NULL;

fail:

    if (new_object != NULL) virtual_machine_object_str_destroy(vm, new_object);
    if (new_str != NULL) virtual_machine_resource_free(vm->resource, new_str);

    return ret;
}

/* mul */
int virtual_machine_object_str_mul(struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_left, const struct virtual_machine_object *object_right, \
        const uint32_t opcode)
{
    int ret = 0;
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_str *object_left_str;
    struct virtual_machine_object_str_internal *object_left_str_internal;
    char *new_str = NULL, *new_str_p;
    size_t new_str_len = 0;
    int loop;
    char *bad_type_name;

    (void)vm;

    if ((object_left == NULL) || (object_right == NULL)) return -MULTIPLE_ERR_NULL_PTR;

    (void)opcode;

    *object_out = NULL;

    /* Str * Int = Str */
    if (object_right->type != OBJECT_TYPE_INT)
    {
        ret = virtual_machine_object_id_to_type_name(&bad_type_name, NULL, object_right->type);
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type, "
                "\'str\' * \'%s\'", ret == 0 ? bad_type_name : "undefined type");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    object_left_str = object_left->ptr;
    object_left_str_internal = object_left_str->ptr_internal;
    if (((struct virtual_machine_object_int *)(object_right->ptr))->value < 0)
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type, "
                "\'str\' * \'negative number\'");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    /* Construct the concatenated string */
    new_str_len = object_left_str_internal->len *
        (size_t)((struct virtual_machine_object_int *)(object_right->ptr))->value;

    new_str = (char *)virtual_machine_resource_malloc(vm->resource, sizeof(char) * (new_str_len + 1));
    if (new_str == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    new_str_p = new_str;

    loop = ((struct virtual_machine_object_int *)(object_right->ptr))->value;
    while (loop-- > 0)
    {
        memcpy(new_str_p, object_left_str_internal->str, object_left_str_internal->len);
        new_str_p += object_left_str_internal->len;
    }
    *new_str_p = '\0';

    /* Create new object */
    new_object = virtual_machine_object_str_new_with_value(vm, new_str, new_str_len);
    if (new_object == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    *object_out = new_object;
    new_object = NULL;

fail:

    if (new_object != NULL) virtual_machine_object_str_destroy(vm, new_object);
    if (new_str != NULL) virtual_machine_resource_free(vm->resource, new_str);

    return ret;
}

int virtual_machine_object_str_binary_equality(struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_left, const struct virtual_machine_object *object_right, \
        const uint32_t opcode)
{
    int ret = 0;
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_str *object_str_left, *object_str_right = NULL;
    struct virtual_machine_object_str_internal *object_str_left_internal, *object_str_right_internal = NULL;

    (void)vm;

    if ((object_left == NULL) || (object_right == NULL)) return -MULTIPLE_ERR_NULL_PTR;

    *object_out = NULL;

    /* Type check */
    if (object_right->type != OBJECT_TYPE_STR)
    {
        switch (opcode)
        {
            case OP_EQ:
                new_object = virtual_machine_object_bool_new_with_value(vm, VIRTUAL_MACHINE_OBJECT_BOOL_VALUE_FALSE);
                goto finish;
            case OP_NE:
                new_object = virtual_machine_object_bool_new_with_value(vm, VIRTUAL_MACHINE_OBJECT_BOOL_VALUE_TRUE);
                goto finish;
        }
    }

    object_str_left = (struct virtual_machine_object_str *)(object_left->ptr);
    object_str_left_internal = object_str_left->ptr_internal; 
    object_str_right = (struct virtual_machine_object_str *)(object_right->ptr);
    object_str_right_internal = object_str_right->ptr_internal; 

    if ((object_str_left_internal->len != object_str_right_internal->len) || \
            (object_str_left_internal->checksum_crc32 != object_str_right_internal->checksum_crc32) || \
            (strncmp(object_str_left_internal->str, object_str_right_internal->str, object_str_left_internal->len) != 0))
    {
        switch (opcode)
        {
            case OP_EQ:
                new_object = virtual_machine_object_bool_new_with_value(vm, VIRTUAL_MACHINE_OBJECT_BOOL_VALUE_FALSE);
                goto finish;
            case OP_NE:
                new_object = virtual_machine_object_bool_new_with_value(vm, VIRTUAL_MACHINE_OBJECT_BOOL_VALUE_TRUE);
                goto finish;
        }
    }
    else
    {
        switch (opcode)
        {
            case OP_EQ:
                new_object = virtual_machine_object_bool_new_with_value(vm, VIRTUAL_MACHINE_OBJECT_BOOL_VALUE_TRUE);
                goto finish;
            case OP_NE:
                new_object = virtual_machine_object_bool_new_with_value(vm, VIRTUAL_MACHINE_OBJECT_BOOL_VALUE_FALSE);
                goto finish;
        }
    }

finish:
    if (new_object == NULL) 
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    *object_out = new_object;

fail:

    return ret;
}

/* Extract string */
int virtual_machine_object_str_extract(char **str_out, size_t *str_len_out, struct virtual_machine_object *object)
{
    struct virtual_machine_object_str *object_str;
    struct virtual_machine_object_str_internal *object_str_internal;

    if (object == NULL) return -MULTIPLE_ERR_VM;

    object_str = object->ptr;
    object_str_internal = object_str->ptr_internal;

    *str_out = object_str_internal->str;
    *str_len_out = object_str_internal->len;

    return 0;
}

