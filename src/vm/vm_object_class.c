/* Class Instance Objects
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
#include <string.h>
#include <stdio.h>

#include "multiple_err.h"

#include "vm_predef.h"
#include "vm_opcode.h"
#include "vm_types.h"

#include "vm_infrastructure.h"
#include "vm_object_aio.h"
#include "vm_gc.h"
#include "vm_err.h"

/* Internal */

struct virtual_machine_object_class_internal *virtual_machine_object_class_internal_new( \
        struct virtual_machine *vm, struct virtual_machine_data_type *data_type)
{
    struct virtual_machine_object_class_internal *new_object_class_internal = NULL;

    /* Create the internal part of class instance */
    new_object_class_internal = (struct virtual_machine_object_class_internal *)virtual_machine_resource_malloc( \
            vm->resource, sizeof(struct virtual_machine_object_class_internal));
    if (new_object_class_internal == NULL)
    { goto fail; }
    new_object_class_internal->properties = NULL;
    new_object_class_internal->data_type = data_type;
    new_object_class_internal->vm = vm;
    new_object_class_internal->destructed = 0;
    new_object_class_internal->destructing = 0;
    new_object_class_internal->properties = virtual_machine_variable_list_new(vm);
    if (new_object_class_internal->properties == NULL) goto fail;

    goto done;
fail:
    if (new_object_class_internal != NULL)
    {
        if (new_object_class_internal->properties != NULL)
        {
            virtual_machine_variable_list_destroy(vm, new_object_class_internal->properties);
        }
        virtual_machine_resource_free(vm->resource, new_object_class_internal);
		new_object_class_internal = NULL;
    }
done:
    return new_object_class_internal;
}

/* This function seems not easy to transform into a generic one, 
 * we use this function to manipulate the new stack frame in a 
 * new thread with 'this' variable pre-loaded */
