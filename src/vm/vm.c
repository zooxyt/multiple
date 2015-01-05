/* Virtual Machine
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

#include <string.h>
#include <stdarg.h>

#include "multiple.h"
#include "multiple_ir.h"
#include "multiple_err.h"
#include "multiple_misc.h"
#include "multiple_prelude.h"
#include "vm_predef.h"
#include "vm_res.h"
#include "vm_opcode.h"
#include "vm_types.h"
#include "vm_infrastructure.h"
#include "vm_object_aio.h"
#include "vm_gc.h"
#include "vm_cpu.h"
#include "vm.h"
#include "vm_dynlib.h"
#include "vm_err.h"


static int virtual_machine_export_section_new_from_ir( \
        struct multiple_error *err, \
        struct virtual_machine *vm, \
        struct multiple_ir *ir, \
        struct virtual_machine_export_section **export_section_out)
{
    int ret = 0;
    struct virtual_machine_export_section *new_export_section = NULL;
    struct multiple_ir_export_section_item *export_section_item_cur; 
    size_t index, index_arg;

    if (ir->export_section == NULL)
    { 
        multiple_error_update(err, -MULTIPLE_ERR_VM, \
                "error: export section of IR not properly generated");
        ret = -MULTIPLE_ERR_VM; 
        goto fail; 
    }

    if ((new_export_section = virtual_machine_export_section_new(vm)) == NULL) 
    { MULTIPLE_ERROR_MALLOC(); ret = -MULTIPLE_ERR_MALLOC; goto fail; }

    if ((new_export_section->exports = (struct virtual_machine_export_section_item *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_export_section_item) * (ir->export_section->size))) == NULL)
    { MULTIPLE_ERROR_MALLOC(); ret = -MULTIPLE_ERR_MALLOC; goto fail; }

    new_export_section->size = ir->export_section->size;

    /* Clean Arguments */
    for (index = 0; index != ir->export_section->size; index++)
    {
        new_export_section->exports[index].args = NULL;
    }

    /* Fill Function names and arguments */
    export_section_item_cur = ir->export_section->begin;
    for (index = 0; index != ir->export_section->size; index++)
    {
        new_export_section->exports[index].name = export_section_item_cur->name;
        new_export_section->exports[index].instrument_number = export_section_item_cur->instrument_number;
        new_export_section->exports[index].args_count = export_section_item_cur->args_count;
        if (export_section_item_cur->args_count > 0)
        {
            if ((new_export_section->exports[index].args = (uint32_t *)virtual_machine_resource_malloc(
                            vm->resource, sizeof(uint32_t) * export_section_item_cur->args_count)) == NULL)
            { MULTIPLE_ERROR_MALLOC(); ret = -MULTIPLE_ERR_MALLOC; goto fail; }
            for (index_arg = 0; index_arg != export_section_item_cur->args_count; index_arg++)
            {
                new_export_section->exports[index].args[index_arg] = export_section_item_cur->args[index_arg];
            }
        }
        else
        {
            new_export_section->exports[index].args = NULL;
        }
        export_section_item_cur = export_section_item_cur->next; 
    }

    *export_section_out = new_export_section;

    goto done;
fail:
    virtual_machine_export_section_destroy(vm, new_export_section);
done:
    return ret;
}

static int virtual_machine_import_section_load_from_ir( \
        struct multiple_error *err, \
        struct multiple_ir *ir, \
        struct virtual_machine_ir_loading_queue *loading_queue, \
        struct virtual_machine_module *module)
{
    int ret = 0;
    struct multiple_ir_import_section_item *item_cur;
    struct virtual_machine_data_section_item *data_section_item;
    uint32_t id;

    if (ir == NULL) 
    { MULTIPLE_ERROR_MALLOC(); ret = -MULTIPLE_ERR_MALLOC; goto fail; }

    if (ir->import_section == NULL) return 0;

    item_cur = ir->import_section->begin;
    while (item_cur != NULL)
    {
        id = item_cur->name;
        if (virtual_machine_module_lookup_data_section_items(module, &data_section_item, id) == LOOKUP_NOT_FOUND)
        {
            multiple_error_update(err, -MULTIPLE_ERR_VM, \
                    "error: can not find data section item which id is %d", id);
            return -MULTIPLE_ERR_VM; 
        } 
        if (data_section_item->type != MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_IDENTIFIER) 
        {
            multiple_error_update(err, -MULTIPLE_ERR_VM, \
                    "error: data section item which id is %d isn't in type identifier", id);
            return -MULTIPLE_ERR_VM; 
        }

        if ((ret = virtual_machine_ir_loading_queue_append(loading_queue, \
                        data_section_item->ptr, (size_t)data_section_item->size, \
                        ir->filename, ir->filename_len, \
                        0)) != 0)
        {
            MULTIPLE_ERROR_INTERNAL();
            ret = -MULTIPLE_ERR_INTERNAL;
            goto fail; 
        }

        item_cur = item_cur->next;
    }

    ret = 0;
fail:
    return ret;
}

static int virtual_machine_module_name_load_from_ir( \
        struct virtual_machine *vm, \
        struct multiple_error *err, \
        struct virtual_machine_module *module, \
        struct multiple_ir *ir)
{
    int ret = 0;
    uint32_t id;
    struct virtual_machine_data_section_item *data_section_item;

    if (ir == NULL) 
    {
        MULTIPLE_ERROR_NULL_PTR();
        return -MULTIPLE_ERR_NULL_PTR;
    }

