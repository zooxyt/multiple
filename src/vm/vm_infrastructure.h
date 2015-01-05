/* Infrastructure of Virtual Machine
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

#ifndef _VM_INFRASTRUCTURE_H_
#define _VM_INFRASTRUCTURE_H_

#include <stdint.h>
#include <stdarg.h>

#include "multiple_ir.h"
#include "multiple_tunnel.h"
#include "vm_res.h"
#include "gc.h"

#include "crc32.h"
#include "spinlock.h"

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

struct vm_err;

/* Default Values */

#define TIME_SLICE_DEFAULT 100


/* Computing Stack */

struct virtual_machine_computing_stack
{
    struct virtual_machine_object *bottom; /* begin */
    struct virtual_machine_object *top; /* end */
    size_t size;
};

struct virtual_machine_computing_stack *virtual_machine_computing_stack_new(struct virtual_machine *vm);
int virtual_machine_computing_stack_destroy(struct virtual_machine *vm, struct virtual_machine_computing_stack *stack);
int virtual_machine_computing_stack_clear(struct virtual_machine *vm, struct virtual_machine_computing_stack *stack);
int virtual_machine_computing_stack_push(struct virtual_machine_computing_stack *stack, struct virtual_machine_object *object);
int virtual_machine_computing_stack_pop(struct virtual_machine *vm, struct virtual_machine_computing_stack *stack);

int virtual_machine_computing_stack_transport(struct virtual_machine *vm, \
        struct virtual_machine_computing_stack *stack_dst, \
        struct virtual_machine_computing_stack *stack_src, \
        size_t size);
int virtual_machine_computing_stack_transport_reversely(struct virtual_machine *vm, \
        struct virtual_machine_computing_stack *stack_dst, \
        struct virtual_machine_computing_stack *stack_src, \
        size_t size);


/* Variable & Variable List */

struct virtual_machine_variable
{
    uint32_t module_id; /* module name */
    uint32_t id; /* variable name */

    struct virtual_machine_object *ptr; /* object body */

    struct virtual_machine_variable *prev;
    struct virtual_machine_variable *next;
};

struct virtual_machine_variable *virtual_machine_variable_new(struct virtual_machine *vm, uint32_t module_id, uint32_t id);
int virtual_machine_variable_destroy(struct virtual_machine *vm, struct virtual_machine_variable *variable);
int virtual_machine_variable_ptr_set(struct virtual_machine_variable *variable, struct virtual_machine_object *ptr);
struct virtual_machine_variable *virtual_machine_variable_clone(struct virtual_machine *vm, struct virtual_machine_variable *variable);

struct virtual_machine_variable *virtual_machine_variable_new_with_configure(struct virtual_machine *vm, \
        uint32_t module_id, uint32_t id, struct virtual_machine_object *object_ptr);


struct virtual_machine_variable_list
{
    struct virtual_machine_variable *begin;
    struct virtual_machine_variable *end;
    size_t size;
};

struct virtual_machine_variable_list *virtual_machine_variable_list_new(struct virtual_machine *vm);
int virtual_machine_variable_list_destroy(struct virtual_machine *vm, struct virtual_machine_variable_list *list);
int virtual_machine_variable_list_append(struct virtual_machine_variable_list *list, struct virtual_machine_variable *variable);
int virtual_machine_variable_list_remove(struct virtual_machine *vm, struct virtual_machine_variable_list *list, struct virtual_machine_variable *variable);
int virtual_machine_variable_list_append_with_configure(struct virtual_machine *vm, struct virtual_machine_variable_list *list, uint32_t module_id, uint32_t id, struct virtual_machine_object *object_ptr);

int virtual_machine_variable_list_lookup(struct virtual_machine_variable **variable_out, \
        struct virtual_machine_variable_list *list, uint32_t module_id, uint32_t id);
struct virtual_machine_running_stack;
int  virtual_machine_variable_list_lookup_from_environment_entrance( \
        struct virtual_machine *vm, \
        struct virtual_machine_variable **variable_out, \
        struct virtual_machine_variable_list **variable_list_out, \
        struct virtual_machine_object *object_environment_entrance, \
        uint32_t module_id, uint32_t id, int one_frame_limit);
int virtual_machine_variable_list_update_with_configure(struct virtual_machine *vm, struct virtual_machine_variable_list *list, uint32_t module_id, uint32_t id, struct virtual_machine_object *object_ptr);

struct virtual_machine_variable_list *virtual_machine_variable_list_clone(struct virtual_machine *vm, \
        struct virtual_machine_variable_list *variable_list);


/* Environment Stack Frame */

struct virtual_machine_environment_stack_frame
{
    /* Variables on stack */
    struct virtual_machine_variable_list *variables;

    /* Computing Stack */
    struct virtual_machine_computing_stack *computing_stack; 

    /* Environment Entrance */
    struct virtual_machine_object *environment_entrance;

    /* Program Counter */
    uint32_t pc; 
    struct virtual_machine_module *module;
    size_t args_count;
    int closure;
    uint32_t trap_pc;
    int trap_enabled;

    struct virtual_machine_environment_stack_frame *next;
    struct virtual_machine_environment_stack_frame *prev;
};
struct virtual_machine_environment_stack_frame *virtual_machine_environment_stack_frame_new_blank( \
        struct virtual_machine *vm);