static int virtual_machine_run_internal_function(struct virtual_machine *vm, \
        struct virtual_machine_object_class_internal *object_class_internal, 
        const char *module_name, const size_t module_name_len, \
        const char *function_name, const size_t function_name_len)
{
    int ret = 0;
    int ret_lookup;

    struct virtual_machine_module *module_target;
    uint32_t function_instrument_number;

    struct virtual_machine_object *new_object_this = NULL;
    struct virtual_machine_object_class *new_object_class_this = NULL;
    uint32_t id_this;
    struct virtual_machine_variable *new_variable_this = NULL;
    struct virtual_machine_running_stack_frame *new_running_stack_frame = NULL;
    struct virtual_machine_thread *new_thread = NULL;

    /* Lookup in all modules */
    ret_lookup = virtual_machine_module_lookup_by_name(&module_target, \
                vm, \
                module_name, module_name_len);
    if (ret_lookup == LOOKUP_NOT_FOUND)
    {
        vm_err_update(vm->r, -VM_ERR_JUMP_TARGET_NOT_FOUND, \
                "runtime error: module \'%s\' not imported", \
                module_name);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    else if (ret_lookup < 0)
    {
        ret = ret_lookup;
        goto fail;
    }

    /* Lookup instrument */
    if (virtual_machine_module_function_lookup_by_name( \
                &function_instrument_number, \
                module_target, \
                function_name, function_name_len) == LOOKUP_NOT_FOUND)
    {
        vm_err_update(vm->r, -VM_ERR_JUMP_TARGET_NOT_FOUND, \
                "runtime error: function \'%s\' not found", \
                function_name);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    /* Create 'this' object */
    if ((new_object_class_this = (struct virtual_machine_object_class *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_object_class))) == NULL)
    { goto fail; }
    new_object_class_this->ptr_internal = object_class_internal;
    if ((new_object_this = _virtual_machine_object_new(vm, OBJECT_TYPE_CLASS)) == NULL)
    { goto fail; }
    if (_virtual_machine_object_ptr_set(new_object_this, new_object_class_this) != 0)
    { goto fail; }

    new_object_class_this = NULL;

    /* Create 'this' variable */
    if (virtual_machine_module_lookup_data_section_id(&id_this, module_target, \
            VM_PREDEF_CLASS_THIS, VM_PREDEF_CLASS_THIS_LEN, MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_IDENTIFIER) == LOOKUP_NOT_FOUND)
    {
        VM_ERR_INTERNAL(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    if ((new_variable_this = virtual_machine_variable_new(vm, module_target->id, id_this)) == NULL)
    { ret = -MULTIPLE_ERR_VM; goto fail; }
    new_variable_this->ptr = new_object_this;
    new_object_this = NULL;

    /* Running Stack Frame */
    new_running_stack_frame = virtual_machine_running_stack_frame_new_with_configure(vm, \
            module_target, function_instrument_number, \
            0, /* It doesn't needs arguments as we directly put 'this' into variable */
            0, /* No closure */
            0, /* Trap PC */
            0, /* Trap Enabled */
            NULL, /* No environment */
            NULL /* No variable list */
            );
    if (new_running_stack_frame == NULL) goto fail;

    /* 'this' Variable */
    if ((ret = virtual_machine_variable_list_append(new_running_stack_frame->variables, new_variable_this)) != 0)
    { goto fail; }
    new_variable_this = NULL;

    /* New Thread */
    if ((new_thread = virtual_machine_thread_new(vm)) == NULL)
    { goto fail; }

    /* Running Stack */
    if ((ret = virtual_machine_running_stack_push(new_thread->running_stack, new_running_stack_frame)) != 0)
    { goto fail; }
    new_running_stack_frame = NULL;

    if ((ret = virtual_machine_thread_list_append(vm->threads, new_thread)) != 0)
    { goto fail; }

    /* Warning: Because the new running stack frame (Including the new thread) been created, 
     * but not been marked, in the finalizing process, then we just mark them here. */
    virtual_machine_marks_thread(new_thread, VIRTUAL_MACHINE_GARBAGE_COLLECT_MAJOR);

    new_thread = NULL;

    goto done;
fail:
    if (new_object_class_this != NULL) virtual_machine_resource_free(vm->resource, new_object_class_this);
    if (new_object_this != NULL) virtual_machine_object_class_destroy(vm, new_object_this);
    if (new_variable_this != NULL) virtual_machine_variable_destroy(vm, new_variable_this);
    if (new_running_stack_frame != NULL) virtual_machine_running_stack_frame_destroy(vm, new_running_stack_frame);
    if (new_thread != NULL) virtual_machine_thread_destroy(vm, new_thread);
done:
    return ret;
}

int virtual_machine_object_class_internal_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object_class_internal *object_class_internal, int *confirm)
{
    int ret = 0;
    struct virtual_machine_data_type_method *destructor = NULL;

	if (object_class_internal == NULL) return -MULTIPLE_ERR_NULL_PTR;
	if (confirm == NULL) return -MULTIPLE_ERR_NULL_PTR;

    /* Customize destructor */
    if ((object_class_internal->destructing != 0) && \
            (object_class_internal->destructed == 0))
    {
        /* Do nothing */
    }
    else if ((object_class_internal->data_type->methods != NULL) && \
            (object_class_internal->data_type->methods->destructor != NULL) &&
            (object_class_internal->destructed == 0) && (confirm != NULL))
    {
        /* There is customize destructor */
        destructor = object_class_internal->data_type->methods->destructor;
        if ((ret = virtual_machine_run_internal_function(vm, \
                        object_class_internal, \
                        destructor->module_name, destructor->module_name_len, \
                        destructor->def_name, destructor->def_name_len)) != 0)
        { goto fail; }
        /* Not yet finished executing customize destructor, yield at this time */
        *confirm = VIRTUAL_MACHINE_GARBAGE_COLLECT_POSTPOND;
        object_class_internal->destructing = 1;
    }
    else
    {
        /* Free properties */
        if (object_class_internal->properties != NULL)
        {
            virtual_machine_variable_list_destroy(vm, object_class_internal->properties);
        }
        virtual_machine_resource_free(vm->resource, object_class_internal);
        *confirm = VIRTUAL_MACHINE_GARBAGE_COLLECT_CONFIRM;
    }
fail:
    return ret;
}

int virtual_machine_object_class_internal_properties_set( \
        struct virtual_machine *vm,
        struct virtual_machine_object_class_internal *object_class_internal, \
        uint32_t module_id, uint32_t id, \
        struct virtual_machine_object *value)
{
    int ret = 0;

    if (object_class_internal == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if ((ret = virtual_machine_variable_list_append_with_configure(vm, object_class_internal->properties, module_id, id, value)) != 0)
    { goto fail; }

fail:
    return ret;
}


/* Confirm the execution of a destructor */
int virtual_machine_object_destructor_confirm(struct virtual_machine_object *object_this, \
        struct virtual_machine *vm)
{
    int ret = 0;
    struct virtual_machine_object_class *object_class = NULL;
    struct virtual_machine_object_class_internal *object_class_internal = NULL;

    if (object_this == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (object_this->type != OBJECT_TYPE_CLASS)
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }

    object_class = object_this->ptr;
    object_class_internal = object_class->ptr_internal;
    object_class_internal->destructed = 1;

    goto done;
fail:
done:
    return ret;
}


int virtual_machine_object_class_internal_properties_get( \
        struct virtual_machine *vm, \
        struct virtual_machine_object_class_internal *object_class_internal, \
        struct virtual_machine_object **object_value_out, \
        uint32_t module_id, uint32_t id)
{
    int ret = 0;
    struct virtual_machine_variable *var = NULL;
    struct virtual_machine_object *new_object = NULL;

    if ((ret = virtual_machine_variable_list_lookup(&var, object_class_internal->properties, module_id, id)) != LOOKUP_FOUND)
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: property \'%d:%d\' not found", module_id, id);
        goto fail; 
    }

    if ((new_object = virtual_machine_object_clone(vm, var->ptr)) == NULL)
    {
        goto fail;
    }
    *object_value_out = new_object; new_object = NULL;
    goto done;
fail:
    if (new_object != NULL)
    {
        virtual_machine_object_destroy(vm, new_object);
    }
done:
    return ret;
}


/* Internal Marker */
int virtual_machine_object_class_internal_marker(void *object_internal)
{
    struct virtual_machine_object_class_internal *object_class_internal;

    object_class_internal = object_internal;
    virtual_machine_marks_variable_list(object_class_internal->properties, VIRTUAL_MACHINE_GARBAGE_COLLECT_MAJOR);

    return 0;
}

/* Internal Collector */
int virtual_machine_object_class_internal_collector(void *object_internal, int *confirm)
{
    struct virtual_machine_object_class_internal *object_class_internal;

    object_class_internal = object_internal;
    virtual_machine_object_class_internal_destroy(object_class_internal->vm, object_class_internal, confirm);

    return 0;
}


/* Create a new class instance object with specified value */
struct virtual_machine_object *virtual_machine_object_class_new_with_value( \
        struct virtual_machine *vm, struct virtual_machine_data_type *data_type)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_class *new_object_class = NULL;

    /* Create class object */
    if ((new_object_class = (struct virtual_machine_object_class *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_object_class))) == NULL)
    { goto fail; }
    new_object_class->ptr_internal = NULL;
    new_object_class->ptr_internal = virtual_machine_object_class_internal_new(vm, data_type);
    if (new_object_class->ptr_internal == NULL) { goto fail; }

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_CLASS)) == NULL)
    { goto fail; }

    if (_virtual_machine_object_ptr_set(new_object, new_object_class) != 0)
    { goto fail; }

    if (virtual_machine_resource_reference_register( \
                vm->gc_stub, \
                new_object, \
                new_object_class->ptr_internal, \
                &virtual_machine_object_class_internal_marker, \
                &virtual_machine_object_class_internal_collector) != 0)
    { goto fail; }

    new_object_class = NULL;

    return new_object;
