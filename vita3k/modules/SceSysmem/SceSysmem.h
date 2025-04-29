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

#pragma once

#include <module/module.h>
#include <kernel/types.h>

struct SceKernelPARange
{
    SceUInt32 addr;
    SceSize   size;
};

struct SceKernelPAVector // size is 0x14 on FW 0.990
{
    SceSize   size;          // Size of this structure
    SceUInt32 pRanges_size;  // Ex: 8
    SceUInt32 nDataInVector; // Must be <= 8
    SceUInt32 count;
    Ptr<SceKernelPARange> pRanges;
};

struct SceKernelAllocMemBlockOpt
{
    SceSize   size;
    SceUInt32 attr;

    // only works if SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_HAS_ALIGNMENT is set
    SceSize   alignment;

    // These two only work if SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_HAS_BASENAME is set
    SceUInt32       uidBaseBlock;
    Ptr<const char> strBaseBlockName;

    // Depending on the size, there are more members.
};

struct SceKernelAllocMemBlockOptKernel // size is 0x58 on FW 3.60
{
    SceSize   size;  // Size of this structure
    SceUInt32 unk_4; // Never used?
    SceUInt32 attr;

    Ptr<void> vbase; // Virtual address memblock should be mapped at
    Ptr<void> pbase; // Physical address to use as base

    SceSize   alignment;

    SceSize   extraLow;  // Size of extra area starting from memblock vbase
    SceSize   extraHigh; // Size of extra area starting "after" memblock - extra area is ?not mapped?

    SceUID    baseMemBlock; // UID of memblock this one is based on

    SceUID    pid;
    Ptr<SceKernelPAVector> pPAV;
    SceSize   roundupUnitSize; // Used to roundup memblock vsize
    SceUInt8  domain;

    // All of these are related to SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_HAS_0x20
    SceUInt32 unk_34; 
    SceUInt32 unk_38;
    SceUInt32 unk_3C;
    SceUInt32 unk_40;
    SceUInt32 unk_44;
    SceUInt32 unk_48;
    SceUInt32 unk_4C;
    SceUInt32 unk_50;
    SceUInt32 unk_54;
};

// Attributes to enable some SceKernelAllocMemBlockOpt members. 
static constexpr SceUInt32 SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_HAS_VBASE        = 0x00000001;
static constexpr SceUInt32 SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_HAS_PBASE        = 0x00000002;
static constexpr SceUInt32 SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_HAS_ALIGNMENT    = 0x00000004;
static constexpr SceUInt32 SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_HAS_EXTRA_LOW    = 0x00000008;
static constexpr SceUInt32 SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_HAS_EXTRA_HIGH   = 0x00000010;
static constexpr SceUInt32 SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_HAS_0x20         = 0x00000020;
static constexpr SceUInt32 SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_HAS_BASE         = 0x00000040;
static constexpr SceUInt32 SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_HAS_PID          = 0x00000080;
static constexpr SceUInt32 SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_HAS_BASENAME     = 0x00000200; // Only available for sceKernelAllocMemBlockForUser - kernel callers must use HAS_BASE instead
static constexpr SceUInt32 SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_HAS_PPAV         = 0x00001000; // Official name may be HAS_PVECTOR
static constexpr SceUInt32 SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_HAS_ROUNDUP      = 0x00002000;
static constexpr SceUInt32 SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_HAS_0x4000       = 0x00004000; // Some options when registering SceKernelBlock with guid
static constexpr SceUInt32 SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_HAS_DOMAIN       = 0x00010000;
static constexpr SceUInt32 SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_NOPHYPAGE        = 0x00020000;
static constexpr SceUInt32 SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_DONT_MAP         = 0x00040000; // Do not map the memory block in the virtual address space (unofficial name)
static constexpr SceUInt32 SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_PHYCONT	   = 0x00200000; // Memory area is physically continuous.
static constexpr SceUInt32 SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_UNK              = 0x00400000;
static constexpr SceUInt32 SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_SHARE_VBASE      = 0x00800000; // Memblock shares vbase with base block - requires HAS_BASE
static constexpr SceUInt32 SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_SHARE_PHYPAGE    = 0x01000000; // Memblock shares physical pages with base block - requires HAS_BASE
static constexpr SceUInt32 SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_ALLOW_PARTIAL_OP = 0x04000000; // Allow partial operation. ?What does that mean?

DECL_EXPORT(SceUID, sceKernelAllocMemBlock, const char *pName, SceKernelMemBlockType type, SceSize size, SceKernelAllocMemBlockOpt *optp);
DECL_EXPORT(SceUID, sceKernelFindMemBlockByAddr, Address addr, uint32_t size);
DECL_EXPORT(int, sceKernelFreeMemBlock, SceUID uid);

// We need to use things from SceSysmem, so it must be implemented here.
DECL_EXPORT(SceUID, sceKernelAllocMemBlockForDriver, const char *name, SceKernelMemBlockType type, SceSize vsize, SceKernelAllocMemBlockOptKernel *pOpt);