struct virtual_machine_environment_stack_frame *virtual_machine_environment_stack_frame_new_with_configure( \
        struct virtual_machine *vm, \
        struct virtual_machine_variable_list *variables, \
        struct virtual_machine_computing_stack *computing_stack, \
        struct virtual_machine_object *environment_entrance, \
        uint32_t pc, \
        struct virtual_machine_module *module, \
        size_t args_count, \
        int closure, \
        uint32_t trap_pc, \
        int trap_enabled);
int virtual_machine_environment_stack_frame_destroy(struct virtual_machine *vm, \
        struct virtual_machine_environment_stack_frame *environment_stack_frame);
struct virtual_machine_environment_stack_frame *virtual_machine_environment_stack_frame_clone(struct virtual_machine *vm, \
        struct virtual_machine_environment_stack_frame *environment_stack_frame);

/* Environment Stack */

struct virtual_machine_environment_stack
{
    struct virtual_machine_environment_stack_frame *begin;
    struct virtual_machine_environment_stack_frame *end;
    size_t size;
};
struct virtual_machine_environment_stack *virtual_machine_environment_stack_new(struct virtual_machine *vm);
int virtual_machine_environment_stack_destroy(struct virtual_machine *vm, \
        struct virtual_machine_environment_stack *environment_stack);
int virtual_machine_environment_stack_append(struct virtual_machine_environment_stack *environment_stack, \
        struct virtual_machine_environment_stack_frame *new_environment_stack_frame);
struct virtual_machine_environment_stack *virtual_machine_environment_stack_clone(struct virtual_machine *vm, \
        struct virtual_machine_environment_stack *environment_stack);


/* Running Stack */

/* Mask is used for redirecting accesses to the variables 
 * with the same name */
struct virtual_machine_mask_stack_frame
{
    struct virtual_machine_variable_list *variables;
    struct virtual_machine_mask_stack_frame *next;
    struct virtual_machine_mask_stack_frame *prev;
};
struct virtual_machine_mask_stack_frame *virtual_machine_mask_stack_frame_new(struct virtual_machine *vm);
int virtual_machine_mask_stack_frame_destroy(struct virtual_machine *vm, \
        struct virtual_machine_mask_stack_frame *mask_stack_frame);

struct virtual_machine_mask_stack
{
    struct virtual_machine_mask_stack_frame *top;
    struct virtual_machine_mask_stack_frame *bottom;
    size_t size;
};
struct virtual_machine_mask_stack *virtual_machine_mask_stack_new(struct virtual_machine *vm);
int virtual_machine_mask_stack_destroy(struct virtual_machine *vm, \
        struct virtual_machine_mask_stack *mask_stack);
int virtual_machine_mask_stack_push(struct virtual_machine_mask_stack *mask_stack, \
        struct virtual_machine_mask_stack_frame *new_mask_stack_frame);
int virtual_machine_mask_stack_pop(struct virtual_machine *vm, \
        struct virtual_machine_mask_stack *mask_stack);
int virtual_machine_mask_stack_push_blank(struct virtual_machine *vm, \
        struct virtual_machine_mask_stack *mask_stack);

struct virtual_machine_running_stack_frame
{
    /* Current Module */
    struct virtual_machine_module *module;

    /* Place to hold intermediate data of computing */
    struct virtual_machine_computing_stack *computing_stack; 

    /* Place to hold pre-pushed arguments */
    struct virtual_machine_computing_stack *arguments; 

    /* Argument Count */
    size_t args_count; 

    /* Variables on stack */
    struct virtual_machine_variable_list *variables;

    /* Initialize PC (for identify the same frame that yield before)*/
    uint32_t pc_start;

    /* Program Counter */
    uint32_t pc; 

    /* Generators */
    struct virtual_machine_running_stack_frame_list *generators;

    /* Closure (Access Upper Frame) */
    int closure;

    /* Trap */
    uint32_t trap_pc;
    int trap_enabled;

    /* Environment (Created by Closure)*/
    struct virtual_machine_object *environment_entrance;

    struct virtual_machine_running_stack_frame *next;
    struct virtual_machine_running_stack_frame *prev;
};

struct virtual_machine_running_stack_frame *virtual_machine_running_stack_frame_new(struct virtual_machine *vm);
int virtual_machine_running_stack_frame_destroy(struct virtual_machine *vm, struct virtual_machine_running_stack_frame *stack_frame);

struct virtual_machine_running_stack_frame *virtual_machine_running_stack_frame_new_with_configure(struct virtual_machine *vm, \
        struct virtual_machine_module *module, uint32_t pc, \
        size_t args_count, int closure, uint32_t trap_pc, int trap_enabled, \
        struct virtual_machine_object *environment, \
        struct virtual_machine_variable_list *variables);
struct virtual_machine_running_stack_frame *virtual_machine_running_stack_frame_get_generator( \
        struct virtual_machine_running_stack_frame *current_frame, \
        struct virtual_machine_module *module, \
        uint32_t generator_pc);


struct virtual_machine_running_stack_frame_list
{
    struct virtual_machine_running_stack_frame *begin; 
    struct virtual_machine_running_stack_frame *end; 
    size_t size;
};