fail:
    if (new_object_class != NULL) 
    {
        if (new_object_class->ptr_internal != NULL) virtual_machine_object_class_internal_destroy(vm, new_object_class->ptr_internal, NULL);
        virtual_machine_resource_free(vm->resource, new_object_class);
    }
    if (new_object != NULL) _virtual_machine_object_destroy(vm, new_object);

    return NULL;
}

/* Destroy a class instance object */
int virtual_machine_object_class_destroy(struct virtual_machine *vm, \
        struct virtual_machine_object *object)
{
    struct virtual_machine_object_class *object_class = NULL;

    if (object == NULL) return -MULTIPLE_ERR_NULL_PTR;

    object_class = (struct virtual_machine_object_class *)object->ptr;
    if (object_class != NULL)
    {
        /* Just destroy the 'shell' part, not the kernel */
        /*
         * if (object_class->ptr != NULL) virtual_machine_object_class_internal_destroy(object_class->ptr);
         */
        virtual_machine_resource_free(vm->resource, object_class);
    }
    _virtual_machine_object_destroy(vm, object);

    return 0;
}

/* Clone a class instance object */
struct virtual_machine_object *virtual_machine_object_class_clone( \
        struct virtual_machine *vm, const struct virtual_machine_object *object)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_class *object_class = NULL;
    struct virtual_machine_object_class *new_object_class = NULL;

    if ((object == NULL) || (object->ptr == NULL)) return NULL;

    object_class = object->ptr;

    if ((new_object_class = (struct virtual_machine_object_class *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_object_class))) == NULL)
    { goto fail; }
    new_object_class->ptr_internal = NULL;
    new_object_class->ptr_internal = object_class->ptr_internal;

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_CLASS)) == NULL)
    { goto fail; }
    if (_virtual_machine_object_ptr_set(new_object, new_object_class) != 0)
    { goto fail; }
    new_object_class = NULL;

    return new_object;
