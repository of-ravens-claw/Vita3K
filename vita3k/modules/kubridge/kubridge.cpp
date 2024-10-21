#include <module/module.h>

#include <kernel/types.h>
#include <kernel/state.h>
#include <cpu/functions.h>
#include <util/lock_and_find.h>

#pragma region kubridge structures and defines

#define KU_KERNEL_PROT_NONE (0x00)
#define KU_KERNEL_PROT_READ (0x40)
#define KU_KERNEL_PROT_WRITE (0x20)
#define KU_KERNEL_PROT_EXEC (0x10)

#define KU_KERNEL_MEM_COMMIT_ATTR_HAS_BASE (0x1)

#define KU_KERNEL_EXCEPTION_TYPE_DATA_ABORT 0
#define KU_KERNEL_EXCEPTION_TYPE_PREFETCH_ABORT 1
#define KU_KERNEL_EXCEPTION_TYPE_UNDEFINED_INSTRUCTION 2

typedef struct KuKernelExceptionContext {
    SceUInt32 r0;
    SceUInt32 r1;
    SceUInt32 r2;
    SceUInt32 r3;
    SceUInt32 r4;
    SceUInt32 r5;
    SceUInt32 r6;
    SceUInt32 r7;
    SceUInt32 r8;
    SceUInt32 r9;
    SceUInt32 r10;
    SceUInt32 r11;
    SceUInt32 r12;
    SceUInt32 sp;
    SceUInt32 lr;
    SceUInt32 pc;
    SceUInt64 vfpRegisters[32];
    SceUInt32 SPSR;
    SceUInt32 FPSCR;
    SceUInt32 FPEXC;
    SceUInt32 FSR;
    SceUInt32 FAR;
    SceUInt32 exceptionType;
} KuKernelExceptionContext;

typedef Ptr<void(Ptr<KuKernelExceptionContext> ctx)> KuKernelExceptionHandler;

typedef struct KuKernelExceptionHandlerOpt {
    SceSize size;
} KuKernelExceptionHandlerOpt;

typedef struct KuKernelMemCommitOpt {
    SceSize size;
    SceUInt32 attr;
    SceUID baseBlock;
    SceUInt32 baseOffset;
} KuKernelMemCommitOpt;

typedef struct SceKernelAddrPair {
    uint32_t addr; //!< Address
    uint32_t length; //!< Length
} SceKernelAddrPair;

typedef struct SceKernelPaddrList {
    uint32_t size; //!< sizeof(SceKernelPaddrList)
    uint32_t list_size; //!< Size in elements of the list array
    uint32_t ret_length; //!< Total physical size of the memory pairs
    uint32_t ret_count; //!< Number of elements of list filled by ksceKernelGetPaddrList
    Ptr<SceKernelAddrPair> list; //!< Array of physical addresses and their lengths pairs
} SceKernelPaddrList;

typedef struct SceKernelAllocMemBlockKernelOpt {
    SceSize size; //!< sizeof(SceKernelAllocMemBlockKernelOpt)
    SceUInt32 field_4;
    SceUInt32 attr; //!< OR of SceKernelAllocMemBlockAttr
    SceUInt32 field_C;
    SceUInt32 paddr;
    SceSize alignment;
    SceUInt32 extraLow;
    SceUInt32 extraHigh;
    SceUInt32 mirror_blockid;
    SceUID pid;
    Ptr<SceKernelPaddrList> paddr_list;
    SceUInt32 field_2C;
    SceUInt32 field_30;
    SceUInt32 field_34;
    SceUInt32 field_38;
    SceUInt32 field_3C;
    SceUInt32 field_40;
    SceUInt32 field_44;
    SceUInt32 field_48;
    SceUInt32 field_4C;
    SceUInt32 field_50;
    SceUInt32 field_54;
} SceKernelAllocMemBlockKernelOpt;

