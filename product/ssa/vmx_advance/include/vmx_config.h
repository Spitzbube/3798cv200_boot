/******************************************************************************

          Copyright (C), 2001-2015, Hisilicon Technologies Co., Ltd.
 ******************************************************************************
 File Name       : vmx_config.h
 Version         : Initial draft
 Author          : Device Chipset STB Development Dept
 Created Date    : 2016-07-29
 Last Modified by: l00163273
 Description     :
 Function List   :
 Change History  :
 Version  Date        Author     DefectNum         Description
 main\1   2016-07-29  l00163273  N/A               Create this file.
 ******************************************************************************/

#ifndef __VMX_CONFIG_H__
#define __VMX_CONFIG_H__

/******************************************************************************/
/*  Header file references                                                    */
/*  Quote order                                                               */
/*  1. Global header file                                                     */
/*  2. Public header files of other modules                                   */
/*  3. Header files within this module                                        */
/*  4. Private files within this module (.inc)                                */
/******************************************************************************/

#include "hi_type.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

/*************************** Macro Definition *********************************/
/** \addtogroup     VMX_CONFIG */
/** @{ */  /** <!-- [VMX_CONFIG] */

#define SSA_LEN_COMMAND                 (1024)
#define SSA_LEN_IMG_HEADER_SIGN         (256)
#define SSA_LEN_IMG_HEADER_SIGN_POS     (16)
#define SSA_LEN_IMG_HEADER              (SSA_LEN_IMG_HEADER_SIGN + SSA_LEN_IMG_HEADER_SIGN_POS)

#if defined(HI_ANDROID_BOOT_SUPPORT)
#define SSA_VERIFY_ADDR_KERNEL          (HI_U8*)(0x01FFFFC0 - SSA_LEN_IMG_HEADER)
#define SSA_VERIFY_ADDR_LOADER          (HI_U8*)(0x01FFFFC0 - SSA_LEN_IMG_HEADER)
#define SSA_VERIFY_ADDR_BOOTARGS        (HI_U8*)(0x08000000)
#define SSA_VERIFY_ADDR_ROOTFS          (HI_U8*)(0x08000000)
#define SSA_VERIFY_ADDR_RESERVE         (HI_U8*)(0x0C000000)

#define SSA_VERIFY_SIZE_KERNEL          (0x02000000)
#define SSA_VERIFY_SIZE_LOADER          (0x02000000)
#define SSA_VERIFY_SIZE_BOOTARGS        (0x00100000)
#define SSA_VERIFY_SIZE_ROOTFS          (0x04000000)
#define SSA_VERIFY_SIZE_VERIFY          (0x04000000)

#else
#define SSA_VERIFY_ADDR_KERNEL          (HI_U8*)(0x01FFFFC0 - SSA_LEN_IMG_HEADER)
#define SSA_VERIFY_ADDR_LOADER          (HI_U8*)(0x01FFFFC0 - SSA_LEN_IMG_HEADER)
#define SSA_VERIFY_ADDR_BOOTARGS        (HI_U8*)(0x08000000)
#define SSA_VERIFY_ADDR_ROOTFS          (HI_U8*)(0x08000000)
#define SSA_VERIFY_ADDR_RESERVE         (HI_U8*)(0x0A000000)

#define SSA_VERIFY_SIZE_KERNEL          (0x02000000)
#define SSA_VERIFY_SIZE_LOADER          (0x02000000)
#define SSA_VERIFY_SIZE_BOOTARGS        (0x02000000)
#define SSA_VERIFY_SIZE_ROOTFS          (0x02000000)
#define SSA_VERIFY_SIZE_VERIFY          (0x02000000)
#endif

#define SSA_PARTSIZE_BOOT               (0x00100000)
#define SSA_PARTSIZE_BOOTARGS           (0x00100000)

/** Partition define */
#define SSA_PARTNAME_BOOT                   "boot"
#define SSA_PARTNAME_BOOTARGS               "bootargs"
#define SSA_PARTNAME_LOGO                   "logo"
#define SSA_PARTNAME_LOADER                 "loader"
#define SSA_PARTNAME_KERNEL                 "kernel"
#define SSA_PARTNAME_SWINFO                 "softwareinfo"

#if defined(HI_ANDROID_BOOT_SUPPORT)
#define SSA_PARTNAME_ROOTFS                 "shatable"
#else
#define SSA_PARTNAME_ROOTFS                 "rootfs"
#endif

/** @} */  /*! <!-- Macro Definition End */

/*************************** Structure Definition *****************************/
/** \addtogroup     VMX_CONFIG */
/** @{ */  /** <!-- [VMX_CONFIG] */

/****** Enumeration definition ************/

/****** Structure definition **************/

/****** Union definition ******************/

/****** Global variable declaration *******/

/** @}*/  /** <!-- ==== Structure Definition End ====*/

/******************************* API Declaration ******************************/
/** \addtogroup     VMX_CONFIG */
/** @{ */  /** <!-- [VMX_CONFIG] */

/** @}*/  /** <!-- ==== API Declaration End ====*/

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif/*__VMX_CONFIG_H__*/