    if (module->name != NULL) 
    {
        free(module->name);
        module->name = NULL;
    }
    if ((ir->module_section != NULL) && (ir->module_section->enabled != 0))
    {
        id = ir->module_section->name;
        if (virtual_machine_module_lookup_data_section_items(module, &data_section_item, id) == LOOKUP_NOT_FOUND)
        {
            multiple_error_update(err, -MULTIPLE_ERR_ATOMIC, "error: can not find data section item which id is %d", id);
            return -MULTIPLE_ERR_ATOMIC; 
        }
        if (data_section_item->type != MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_IDENTIFIER) 
        {
            multiple_error_update(err, -MULTIPLE_ERR_ATOMIC, "error: data section item which id is %d isn't in type identifier", id);
            return -MULTIPLE_ERR_ATOMIC; 
        }

        module->name_len = (size_t)data_section_item->size;
        module->name = (char *)virtual_machine_resource_malloc( \
                vm->resource, sizeof(char) * (module->name_len + 1));
        if (module->name == NULL) 
        {
            MULTIPLE_ERROR_MALLOC();
            ret = -MULTIPLE_ERR_MALLOC;
            goto fail;
        }
        memcpy(module->name, data_section_item->ptr, module->name_len);
        module->name[module->name_len] = '\0';
    }
    else
    {
        module->name_len = strlen("noname");
        module->name = (char *)virtual_machine_resource_malloc(vm->resource, sizeof(char) * (module->name_len + 1));
        if (module->name == NULL) 
        {
            MULTIPLE_ERROR_MALLOC();
            ret = -MULTIPLE_ERR_MALLOC;
            goto fail;
        }
        memcpy(module->name, "noname", module->name_len);
        module->name[module->name_len] = '\0';
    }

fail:
    return ret;
}

static int virtual_machine_data_section_new_from_ir( \
        struct multiple_error *err, \
        struct virtual_machine *vm, \
        struct multiple_ir *ir, \
        struct virtual_machine_data_section **data_section_out, \
        struct virtual_machine_module *module)
{
    int ret = 0;
    size_t index;
    struct virtual_machine_data_section *new_data_section = NULL;
    struct multiple_ir_data_section_item *data_section_item_cur;

    if (ir->data_section == NULL)
    { 
        multiple_error_update(err, -MULTIPLE_ERR_VM, \
                "error: data section of IR not properly generated");
        ret = -MULTIPLE_ERR_VM; 
        goto fail; 
    }

    if ((new_data_section = virtual_machine_data_section_new(vm, ir->data_section->size)) == NULL)
    { MULTIPLE_ERROR_MALLOC(); ret = -MULTIPLE_ERR_MALLOC; goto fail; }

    index = 0;
    data_section_item_cur = ir->data_section->begin;
    while (data_section_item_cur != NULL)
    {
        new_data_section->items[index].module = module;
        new_data_section->items[index].id = data_section_item_cur->id;
        new_data_section->items[index].type = data_section_item_cur->type;
        switch (data_section_item_cur->type)
        {
            case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_INT:
                new_data_section->items[index].size = sizeof(int);
                if ((new_data_section->items[index].ptr = (void *)virtual_machine_resource_malloc( \
                                vm->resource, sizeof(int))) == NULL)
                { MULTIPLE_ERROR_MALLOC(); ret = -MULTIPLE_ERR_MALLOC; goto fail; }
                *((int *)(new_data_section->items[index].ptr)) = data_section_item_cur->u.value_int;
                break;
            case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_BOOL:
                new_data_section->items[index].size = sizeof(int);
                if ((new_data_section->items[index].ptr = (void *)virtual_machine_resource_malloc( \
                                vm->resource, sizeof(int))) == NULL)
                { MULTIPLE_ERROR_MALLOC(); ret = -MULTIPLE_ERR_MALLOC; goto fail; }
                *((int *)(new_data_section->items[index].ptr)) = data_section_item_cur->u.value_bool;
                break;
            case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_CHAR:
                new_data_section->items[index].size = sizeof(uint32_t);
                if ((new_data_section->items[index].ptr = (void *)virtual_machine_resource_malloc( \
                                vm->resource, sizeof(uint32_t))) == NULL)
                { MULTIPLE_ERROR_MALLOC(); ret = -MULTIPLE_ERR_MALLOC; goto fail; }
                *((uint32_t *)(new_data_section->items[index].ptr)) = data_section_item_cur->u.value_char;
                break;
            case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_FLOAT:
                new_data_section->items[index].size = sizeof(double);
                if ((new_data_section->items[index].ptr = (void *)virtual_machine_resource_malloc( \
                                vm->resource, sizeof(double))) == NULL)
                { MULTIPLE_ERROR_MALLOC(); ret = -MULTIPLE_ERR_MALLOC; goto fail; }
                *((double *)(new_data_section->items[index].ptr)) = data_section_item_cur->u.value_float;
                break;
            case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_IDENTIFIER:
            case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_STR:
                new_data_section->items[index].size = data_section_item_cur->size;
                if ((new_data_section->items[index].ptr = (void *)virtual_machine_resource_malloc( \
                                vm->resource, (size_t)(data_section_item_cur->size) + 1)) == NULL)
                { MULTIPLE_ERROR_MALLOC(); ret = -MULTIPLE_ERR_MALLOC; goto fail; }
                memcpy(new_data_section->items[index].ptr, \
                        data_section_item_cur->u.value_str.str, 
                        data_section_item_cur->u.value_str.len);
                ((char *)new_data_section->items[index].ptr)[data_section_item_cur->u.value_str.len] = '\0';
                break;
            case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_NONE:
                new_data_section->items[index].size = 4;
                if ((new_data_section->items[index].ptr = (void *)virtual_machine_resource_malloc( \
                                vm->resource, sizeof(int))) == NULL)
                { MULTIPLE_ERROR_MALLOC(); ret = -MULTIPLE_ERR_MALLOC; goto fail; }
                *((int *)(new_data_section->items[index].ptr)) = 0;
                break;
            case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_NAN:
                new_data_section->items[index].size = 4;
                if ((new_data_section->items[index].ptr = (void *)virtual_machine_resource_malloc( \
                                vm->resource, sizeof(int))) == NULL)
                { MULTIPLE_ERROR_MALLOC(); ret = -MULTIPLE_ERR_MALLOC; goto fail; }
                *((int *)(new_data_section->items[index].ptr)) = data_section_item_cur->u.signed_nan;
                break;
            case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_INF:
                new_data_section->items[index].size = 4;
                if ((new_data_section->items[index].ptr = (void *)virtual_machine_resource_malloc( \
                                vm->resource, sizeof(int))) == NULL)
                { MULTIPLE_ERROR_MALLOC(); ret = -MULTIPLE_ERR_MALLOC; goto fail; }
                *((int *)(new_data_section->items[index].ptr)) = data_section_item_cur->u.signed_inf;
                break;
            case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_UNKNOWN:
                MULTIPLE_ERROR_INTERNAL(); ret = -MULTIPLE_ERR_INTERNAL; goto fail;
        }

        index++;
        data_section_item_cur = data_section_item_cur->next; 
    }

