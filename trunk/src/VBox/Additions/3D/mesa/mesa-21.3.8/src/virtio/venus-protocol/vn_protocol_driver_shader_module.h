/* This file is generated by venus-protocol.  See vn_protocol_driver.h. */

/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

#ifndef VN_PROTOCOL_DRIVER_SHADER_MODULE_H
#define VN_PROTOCOL_DRIVER_SHADER_MODULE_H

#include "vn_instance.h"
#include "vn_protocol_driver_structs.h"

/* struct VkShaderModuleCreateInfo chain */

static inline size_t
vn_sizeof_VkShaderModuleCreateInfo_pnext(const void *val)
{
    /* no known/supported struct */
    return vn_sizeof_simple_pointer(NULL);
}

static inline size_t
vn_sizeof_VkShaderModuleCreateInfo_self(const VkShaderModuleCreateInfo *val)
{
    size_t size = 0;
    /* skip val->{sType,pNext} */
    size += vn_sizeof_VkFlags(&val->flags);
    size += vn_sizeof_size_t(&val->codeSize);
    if (val->pCode) {
        size += vn_sizeof_array_size(val->codeSize / 4);
        size += vn_sizeof_uint32_t_array(val->pCode, val->codeSize / 4);
    } else {
        size += vn_sizeof_array_size(0);
    }
    return size;
}

static inline size_t
vn_sizeof_VkShaderModuleCreateInfo(const VkShaderModuleCreateInfo *val)
{
    size_t size = 0;

    size += vn_sizeof_VkStructureType(&val->sType);
    size += vn_sizeof_VkShaderModuleCreateInfo_pnext(val->pNext);
    size += vn_sizeof_VkShaderModuleCreateInfo_self(val);

    return size;
}

static inline void
vn_encode_VkShaderModuleCreateInfo_pnext(struct vn_cs_encoder *enc, const void *val)
{
    /* no known/supported struct */
    vn_encode_simple_pointer(enc, NULL);
}

static inline void
vn_encode_VkShaderModuleCreateInfo_self(struct vn_cs_encoder *enc, const VkShaderModuleCreateInfo *val)
{
    /* skip val->{sType,pNext} */
    vn_encode_VkFlags(enc, &val->flags);
    vn_encode_size_t(enc, &val->codeSize);
    if (val->pCode) {
        vn_encode_array_size(enc, val->codeSize / 4);
        vn_encode_uint32_t_array(enc, val->pCode, val->codeSize / 4);
    } else {
        vn_encode_array_size(enc, 0);
    }
}

static inline void
vn_encode_VkShaderModuleCreateInfo(struct vn_cs_encoder *enc, const VkShaderModuleCreateInfo *val)
{
    assert(val->sType == VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO);
    vn_encode_VkStructureType(enc, &(VkStructureType){ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO });
    vn_encode_VkShaderModuleCreateInfo_pnext(enc, val->pNext);
    vn_encode_VkShaderModuleCreateInfo_self(enc, val);
}

static inline size_t vn_sizeof_vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule)
{
    const VkCommandTypeEXT cmd_type = VK_COMMAND_TYPE_vkCreateShaderModule_EXT;
    const VkFlags cmd_flags = 0;
    size_t cmd_size = vn_sizeof_VkCommandTypeEXT(&cmd_type) + vn_sizeof_VkFlags(&cmd_flags);

    cmd_size += vn_sizeof_VkDevice(&device);
    cmd_size += vn_sizeof_simple_pointer(pCreateInfo);
    if (pCreateInfo)
        cmd_size += vn_sizeof_VkShaderModuleCreateInfo(pCreateInfo);
    cmd_size += vn_sizeof_simple_pointer(pAllocator);
    if (pAllocator)
        assert(false);
    cmd_size += vn_sizeof_simple_pointer(pShaderModule);
    if (pShaderModule)
        cmd_size += vn_sizeof_VkShaderModule(pShaderModule);

    return cmd_size;
}

static inline void vn_encode_vkCreateShaderModule(struct vn_cs_encoder *enc, VkCommandFlagsEXT cmd_flags, VkDevice device, const VkShaderModuleCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule)
{
    const VkCommandTypeEXT cmd_type = VK_COMMAND_TYPE_vkCreateShaderModule_EXT;

    vn_encode_VkCommandTypeEXT(enc, &cmd_type);
    vn_encode_VkFlags(enc, &cmd_flags);

    vn_encode_VkDevice(enc, &device);
    if (vn_encode_simple_pointer(enc, pCreateInfo))
        vn_encode_VkShaderModuleCreateInfo(enc, pCreateInfo);
    if (vn_encode_simple_pointer(enc, pAllocator))
        assert(false);
    if (vn_encode_simple_pointer(enc, pShaderModule))
        vn_encode_VkShaderModule(enc, pShaderModule);
}