fail:
    return NULL;
}

/* print */
int virtual_machine_object_class_print(const struct virtual_machine_object *object)
{
    (void)object;
    return 0;
}

/* Methods Invoke */
int virtual_machine_object_class_method(struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        const struct virtual_machine_object *object_method, \
        const struct virtual_machine_object *object_args, unsigned int object_args_count, \
        struct virtual_machine *vm)
{
    int ret = 0;
    struct virtual_machine_object_class *object_class = NULL;
    struct virtual_machine_data_type *data_type_target;
    struct virtual_machine_data_type_method *data_type_method_target;

    char *method_id;
    size_t method_id_len;

    *object_out = NULL;

    if (object_src == NULL) return -MULTIPLE_ERR_NULL_PTR;
    if (object_src->type != OBJECT_TYPE_CLASS)
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }

    (void)object_args;
    (void)object_args_count;

    if (object_method == NULL) return -MULTIPLE_ERR_NULL_PTR;
    if (object_method->type != OBJECT_TYPE_IDENTIFIER)
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }

    /* Get method name */
    if ((ret = virtual_machine_object_identifier_get_value(object_method, &method_id, &method_id_len)) != 0)
    { goto fail; }

    /* Lookup the specified class */
    object_class = (struct virtual_machine_object_class *)(object_src->ptr);
    data_type_target = ((struct virtual_machine_object_class_internal *)(object_class->ptr_internal))->data_type;

    if (virtual_machine_data_type_method_list_lookup(data_type_target->methods, &data_type_method_target, method_id, method_id_len) == LOOKUP_NOT_FOUND)
    {
        vm_err_update(vm->r, -VM_ERR_NO_SUCH_METHOD, \
                "runtime error: no method \'%s\' for \'%s\'", method_id, data_type_target->name);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    if ((*object_out = virtual_machine_object_identifier_new_with_value( \
                    vm, \
                    data_type_method_target->def_name, data_type_method_target->def_name_len, \
                    data_type_method_target->def_name_module_id, data_type_method_target->def_name_data_id, \
                    data_type_method_target->module_name, data_type_method_target->module_name_len, \
                    data_type_method_target->module_name_module_id, data_type_method_target->module_name_data_id)) == NULL)
    { goto fail; }

    goto done;
fail:
done:
    return ret;
}

/* Methods for built-in types */
int virtual_machine_object_class_built_in_types_method(struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        const struct virtual_machine_object *object_method, \
        const struct virtual_machine_object *object_args, unsigned int object_args_count, \
        struct virtual_machine *vm)
{
    int ret = 0;
    struct virtual_machine_data_type *data_type_target;
    struct virtual_machine_data_type_method *data_type_method_target;

    char *built_in_type_name;
    size_t built_in_type_name_len;

    char *method_id;
    size_t method_len;

    *object_out = NULL;