struct virtual_machine_running_stack_frame_list *virtual_machine_running_stack_frame_list_new(struct virtual_machine *vm);
int virtual_machine_running_stack_frame_list_destroy(struct virtual_machine *vm, \
        struct virtual_machine_running_stack_frame_list *list);
int virtual_machine_running_stack_frame_list_append( \
        struct virtual_machine_running_stack_frame_list *list, \
        struct virtual_machine_running_stack_frame *new_frame);


struct virtual_machine_running_stack
{
    struct virtual_machine_running_stack_frame *bottom; 
    struct virtual_machine_running_stack_frame *top; 
    size_t size;
};

struct virtual_machine_running_stack *virtual_machine_running_stack_new(struct virtual_machine *vm);
int virtual_machine_running_stack_destroy(struct virtual_machine *vm, \
        struct virtual_machine_running_stack *stack);
int virtual_machine_running_stack_push( \
        struct virtual_machine_running_stack *stack, \
        struct virtual_machine_running_stack_frame *new_frame);
int virtual_machine_running_stack_pop(struct virtual_machine *vm, \
        struct virtual_machine_running_stack *stack);

int virtual_machine_running_stack_push_with_configure(struct virtual_machine *vm, \
        struct virtual_machine_running_stack *stack, \
        struct virtual_machine_module *module, uint32_t pc, \
        size_t args_count, int closure, \
        uint32_t trap_pc, int trap_enabled, \
        struct virtual_machine_object *environment_entrance, \
        struct virtual_machine_variable_list *variables);
int virtual_machine_running_stack_lift_merge(struct virtual_machine *vm, \
        struct virtual_machine_running_stack *stack);


/* Message */

struct virtual_machine_message
{
    struct virtual_machine_object *object;
    uint32_t tid_from;

    struct virtual_machine_message *prev;
    struct virtual_machine_message *next;
};

struct virtual_machine_message *virtual_machine_message_new(struct virtual_machine *vm);
int virtual_machine_message_destroy(struct virtual_machine *vm, \
        struct virtual_machine_message *message);

struct virtual_machine_message_queue
{
    struct virtual_machine_message *begin;
    struct virtual_machine_message *end;

    size_t size;
};

struct virtual_machine_message_queue *virtual_machine_message_queue_new(struct virtual_machine *vm);
int virtual_machine_message_queue_destroy(struct virtual_machine *vm, struct virtual_machine_message_queue *queue);
int virtual_machine_message_queue_push(struct virtual_machine_message_queue *queue, struct virtual_machine_message *new_message);
int virtual_machine_message_queue_push_with_configure(struct virtual_machine *vm, \
        struct virtual_machine_message_queue *queue, \
        struct virtual_machine_object *new_object, uint32_t tid);
int virtual_machine_message_queue_pop( \
        struct virtual_machine_message_queue *queue, struct virtual_machine_message **message_out);


/* Threading */

enum 
{
    VIRTUAL_MACHINE_THREAD_STATE_NORMAL = 0, 
    VIRTUAL_MACHINE_THREAD_STATE_SUSPENDED = 1, 
};
enum 
{
    VIRTUAL_MACHINE_THREAD_TYPE_NORMAL = 0, 
    VIRTUAL_MACHINE_THREAD_TYPE_DEBUGGER = 1, 
};
struct continuation_list;
struct virtual_machine_thread
{
    /* Running Stack */
    struct virtual_machine_running_stack *running_stack;

    /* Mailbox */
    struct virtual_machine_message_queue *messages;

	/* Thread ID */
	uint32_t tid;

    /* Continuation List */
    struct continuation_list *continuations;

    int state;
    int type;
    int zombie; /* for marking suicide */

    struct virtual_machine_thread *next;
    struct virtual_machine_thread *prev;
};

struct virtual_machine_thread *virtual_machine_thread_new(struct virtual_machine *vm);
int virtual_machine_thread_destroy(struct virtual_machine *vm, \
        struct virtual_machine_thread *thread);
struct virtual_machine_thread *virtual_machine_thread_clone(struct virtual_machine *vm, \
        struct virtual_machine_thread *thread);

struct virtual_machine_thread *virtual_machine_thread_new_with_configure(struct virtual_machine *vm, \
        struct virtual_machine_module *module, uint32_t pc, \
        size_t args_count);


struct virtual_machine_thread_list
{
    uint32_t tid_pool;

    struct virtual_machine_thread *begin;
    struct virtual_machine_thread *end;
    size_t size;
};

struct virtual_machine_thread_list *virtual_machine_thread_list_new(struct virtual_machine *vm);
int virtual_machine_thread_list_destroy(struct virtual_machine *vm, struct virtual_machine_thread_list *list);
int virtual_machine_thread_list_print(struct virtual_machine *vm);
int virtual_machine_thread_list_clear(struct virtual_machine *vm, struct virtual_machine_thread_list *list);
int virtual_machine_thread_list_append(struct virtual_machine_thread_list *list, struct virtual_machine_thread *new_thread);
int virtual_machine_thread_list_remove(struct virtual_machine *vm, struct virtual_machine_thread_list *list, struct virtual_machine_thread *thread);
int virtual_machine_thread_list_remove_by_tid(struct virtual_machine *vm, struct virtual_machine_thread_list *list, uint32_t tid);
int virtual_machine_thread_list_set_state_by_tid(struct virtual_machine *vm, \
        struct virtual_machine_thread_list *list, uint32_t tid, int state);
