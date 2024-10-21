#include <module/module.h>

#include <kernel/types.h>
#include <kernel/state.h>

#pragma region taiHEN defines

/** PID for kernel process */
#define KERNEL_PID (0x10005)

/** Fake library NID indicating that any library NID would match. */
#define TAI_ANY_LIBRARY (0xFFFFFFFF)

/** Fake module NID indicating that any module NID would match. */
#define TAI_IGNORE_MODULE_NID (0xFFFFFFFF)

/** Fake module name indicating the current process's main module. */
#define TAI_MAIN_MODULE ((const char*)0)

/**
 * @brief      Extended module information
 *
 *             This supplements the output of `sceKernelGetModuleInfo`
 */
struct tai_module_info_t
{
    SceSize    size;           ///< Structure size, set to sizeof(tai_module_info_t)
    SceUID     modid;          ///< Module UID
    SceUInt32  module_nid;     ///< Module NID
    char       name[27];       ///< Module name
    SceUIntPtr exports_start;  ///< Pointer to export table in process address space
    SceUIntPtr exports_end;    ///< Pointer to end of export table
    SceUIntPtr imports_start;  ///< Pointer to import table in process address space
    SceUIntPtr imports_end;    ///< Pointer to end of import table
};

/**
 * @brief      Pass hook arguments to kernel
 */
struct tai_hook_args_t
{
    SceSize         size;
    Ptr<const char> module;
    SceUInt32       library_nid;
    SceUInt32       func_nid;
    Ptr<const void> hook_func;
};

/**
 * @brief      Pass offset arguments to kernel
 */
struct tai_offset_args_t
{
    SceSize         size;
    SceUID          modid;
    SceInt32        segidx;
    SceUInt32       offset;
    SceBool         thumb;
    Ptr<const void> source;
    SceSize         source_size;
};

/**
 * @brief      Pass module arguments to kernel
 */
struct tai_module_args_t
{
    SceSize   size;
    SceUID    pid;
    SceSize   args;
    Ptr<void> argp;
    SceInt32  flags;
};

/**
 * @brief      Hook information
 *
 *             This reference is created on new hooks and is up to the caller to
 *             keep track of. The client is responsible for cleanup by passing
 *             the reference back to taiHEN when needed.
 */
typedef SceUIntPtr tai_hook_ref_t;

#pragma endregion

#pragma region taiHEN internal defines

#define TAI_SUCCESS 0
#define TAI_ERROR_SYSTEM 0x90010000
#define TAI_ERROR_MEMORY 0x90010001
#define TAI_ERROR_NOT_FOUND 0x90010002
#define TAI_ERROR_INVALID_ARGS 0x90010003
#define TAI_ERROR_INVALID_KERNEL_ADDR 0x90010004
#define TAI_ERROR_PATCH_EXISTS 0x90010005
#define TAI_ERROR_HOOK_ERROR 0x90010006
#define TAI_ERROR_NOT_IMPLEMENTED 0x90010007
#define TAI_ERROR_USER_MEMORY 0x90010008
#define TAI_ERROR_NOT_ALLOWED 0x90010009
#define TAI_ERROR_STUB_NOT_RESOLVED 0x9001000A
#define TAI_ERROR_INVALID_MODULE 0x9001000B
#define TAI_ERROR_MODULE_OVERFLOW 0x9001000C
#define TAI_ERROR_BLOCKING 0x9001000D

#pragma endregion

SceUID get_main_module_uid(EmuEnvState& emuenv)
{
    for (auto [module_id, module] : emuenv.kernel.loaded_modules) 
    {
        if (module->info.path == "app0:" + emuenv.self_path) 
            return module_id;
    }

    return TAI_ERROR_INVALID_MODULE;
}

SceUID get_module_uid_by_name(EmuEnvState& emuenv, const char* module_name)
{
    for (auto [module_id, module] : emuenv.kernel.loaded_modules)
    {
        if (strcmp(module->info.module_name, module_name) == 0)
            return module_id;
    }

    return TAI_ERROR_INVALID_MODULE;
}