// Deprecated
#define KU_KERNEL_ABORT_TYPE_DATA_ABORT 0
#define KU_KERNEL_ABORT_TYPE_PREFETCH_ABORT 1

typedef struct KuKernelAbortContext {
    SceUInt32 r0;
    SceUInt32 r1;
    SceUInt32 r2;
    SceUInt32 r3;
    SceUInt32 r4;
    SceUInt32 r5;
    SceUInt32 r6;
    SceUInt32 r7;
    SceUInt32 r8;
    SceUInt32 r9;
    SceUInt32 r10;
    SceUInt32 r11;
    SceUInt32 r12;
    SceUInt32 sp;
    SceUInt32 lr;
    SceUInt32 pc;
    SceUInt64 vfpRegisters[32];
    SceUInt32 SPSR;
    SceUInt32 FPSCR;
    SceUInt32 FPEXC;
    SceUInt32 FSR;
    SceUInt32 FAR;
    SceUInt32 abortType;
} KuKernelAbortContext;

typedef Ptr<void(Ptr<KuKernelAbortContext> ctx)> KuKernelAbortHandler;

typedef struct KuKernelAbortHandlerOpt {
    SceSize size; //!< Size of structure
} KuKernelAbortHandlerOpt;

#pragma endregion

EXPORT(SceUID, kuKernelAllocMemBlock, const char *name, SceKernelMemBlockType type,
    SceSize size, SceKernelAllocMemBlockKernelOpt *opt)
{
    return UNIMPLEMENTED();
}

EXPORT(void, kuKernelFlushCaches, Ptr<void> ptr, SceSize len)
{
    // We should probably do it for multiple threads, and not just the current one...
    const ThreadStatePtr thread = lock_and_find(thread_id, emuenv.kernel.threads, emuenv.kernel.mutex);
    invalidate_jit_cache(*(thread->cpu), ptr.address(), len);
}

EXPORT(int, kuKernelCpuUnrestrictedMemcpy, Ptr<void> dst, const void *src, SceSize len)
{
    // Should work fine? based on sceClibMemcpy
    memcpy(dst.get(emuenv.mem), src, len);
    return 0;
}

EXPORT(int, kuPowerGetSysClockFrequency)
{
    STUBBED("Fixed value (222)");
    return 222;
}

EXPORT(int, kuPowerSetSysClockFrequency, int freq)
{
    return STUBBED("doing nothing");
}

EXPORT(int, kuKernelMemProtect, Ptr<void> addr, SceSize len, SceUInt32 prot)
{
    return UNIMPLEMENTED();
}

EXPORT(SceUID, kuKernelMemReserve, Ptr<void*> addr, SceSize size, SceKernelMemBlockType memBlockType)
{
    return UNIMPLEMENTED();
}

EXPORT(int, kuKernelMemCommit, Ptr<void> addr, SceSize len, SceUInt32 prot, KuKernelMemCommitOpt *pOpt)
{
    return UNIMPLEMENTED();
}

EXPORT(int, kuKernelMemDecommit, Ptr<void> addr, SceSize len)
{
    return UNIMPLEMENTED();
}

EXPORT(int, kuKernelRegisterExceptionHandler, SceUInt32 exceptionType, KuKernelExceptionHandler pHandler,
    KuKernelExceptionHandler *pOldHandler, KuKernelExceptionHandlerOpt *pOpt)
{
    return UNIMPLEMENTED();
}

EXPORT(void, kuKernelReleaseExceptionHandler, SceUInt32 exceptionType)
{
    UNIMPLEMENTED();
}

// Deprecated
EXPORT(int, kuKernelRegisterAbortHandler, KuKernelAbortHandler pHandler,
    KuKernelAbortHandler *pOldHandler, KuKernelAbortHandlerOpt *pOpt)
{
    return UNIMPLEMENTED();
}

EXPORT(void, kuKernelReleaseAbortHandler)
{
    UNIMPLEMENTED();
}