int virtual_machine_thread_list_is_tid_exists(struct virtual_machine_thread_list *list, uint32_t tid);

int virtual_machine_thread_list_append_with_configure(struct virtual_machine *vm, \
        struct virtual_machine_thread_list *list, \
        struct virtual_machine_module *module, uint32_t pc, \
        size_t args_count);


/* Mutex */

#define VIRTUAL_MACHINE_MUTEX_LOCKED 0
#define VIRTUAL_MACHINE_MUTEX_UNLOCKED 1
struct virtual_machine_mutex
{
    uint32_t id;
    int state;

    struct virtual_machine_mutex *prev;
    struct virtual_machine_mutex *next;
};
struct virtual_machine_mutex *virtual_machine_mutex_new(struct virtual_machine *vm);
int virtual_machine_mutex_destroy(struct virtual_machine *vm, struct virtual_machine_mutex *mutex);

struct virtual_machine_mutex_list
{
    uint32_t mutex_id_pool;

    struct virtual_machine_mutex *begin;
    struct virtual_machine_mutex *end;
    size_t size;
};
struct virtual_machine_mutex_list *virtual_machine_mutex_list_new(struct virtual_machine *vm);
int virtual_machine_mutex_list_destroy(struct virtual_machine *vm, struct virtual_machine_mutex_list *list);
int virtual_machine_mutex_list_append(struct virtual_machine_mutex_list *list, \
        struct virtual_machine_mutex *new_mutex);
int virtual_machine_mutex_list_remove_by_id( \
        struct virtual_machine *vm, \
        struct virtual_machine_mutex_list *list, \
        uint32_t id);
int virtual_machine_mutex_list_state_by_id( \
        struct virtual_machine_mutex_list *list, \
        uint32_t id);
int virtual_machine_mutex_list_lock_by_id( \
        struct virtual_machine_mutex_list *list, \
        uint32_t id);
int virtual_machine_mutex_list_unlock_by_id( \
        struct virtual_machine_mutex_list *list, \
        uint32_t id);


/* Semaphore */

#define VIRTUAL_MACHINE_SEMAPHORE_TRY_BUSY (-1)
#define VIRTUAL_MACHINE_SEMAPHORE_NO_FOUND (-2) 

#define VIRTUAL_MACHINE_SEMAPHORE_CHANNEL_AVALIABLE 1 
#define VIRTUAL_MACHINE_SEMAPHORE_CHANNEL_UNAVALIABLE 0

enum
{
    VIRTUAL_MACHINE_SEMAPHORE_THREAD_QUEUE_ITEM_TYPE_UNKNOWN, 
    VIRTUAL_MACHINE_SEMAPHORE_THREAD_QUEUE_ITEM_TYPE_VM, 
    VIRTUAL_MACHINE_SEMAPHORE_THREAD_QUEUE_ITEM_TYPE_NATIVE
};

struct virtual_machine_semaphore_thread_queue_item
{
    int type;
    void *ptr_thread;
    struct virtual_machine_semaphore_thread_queue_item *next;
};
struct virtual_machine_semaphore_thread_queue_item *
virtual_machine_semaphore_thread_queue_item_new( \
        struct virtual_machine *vm, \
        int type, \
        void *ptr_thread);
int virtual_machine_semaphore_thread_queue_item_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_semaphore_thread_queue_item *item);

struct virtual_machine_semaphore_thread_queue
{
    struct virtual_machine_semaphore_thread_queue_item *begin;
    struct virtual_machine_semaphore_thread_queue_item *end;
    size_t size;
};
struct virtual_machine_semaphore_thread_queue *virtual_machine_semaphore_thread_queue_new(struct virtual_machine *vm);
int virtual_machine_semaphore_thread_queue_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_semaphore_thread_queue *queue);

int virtual_machine_semaphore_thread_queue_push( \
        struct virtual_machine *vm, \
        struct virtual_machine_semaphore_thread_queue *queue, \
        void *ptr_thread);
int virtual_machine_semaphore_thread_queue_push_native( \
        struct virtual_machine *vm, \
        struct virtual_machine_semaphore_thread_queue *queue, \
        int *channel);
int virtual_machine_semaphore_thread_queue_head_type( \
        struct virtual_machine_semaphore_thread_queue *queue);
void *virtual_machine_semaphore_thread_queue_pop( \
        struct virtual_machine *vm, \
        struct virtual_machine_semaphore_thread_queue *queue);
int *virtual_machine_semaphore_thread_queue_pop_native( \
        struct virtual_machine *vm, \
        struct virtual_machine_semaphore_thread_queue *queue);

struct virtual_machine_semaphore
{
    uint32_t id;
    volatile int value;
    struct virtual_machine_semaphore_thread_queue *suspended_threads;

    struct virtual_machine_semaphore *prev;
    struct virtual_machine_semaphore *next;
};
struct virtual_machine_semaphore *virtual_machine_semaphore_new(struct virtual_machine *vm, int value);
int virtual_machine_semaphore_destroy(struct virtual_machine *vm, struct virtual_machine_semaphore *semaphore);

