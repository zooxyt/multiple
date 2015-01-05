/* Multiple Frontend Register
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

#ifndef _MULTIPLE_REGISTER_H_
#define _MULTIPLE_REGISTER_H_

/* Moo */
if ((ret = multiple_frontend_list_register(err, frontend_list, \
                MOO_FRONTNAME, \
                MOO_FULLNAME, \
                MOO_EXT, \
                &moo_stub_create, \
                &moo_stub_destroy, \
                &moo_stub_debug_info_set, \
                &moo_stub_optimize_set, \
                &moo_stub_tokens_print, \
                &moo_stub_reconstruct, \
                &moo_stub_irgen\
                )) != 0)
{
    goto fail;
}

/* Scheme */
if ((ret = multiple_frontend_list_register(err, frontend_list, \
                MLS_FRONTNAME, \
                MLS_FULLNAME, \
                MLS_EXT, \
                &mls_stub_create, \
                &mls_stub_destroy, \
                &mls_stub_debug_info_set, \
                NULL, \
                &mls_stub_tokens_print, \
                &mls_stub_reconstruct, \
                &mls_stub_irgen\
                )) != 0)
{
    goto fail;
}

/* Lua */
if ((ret = multiple_frontend_list_register(err, frontend_list, \
                MLUA_FRONTNAME, \
                MLUA_FULLNAME, \
                MLUA_EXT, \
                &mlua_stub_create, \
                &mlua_stub_destroy, \
                &mlua_stub_debug_info_set, \
                NULL, \
                &mlua_stub_tokens_print, \
                &mlua_stub_reconstruct, \
                &mlua_stub_irgen\
                )) != 0)
{
    goto fail;
}

/* JavaScript */
if ((ret = multiple_frontend_list_register(err, frontend_list, \
                MJS_FRONTNAME, \
                MJS_FULLNAME, \
                MJS_EXT, \
                &mjs_stub_create, \
                &mjs_stub_destroy, \
                &mjs_stub_debug_info_set, \
                NULL, \
                &mjs_stub_tokens_print, \
                &mjs_stub_reconstruct, \
                &mjs_stub_irgen\
                )) != 0)
{
    goto fail;
}

/* BrainF*ck */
if ((ret = multiple_frontend_list_register(err, frontend_list, \
                MBF_FRONTNAME, \
                MBF_FULLNAME, \
                MBF_EXT, \
                &mbf_stub_create, \
                &mbf_stub_destroy, \
                &mbf_stub_debug_info_set, \
                NULL, \
                &mbf_stub_tokens_print, \
                &mbf_stub_reconstruct, \
                &mbf_stub_irgen\
                )) != 0)
{
    goto fail;
}

/* False */
if ((ret = multiple_frontend_list_register(err, frontend_list, \
                MF_FRONTNAME, \
                MF_FULLNAME, \
                MF_EXT, \
                &mf_stub_create, \
                &mf_stub_destroy, \
                &mf_stub_debug_info_set, \
                NULL, \
                &mf_stub_tokens_print, \
                &mf_stub_reconstruct, \
                &mf_stub_irgen\
                )) != 0)
{
    goto fail;
}

/**//* Standard ML */
/*if ((ret = multiple_frontend_list_register(err, frontend_list, \*/
/*MSML_FRONTNAME, \*/
/*MSML_FULLNAME, \*/
/*MSML_EXT, \*/
/*&msml_stub_create, \*/
/*&msml_stub_destroy, \*/
/*&msml_stub_debug_info_set, \*/
/*NULL, \*/
/*&msml_stub_tokens_print, \*/
/*&msml_stub_reconstruct, \*/
/*&msml_stub_irgen\*/
/*)) != 0)*/
/*{*/
/*goto fail;*/
/*}*/

/**//* Unlambda */
/*if ((ret = multiple_frontend_list_register(err, frontend_list, \*/
/*MUNL_FRONTNAME, \*/
/*MUNL_FULLNAME, \*/
/*MUNL_EXT, \*/
/*&munl_stub_create, \*/
/*&munl_stub_destroy, \*/
/*&munl_stub_debug_info_set, \*/
/*NULL, \*/
/*&munl_stub_tokens_print, \*/
/*&munl_stub_reconstruct, \*/
/*&munl_stub_icodegen\*/
/*)) != 0)*/
/*{*/
/*goto fail;*/
/*}*/

/**//* OCaml */
/*if ((ret = multiple_frontend_list_register(err, frontend_list, \*/
/*MOCAML_FRONTNAME, \*/
/*MOCAML_FULLNAME, \*/
/*MOCAML_EXT, \*/
/*&mocaml_stub_create, \*/
/*&mocaml_stub_destroy, \*/
/*&mocaml_stub_debug_info_set, \*/
/*NULL, \*/
/*&mocaml_stub_tokens_print, \*/
/*&mocaml_stub_reconstruct, \*/
/*&mocaml_stub_icodegen\*/
/*)) != 0)*/
/*{*/
/*goto fail;*/
/*}*/

/**//* C */
/*if ((ret = multiple_frontend_list_register(err, frontend_list, \*/
/*MC_FRONTNAME, \*/
/*MC_FULLNAME, \*/
/*MC_EXT, \*/
/*&mc_stub_create, \*/
/*&mc_stub_destroy, \*/
/*&mc_stub_debug_info_set, \*/
/*NULL, \*/
/*&mc_stub_tokens_print, \*/
/*&mc_stub_reconstruct, \*/
/*&mc_stub_icodegen\*/
/*)) != 0)*/
/*{*/
/*goto fail;*/
/*}*/

#endif

