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
 */

#ifndef __UEFI_UTILS_H__
#define __UEFI_UTILS_H__

struct EFI_LOAD_OPTION {
	UINT32 Attributes;
	UINT16 FilePathListLength;
} __attribute__((packed));

EFI_STATUS ConvertBmpToGopBlt (VOID *BmpImage, UINTN BmpImageSize,
			       VOID **GopBlt, UINTN *GopBltSize,
			       UINTN *PixelHeight, UINTN *PixelWidth);
EFI_STATUS gop_display_blt(EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Blt, UINTN blt_size, UINTN height, UINTN width);
EFI_STATUS uefi_read_file(EFI_FILE_IO_INTERFACE *io, CHAR16 *filename, void **data, UINTN *size);
EFI_STATUS find_device_partition(const EFI_GUID *guid, EFI_HANDLE **handles, UINTN *no_handles);

#endif /* __UEFI_UTILS_H__ */