struct virtual_machine_semaphore_list
{
    uint32_t semaphore_id_pool;
    int confirm_terminated;
    mutex_t lock;

    struct virtual_machine_semaphore *begin;
    struct virtual_machine_semaphore *end;
    size_t size;
};
struct virtual_machine_semaphore_list *virtual_machine_semaphore_list_new(struct virtual_machine *vm);
int virtual_machine_semaphore_list_destroy(struct virtual_machine *vm, struct virtual_machine_semaphore_list *list);
int virtual_machine_semaphore_list_append(struct virtual_machine_semaphore_list *list, \
        struct virtual_machine_semaphore *new_semaphore);
int virtual_machine_semaphore_list_remove_by_id( \
        struct virtual_machine *vm, \
        struct virtual_machine_semaphore_list *list, \
        uint32_t id);

/* Get Semaphore Value */
int virtual_machine_semaphore_list_value( \
        struct virtual_machine_semaphore_list *list, \
        uint32_t id);

/* P Operation (By Virtual Machine) 
 * If resource avaliable perform P operation, 
 * or just return VIRTUAL_MACHINE_SEMAPHORE_TRY_BUSY */
int virtual_machine_semaphore_list_try_p( \
        struct virtual_machine *vm, \
        struct virtual_machine_thread *vm_thread, \
        struct virtual_machine_semaphore_list *list, \
        uint32_t id);

/* P Operation (By Native) */
int virtual_machine_semaphore_list_p( \
        struct virtual_machine *vm, \
        struct virtual_machine_semaphore_list *list, \
        uint32_t id);

/* V Operation (By Virtual Machine) */
int virtual_machine_semaphore_list_v( \
        struct virtual_machine *vm, \
        struct virtual_machine_semaphore_list *list, \
        uint32_t id);


/* Virtual Machine */

/* .data Section */
struct virtual_machine_data_section_item
{
    uint32_t id;
    enum multiple_ir_data_section_item_type type;
    uint32_t size;
    void *ptr;
    struct virtual_machine_module *module;
};

struct virtual_machine_data_section
{
    struct virtual_machine_data_section_item *items;
    size_t size;
};

struct virtual_machine_data_section *virtual_machine_data_section_new(struct virtual_machine *vm, size_t size);
int virtual_machine_data_section_destroy(struct virtual_machine *vm, struct virtual_machine_data_section *data_section);


/* .text Section */
struct virtual_machine_text_section_instrument
{
    uint32_t opcode;
    uint32_t operand;

    uint32_t data_id;
};

struct virtual_machine_text_section
{
    struct virtual_machine_text_section_instrument *instruments;
    size_t size;
};

struct virtual_machine_text_section *virtual_machine_text_section_new(struct virtual_machine *vm, size_t size);
int virtual_machine_text_section_destroy(struct virtual_machine *vm, struct virtual_machine_text_section *text_section);


/* .export Section */
struct virtual_machine_export_section_item
{
	uint32_t instrument_number;
	uint32_t name;
	uint32_t *args;
	size_t args_count;
};

struct virtual_machine_export_section
{
    struct virtual_machine_export_section_item *exports;
    size_t size;
};

struct virtual_machine_export_section *virtual_machine_export_section_new(struct virtual_machine *vm);
int virtual_machine_export_section_destroy(struct virtual_machine *vm, struct virtual_machine_export_section *export_section);


/* .debug Section */
struct virtual_machine_debug_section_item
{
    uint32_t asm_ln;
    uint32_t source_ln_start;
    uint32_t source_ln_end;
};

struct virtual_machine_debug_section
{
    struct virtual_machine_debug_section_item *debugs;
    size_t size;
};

struct virtual_machine_debug_section *virtual_machine_debug_section_new(struct virtual_machine *vm);
int virtual_machine_debug_section_destroy(struct virtual_machine *vm, struct virtual_machine_debug_section *debug_section);


/* .source Section */
struct virtual_machine_source_section
{
    char *code;
    size_t len;
};
struct virtual_machine_source_section *virtual_machine_source_section_new(struct virtual_machine *vm);
int virtual_machine_source_section_destroy(struct virtual_machine *vm, struct virtual_machine_source_section *source_section);


#define TIME_SLICE_DEFAULT 100
#define STACK_SIZE_DEFAULT 100

struct virtual_machine_data_type;
struct virtual_machine_data_type_list;

/* Virtual Machine Module */
struct virtual_machine_module
{
    uint32_t id; /* Dynamically assigned with a unique number */

    /* Data Types */
    struct virtual_machine_data_type_list *data_types;

    /* Program data, should not been touched after loaded */
    struct virtual_machine_data_section *data_section;
    struct virtual_machine_text_section *text_section;
    struct virtual_machine_export_section *export_section;
    struct virtual_machine_debug_section *debug_section;
    struct virtual_machine_source_section *source_section;

    /* Name */
    char *name;
    size_t name_len;

    /* Module Variable */
    struct virtual_machine_variable_list *variables;

    struct virtual_machine_module *next;
};
struct virtual_machine_module *virtual_machine_module_new(struct virtual_machine *vm, uint32_t id);
int virtual_machine_module_destroy(struct virtual_machine *vm, struct virtual_machine_module *module);

