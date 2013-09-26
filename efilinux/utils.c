/*
 * Copyright (c) 2013, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer
 *      in the documentation and/or other materials provided with the
 *      distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "utils.h"

EFI_STATUS str_to_stra(CHAR8 *dst, CHAR16 *src, UINTN len)
{
        UINTN i;

        /* This is NOT how to do UTF16 to UTF8 conversion. For now we're just
         * going to hope that nobody's putting non-ASCII characters in
         * the source string! We'll at least abort with an error
         * if we see any funny stuff */
        for (i = 0; i < len; i++) {
                if (src[i] > 0x7F)
                        return EFI_INVALID_PARAMETER;

                dst[i] = (CHAR8)src[i];
                if (!src[i])
                        break;
        }
        return EFI_SUCCESS;
}


VOID error(CHAR16 *str, EFI_STATUS ret)
{
        Print(L"ERROR %s: %r\n", str, ret);
        uefi_call_wrapper(BS->Stall, 1, 2 * 1000 * 1000);
}

VOID StrNCpy(OUT CHAR16 *dest, IN const CHAR16 *src, UINT32 n)
{
        UINT32 i;

        for (i = 0; i < n && src[i] != 0; i++)
                dest[i] = src[i];
        for ( ; i < n; i++)
                dest[i] = 0;
}


UINT8 getdigit(IN CHAR16 *str)
{
        CHAR16 bytestr[3];
        bytestr[2] = 0;
        StrNCpy(bytestr, str, 2);
        return (UINT8)xtoi(bytestr);
}


EFI_STATUS string_to_guid(IN CHAR16 *in_guid_str, OUT EFI_GUID *guid)
{
        CHAR16 gstr[37];
        int i;

        StrNCpy(gstr, in_guid_str, 36);
        gstr[36] = 0;
        gstr[8] = 0;
        gstr[13] = 0;
        gstr[18] = 0;
        guid->Data1 = (UINT32)xtoi(gstr);
        guid->Data2 = (UINT16)xtoi(&gstr[9]);
        guid->Data3 = (UINT16)xtoi(&gstr[14]);

        guid->Data4[0] = getdigit(&gstr[19]);
        guid->Data4[1] = getdigit(&gstr[21]);
        for (i = 0; i < 6; i++)
                guid->Data4[i + 2] = getdigit(&gstr[24 + (i * 2)]);

        return EFI_SUCCESS;
}

UINT32 swap_bytes32(UINT32 n)
{
        return ((n & 0x000000FF) << 24) |
               ((n & 0x0000FF00) << 8 ) |
               ((n & 0x00FF0000) >> 8 ) |
               ((n & 0xFF000000) >> 24);
}


UINT16 swap_bytes16(UINT16 n)
{
        return ((n & 0x00FF) << 8) | ((n & 0xFF00) >> 8);
}


void copy_and_swap_guid(EFI_GUID *dst, const EFI_GUID *src)
{
        memcpy((CHAR8 *)&dst->Data4, (CHAR8 *)src->Data4, sizeof(src->Data4));
        dst->Data1 = swap_bytes32(src->Data1);
        dst->Data2 = swap_bytes16(src->Data2);
        dst->Data3 = swap_bytes16(src->Data3);
}

EFI_STATUS open_partition(
                IN const EFI_GUID *guid,
                OUT UINT32 *MediaIdPtr,
                OUT EFI_BLOCK_IO **BlockIoPtr,
                OUT EFI_DISK_IO **DiskIoPtr)
{
        EFI_STATUS ret;
        EFI_BLOCK_IO *BlockIo;
        EFI_DISK_IO *DiskIo;
        UINT32 MediaId;
        UINTN NoHandles = 0;
        EFI_HANDLE *HandleBuffer = NULL;

        /* Get a handle on the partition containing the boot image */
        ret = LibLocateHandleByDiskSignature(
                        MBR_TYPE_EFI_PARTITION_TABLE_HEADER,
                        SIGNATURE_TYPE_GUID,
                        (void *)guid,
                        &NoHandles,
                        &HandleBuffer);
        if (EFI_ERROR(ret) || NoHandles == 0) {
                /* Workaround for old installers which incorrectly wrote
                 * GUIDs strings as little-endian */
                EFI_GUID g;
                copy_and_swap_guid(&g, guid);
                ret = LibLocateHandleByDiskSignature(
                                MBR_TYPE_EFI_PARTITION_TABLE_HEADER,
                                SIGNATURE_TYPE_GUID,
                                (void *)&g,
                                &NoHandles,
                                &HandleBuffer);
                if (EFI_ERROR(ret)) {
                        error(L"LibLocateHandle", ret);
                        return ret;
                }
        }
        if (NoHandles != 1) {
                Print(L"%d handles found for GUID, expecting 1: %g\n",
                                NoHandles, guid);
                ret = EFI_VOLUME_CORRUPTED;
                goto out;
        }

        /* Instantiate BlockIO and DiskIO protocols so we can read various data */
        ret = uefi_call_wrapper(BS->HandleProtocol, 3, HandleBuffer[0],
                        &BlockIoProtocol,
                        (void **)&BlockIo);
        if (EFI_ERROR(ret)) {
                error(L"HandleProtocol (BlockIoProtocol)", ret);
                goto out;;
        }
        ret = uefi_call_wrapper(BS->HandleProtocol, 3, HandleBuffer[0],
                        &DiskIoProtocol, (void **)&DiskIo);
        if (EFI_ERROR(ret)) {
                error(L"HandleProtocol (DiskIoProtocol)", ret);
                goto out;
        }
        MediaId = BlockIo->Media->MediaId;

        *MediaIdPtr = MediaId;
        *BlockIoPtr = BlockIo;
        *DiskIoPtr = DiskIo;
out:
        FreePool(HandleBuffer);
        return ret;
}


