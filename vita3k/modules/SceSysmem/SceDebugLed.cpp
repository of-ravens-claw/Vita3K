// Vita3K emulator project
// Copyright (C) 2025 Vita3K team
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

#include <module/module.h>

EXPORT(int, sceDebugLedInvokeHandle0) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceDebugLedInvokeHandle1) {
    return UNIMPLEMENTED();
}

// TODO: Display these somewhere...
SceUInt32 g_DebugGPI = 0; // I think all of this is used? It's located in Debug Settings
SceUInt32 g_DebugGPO = 0; // Only the lower 8 bits are used

EXPORT(SceUInt32, sceKernelGetGPI) {
    printf("sceKernelGetGPI - %x\n", g_DebugGPI);
    return g_DebugGPI;
}

EXPORT(int, sceKernelSetGPO, SceUInt32 gpo) {
    printf("sceKernelSetGPO(%d) - %x\n", gpo, g_DebugGPO);
    g_DebugGPO = gpo;
    return 0;
}
