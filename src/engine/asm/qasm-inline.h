/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Daemon source code.

Daemon source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
#ifndef __ASM_INLINE_I386__
#define __ASM_INLINE_I386__

#include "../qcommon/q_platform.h"

#if idx64 || idx64_32
#  define EAX "%%rax"
#  define EBX "%%rbx"
#  define ESP "%%rsp"
#  define EDI "%%rdi"
#else
#  define EAX "%%eax"
#  define EBX "%%ebx"
#  define ESP "%%esp"
#  define EDI "%%edi"
#endif

#endif