static inline size_t vn_sizeof_vkCreateShaderModule_reply(VkDevice device, const VkShaderModuleCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule)
{
    const VkCommandTypeEXT cmd_type = VK_COMMAND_TYPE_vkCreateShaderModule_EXT;
    size_t cmd_size = vn_sizeof_VkCommandTypeEXT(&cmd_type);

    VkResult ret;
    cmd_size += vn_sizeof_VkResult(&ret);
    /* skip device */
    /* skip pCreateInfo */
    /* skip pAllocator */
    cmd_size += vn_sizeof_simple_pointer(pShaderModule);
    if (pShaderModule)
        cmd_size += vn_sizeof_VkShaderModule(pShaderModule);

    return cmd_size;
}

static inline VkResult vn_decode_vkCreateShaderModule_reply(struct vn_cs_decoder *dec, VkDevice device, const VkShaderModuleCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule)
{
    VkCommandTypeEXT command_type;
    vn_decode_VkCommandTypeEXT(dec, &command_type);
    assert(command_type == VK_COMMAND_TYPE_vkCreateShaderModule_EXT);

    VkResult ret;
    vn_decode_VkResult(dec, &ret);
    /* skip device */
    /* skip pCreateInfo */
    /* skip pAllocator */
    if (vn_decode_simple_pointer(dec)) {
        vn_decode_VkShaderModule(dec, pShaderModule);
    } else {
        pShaderModule = NULL;
    }

    return ret;
}

static inline size_t vn_sizeof_vkDestroyShaderModule(VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks* pAllocator)
{
    const VkCommandTypeEXT cmd_type = VK_COMMAND_TYPE_vkDestroyShaderModule_EXT;
    const VkFlags cmd_flags = 0;
    size_t cmd_size = vn_sizeof_VkCommandTypeEXT(&cmd_type) + vn_sizeof_VkFlags(&cmd_flags);

    cmd_size += vn_sizeof_VkDevice(&device);
    cmd_size += vn_sizeof_VkShaderModule(&shaderModule);
    cmd_size += vn_sizeof_simple_pointer(pAllocator);
    if (pAllocator)
        assert(false);

    return cmd_size;
}

static inline void vn_encode_vkDestroyShaderModule(struct vn_cs_encoder *enc, VkCommandFlagsEXT cmd_flags, VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks* pAllocator)
{
    const VkCommandTypeEXT cmd_type = VK_COMMAND_TYPE_vkDestroyShaderModule_EXT;

    vn_encode_VkCommandTypeEXT(enc, &cmd_type);
    vn_encode_VkFlags(enc, &cmd_flags);

    vn_encode_VkDevice(enc, &device);
    vn_encode_VkShaderModule(enc, &shaderModule);
    if (vn_encode_simple_pointer(enc, pAllocator))
        assert(false);
}

static inline size_t vn_sizeof_vkDestroyShaderModule_reply(VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks* pAllocator)
{
    const VkCommandTypeEXT cmd_type = VK_COMMAND_TYPE_vkDestroyShaderModule_EXT;
    size_t cmd_size = vn_sizeof_VkCommandTypeEXT(&cmd_type);

    /* skip device */
    /* skip shaderModule */
    /* skip pAllocator */

    return cmd_size;
}

static inline void vn_decode_vkDestroyShaderModule_reply(struct vn_cs_decoder *dec, VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks* pAllocator)
{
    VkCommandTypeEXT command_type;
    vn_decode_VkCommandTypeEXT(dec, &command_type);
    assert(command_type == VK_COMMAND_TYPE_vkDestroyShaderModule_EXT);

    /* skip device */
    /* skip shaderModule */
    /* skip pAllocator */
}

