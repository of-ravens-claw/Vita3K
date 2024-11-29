#include <module/module.h>

#include <kernel/types.h>
#include <kernel/state.h>
#include <cpu/functions.h>
#include <util/lock_and_find.h>

#include <modules/SceSysmem/SceSysmem.h>

#pragma region kubridge structures and defines

#define KU_KERNEL_PROT_NONE (0x00)
#define KU_KERNEL_PROT_READ (0x40)
#define KU_KERNEL_PROT_WRITE (0x20)
#define KU_KERNEL_PROT_EXEC (0x10)

#define KU_KERNEL_MEM_COMMIT_ATTR_HAS_BASE (0x1)

#define KU_KERNEL_EXCEPTION_TYPE_DATA_ABORT 0
#define KU_KERNEL_EXCEPTION_TYPE_PREFETCH_ABORT 1
#define KU_KERNEL_EXCEPTION_TYPE_UNDEFINED_INSTRUCTION 2

struct KuKernelExceptionContext
{
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
};

typedef Ptr<void(Ptr<KuKernelExceptionContext> ctx)> KuKernelExceptionHandler;

struct KuKernelExceptionHandlerOpt
{
    SceSize size;
};

 struct KuKernelMemCommitOpt
{
    SceSize size;
    SceUInt32 attr;
    SceUID baseBlock;
    SceUInt32 baseOffset;
};

// Deprecated
typedef KuKernelExceptionContext KuKernelAbortContext;
typedef struct KuKernelAbortHandlerOpt KuKernelAbortHandlerOpt;
typedef KuKernelExceptionHandler KuKernelAbortHandler;

#pragma endregion

// The actual implementation lies in SceSysmem.cpp
EXPORT(SceUID, kuKernelAllocMemBlock, const char* name, SceKernelMemBlockType type, SceSize vsize, SceKernelAllocMemBlockOptKernel* pOpt)
{
    return CALL_EXPORT(sceKernelAllocMemBlockForDriver, name, type, vsize, pOpt);
}

EXPORT(void, kuKernelFlushCaches, Ptr<void> ptr, SceSize len)
{
    const std::lock_guard lock(emuenv.kernel.mutex);
    for (const auto& thread : emuenv.kernel.threads)
        invalidate_jit_cache(*(thread.second->cpu), ptr.address(), len);
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

// Vita3K doesn't really support memory protection so we ignore these.
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