    if (object_src == NULL) return -MULTIPLE_ERR_NULL_PTR;
    if (object_src->type == OBJECT_TYPE_CLASS)
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }

    (void)object_args;
    (void)object_args_count;

    if (object_method == NULL) return -MULTIPLE_ERR_NULL_PTR;
    if (object_method->type != OBJECT_TYPE_IDENTIFIER)
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }

    /* Get method name */
    if ((ret = virtual_machine_object_identifier_get_value(object_method, &method_id, &method_len)) != 0)
    { goto fail; }

    /* Lookup the specified data_type */
    if ((virtual_machine_object_id_to_type_name(&built_in_type_name, &built_in_type_name_len, object_src->type)) != 0)
    {
        VM_ERR_INTERNAL(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }

    data_type_target = vm->data_types->begin;
    while (data_type_target != NULL)
    {
        if ((data_type_target->name_len == built_in_type_name_len) && \
                (strncmp(built_in_type_name, data_type_target->name, built_in_type_name_len) == 0))
        { break; }
        data_type_target = data_type_target->next;
    }
    if (data_type_target == NULL)
    {
        vm_err_update(vm->r, -VM_ERR_NO_METHOD, \
                "runtime error: object in type \'%s\' doesn't have methods", built_in_type_name);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    if (virtual_machine_data_type_method_list_lookup(data_type_target->methods, &data_type_method_target, method_id, method_len) == LOOKUP_NOT_FOUND)
    {
        vm_err_update(vm->r, -VM_ERR_NO_SUCH_METHOD, \
                "runtime error: no method \'%s\' for \'%s\'", method_id, data_type_target->name);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }


    if ((*object_out = virtual_machine_object_identifier_new_with_value( \
                    vm, \
                    data_type_method_target->def_name, data_type_method_target->def_name_len, \
                    data_type_method_target->def_name_module_id, data_type_method_target->def_name_data_id, \
                    data_type_method_target->module_name, data_type_method_target->module_name_len, \
                    data_type_method_target->module_name_module_id, data_type_method_target->module_name_data_id)) == NULL)
    { goto fail; }

    goto done;
fail:
done:
    return ret;
}

/* Property Get */
int virtual_machine_object_class_property_get(struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        const struct virtual_machine_object *object_property, \
        struct virtual_machine *vm)
{
    int ret = 0;
    struct virtual_machine_variable *variable_target = NULL;
    struct virtual_machine_object_class *object_class;
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_identifier *object_identifier_property;

    struct virtual_machine_object *object_src_solved = NULL;

    if (object_src == NULL) return -MULTIPLE_ERR_NULL_PTR;
    if (object_property == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if ((ret = virtual_machine_variable_solve(&object_src_solved, (struct virtual_machine_object *)object_src, NULL, 1, vm)) != 0)
    { goto fail; }

    if (object_src_solved->type != OBJECT_TYPE_CLASS)
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }

    if (object_property->type != OBJECT_TYPE_IDENTIFIER)
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }

    object_identifier_property = object_property->ptr;
    object_class = object_src_solved->ptr;

    if (virtual_machine_variable_list_lookup(&variable_target, object_class->ptr_internal->properties, \
                object_identifier_property->id_module_id,
                object_identifier_property->id_data_id) == LOOKUP_NOT_FOUND)
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: property \'%s\' not found", \
                ((struct virtual_machine_object_identifier *)(object_property->ptr))->id);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    if ((new_object = virtual_machine_object_clone(vm, variable_target->ptr)) == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        ret = MULTIPLE_ERR_VM;
        goto fail;
    }
    *object_out = new_object; new_object = NULL;

    goto done;
fail:
    if (new_object != NULL) virtual_machine_object_destroy(vm, new_object);
done:
    if (object_src_solved != NULL) virtual_machine_object_destroy(vm, object_src_solved);
    return ret;
}

/* Property Set */
int virtual_machine_object_class_property_set(struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        const struct virtual_machine_object *object_property, \
        const struct virtual_machine_object *object_value, \
        struct virtual_machine *vm)
{
    int ret = 0;
    struct virtual_machine_object_class *object_class;
    struct virtual_machine_object_identifier *object_identifier_property;

    struct virtual_machine_object *object_src_solved = NULL;
    struct virtual_machine_object *object_value_solved = NULL;

    if (object_src == NULL) return -MULTIPLE_ERR_NULL_PTR;
    if (object_property == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if ((ret = virtual_machine_variable_solve(&object_src_solved, (struct virtual_machine_object *)object_src, NULL, 1, vm)) != 0)
    { goto fail; }
    if ((ret = virtual_machine_variable_solve(&object_value_solved, (struct virtual_machine_object *)object_value, NULL, 1, vm)) != 0)
    { goto fail; }

    if (object_src_solved->type != OBJECT_TYPE_CLASS)
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }

    if (object_property->type != OBJECT_TYPE_IDENTIFIER)
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }

    object_identifier_property = object_property->ptr;
    object_class = object_src_solved->ptr;

    ret = virtual_machine_variable_list_update_with_configure(vm, \
            object_class->ptr_internal->properties, \
            object_identifier_property->id_module_id, 
            object_identifier_property->id_data_id, 
            object_value_solved);
    if (ret != 0)
    {
        goto fail;
    }

    *object_out = object_src_solved; object_src_solved = NULL;

    goto done;
fail:
done:
    if (object_src_solved != NULL) virtual_machine_object_destroy(vm, object_src_solved);
    if (object_value_solved != NULL) virtual_machine_object_destroy(vm, object_value_solved);
    return ret;
}