/* Virtual Machine Module List */
struct virtual_machine_module_list
{
    struct virtual_machine_module *begin;
    struct virtual_machine_module *end;

    size_t size;
};
struct virtual_machine_module_list *virtual_machine_module_list_new(struct virtual_machine *vm);
int virtual_machine_module_list_destroy(struct virtual_machine *vm, struct virtual_machine_module_list *list);
int virtual_machine_module_list_append(struct virtual_machine_module_list *list, struct virtual_machine_module *new_module);

/* Virtual Machine Shared Library */
struct virtual_machine_shared_library
{
    char *name;
    size_t len;
    void *handle;

    struct virtual_machine_shared_library *next;
};
struct virtual_machine_shared_library *virtual_machine_shared_library_new(struct virtual_machine *vm);
int virtual_machine_shared_library_destroy(struct virtual_machine *vm, struct virtual_machine_shared_library *shared_library);
struct virtual_machine_shared_library *virtual_machine_shared_library_new_with_configure(struct virtual_machine *vm, char *name, size_t len, void *handle);

struct virtual_machine_shared_library_list
{
    struct virtual_machine_shared_library *begin;
    struct virtual_machine_shared_library *end;

    size_t size;
};

struct virtual_machine_shared_library_list *virtual_machine_shared_library_list_new(struct virtual_machine *vm);
int virtual_machine_shared_library_list_destroy(struct virtual_machine *vm, struct virtual_machine_shared_library_list *list);
int virtual_machine_shared_library_list_append(struct virtual_machine_shared_library_list *list, struct virtual_machine_shared_library *new_lib);
int virtual_machine_shared_library_list_lookup(struct virtual_machine_shared_library_list *list, void **handle_out, char *name, size_t len);


/* External Event */
struct virtual_machine_external_event
{
    uint32_t id;
	int raised;
	
	/* Call back function */
	struct virtual_machine_object *object_callback;
	/* Arguments passed in */
    struct virtual_machine_computing_stack *args;

    struct virtual_machine_external_event *next;
};
struct virtual_machine_external_event *virtual_machine_external_event_new(struct virtual_machine *vm, \
        struct virtual_machine_object *object_callback, uint32_t id);
int virtual_machine_external_event_destroy(struct virtual_machine *vm, \
        struct virtual_machine_external_event *external_event);

struct virtual_machine_external_event_list
{
    struct virtual_machine_external_event *begin;
    struct virtual_machine_external_event *end;

    uint32_t id; /* id pool */
    mutex_t lock;
};
struct virtual_machine_external_event_list *virtual_machine_external_event_list_new(struct virtual_machine *vm);
int virtual_machine_external_event_list_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_external_event_list *external_event_list);
int virtual_machine_external_event_list_append( \
        struct virtual_machine_external_event_list *external_event_list, \
        struct virtual_machine_external_event *new_external_event);
uint32_t virtual_machine_external_event_list_get_id( \
        struct virtual_machine_external_event_list *event_list);
struct virtual_machine_external_event *virtual_machine_external_event_list_lookup( \
        struct virtual_machine *vm, \
        struct virtual_machine_external_event_list *external_event_list, uint32_t id);
int virtual_machine_external_event_list_raise( \
        struct virtual_machine *vm, \
        struct virtual_machine_external_event_list *external_event_list, uint32_t id);
int virtual_machine_external_event_list_count( \
        struct virtual_machine *vm, \
        struct virtual_machine_external_event_list *external_event_list, uint32_t id, size_t *count);


/* Debugger */
/* Storing the break-point of a step for debugging */
struct virtual_machine_debugger
{
    /* Debugger */
    struct virtual_machine_thread *tp_debugger;

    /* Target */
    struct virtual_machine_thread *tp_target;
};
struct virtual_machine_debugger *virtual_machine_debugger_new(void);
int virtual_machine_debugger_destroy(struct virtual_machine_debugger *debugger);


/* Virtual Machine */
struct virtual_machine
{
    /* Resource */
    struct virtual_machine_resource *resource;

    /* Garbage Collection */
    gc_stub_t *gc_stub;

    /* Modules */
    struct virtual_machine_module_list *modules;

    /* Shared Libraries */
    struct virtual_machine_shared_library_list *shared_libraries;

    /* Threads in Virtual Machine */
    struct virtual_machine_thread_list *threads;

    /* Global Data Types */
    /* Registered data types (including native and class) */
    struct virtual_machine_data_type_list *data_types;

    /* Thread pointer, current thread */
    struct virtual_machine_thread *tp;

    /* Debugger */
    struct virtual_machine_debugger *debugger;

    /* Settings */
    /* Instrument number for every time executing 
     * After hitting the limit, switch to the next thread */
    size_t time_slice; 
    size_t step_in_time_slice;

    size_t stack_size; /* Maximum number of running stack frames */

    /* Virtual Machine Runtime Error */
    /* Note: This is just a reference to a real instant of error handler 
     * because the error information will be kept after 
     * virtual machine being destroy */
    struct vm_err *r;

    /* Global Variables */
    struct virtual_machine_variable_list *variables_global;

    /* Built-in variables */
    struct virtual_machine_variable_list *variables_builtin;