SceKernelModuleInfo *get_sce_module_info_from_uid(EmuEnvState& emuenv, SceUID modid)
{
    for (auto [module_id, module] : emuenv.kernel.loaded_modules) 
    {
        if (module_id == modid)
            return &module->info;
    }
    return nullptr;
}

EXPORT(SceUID, taiHookFunctionExportForUser, /*out*/ tai_hook_ref_t *p_hook, /*in*/ tai_hook_args_t *args)
{
    UNIMPLEMENTED();
    return TAI_ERROR_NOT_IMPLEMENTED;
}

EXPORT(SceUID, taiHookFunctionImportForUser, /*out*/ tai_hook_ref_t *p_hook, /*in*/ tai_hook_args_t *args)
{
    UNIMPLEMENTED();
    return TAI_ERROR_NOT_IMPLEMENTED;
}

EXPORT(SceUID, taiHookFunctionOffsetForUser, /*out*/ tai_hook_ref_t *p_hook, /*in*/ tai_offset_args_t *args)
{
    UNIMPLEMENTED();
    return TAI_ERROR_NOT_IMPLEMENTED;
}

EXPORT(int, taiGetModuleInfo, const char *module_name, /*in*/ tai_module_info_t *info)
{
    const std::lock_guard lock(emuenv.kernel.mutex);
    const bool bMainModule = module_name == TAI_MAIN_MODULE;

    if (info->size < sizeof(tai_module_info_t)) 
    {
        LOG_ERROR("Structure size too small: {}", info->size);
        return TAI_ERROR_SYSTEM;
    }

    SceUID modid;
    if (bMainModule)
        modid = get_main_module_uid(emuenv);
    else
        modid = get_module_uid_by_name(emuenv, module_name);

    auto &mod_info = *get_sce_module_info_from_uid(emuenv, modid);

    // we need to add stuff to the v3k kernel impl.
    info->modid = modid;
    info->module_nid = 0xEE10DD7A;
    strncpy(info->name, mod_info.module_name, 27);
    info->name[26] = '\0';
    info->exports_start = 0;
    info->exports_end = 0;
    info->imports_start = 0;
    info->imports_end = 0;

    STUBBED("Fake plant, NFS MW (JP), 1.00");
    return TAI_SUCCESS;
}

EXPORT(int, taiHookRelease, SceUID tai_uid, tai_hook_ref_t hook)
{
    UNIMPLEMENTED();
    return TAI_ERROR_NOT_IMPLEMENTED;
}

EXPORT(SceUID, taiInjectAbs, void *dest, const void *src, SceSize size)
{
    memcpy(dest, src, size);
    STUBBED("Call memcpy");
    return 1;
}

EXPORT(SceUID, taiInjectDataForUser, /*in*/ tai_offset_args_t *args)
{
    SceUID modid = args->modid;
    int segidx = args->segidx;
    uint32_t offset = args->offset;
    Ptr<const void> data = args->source;
    size_t size = args->source_size;

    auto& mod_info = *get_sce_module_info_from_uid(emuenv, modid);

    Address addr = mod_info.segments[segidx].vaddr.address() + offset;
    Ptr<void> ptr(addr);

    // Just call memcpy
    memcpy(ptr.get(emuenv.mem), data.get(emuenv.mem), size);

    STUBBED("Doesn't support releasing hooks");
    return 1; // a lot of people use checks like `if (hook_uid >= 0) taiHookRelease(hook_ref)`, please them.
}

EXPORT(int, taiInjectRelease, SceUID tai_uid)
{
    UNIMPLEMENTED();
    return TAI_ERROR_NOT_IMPLEMENTED;
}

EXPORT(int, taiGetModuleExportFunc, /*in*/ const char *modname, 
    /*in*/ uint32_t libnid, /*in*/ uint32_t funcnid, /*out*/ uintptr_t *func)
{
    UNIMPLEMENTED();
    return TAI_ERROR_NOT_IMPLEMENTED;
}