    *data_section_out = new_data_section;

    goto done;
fail:
    if (new_data_section != NULL)
    { virtual_machine_data_section_destroy(vm, new_data_section); }
done:
    return ret;
}

static int virtual_machine_text_section_new_from_ir( \
        struct multiple_error *err, \
        struct virtual_machine *vm, \
        struct multiple_ir *ir, \
        struct virtual_machine_text_section **text_section_out, \
        struct virtual_machine_module *module)
{
    int ret = 0;
    struct virtual_machine_text_section *new_virtual_machine_text_section = NULL;
    struct multiple_ir_text_section_item *text_section_item_cur = NULL;
    size_t index;
    int operand_type;
    struct virtual_machine_data_section_item *item_target;

    if (ir->text_section == NULL)
    { 
        multiple_error_update(err, -MULTIPLE_ERR_VM, \
                "error: text section of IR not properly generated");
        ret = -MULTIPLE_ERR_VM; 
        goto fail; 
    }

    if ((new_virtual_machine_text_section = virtual_machine_text_section_new(vm, ir->text_section->size)) == NULL)
    { MULTIPLE_ERROR_MALLOC(); ret = -MULTIPLE_ERR_MALLOC; goto fail; }

    index = 0;
    text_section_item_cur = ir->text_section->begin;
    while (text_section_item_cur != NULL)
    {
        new_virtual_machine_text_section->instruments[index].opcode = text_section_item_cur->opcode;
        new_virtual_machine_text_section->instruments[index].operand = text_section_item_cur->operand;
        new_virtual_machine_text_section->instruments[index].data_id = 0;

        virtual_machine_instrument_to_operand_type(&operand_type, text_section_item_cur->opcode);

        if (operand_type == OPERAND_TYPE_RES)
        {
            if (virtual_machine_module_lookup_data_section_items(module, \
                    &item_target, text_section_item_cur->operand) != LOOKUP_FOUND)
            {
                multiple_error_update(err, -MULTIPLE_ERR_VM, \
                        "error: resource %u not found", \
                        (unsigned int)text_section_item_cur->operand);
                ret = -MULTIPLE_ERR_VM; 
                goto fail;
            }
            new_virtual_machine_text_section->instruments[index].data_id = (uint32_t)(item_target - module->data_section->items);
        }

        index++;
        text_section_item_cur = text_section_item_cur->next; 
    }
    new_virtual_machine_text_section->size = ir->text_section->size;

    *text_section_out = new_virtual_machine_text_section;

    goto done;
fail:
    if (new_virtual_machine_text_section != NULL)
    { virtual_machine_text_section_destroy(vm, new_virtual_machine_text_section); }
done:
    return ret;
}


static int virtual_machine_debug_section_new_from_ir( \
        struct multiple_error *err, \
        struct virtual_machine *vm, \
        struct multiple_ir *ir, \
        struct virtual_machine_debug_section **debug_section_out)
{
    int ret = 0;
    struct virtual_machine_debug_section *new_virtual_machine_debug_section = NULL;
    struct multiple_ir_debug_section_item *debug_section_item_cur = NULL;
    size_t index;

    if ((new_virtual_machine_debug_section = virtual_machine_debug_section_new(vm)) == NULL)
    { MULTIPLE_ERROR_MALLOC(); ret = -MULTIPLE_ERR_MALLOC; goto fail; }

    new_virtual_machine_debug_section->size = ir->debug_section->size;
    if (ir->debug_section->size != 0)
    {
        new_virtual_machine_debug_section->debugs = (struct virtual_machine_debug_section_item *)virtual_machine_resource_malloc( \
                vm->resource, sizeof(struct virtual_machine_debug_section_item) * ir->debug_section->size);
        if (new_virtual_machine_debug_section->debugs == NULL) 
        { MULTIPLE_ERROR_MALLOC(); ret = -MULTIPLE_ERR_MALLOC; goto fail; }

        debug_section_item_cur = ir->debug_section->begin;
        index = 0;
        while (debug_section_item_cur != NULL)
        {
			if (index >= ir->debug_section->size)
			{
                multiple_error_update(err, -MULTIPLE_ERR_VM, \
                        "error: export section of IR not properly generated");
                ret = -MULTIPLE_ERR_VM; 
                goto fail; 
			}

            new_virtual_machine_debug_section->debugs[index].asm_ln = debug_section_item_cur->line_number_asm;
            new_virtual_machine_debug_section->debugs[index].source_ln_start = debug_section_item_cur->line_number_source_start;
            new_virtual_machine_debug_section->debugs[index].source_ln_end = debug_section_item_cur->line_number_source_end;

            debug_section_item_cur = debug_section_item_cur->next;
            index += 1;
        }
    }

    *debug_section_out = new_virtual_machine_debug_section;

    goto done;
fail:
    if (new_virtual_machine_debug_section != NULL)
    { virtual_machine_debug_section_destroy(vm, new_virtual_machine_debug_section); }
done:
    return ret;
}

static int virtual_machine_source_section_new_from_ir( \
        struct multiple_error *err, \
        struct virtual_machine *vm, \
        struct multiple_ir *ir, \
        struct virtual_machine_source_section **source_section_out)
{
    int ret = 0;
    struct virtual_machine_source_section *new_virtual_machine_source_section = NULL;

    if ((new_virtual_machine_source_section = virtual_machine_source_section_new(vm)) == NULL)
    { MULTIPLE_ERROR_MALLOC(); ret = -MULTIPLE_ERR_MALLOC; goto fail; }

