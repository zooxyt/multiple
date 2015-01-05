/* Virtual Machine CPU : Interrupt
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

#ifndef _VM_CPU_INT_H_
#define _VM_CPU_INT_H_

enum 
{
    VM_INT_ARGS = 1,
    VM_INT_GC = 2,
    VM_INT_EE_ALLOC = 3, /* Allocate External Event Number */
    VM_INT_EE_INSTALL = 4, /* Install External Event */
    VM_INT_EE_UNINSTALL = 5, /* Uninstall External Event */
    VM_INT_EE_WAIT = 6, /* Wait for External Event */
    VM_INT_DBG_START = 7, /* Start Debugging */
};

int virtual_machine_interrupt(struct virtual_machine *vm);

#endif 