    /* External Functions */
    struct multiple_stub_function_list *external_functions;

    /* External Events */
    struct virtual_machine_external_event_list *external_events;
    int keep_dll;

    /* Mutexes */
    struct virtual_machine_mutex_list *mutexes;
    /* Semaphores */
    struct virtual_machine_semaphore_list *semaphores;
    int locked;

    /* Enable Debug Mode */
    int debug_mode;

    /* Print Brief Debug Info */
    int debug_info;

    /* Global Interpreter Lock */
    mutex_t gil;

    uint32_t interrupt_enabled;
};

struct virtual_machine *virtual_machine_new(struct virtual_machine_startup *startup, struct vm_err *r);
int virtual_machine_destroy(struct virtual_machine *vm, int error_occurred);

/* Interrupt Enabled */
#define VIRTUAL_MACHINE_IE_GC (1U<<0)
#define VIRTUAL_MACHINE_IE_ALL (VIRTUAL_MACHINE_IE_GC)
#define VIRTUAL_MACHINE_IE_NONE ~(VIRTUAL_MACHINE_IE_GC)

int virtual_machine_interrupt_init(struct virtual_machine *vm);
int virtual_machine_interrupt_gc_enable(struct virtual_machine *vm);
int virtual_machine_interrupt_gc_disable(struct virtual_machine *vm);

/* GIL */
int virtual_machine_gil_lock(struct virtual_machine *vm);
int virtual_machine_gil_unlock(struct virtual_machine *vm);

/* Utilities */

#define LOOKUP_FOUND 1
#define LOOKUP_NOT_FOUND 0
int virtual_machine_module_lookup_data_section_items(struct virtual_machine_module *module, struct virtual_machine_data_section_item **item_out, uint32_t id);
int virtual_machine_module_lookup_data_section_id(uint32_t *id_out, struct virtual_machine_module *module, const char *name, const size_t len, const uint32_t type);
int virtual_machine_module_lookup_by_name(struct virtual_machine_module **module_out, \
        struct virtual_machine *vm, \
        const char *module_name, const size_t module_name_len);
int virtual_machine_module_lookup_by_id(struct virtual_machine_module **module_out, \
        struct virtual_machine *vm, uint32_t id);

int virtual_machine_module_function_lookup_by_id(uint32_t *function_instrument_number_out, struct virtual_machine_module *module, const uint32_t function_id);
int virtual_machine_module_function_lookup_by_name(uint32_t *function_instrument_number_out, \
        struct virtual_machine_module *module, \
        const char *function_name, const size_t function_name_len);

int virtual_machine_function_lookup_external_function( \
        struct virtual_machine *vm, \
        int (**func_out)(struct multiple_stub_function_args *args), \
        struct multiple_stub_function_args **args_out, \
        char *function_name, size_t function_name_len);


/* Solve variables into specific values */
int virtual_machine_variable_solve( \
        struct virtual_machine_object **object_dst, \
        const struct virtual_machine_object *object_src, \
        const struct virtual_machine_running_stack_frame *target_frame, \
        const int marking_error, /* While solving failed, runtime error happens */ \
        struct virtual_machine *vm);


/* Loading queue 
 * For recording modules required by modules while loading in virtual machine */

struct virtual_machine_ir_loading_queue_item
{
    char *module_name;
    size_t module_name_len;
    char *pathname;
    size_t pathname_len;
    int loaded;

    struct virtual_machine_ir_loading_queue_item *next;
};

struct virtual_machine_ir_loading_queue_item *icode_loading_queue_item_new(void);
int icode_loading_queue_item_destroy(struct virtual_machine_ir_loading_queue_item *item);
struct virtual_machine_ir_loading_queue_item *icode_loading_queue_item_new_with_configure( \
        char *module_name, size_t module_name_len, char *pathname, size_t pathname_len);

struct virtual_machine_ir_loading_queue
{
    struct virtual_machine_ir_loading_queue_item *begin;
    struct virtual_machine_ir_loading_queue_item *end;
    size_t size;
};

struct virtual_machine_ir_loading_queue *virtual_machine_ir_loading_queue_new(void);
int virtual_machine_ir_loading_queue_destroy(struct virtual_machine_ir_loading_queue *queue);
int virtual_machine_ir_loading_queue_append(struct virtual_machine_ir_loading_queue *queue, \
        char *module_name, size_t module_name_len, \
        char *pathname, size_t pathname_len, int loaded);
int icode_loading_queue_is_exists(struct virtual_machine_ir_loading_queue *queue, \
        char *module_name, size_t module_name_len);
int virtual_machine_ir_loading_queue_is_unloaded(struct virtual_machine_ir_loading_queue_item **queue_item_out, struct virtual_machine_ir_loading_queue *queue);


/* Data Type Method */
#define VIRTUAL_MACHINE_DATA_TYPE_METHOD_TYPE_NORMAL 0
#define VIRTUAL_MACHINE_DATA_TYPE_METHOD_TYPE_CONSTRUCTOR 1
#define VIRTUAL_MACHINE_DATA_TYPE_METHOD_TYPE_DESTRUCTOR 2
struct virtual_machine_data_type_method
{
    /* Method Name */ 
    char *method_name;
    size_t method_name_len;