    if ((ir->source_section == NULL) || (ir->source_section->size == 0)) goto finish;
    new_virtual_machine_source_section->len = ir->source_section->size;
    new_virtual_machine_source_section->code = (char *)virtual_machine_resource_malloc( \
            vm->resource, sizeof(char) * (ir->source_section->size + 1));
    if (new_virtual_machine_source_section->code == NULL) 
    { MULTIPLE_ERROR_MALLOC(); ret = -MULTIPLE_ERR_MALLOC; goto fail; }
    memcpy(new_virtual_machine_source_section->code, ir->source_section->data, ir->source_section->size);
    new_virtual_machine_source_section->code[ir->source_section->size] = '\0';

finish:
    *source_section_out = new_virtual_machine_source_section;

    goto done;
fail:
    if (new_virtual_machine_source_section != NULL) virtual_machine_source_section_destroy(vm, new_virtual_machine_source_section);
done:
    return ret;
}

/* Initialize virtual machine module with ir */
static int virtual_machine_module_load_ir( \
        struct multiple_error *err, \
        struct virtual_machine *vm, \
        struct multiple_ir *ir, \
        struct virtual_machine_module *module, \
        struct virtual_machine_ir_loading_queue *loading_queue)
{
    int ret = 0;

    if ((module == NULL) || (ir == NULL))
    { MULTIPLE_ERROR_NULL_PTR(); ret = -MULTIPLE_ERR_NULL_PTR; goto fail; }

    if ((ret = virtual_machine_data_section_new_from_ir(err, vm, ir, \
                    &module->data_section, module)) != 0)
    { goto fail; }

    if ((ret = virtual_machine_text_section_new_from_ir(err, vm, ir, \
                    &module->text_section, module)) != 0)
    { goto fail; }

    if ((ret = virtual_machine_export_section_new_from_ir(err, vm, ir, \
                    &module->export_section)) != 0)
    { goto fail; }

    if ((ret = virtual_machine_debug_section_new_from_ir(err, vm, ir, \
                    &module->debug_section)) != 0)
    { goto fail; }

    if ((ret = virtual_machine_source_section_new_from_ir(err, vm, ir, \
                    &module->source_section)) != 0)
    { goto fail; }

    if ((ret = virtual_machine_import_section_load_from_ir(err, ir, \
                    loading_queue, module)) != 0)
    { goto fail; }

    if ((ret = virtual_machine_module_name_load_from_ir(vm, err, module, ir)) != 0)
    { return ret; }

    if ((ret = virtual_machine_ir_loading_queue_append(loading_queue, \
                    module->name, module->name_len, \
                    ir->filename, ir->filename_len, 1)) != 0)
    { return ret; }

    goto done;
fail:
done:
    return ret;
}

/* Initialize virtual machine with ir */
static int virtual_machine_load_ir( \
        struct multiple_error *err, \
        struct virtual_machine *vm, \
        struct multiple_ir *ir, \
        struct virtual_machine_ir_loading_queue *loading_queue)
{
    int ret = 0;
    struct virtual_machine_module *new_module = NULL;

    if ((vm == NULL) || (ir == NULL))
    {
        MULTIPLE_ERROR_NULL_PTR();
        return -MULTIPLE_ERR_NULL_PTR;
    }

    /* The id of the new module been set to the current loaded module.
     * Should be safe currently */
    if ((new_module = virtual_machine_module_new(vm, (uint32_t)(vm->modules->size))) == NULL)
    { MULTIPLE_ERROR_MALLOC(); ret = -MULTIPLE_ERR_MALLOC; goto fail; }

    if ((ret = virtual_machine_module_load_ir(err, vm, ir, new_module, loading_queue)) != 0) goto fail;

    if ((ret = virtual_machine_module_list_append(vm->modules, new_module)) != 0) goto fail;
    ret = 0;
    goto done;
fail:
    if (new_module != NULL)
    {
        virtual_machine_module_destroy(vm, new_module);
    }
done:
    return ret;
}

static int print_white_space(FILE *fp_out, int size)
{
    if (size < 0) return 0;
    while (size-- > 0) { fputc(' ', fp_out); }
    return 0;
}

static int virtual_machine_module_run_function_debug_info(struct multiple_error *err, \
        struct virtual_machine *vm, \
        struct virtual_machine_module *module, \
        uint32_t function_instrument_number, \
        size_t args_count, \
        ...) /* Arguments passed in */
{
    int ret = 0;

    /* Debugger */
    struct virtual_machine_running_stack_frame *current_frame;
    uint32_t opcode, operand;
    char *asm_str;
    size_t asm_len;
    int written_len;

    struct virtual_machine_data_section_item *data_section_item_operand = NULL;
    int operand_type;

    (void)module;
    (void)function_instrument_number;
    (void)args_count;

    /*
    char debugger_command[DEBUGGER_COMMAND_LEN_MAX];
    size_t debugger_command_len;
    */

    /* PC */
    written_len = printf("%u", vm->tp->running_stack->top->pc); 
    print_white_space(stdout, 10 - written_len);

    current_frame = vm->tp->running_stack->top;
	opcode = vm->tp->running_stack->top->module->text_section->instruments[(size_t)current_frame->pc].opcode;
	operand = vm->tp->running_stack->top->module->text_section->instruments[(size_t)current_frame->pc].operand;

    /* Instrument */
    if ((ret = virtual_machine_opcode_to_instrument(&asm_str, &asm_len, opcode)) == 0)
    {
        /* Do nothing */
    }
    else
    {
        multiple_error_update(err, -MULTIPLE_ERR_ASM, "error: unsupported opcode %d\n", opcode);
        ret = -MULTIPLE_ERR_ASM; 
        goto fail; 
    }
    fwrite(asm_str, asm_len, 1, stdout);
    print_white_space(stdout, (int)(12 - asm_len));
    printf(" ");

    /* Operand */
    virtual_machine_instrument_to_operand_type(&operand_type, opcode);
    if (operand_type == OPERAND_TYPE_RES)
    {
        if (virtual_machine_module_lookup_data_section_items(current_frame->module, &data_section_item_operand, operand) != LOOKUP_FOUND)
        { VM_ERR_INTERNAL(vm->r); return -MULTIPLE_ERR_VM; }
        switch (data_section_item_operand->type)
        {
            case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_IDENTIFIER:
				fwrite(data_section_item_operand->ptr, (size_t)data_section_item_operand->size, 1, stdout);
                break;
            case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_STR:
                fputc('"', stdout);
                fwrite(data_section_item_operand->ptr, (size_t)data_section_item_operand->size, 1, stdout);
                fputc('"', stdout);
                break;
            case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_NONE:
                printf("none");
                break;
            case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_BOOL:
                switch (*((int *)(data_section_item_operand->ptr)))
                {
                    case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_BOOL_FALSE:
                        printf("false");
                        break;
                    case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_BOOL_TRUE:
                        printf("true");
                        break;
                }
                break;
            case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_INT:
                printf("%d", *((int *)(data_section_item_operand->ptr)));
                break;
            default:
                printf("res:%u", operand);
                break;
        }
    }
    else if (operand_type == OPERAND_TYPE_NIL)
    {
    }
    else if (operand_type == OPERAND_TYPE_NUM)
    {
        printf("%u", operand);
    }
    else
    {
        printf("res:%u", operand);
    }

    /* Module */
    /*printf("      (module:%s, thread:%u)  ", current_frame->module->name, (unsigned int)vm->tp->tid);*/

    /* Line feed */
    printf("\n");

    /*
    printf("- ");
    fflush(stdout);
    fgets(debugger_command, DEBUGGER_COMMAND_LEN_MAX, stdin);
    debugger_command_len = strlen(debugger_command) - 1;
    debugger_command[debugger_command_len] = '\0';
    */

fail:
    return ret;
}