static inline void vn_submit_vkCreateShaderModule(struct vn_instance *vn_instance, VkCommandFlagsEXT cmd_flags, VkDevice device, const VkShaderModuleCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule, struct vn_instance_submit_command *submit)
{
    uint8_t local_cmd_data[VN_SUBMIT_LOCAL_CMD_SIZE];
    void *cmd_data = local_cmd_data;
    size_t cmd_size = vn_sizeof_vkCreateShaderModule(device, pCreateInfo, pAllocator, pShaderModule);
    if (cmd_size > sizeof(local_cmd_data)) {
        cmd_data = malloc(cmd_size);
        if (!cmd_data)
            cmd_size = 0;
    }
    const size_t reply_size = cmd_flags & VK_COMMAND_GENERATE_REPLY_BIT_EXT ? vn_sizeof_vkCreateShaderModule_reply(device, pCreateInfo, pAllocator, pShaderModule) : 0;

    struct vn_cs_encoder *enc = vn_instance_submit_command_init(vn_instance, submit, cmd_data, cmd_size, reply_size);
    if (cmd_size) {
        vn_encode_vkCreateShaderModule(enc, cmd_flags, device, pCreateInfo, pAllocator, pShaderModule);
        vn_instance_submit_command(vn_instance, submit);
        if (cmd_data != local_cmd_data)
            free(cmd_data);
    }
}

static inline void vn_submit_vkDestroyShaderModule(struct vn_instance *vn_instance, VkCommandFlagsEXT cmd_flags, VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks* pAllocator, struct vn_instance_submit_command *submit)
{
    uint8_t local_cmd_data[VN_SUBMIT_LOCAL_CMD_SIZE];
    void *cmd_data = local_cmd_data;
    size_t cmd_size = vn_sizeof_vkDestroyShaderModule(device, shaderModule, pAllocator);
    if (cmd_size > sizeof(local_cmd_data)) {
        cmd_data = malloc(cmd_size);
        if (!cmd_data)
            cmd_size = 0;
    }
    const size_t reply_size = cmd_flags & VK_COMMAND_GENERATE_REPLY_BIT_EXT ? vn_sizeof_vkDestroyShaderModule_reply(device, shaderModule, pAllocator) : 0;

    struct vn_cs_encoder *enc = vn_instance_submit_command_init(vn_instance, submit, cmd_data, cmd_size, reply_size);
    if (cmd_size) {
        vn_encode_vkDestroyShaderModule(enc, cmd_flags, device, shaderModule, pAllocator);
        vn_instance_submit_command(vn_instance, submit);
        if (cmd_data != local_cmd_data)
            free(cmd_data);
    }
}

static inline VkResult vn_call_vkCreateShaderModule(struct vn_instance *vn_instance, VkDevice device, const VkShaderModuleCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule)
{
    VN_TRACE_FUNC();

    struct vn_instance_submit_command submit;
    vn_submit_vkCreateShaderModule(vn_instance, VK_COMMAND_GENERATE_REPLY_BIT_EXT, device, pCreateInfo, pAllocator, pShaderModule, &submit);
    struct vn_cs_decoder *dec = vn_instance_get_command_reply(vn_instance, &submit);
    if (dec) {
        const VkResult ret = vn_decode_vkCreateShaderModule_reply(dec, device, pCreateInfo, pAllocator, pShaderModule);
        vn_instance_free_command_reply(vn_instance, &submit);
        return ret;
    } else {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
}

static inline void vn_async_vkCreateShaderModule(struct vn_instance *vn_instance, VkDevice device, const VkShaderModuleCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule)
{
    struct vn_instance_submit_command submit;
    vn_submit_vkCreateShaderModule(vn_instance, 0, device, pCreateInfo, pAllocator, pShaderModule, &submit);
}

static inline void vn_call_vkDestroyShaderModule(struct vn_instance *vn_instance, VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks* pAllocator)
{
    VN_TRACE_FUNC();

    struct vn_instance_submit_command submit;
    vn_submit_vkDestroyShaderModule(vn_instance, VK_COMMAND_GENERATE_REPLY_BIT_EXT, device, shaderModule, pAllocator, &submit);
    struct vn_cs_decoder *dec = vn_instance_get_command_reply(vn_instance, &submit);
    if (dec) {
        vn_decode_vkDestroyShaderModule_reply(dec, device, shaderModule, pAllocator);
        vn_instance_free_command_reply(vn_instance, &submit);
    }
}

static inline void vn_async_vkDestroyShaderModule(struct vn_instance *vn_instance, VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks* pAllocator)
{
    struct vn_instance_submit_command submit;
    vn_submit_vkDestroyShaderModule(vn_instance, 0, device, shaderModule, pAllocator, &submit);
}

#endif /* VN_PROTOCOL_DRIVER_SHADER_MODULE_H */