    /* Where the method function locates */
    char *def_name;
    size_t def_name_len;
    uint32_t def_name_module_id;
    uint32_t def_name_data_id;

    char *module_name;
    size_t module_name_len;
    uint32_t module_name_data_id;
    uint32_t module_name_module_id;

    int method_type;

    struct virtual_machine_data_type_method *next;
};
struct virtual_machine_data_type_method *virtual_machine_data_type_method_new(\
        struct virtual_machine *vm, \
        const char *method_name, const size_t method_name_len, \
        const char *def_name, const size_t def_name_len, const uint32_t def_name_module_id, const uint32_t def_name_data_id, \
        const char *module_name, const size_t module_name_len, const uint32_t module_name_module_id, const uint32_t module_name_data_id, \
        int method_type);
int virtual_machine_data_type_method_destroy(struct virtual_machine *vm, struct virtual_machine_data_type_method *method);

struct virtual_machine_data_type_method_list
{
    struct virtual_machine_data_type_method *begin;
    struct virtual_machine_data_type_method *end;
    size_t size;

    /* Special Methods */
    struct virtual_machine_data_type_method *constructor;
    struct virtual_machine_data_type_method *destructor;
};
struct virtual_machine_data_type_method_list *virtual_machine_data_type_method_list_new(struct virtual_machine *vm);
int virtual_machine_data_type_method_list_destroy(struct virtual_machine *vm, \
        struct virtual_machine_data_type_method_list *list);
int virtual_machine_data_type_method_list_append( \
        struct virtual_machine_data_type_method_list *list, \
        struct virtual_machine_data_type_method *new_method);
int virtual_machine_data_type_method_list_lookup( \
        struct virtual_machine_data_type_method_list *list, \
        struct virtual_machine_data_type_method **method_out, const char *method_name, const size_t method_name_len);


/* Customize Data Type */
struct virtual_machine_data_type
{
    char *name;
    size_t name_len;

    /* Registered methods */
    struct virtual_machine_data_type_method_list *methods;

    /* Native,
     * means, written in 'native' programming language and
     * working via binary interface */
    int native;

    /* Working with Native Data Types */
    int (*func_destroy)(const void *ptr);
    void *(*func_clone)(const void *ptr);
    int (*func_print)(const void *ptr);
    int (*func_eq)(const void *ptr_left, const void *ptr_right);

    struct virtual_machine_data_type *next;
};

struct virtual_machine_data_type *virtual_machine_data_type_new( \
        struct virtual_machine *vm, \
        const char *name, const size_t len);
int virtual_machine_data_type_set_native( \
        struct virtual_machine_data_type *data_type, int native);
int virtual_machine_data_type_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_data_type *data_type);

struct virtual_machine_data_type_list
{
    struct virtual_machine_data_type *begin;
    struct virtual_machine_data_type *end;

    size_t size;
};

struct virtual_machine_data_type_list *virtual_machine_data_type_list_new(struct virtual_machine *vm);
int virtual_machine_data_type_list_destroy(struct virtual_machine *vm, \
        struct virtual_machine_data_type_list *list);
int virtual_machine_data_type_list_append( \
        struct virtual_machine_data_type_list *list, \
        struct virtual_machine_data_type *new_data_type);
int virtual_machine_data_type_list_append_with_configure(struct virtual_machine *vm, \
        struct virtual_machine_data_type_list *list, \
        const char *name, const size_t len);
int virtual_machine_data_type_list_lookup( \
        struct virtual_machine_data_type **data_type_out, \
        struct virtual_machine_data_type_list *data_type_list, \
        const char *name, const size_t name_len);

int virtual_machine_data_type_list_method_add_with_configure(struct virtual_machine *vm, \
        struct virtual_machine_data_type_list *list, \
        const char *method_name, const size_t method_name_len, \
        const char *data_type_name, const size_t data_type_len, \
        const char *def_name, const size_t def_name_len, const uint32_t def_name_module_id, const uint32_t def_name_data_id, \
        const char *module_name, const size_t module_name_len, const uint32_t module_name_module_id, const uint32_t module_name_data_id, \
        int method_type);


/* Marks */
int virtual_machine_marks_object(struct virtual_machine_object *object, int type);
int virtual_machine_marks_object(struct virtual_machine_object *object, int type);
int virtual_machine_marks_variable_list(struct virtual_machine_variable_list *list, int type);
int virtual_machine_marks_computing_stack(struct virtual_machine_computing_stack *computing_stack, int type);
int virtual_machine_marks_environment_stack_frame(struct virtual_machine_environment_stack_frame *environment_stack_frame, int type);
int virtual_machine_marks_environment_stack(struct virtual_machine_environment_stack *environment, int type);
int virtual_machine_marks_thread(struct virtual_machine_thread *thread, int type);

/* Garbage Collect */
#define VIRTUAL_MACHINE_GARBAGE_COLLECT_CONFIRM (GC_GARBAGE_COLLECT_CONFIRM)
#define VIRTUAL_MACHINE_GARBAGE_COLLECT_POSTPOND (GC_GARBAGE_COLLECT_POSTPOND)
int virtual_machine_garbage_collect(struct virtual_machine *vm);
int virtual_machine_garbage_collect_minor(struct virtual_machine *vm);

#endif