static struct virtual_machine_external_event *virtual_machine_external_event_capture(struct virtual_machine *vm)
{
    struct virtual_machine_external_event *external_event_cur; 

    external_event_cur = vm->external_events->begin;
    while (external_event_cur != NULL)
    {
        if (external_event_cur->raised != 0) break;
        external_event_cur = external_event_cur->next; 
    }
    return external_event_cur;
}

static int virtual_machine_module_run_function(struct multiple_error *err, \
        struct virtual_machine *vm, \
        struct virtual_machine_module *module, \
        uint32_t function_instrument_number, \
        size_t args_count);

static int virtual_machine_external_event_process(struct multiple_error *err, \
        struct virtual_machine *vm)
{
    int ret = 0;
    struct virtual_machine_external_event *external_event_target; 
    struct virtual_machine_object_func *object_function = NULL;
    struct virtual_machine_object_func_internal *object_function_internal = NULL;
    struct virtual_machine_module *module_target;
    uint32_t function_instrument_number;
    struct virtual_machine_thread *new_thread = NULL;
    size_t args_count;

    (void)err;

    for (;;)
    {
        /* Lock */
        thread_mutex_lock(&vm->external_events->lock);
        external_event_target = virtual_machine_external_event_capture(vm);
        /* Unlock */
        thread_mutex_unlock(&vm->external_events->lock);

        if (external_event_target == NULL) break;

        /* Lock */
        thread_mutex_lock(&vm->external_events->lock);

        /* Turn off raised */
        external_event_target->raised -= 1;

        /* Append new thread */
        object_function = external_event_target->object_callback->ptr;
        object_function_internal = object_function->ptr_internal; 

        /* Internal Function */
        module_target = object_function_internal->module;
        function_instrument_number = object_function_internal->pc;

        /* Arguments */
        args_count = external_event_target->args->size;

        /* Append a new thread */ 
        /* TODO: This place not been protected by lock */
        if ((new_thread = virtual_machine_thread_new_with_configure(vm, \
                        module_target, function_instrument_number, \
                        args_count)) == NULL)
        {
            thread_mutex_unlock(&vm->external_events->lock);
            ret = -MULTIPLE_ERR_MALLOC; goto fail; 
        }

        ret = virtual_machine_computing_stack_transport_reversely( \
                vm, \
                new_thread->running_stack->top->arguments, \
                external_event_target->args, \
                external_event_target->args->size);
        if (ret != 0) 
        {
            thread_mutex_unlock(&vm->external_events->lock);
            goto fail; 
        }

        while (args_count > 0)
        {
            if ((ret = virtual_machine_computing_stack_pop(vm, external_event_target->args)) != 0)
            {
                thread_mutex_unlock(&vm->external_events->lock);
                goto fail;
            }
            args_count--;
        }
        if ((ret = virtual_machine_thread_list_append(vm->threads, new_thread)) != 0)
        {
            thread_mutex_unlock(&vm->external_events->lock);
            goto fail; 
        }

        /* Switch new thread as current thread */
        vm->tp = new_thread; new_thread = NULL;

        /* Unlock */
        thread_mutex_unlock(&vm->external_events->lock);
    }

fail:
    if (new_thread != NULL) virtual_machine_thread_destroy(vm, new_thread);
    return ret;
}

#define VIRTUAL_MACHINE_NEXT_THREAD_SLEEP 1
static int virtual_machine_next_thread( \
        struct virtual_machine *vm)
{
    struct virtual_machine_thread *old_thread;

    /* Prevent NULL Current Thread Pointer */
    if (vm->tp == NULL) { vm->tp = vm->threads->begin; }

    /* Only One Thread */
    if (vm->threads->size == 1) 
    {
        if ((vm->debug_mode == 0) && \
                (vm->tp->type == VIRTUAL_MACHINE_THREAD_TYPE_NORMAL)) return 0;
        else if ((vm->debug_mode == 1) && \
                (vm->tp->type == VIRTUAL_MACHINE_THREAD_TYPE_DEBUGGER)) return 0;
        return VIRTUAL_MACHINE_NEXT_THREAD_SLEEP; 
    }

    /* Multiple Threads */
    old_thread = vm->tp;
    /* Next thread */
    vm->tp = (vm->tp->next != NULL) ? vm->tp->next : vm->threads->begin;

