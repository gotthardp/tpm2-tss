/* SPDX-License-Identifier: BSD-2-Clause */
/***********************************************************************;
 * Copyright (c) 2015 - 2017, Intel Corporation
 * All rights reserved.
 ***********************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h" // IWYU pragma: keep
#endif

#include "sysapi_util.h"      // for _TSS2_SYS_CONTEXT_BLOB, syscontext_cast
#include "tss2_common.h"      // for TSS2_RC, TSS2_SYS_RC_BAD_REFERENCE
#include "tss2_mu.h"          // for Tss2_MU_UINT16_Marshal, Tss2_MU_TPM2B_D...
#include "tss2_sys.h"         // for TSS2_SYS_CONTEXT, TSS2L_SYS_AUTH_COMMAND
#include "tss2_tpm2_types.h"  // for TPM2B_DIGEST, TPM2B_ENCRYPTED_SECRET

TSS2_RC Tss2_Sys_MakeCredential_Prepare(
    TSS2_SYS_CONTEXT *sysContext,
    TPMI_DH_OBJECT handle,
    const TPM2B_DIGEST *credential,
    const TPM2B_NAME *objectName)
{
    _TSS2_SYS_CONTEXT_BLOB *ctx = syscontext_cast(sysContext);
    TSS2_RC rval;

    if (!ctx)
        return TSS2_SYS_RC_BAD_REFERENCE;

    rval = CommonPreparePrologue(ctx, TPM2_CC_MakeCredential);
    if (rval)
        return rval;

    rval = Tss2_MU_UINT32_Marshal(handle, ctx->cmdBuffer,
                                  ctx->maxCmdSize,
                                  &ctx->nextData);
    if (rval)
        return rval;

    if (!credential) {
        ctx->decryptNull = 1;

        rval = Tss2_MU_UINT16_Marshal(0, ctx->cmdBuffer,
                                      ctx->maxCmdSize,
                                      &ctx->nextData);
    } else {

        rval = Tss2_MU_TPM2B_DIGEST_Marshal(credential, ctx->cmdBuffer,
                                            ctx->maxCmdSize,
                                            &ctx->nextData);
    }

    if (rval)
        return rval;

    if (!objectName) {
        rval = Tss2_MU_UINT16_Marshal(0, ctx->cmdBuffer,
                                      ctx->maxCmdSize,
                                      &ctx->nextData);

    } else {

        rval = Tss2_MU_TPM2B_NAME_Marshal(objectName, ctx->cmdBuffer,
                                          ctx->maxCmdSize,
                                          &ctx->nextData);
    }

    if (rval)
        return rval;

    ctx->decryptAllowed = 1;
    ctx->encryptAllowed = 1;
    ctx->authAllowed = 1;

    return CommonPrepareEpilogue(ctx);
}

TSS2_RC Tss2_Sys_MakeCredential_Complete(
    TSS2_SYS_CONTEXT *sysContext,
    TPM2B_ID_OBJECT *credentialBlob,
    TPM2B_ENCRYPTED_SECRET *secret)
{
    _TSS2_SYS_CONTEXT_BLOB *ctx = syscontext_cast(sysContext);
    TSS2_RC rval;

    if (!ctx)
        return TSS2_SYS_RC_BAD_REFERENCE;

    rval = CommonComplete(ctx);
    if (rval)
        return rval;

    rval = Tss2_MU_TPM2B_ID_OBJECT_Unmarshal(ctx->cmdBuffer,
                                             ctx->maxCmdSize,
                                             &ctx->nextData,
                                             credentialBlob);
    if (rval)
        return rval;

    return Tss2_MU_TPM2B_ENCRYPTED_SECRET_Unmarshal(ctx->cmdBuffer,
                                                    ctx->maxCmdSize,
                                                    &ctx->nextData,
                                                    secret);
}

TSS2_RC Tss2_Sys_MakeCredential(
    TSS2_SYS_CONTEXT *sysContext,
    TPMI_DH_OBJECT handle,
    TSS2L_SYS_AUTH_COMMAND const *cmdAuthsArray,
    const TPM2B_DIGEST *credential,
    const TPM2B_NAME *objectName,
    TPM2B_ID_OBJECT *credentialBlob,
    TPM2B_ENCRYPTED_SECRET *secret,
    TSS2L_SYS_AUTH_RESPONSE *rspAuthsArray)
{
    _TSS2_SYS_CONTEXT_BLOB *ctx = syscontext_cast(sysContext);
    TSS2_RC rval;

    rval = Tss2_Sys_MakeCredential_Prepare(sysContext, handle, credential, objectName);
    if (rval)
        return rval;

    rval = CommonOneCall(ctx, cmdAuthsArray, rspAuthsArray);
    if (rval)
        return rval;

    return Tss2_Sys_MakeCredential_Complete(sysContext, credentialBlob, secret);
}
