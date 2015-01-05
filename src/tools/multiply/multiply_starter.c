/* Multiply -- Multiple IR Generator
 * Starter - A Backend Framework for Multiple
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

/* Multiple */
#include "multiple_err.h"
#include "multiple_ir.h"

/* Low-level things */
#include "multiply.h"
/* Floating Code Block */
#include "multiply_fcb.h"
/* Built-in Procedure */
#include "multiply_bip.h"
/* Offset */
#include "multiply_offset.h"
/* Labeled Offset */
#include "multiply_lbl_offset.h"
/* Assembler */
#include "multiply_assembler.h"

/* Starter */
#include "multiply_starter.h"


multiply_starter_t *multiply_starter_new(multiply_starter_feature_t feature)
{
    multiply_starter_t *new_starter = NULL;

    new_starter = (multiply_starter_t *)malloc(sizeof(multiply_starter_t));
    if (new_starter == NULL)
    { goto fail; }
    new_starter->res_id = NULL;
    new_starter->bip_list = NULL;
    new_starter->fcb_list = NULL;
    new_starter->map_offset_label_list = NULL;
    new_starter->offset_item_pack_stack = NULL;

    if ((new_starter->res_id = multiply_resource_id_pool_new()) == NULL)
    { goto fail; }
    if ((new_starter->fcb_list = multiply_fcb_block_list_new()) == NULL)
    { goto fail; }

    /* Build in Procedure */
    if (feature & MULTIPLY_STARTER_FEATURE_BIP)
    {
        if ((new_starter->bip_list = multiply_customizable_built_in_procedure_list_new()) == NULL)
        { goto fail; }
    }
    /* Break offset pack */
    if (feature & MULTIPLY_STARTER_FEATURE_BREAK)
    {
        /* Do nothing */
    }
    /* Labeled offset pack */
    if (feature & MULTIPLY_STARTER_FEATURE_LABEL)
    {
        if ((new_starter->offset_item_pack_stack = multiply_offset_item_pack_stack_new()) == NULL)
        { goto fail; }
        if ((new_starter->map_offset_label_list = multiply_map_offset_label_list_new()) == NULL)
        { goto fail; }
    }

    goto done;
fail:
    if (new_starter != NULL)
    {
        multiply_starter_destroy(new_starter);
        new_starter = NULL;
    }
done:
    return new_starter;
}

int multiply_starter_destroy(multiply_starter_t *starter)
{
    if (starter->res_id != NULL) 
    { multiply_resource_id_pool_destroy(starter->res_id); }
    if (starter->bip_list != NULL)
    { multiply_customizable_built_in_procedure_list_destroy(starter->bip_list); }
    if (starter->fcb_list != NULL)
    { multiply_fcb_block_list_destroy(starter->fcb_list); }
    if (starter->map_offset_label_list != NULL)
    { multiply_map_offset_label_list_destroy(starter->map_offset_label_list); }
    if (starter->offset_item_pack_stack != NULL)
    { multiply_offset_item_pack_stack_destroy(starter->offset_item_pack_stack); }
    free(starter);

    return 0;
}

int multiply_starter_generate_ir( \
        struct multiple_error *err, \
        struct multiple_ir **ir_out, \
        multiply_starter_t *starter)
{
    int ret = 0;
    struct multiple_ir *new_ir = NULL;

    if ((new_ir = multiple_ir_new()) == NULL)
    { MULTIPLE_ERROR_MALLOC(); ret = -MULTIPLE_ERR_MALLOC; goto fail; }

    (void)starter;

    *ir_out = new_ir;

    goto done;
fail:
    if (new_ir != NULL)
    {
        multiple_ir_destroy(new_ir);
    }
done:
    return ret;
}