    for (;;)
    {
        if ((vm->debug_mode == 0) && \
                (vm->tp->type == VIRTUAL_MACHINE_THREAD_TYPE_NORMAL)) return 0;
        else if ((vm->debug_mode == 1) && \
                (vm->tp->type == VIRTUAL_MACHINE_THREAD_TYPE_DEBUGGER)) return 0;
        /* Next thread */
        vm->tp = (vm->tp->next != NULL) ? vm->tp->next : vm->threads->begin;
        /* Duplicate */
        if (vm->tp == old_thread) { return VIRTUAL_MACHINE_NEXT_THREAD_SLEEP; }
    }

}

static int virtual_machine_module_run_function(struct multiple_error *err, \
        struct virtual_machine *vm, \
        struct virtual_machine_module *module, \
        uint32_t function_instrument_number, \
        size_t args_count) /* Arguments passed in */
{
    int ret = 0;
    struct virtual_machine_module *init_module;
    struct virtual_machine_thread *thread_to_kill;

    init_module = module;

    /* Create the initial thread */
    if ((ret = virtual_machine_thread_list_append_with_configure(vm, vm->threads, \
                    init_module, function_instrument_number, 
                    args_count)) != 0)
    { goto fail; }

    /* Set the current thread pointer */
    vm->tp = vm->threads->begin;

    for (;;)
    {
        /* GIL Lock */
        thread_mutex_lock(&vm->gil);

        /* External Event */
        if ((virtual_machine_external_event_process(err, vm)) != 0)
        { goto fail_and_unlock_gil; }
        if (vm_err_occurred(vm->r) != 0) 
        { goto fail_and_unlock_gil; }

        if (vm->tp->state == VIRTUAL_MACHINE_THREAD_STATE_SUSPENDED)
        {
            /* Skip suspended thread */
            do_rest();
            virtual_machine_next_thread(vm);
        }
        else if (vm->tp->state == VIRTUAL_MACHINE_THREAD_STATE_NORMAL)
        {
            /* Debug Info */
            if (vm->debug_info != 0)
            { virtual_machine_module_run_function_debug_info(err, vm, module, function_instrument_number, args_count); }

            /* Execute an instrument */
            if ((ret = virtual_machine_thread_step(err, vm)) != 0)
            { goto fail_and_unlock_gil; }
            if (vm_err_occurred(vm->r) != 0) 
            { goto fail_and_unlock_gil; }

            /* No living thread, exit */
            if (vm->tp == NULL) {
                virtual_machine_garbage_collect(vm);
                vm->tp = vm->threads->begin;
                if (vm->tp == NULL) 
                {
                    thread_mutex_unlock(&vm->gil);
                    break;
                }
            }
            /* Out of Running Stack Frame */
            if ((vm->tp != NULL) && (vm->tp->running_stack->size == 0))
            {
                thread_to_kill = vm->tp;
                /* Next Thread */
                if (virtual_machine_next_thread(vm) == VIRTUAL_MACHINE_NEXT_THREAD_SLEEP)
                { do_rest(); }
                virtual_machine_thread_list_remove(vm, vm->threads, thread_to_kill);
                if (vm->threads->size == 0)
                {
                    /* No thread remain */
                    virtual_machine_garbage_collect(vm);
                    if (vm->threads->size != 0)
                    {
                        vm->tp = vm->threads->begin;
                    }
                    else 
                    {
                        thread_mutex_unlock(&vm->gil);
                        break; 
                    }
                }
            }

            /* Locked */
            if (vm->locked != 0)
            {
                /* Next Thread */
                if (virtual_machine_next_thread(vm) == VIRTUAL_MACHINE_NEXT_THREAD_SLEEP)
                { do_rest(); }
                vm->locked = 0;
            }
            /* Our of time slice */
            else if (vm->step_in_time_slice >= vm->time_slice)
            {
                /* Next Thread */
                if (virtual_machine_next_thread(vm) == VIRTUAL_MACHINE_NEXT_THREAD_SLEEP)
                { do_rest(); }
                /* Garbage Collection */
                if ((vm->interrupt_enabled & VIRTUAL_MACHINE_IE_GC) != 0)
                {
                    virtual_machine_garbage_collect_and_feedback(vm); 
                }
                vm->step_in_time_slice = 0;
            }
            /* Running Stack Overflow */
            if ((vm->tp != NULL) && (vm->tp->running_stack->size > vm->stack_size))
            {
                if (vm->threads->size > 1)
                {
                    vm_err_update(vm->r, -VM_ERR_STACK_OVERFLOW, \
                            "runtime error: running stack of thread \'%u\' overflow", vm->tp->tid);
                }
                else
                {
                    vm_err_update(vm->r, -VM_ERR_STACK_OVERFLOW, \
                            "runtime error: running stack of thread overflow");
                }
                ret = -VM_ERR_STACK_OVERFLOW;
                goto fail_and_unlock_gil;
            }
        }

        /* GIL Unlock */
        thread_mutex_unlock(&vm->gil);
    }

    ret = 0;
    goto done;
fail_and_unlock_gil:
    thread_mutex_unlock(&vm->gil);
fail:
done:
    return ret;
}

static int virtual_machine_run_function_head_module(struct multiple_error *err, \
        struct virtual_machine *vm, \
        struct virtual_machine_module *module_main, \
        const char *function_name, const size_t function_name_len)
{
    int ret = 0;
    uint32_t function_instrument_number;

    /* Error passing */
    vm->r->opcode = 0;
    vm->r->operand = 0;
    vm->r->pc = 0;
    vm->r->module = vm->modules->begin;

    /* Initial module */
    if (module_main == NULL)
    {
        vm_err_update(vm->r, -VM_ERR_NO_AVALIABLE_MODULE, \
                "runtime error: no availiable module for initialization");
        ret = -VM_ERR_NO_AVALIABLE_MODULE;
        goto fail;
    }

    if (virtual_machine_module_function_lookup_by_name( \
                &function_instrument_number, \
                module_main, \
                function_name, function_name_len) == LOOKUP_NOT_FOUND)
    {
        vm_err_update(vm->r, -VM_ERR_JUMP_TARGET_NOT_FOUND, \
                "runtime error: function \'%s\' not found", function_name);
        ret = -VM_ERR_JUMP_TARGET_NOT_FOUND;
        goto fail;
    }

    if ((ret = virtual_machine_module_run_function(err, vm, \
                    module_main, function_instrument_number, \
                    0)) != 0) goto fail;

    ret = 0;
    goto done;
fail:
done:
    return ret;
}

static int pathname_concat(char *pathname_out, size_t pathname_out_len, \
        const char *pathname_lib, const size_t pathname_lib_len, \
        const char *pathname_module, const size_t pathname_module_len, \
        const char *pathname_ext, const size_t pathname_ext_len)
{
    char *pathname_out_p = pathname_out;

    if (pathname_lib_len + pathname_module_len + pathname_ext_len + 1 > pathname_out_len)
    { return -1; }

    pathname_out_p = pathname_out;
    memcpy(pathname_out_p, pathname_lib, pathname_lib_len);
    pathname_out_p += pathname_lib_len;
    memcpy(pathname_out_p, pathname_module, pathname_module_len);
    pathname_out_p += pathname_module_len;
    *pathname_out_p++ = '.';
    memcpy(pathname_out_p , pathname_ext, pathname_ext_len);
    pathname_out_p += pathname_ext_len;
    *pathname_out_p ++ = '\0';

    return 0;
}

static int vm_load_module(struct multiple_error *err, \
        struct multiple_stub **stub_out, \
        char *module_name, size_t module_name_len, \
        char *pathname_startup)
{
    int ret = 0;
    char lib_path[PATHNAME_LEN_MAX];
    size_t lib_path_len;
    char module_file_path[PATHNAME_LEN_MAX];
    char *pathname_startup_dir = NULL;
    size_t pathname_startup_dir_len = 0;

    struct multiple_frontend_list *new_frontend_list = NULL;
    struct multiple_frontend *frontend_cur;
    struct multiple_stub *new_stub = NULL;
    int found = 0;

    if ((ret = get_library_dir_path(err, lib_path, PATHNAME_LEN_MAX)) != 0) 
    { goto fail; }
    lib_path_len = strlen(lib_path);

    new_frontend_list = multiple_frontend_list_new();
    if (new_frontend_list == NULL) { goto fail; }

    if ((ret = multiple_frontend_list_initialize(err, new_frontend_list)) != 0)
    { goto fail; }

    /* Libraries in the same directory as startup file */ 
    if (pathname_startup != NULL)
    {
        dirname_get(&pathname_startup_dir, &pathname_startup_dir_len, pathname_startup, strlen(pathname_startup));
    }

    found = 0;
    frontend_cur = new_frontend_list->begin;
    while (frontend_cur != NULL)
    {
        if (((pathname_concat(module_file_path, PATHNAME_LEN_MAX, \
                            lib_path, lib_path_len, \
                            module_name, module_name_len, \
                            frontend_cur->frontend_name, strlen(frontend_cur->frontend_name))) == 0) && \
                (is_file_exists(module_file_path) == 1))
        {
            found = 1;
            break;
        }
        else if (((pathname_startup_dir != NULL) && ((pathname_concat(module_file_path, PATHNAME_LEN_MAX, \
                                pathname_startup_dir, pathname_startup_dir_len, \
                                module_name, module_name_len, \
                                frontend_cur->frontend_name, strlen(frontend_cur->frontend_name))) == 0)) && \
                (is_file_exists(module_file_path) == 1))
        {
            found = 1;
            break;
        }

        frontend_cur = frontend_cur->next; 
    }
    if (found == 0) 
    {
        multiple_error_update(err, -MULTIPLE_ERR_VM, "error: module \'%s\' not found \n", module_name);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    if ((ret = multiple_stub_create(err, &new_stub, NULL, MULTIPLE_IO_NULL, module_file_path, MULTIPLE_IO_PATHNAME, NULL)) != 0)
    {
        goto fail;
    }

    *stub_out = new_stub;
    ret = 0;

    goto done;
fail:
    if (new_stub != NULL) multiple_stub_destroy(new_stub);
done:
    if (new_frontend_list != NULL) multiple_frontend_list_destroy(new_frontend_list);
    return ret;
}

static int virtual_machine_destroy_without_shared_libraries(struct virtual_machine *vm)
{
    if (vm == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (vm->threads != NULL) virtual_machine_thread_list_destroy(vm, vm->threads);
    vm->threads = NULL;
    if (vm->modules != NULL) virtual_machine_module_list_destroy(vm, vm->modules);
    vm->modules = NULL;
    if (vm->variables_global != NULL) virtual_machine_variable_list_destroy(vm, vm->variables_global);
    vm->variables_global = NULL;
    if (vm->variables_builtin != NULL) virtual_machine_variable_list_destroy(vm, vm->variables_builtin);
    vm->variables_builtin = NULL;

    return 0;
}

/* Wrapped entrance routine */
int vm_run(struct multiple_error *err, \
        struct vm_err *r, \
        struct multiple_ir *ir, \
        struct virtual_machine_startup *startup, \
        int debug_info, \
        struct multiple_stub_function_list *external_functions)
{
    int ret = 0;
    struct virtual_machine *vm = NULL;
    struct virtual_machine_ir_loading_queue *loading_queue = NULL;
    struct virtual_machine_ir_loading_queue_item *loading_queue_item_unloaded = NULL;
    struct virtual_machine_module *module_cur;

    struct multiple_stub *new_stub = NULL;
    uint32_t function_instrument_number;

    uint32_t module_main_idx = 0;
    struct virtual_machine_module *module_main = NULL;

    int skip_main = 0;

    /* Create Virtual Machine */
    if ((vm = virtual_machine_new(startup, r)) == NULL) 
    {
        MULTIPLE_ERROR_MALLOC();
        ret = -MULTIPLE_ERR_MALLOC;
        goto fail;
    }
    vm->debug_info = debug_info;
    vm->keep_dll = startup->keep_dll;

    /* External Functions */
    vm->external_functions = external_functions;

    /* Loading Queue */
    if ((loading_queue = virtual_machine_ir_loading_queue_new()) == NULL)
    {
        MULTIPLE_ERROR_MALLOC();
        ret = -MULTIPLE_ERR_MALLOC;
        goto fail;
    }

    /* Set pathname */
    multiple_error_update_pathname(err, ir->filename);

    /* Record the index */
    module_main_idx = (uint32_t)(vm->modules->size);

    /* Load initial module into memory */
    if ((ret = virtual_machine_load_ir(err, vm, ir, loading_queue)) != 0)
    { goto fail; }

    while (virtual_machine_ir_loading_queue_is_unloaded(&loading_queue_item_unloaded, loading_queue) != 0)
    {
        /* Set pathname */
        multiple_error_update_pathname(err, loading_queue_item_unloaded->pathname);

        ret = vm_load_module(err, &new_stub, \
                loading_queue_item_unloaded->module_name, \
                loading_queue_item_unloaded->module_name_len, \
                ir->filename);
        if (ret != 0) goto fail;

        if ((ret = multiple_stub_icg(err, new_stub)) != 0) goto fail;

        /* Load initial module into memory */
        if ((ret = virtual_machine_load_ir(err, vm, new_stub->ir, loading_queue)) != 0)
        { goto fail; }

        if ((ret = multiple_stub_destroy(new_stub)) != 0)
        { goto fail; }
        new_stub = NULL;

        loading_queue_item_unloaded->loaded = 1;
    }

    /* Clear pathname */
    multiple_error_update_pathname(err, NULL);

    /* Run __init__ function of each module */
    module_cur = vm->modules->begin;
    while (module_cur != NULL)
    {
        /* __predefined__ */
        if (virtual_machine_module_function_lookup_by_name( \
                    &function_instrument_number, \
                    module_cur, \
                    VM_PREDEF_MODULE_CLASS_STRUCT_DEFINITION, \
                    VM_PREDEF_MODULE_CLASS_STRUCT_DEFINITION_LEN) == LOOKUP_FOUND)
        {
            ret = virtual_machine_module_run_function(err, vm, \
                    module_cur, function_instrument_number, \
                    0);
            if (ret != 0) goto fail;
            if (vm_err_occurred(vm->r) != 0) goto fail;
        }

        /* __init__ */
        if (virtual_machine_module_function_lookup_by_name( \
                    &function_instrument_number, \
                    module_cur, \
                    VM_PREDEF_MODULE_INITIALIZE, \
                    VM_PREDEF_MODULE_INITIALIZE_LEN) == LOOKUP_FOUND)
        {
            ret = virtual_machine_module_run_function(err, vm, \
                    module_cur, function_instrument_number, \
                    0);
            if (ret != 0) goto fail;
            if (vm_err_occurred(vm->r) != 0) goto fail;
        }

        module_cur = module_cur->next;
    }

    /* Run __autorun__ function of each module */
    module_cur = vm->modules->begin;
    while (module_cur != NULL)
    {
        /* __autorun__ */
        if (virtual_machine_module_function_lookup_by_name( \
                    &function_instrument_number, \
                    module_cur, \
                    VM_PREDEF_MODULE_AUTORUN, \
                    VM_PREDEF_MODULE_AUTORUN_LEN) == LOOKUP_FOUND)
        {
            ret = virtual_machine_module_run_function(err, vm, \
                    module_cur, function_instrument_number, \
                    0);
            if (ret != 0) goto fail;
            if (vm_err_occurred(vm->r) != 0) goto fail;
        }

        module_cur = module_cur->next;
    }

    /* Lookup main module */
    module_cur = vm->modules->begin;
    while ((module_cur != NULL) && (module_main_idx-- != 0))
    { module_cur = module_cur->next; }
    if (module_cur == NULL)
    {
        MULTIPLE_ERROR_INTERNAL();
        ret = -MULTIPLE_ERR_MALLOC;
        goto fail;
    }
    module_main = module_cur;

    /* main */
    if (virtual_machine_module_function_lookup_by_name( \
                &function_instrument_number, \
                module_main, \
                VM_PREDEF_MAIN_FUNCTION, \
                VM_PREDEF_MAIN_FUNCTION_LEN) == LOOKUP_NOT_FOUND)
    {
        if (virtual_machine_module_function_lookup_by_name( \
                    &function_instrument_number, \
                    module_main, \
                    VM_PREDEF_MODULE_AUTORUN, \
                    VM_PREDEF_MODULE_AUTORUN_LEN) == LOOKUP_FOUND)
        { skip_main = 1; }
    }

    /* Run the entrance function (main) */
    if (skip_main == 0)
    {

        /* A Blank module for containing module id and id for global variables 
         * has been already loaded at the moment of the creating of the virtual machine,
         * so, the first module been loaded could probably not the module which contains 
         * the main function, we pass it in manually. */

        ret = virtual_machine_run_function_head_module(err, vm, \
                module_main, \
                VM_PREDEF_MAIN_FUNCTION, VM_PREDEF_MAIN_FUNCTION_LEN);
        if (ret != 0) goto fail;
        if (vm_err_occurred(vm->r) != 0) goto fail;
    }

    ret = 0;
    goto done;
fail:
    /* Keeping the dlls for displaying error message properly */
    if (new_stub != NULL)
    {
        new_stub->startup.keep_dll = 1;
        multiple_stub_destroy(new_stub);
    }
    vm_err_final(r);
done:
    if (vm != NULL)
    {
        /* Garbage Collection */
        virtual_machine_garbage_collect(vm);

        virtual_machine_destroy_without_shared_libraries(vm);
        virtual_machine_destroy(vm, r->occurred);
        /*vm_shared_libraries_close(vm);*/
    }
    if (loading_queue != NULL) virtual_machine_ir_loading_queue_destroy(loading_queue);
    return ret;
}

