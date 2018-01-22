/******************************************************************************

          Copyright (C), 2001-2015, Hisilicon Technologies Co., Ltd.
 ******************************************************************************
 File Name       : vmx_ssa.c
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

/******************************************************************************/
/*  Header file references                                                    */
/*  Quote order                                                               */
/*  1. Global header file                                                     */
/*  2. Public header files of other modules                                   */
/*  3. Header files within this module                                        */
/*  4. Private files within this module (.inc)                                */
/******************************************************************************/

#ifdef HI_MINIBOOT_SUPPORT
#include <cpuinfo.h>
#include "run.h"
#include "boot.h"
#include "app.h"
#ifdef reserve_mem_alloc
#undef resmemalign
#undef reserve_mem_alloc
#endif

#else
#include "common.h"
#endif

#ifdef HI_BUILD_WITH_KEYLED
#include "hi_unf_keyled.h"
#endif

#include "hi_drv_pdm.h"
#include "hi_drv_pq.h"
#include "hi_boot_logo.h"
#include "hi_boot_adp.h"
#include "hi_go_common.h"
#include "hi_go_logo.h"

#include "loader_debug.h"
#include "loader_osal.h"
#include "loader_flash.h"
#include "loader_dbinfo.h"
#include "vmx_config.h"
#include "vmx_auth.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#ifdef CONFIG_USB_HOST_BOOTSTRAP
#define LOADER_APPLOADER_IMAGENAME      "apploader.bin"

extern void selfboot_init(void);
extern unsigned int get_selfboot_type(void);
extern int download_bootargs(void);
extern int download_image(const char* imagename);
#endif

#ifdef HI_BUILD_WITH_KEYLED

#define SSA_KEY_TRY_TIMES               (50)
#define SSA_KEY_TRY_INTERVAL            (10000) /** us */

#if defined(HI_BOOT_KEYLED_GPIOKEY) && defined(CHIP_TYPE_hi3716mv420)
#define SSA_KEY_VALUE_MENU              (0x40)
#define SSA_KEY_VALUE_OK                (0x2)
#elif defined(HI_BOOT_KEYLED_GPIOKEY) && defined(CHIP_TYPE_hi3798cv200)
#define SSA_KEY_VALUE_OK                (0x1)
#else
#define SSA_KEY_VALUE_MENU              (0x0)
#define SSA_KEY_VALUE_OK                (0x1)
#endif

static HI_U32 s_auMandatoryUpgradeKey[] =
{
#if defined(HI_BOOT_KEYLED_GPIOKEY) && defined(CHIP_TYPE_hi3798cv200)
    SSA_KEY_VALUE_OK,
#else
    SSA_KEY_VALUE_MENU,
    SSA_KEY_VALUE_OK,
#endif
};

static HI_S32 ssaMandatoryUpgradeCheck(HI_VOID)
{
    HI_S32 i = 0;
    HI_U32 u32PressStatus = 0;
    HI_U32 u32KeyValue = 0;
    HI_S32 s32KeyIndex = 0;
    HI_S32 s32KeyCount = 0;

    HI_S32 s32Ret = HI_FAILURE;

    s32KeyCount = sizeof(s_auMandatoryUpgradeKey) / sizeof(HI_U32);

    for (s32KeyIndex = 0; s32KeyIndex < s32KeyCount; s32KeyIndex++)
    {
        for (i = 0; i < SSA_KEY_TRY_TIMES; i++)
        {
            s32Ret = HI_UNF_KEY_GetValue(&u32PressStatus, &u32KeyValue);
            if (HI_SUCCESS != s32Ret)
            {
                udelay(SSA_KEY_TRY_INTERVAL);
                continue;
            }

            if (0x1 != u32PressStatus)
            {
                udelay(SSA_KEY_TRY_INTERVAL);
                continue;
            }

            if (s_auMandatoryUpgradeKey[s32KeyIndex] == u32KeyValue)
            {
                break;
            }
        }

        if (SSA_KEY_TRY_TIMES == i)
        {
            return HI_FAILURE;
        }

        udelay(SSA_KEY_TRY_INTERVAL);
    }

    return HI_SUCCESS;
}

#endif

static HI_VOID ssaReboot(HI_VOID)
{
    LOADER_FuncEnter();

    setenv("bootcmd", "reset");

    LOADER_FuncExit();
    return;
}

static HI_S32 ssaBootM(HI_U8* pu8MemBuffer)
{
    LOADER_FuncEnter();

#ifdef HI_MINIBOOT_SUPPORT
    kern_load((HI_CHAR*)(pu8MemBuffer));
#else
    HI_CHAR as8BootCmd[SSA_LEN_COMMAND] = {0};

    snprintf(as8BootCmd, sizeof(as8BootCmd), "bootm 0x%x", (HI_U32)pu8MemBuffer);
    run_command(as8BootCmd, 0);
#endif

    LOADER_FuncExit();
    return HI_SUCCESS;
}

static HI_S32 ssaVerifySignature
(
    LOADER_FLASH_OPENPARAM_S*   pstOpenParam,
    HI_U8*                      pu8DataBuffer,
    HI_U8*                      pu8VerifyBuffer,
    HI_U32                      u32BufferSize,
    HI_U32*                     pu32DataOffset,
    HI_U32*                     pu32DataSize
)
{
    HI_U8  u8ErrCode = 0;
    HI_U32 u32ReadSize = 0;
    HI_U32 u32ImageDataSize = 0;
    LOADER_FLASH_INFO_S stFlashInfo;
    HI_HANDLE hFlashHandle = HI_INVALID_HANDLE;
    HI_S32 s32Ret = HI_FAILURE;

    LOADER_FuncEnter();

    hFlashHandle = LOADER_FLASH_Open(pstOpenParam);
    if (HI_INVALID_HANDLE == hFlashHandle)
    {
        LOADER_PrintFuncErr(LOADER_FLASH_Open, LOADER_ERR_FLASH);
        return LOADER_ERR_FLASH;
    }

    u32ReadSize = LOADER_FLASH_Read(hFlashHandle, 0ULL, pu8DataBuffer, u32BufferSize, SSA_LEN_IMG_HEADER);
    if (SSA_LEN_IMG_HEADER > u32ReadSize)
    {
        LOADER_PrintFuncErr(LOADER_FLASH_Read, LOADER_ERR_FLASH);

        LOADER_CHECK(LOADER_FLASH_Close(hFlashHandle));
        return LOADER_ERR_FLASH;
    }

    LOADER_MemSet(&stFlashInfo, 0x00, sizeof(stFlashInfo));
    s32Ret = LOADER_FLASH_GetInfo(hFlashHandle, &stFlashInfo);
    if (HI_SUCCESS != s32Ret)
    {
        LOADER_PrintFuncErr(LOADER_FLASH_GetInfo, s32Ret);

        LOADER_CHECK(LOADER_FLASH_Close(hFlashHandle));
        return s32Ret;
    }

    u32ImageDataSize = *((HI_U32*)pu8DataBuffer);

    if ((SSA_LEN_IMG_HEADER >= stFlashInfo.u64PartSize)
        || (u32ImageDataSize > (stFlashInfo.u64PartSize - SSA_LEN_IMG_HEADER)))
    {
        LOADER_CHECK(LOADER_FLASH_Close(hFlashHandle));
        hFlashHandle = HI_INVALID_HANDLE;

        LOADER_PrintErrCode(LOADER_ERR_ILLEGAL_IMAGE);
        return LOADER_ERR_ILLEGAL_IMAGE;
    }

    u32ReadSize = LOADER_FLASH_Read(hFlashHandle, 0ULL, pu8DataBuffer, u32BufferSize, u32ImageDataSize + SSA_LEN_IMG_HEADER);
    if (u32ImageDataSize > u32ReadSize)
    {
        LOADER_PrintFuncErr(LOADER_FLASH_Read, LOADER_ERR_FLASH);

        LOADER_CHECK(LOADER_FLASH_Close(hFlashHandle));
        return LOADER_ERR_FLASH;
    }

    LOADER_PrintBuffer("Image Head:", pu8DataBuffer, SSA_LEN_IMG_HEADER);
    LOADER_PrintBuffer("Image Data:", pu8DataBuffer + SSA_LEN_IMG_HEADER, 256);

    s32Ret = verifySignature(pu8DataBuffer + SSA_LEN_IMG_HEADER_SIGN_POS,
                             pu8DataBuffer + SSA_LEN_IMG_HEADER,
                             pu8VerifyBuffer,
                             u32ImageDataSize,
                             u32ImageDataSize,
                             0,
                             &u8ErrCode);
    if (1 != s32Ret)
    {
        LOADER_PrintFuncErr(verifySignature, s32Ret);

        if (2 == u8ErrCode)
        {
            LOADER_CHECK(LOADER_FLASH_Write(hFlashHandle, 0ULL, pu8DataBuffer, u32ImageDataSize + SSA_LEN_IMG_HEADER));
        }

        LOADER_CHECK(LOADER_FLASH_Close(hFlashHandle));

        ssaReboot();
        return s32Ret;
    }

    LOADER_CHECK(LOADER_FLASH_Close(hFlashHandle));

    *pu32DataOffset = SSA_LEN_IMG_HEADER;
    *pu32DataSize = u32ImageDataSize;

    LOADER_FuncExit();
    return HI_SUCCESS;
}

static HI_S32 ssaUpgradeCheck(HI_BOOL* pbNeedUpgrade)
{
    LDDB_LOADER_INFO_S stInfoParam;
    HI_S32 s32Ret = HI_FAILURE;

#ifdef CONFIG_USB_HOST_BOOTSTRAP
    if (SELF_BOOT_TYPE_USBHOST == get_selfboot_type())
    {
        selfboot_init();

        s32Ret = download_bootargs();
        if (HI_SUCCESS != s32Ret)
        {
            LOADER_PrintFuncErr(download_bootargs, s32Ret);
            return s32Ret;
        }

        LOADER_MemSet(&stInfoParam, 0x00, sizeof(stInfoParam));
        s32Ret = LDDB_GetLoaderInfo(&stInfoParam);
        if (HI_SUCCESS != s32Ret)
        {
            LOADER_PrintFuncErr(LDDB_SetLoaderInfo, s32Ret);
            LOADER_MemSet(&stInfoParam, 0x00, sizeof(stInfoParam));
        }

        stInfoParam.stTAGParamData.bNeedUpgrade = HI_TRUE;
        stInfoParam.stTAGParamData.bUsbBurn = HI_TRUE;
        s32Ret = LDDB_SetLoaderInfo(&stInfoParam);
        if (HI_SUCCESS != s32Ret)
        {
            LOADER_PrintFuncErr(LDDB_SetLoaderInfo, s32Ret);
            return s32Ret;
        }

#ifdef HI_APPLOADER_SUPPORT
        s32Ret = download_image(LOADER_APPLOADER_IMAGENAME);
        if (HI_SUCCESS != s32Ret)
        {
            LOADER_PrintFuncErr(download_image, s32Ret);
            return s32Ret;
        }
#endif
        *pbNeedUpgrade = HI_TRUE;
        return HI_SUCCESS;
    }
#endif

#ifdef HI_BUILD_WITH_KEYLED
    s32Ret = ssaMandatoryUpgradeCheck();
    if (HI_SUCCESS == s32Ret)
    {
        LOADER_MemSet(&stInfoParam, 0x00, sizeof(stInfoParam));
        s32Ret = LDDB_GetLoaderInfo(&stInfoParam);
        if (HI_SUCCESS != s32Ret)
        {
            LOADER_PrintFuncErr(LDDB_GetLoaderInfo, s32Ret);
            LOADER_MemSet(&stInfoParam, 0x00, sizeof(stInfoParam));
        }

        stInfoParam.stTAGParamData.bNeedUpgrade = HI_TRUE;
        stInfoParam.stTAGParamData.bForceUpgrade = HI_TRUE;
        s32Ret = LDDB_SetLoaderInfo(&stInfoParam);
        if (HI_SUCCESS != s32Ret)
        {
            LOADER_PrintFuncErr(LDDB_SetLoaderInfo, s32Ret);
            return s32Ret;
        }

        *pbNeedUpgrade = HI_TRUE;
        return HI_SUCCESS;
    }
#endif

    LOADER_MemSet(&stInfoParam, 0x00, sizeof(stInfoParam));
    s32Ret = LDDB_GetLoaderInfo(&stInfoParam);
    if (HI_SUCCESS != s32Ret)
    {
        LOADER_PrintFuncErr(LDDB_GetLoaderInfo, s32Ret);
        return s32Ret;
    }

    *pbNeedUpgrade = stInfoParam.stTAGParamData.bNeedUpgrade;

    return HI_SUCCESS;
}

static HI_S32 ssaBootargsAppend(HI_U32 u32RootfsPhyAddr, HI_U32 u32RootfsSize)
{
    HI_CHAR* pcBootargs = HI_NULL_PTR;
    HI_CHAR acBootarg[SSA_LEN_COMMAND] = {0};

    LOADER_FuncEnter();

    pcBootargs = getenv("bootargs");
    if (HI_NULL_PTR == pcBootargs)
    {
        return HI_FAILURE;
    }

#if defined(HI_ANDROID_BOOT_SUPPORT)
    snprintf(acBootarg, sizeof(acBootarg), "%s initmrd=1,0x%x,0x%x", pcBootargs, u32RootfsPhyAddr, u32RootfsSize);
#else
    snprintf(acBootarg, sizeof(acBootarg), "%s initrd=0x%x,0x%x root=/dev/ram ramdisk_size=40960 rootfstype=squashfs", pcBootargs, u32RootfsPhyAddr, u32RootfsSize);
#endif

    setenv("bootargs", acBootarg);

    LOADER_FuncExit();
    return HI_SUCCESS;
}

static HI_S32 ssaShowLogo(HI_VOID)
{
    HI_U32 u32LogoDataAddr = 0;

    HI_GO_LOGO_INFO stLogoInfo;
    HI_LOGO_PARAM_S stLogoParam;
    HI_DISP_PARAM_S stDispParam0;
    HI_DISP_PARAM_S stDispParam1;

    HI_S32 s32Ret = HI_SUCCESS;

    LOADER_FuncEnter();

    s32Ret = HI_DRV_PDM_GetLogoParam(&stLogoParam);
    if (HI_SUCCESS != s32Ret)
    {
        LOADER_PrintFuncErr(HI_DRV_PDM_GetLogoParam, s32Ret);
        return LOADER_ERR_EXTERNAL;
    }

    if (1 != stLogoParam.u32CheckFlag)
    {
        LOADER_FuncExit();
        return HI_SUCCESS;
    }

    s32Ret = HI_ADP_DispInit();
    if (HI_SUCCESS != s32Ret)
    {
        LOADER_PrintFuncErr(HI_ADP_DispInit, s32Ret);
        return LOADER_ERR_EXTERNAL;
    }

    LOADER_MemSet(&stDispParam0, 0x00, sizeof(stDispParam0));
    s32Ret = HI_DRV_PDM_GetDispParam(HI_UNF_DISPLAY0, &stDispParam0);
    if (HI_SUCCESS != s32Ret)
    {
        LOADER_PrintFuncErr(HI_DRV_PDM_GetDispParam, s32Ret);
        return LOADER_ERR_EXTERNAL;
    }

    LOADER_MemSet(&stDispParam1, 0x00, sizeof(stDispParam1));
    s32Ret = HI_DRV_PDM_GetDispParam(HI_UNF_DISPLAY1, &stDispParam1);
    if (HI_SUCCESS != s32Ret)
    {
        LOADER_PrintFuncErr(HI_DRV_PDM_GetDispParam, s32Ret);
        return LOADER_ERR_EXTERNAL;
    }

/** Avoid HDMI PHY Invalid problem
    The following code is valid only for the following chips
    Please do not delete, If there is any need to modify, Please confirm with the release manager. */

#if defined(CHIP_TYPE_hi3798mv100)  \
    || defined(CHIP_TYPE_hi3796mv100)  \
    || defined(CHIP_TYPE_hi3716dv100)  \
    || defined(CHIP_TYPE_hi3716mv410)  \
    || defined(CHIP_TYPE_hi3716mv420)  \

    if ((HI_UNF_DISP_INTF_TYPE_HDMI == stDispParam1.stIntf[HI_UNF_DISP_INTF_TYPE_HDMI].enIntfType)
        && (HI_UNF_HDMI_ID_0 == stDispParam1.stIntf[HI_UNF_DISP_INTF_TYPE_HDMI].unIntf.enHdmi))
    {
        LOADER_CHECK(HI_UNF_HDMI_Init());
        LOADER_CHECK(HI_UNF_HDMI_Open(HI_UNF_HDMI_ID_0, stDispParam1.enFormat));
    }
#endif

    s32Ret = HI_DRV_PDM_GetLogoData(stLogoParam.u32LogoLen, &u32LogoDataAddr);
    if (HI_SUCCESS != s32Ret)
    {
        LOADER_PrintFuncErr(HI_DRV_PDM_GetLogoData, s32Ret);
        return s32Ret;
    }

    LOADER_MemSet(&stLogoInfo, 0x00, sizeof(stLogoInfo));
    stLogoInfo.DispToSD      = (HI_UNF_DISPLAY1 != stDispParam0.enSrcDisp) ? HI_TRUE : HI_FALSE;
    stLogoInfo.Support4KLogo = HI_FALSE;
    stLogoInfo.LogoDataLen   = stLogoParam.u32LogoLen;
    stLogoInfo.LogoDataBuf   = u32LogoDataAddr;

    stLogoInfo.DispColorFmt[CONFIG_GFX_DISP_LOGO_SD]         = stDispParam0.enPixelFormat;
    stLogoInfo.DispVirtScreenWidth[CONFIG_GFX_DISP_LOGO_SD]  = stDispParam0.u32VirtScreenWidth;
    stLogoInfo.DispVirtScreenHeight[CONFIG_GFX_DISP_LOGO_SD] = stDispParam0.u32VirtScreenHeight;

    stLogoInfo.DispColorFmt[CONFIG_GFX_DISP_LOGO_HD]         = stDispParam1.enPixelFormat;
    stLogoInfo.DispVirtScreenWidth[CONFIG_GFX_DISP_LOGO_HD]  = stDispParam1.u32VirtScreenWidth;
    stLogoInfo.DispVirtScreenHeight[CONFIG_GFX_DISP_LOGO_HD] = stDispParam1.u32VirtScreenHeight;

    s32Ret = HI_GO_DisplayLogo(&stLogoInfo);
    if (HI_SUCCESS != s32Ret)
    {
        LOADER_PrintFuncErr(HI_GO_DisplayLogo, s32Ret);
        return LOADER_ERR_EXTERNAL;
    }

    HI_DRV_PDM_SetTagData();

/** Avoid HDMI PHY Invalid problem
    The following code is valid only for the following chips
    Please do not delete, If there is any need to modify, Please confirm with the release manager. */

#if defined(CHIP_TYPE_hi3798mv100)  \
    || defined(CHIP_TYPE_hi3796mv100)  \
    || defined(CHIP_TYPE_hi3716dv100)  \
    || defined(CHIP_TYPE_hi3716mv410)  \
    || defined(CHIP_TYPE_hi3716mv420)  \

    if ((HI_UNF_DISP_INTF_TYPE_HDMI == stDispParam1.stIntf[HI_UNF_DISP_INTF_TYPE_HDMI].enIntfType)
        && (HI_UNF_HDMI_ID_0 == stDispParam1.stIntf[HI_UNF_DISP_INTF_TYPE_HDMI].unIntf.enHdmi))
    {
        LOADER_CHECK(HI_UNF_HDMI_Open(HI_UNF_HDMI_ID_0, stDispParam1.enFormat));
    }
#endif

    LOADER_FuncExit();
    return HI_SUCCESS;
}

static HI_S32 ssaLoadLoaderImage(HI_VOID)
{
    HI_U32 u32Offset = 0;
    HI_U32 u32DataSize = 0;
    LOADER_FLASH_OPENPARAM_S stDataParam;

    HI_S32 s32Ret = HI_FAILURE;

    LOADER_FuncEnter();

    LOADER_PrintInfo("** [TRACE] Verify Loader Image ! **");

    LOADER_MemSet(&stDataParam, 0x00, sizeof(stDataParam));
    stDataParam.pcPartitionName = SSA_PARTNAME_LOADER;
    stDataParam.enFlashType     = HI_FLASH_TYPE_BUTT;
    s32Ret = ssaVerifySignature(&stDataParam,
                                SSA_VERIFY_ADDR_LOADER,
                                SSA_VERIFY_ADDR_RESERVE,
                                SSA_VERIFY_SIZE_LOADER,
                                &u32Offset,
                                &u32DataSize);
    if (HI_SUCCESS != s32Ret)
    {
        LOADER_PrintFuncErr(ssaVerifySignature, s32Ret);
        return s32Ret;
    }

    LOADER_CHECK(ssaBootM(SSA_VERIFY_ADDR_LOADER + u32Offset));

    LOADER_FuncExit();
    return HI_SUCCESS;
}

static HI_S32 ssaLoadKernelImage(HI_VOID)
{
    HI_U32 u32KernelOffset = 0;
    HI_U32 u32RootfsOffset = 0;
    HI_U32 u32KernelDataSize = 0;
    HI_U32 u32RootfsDataSize = 0;
    LOADER_FLASH_OPENPARAM_S stDataParam;

    HI_S32 s32Ret = HI_FAILURE;

    LOADER_FuncEnter();

    LOADER_PrintInfo("** [TRACE] Verify Kernel Image ! **");

    LOADER_MemSet(&stDataParam, 0x00, sizeof(stDataParam));
    stDataParam.pcPartitionName = SSA_PARTNAME_KERNEL;
    stDataParam.enFlashType     = HI_FLASH_TYPE_BUTT;
    s32Ret = ssaVerifySignature(&stDataParam,
                                SSA_VERIFY_ADDR_KERNEL,
                                SSA_VERIFY_ADDR_RESERVE,
                                SSA_VERIFY_SIZE_KERNEL,
                                &u32KernelOffset,
                                &u32KernelDataSize);
    if (HI_SUCCESS != s32Ret)
    {
        LOADER_PrintFuncErr(ssaVerifySignature, s32Ret);
        return s32Ret;
    }

    LOADER_PrintInfo("** [TRACE] Verify Rootfs Image ! **");
    stDataParam.pcPartitionName = SSA_PARTNAME_ROOTFS;
    s32Ret = ssaVerifySignature(&stDataParam,
                                SSA_VERIFY_ADDR_ROOTFS,
                                SSA_VERIFY_ADDR_RESERVE,
                                SSA_VERIFY_SIZE_ROOTFS,
                                &u32RootfsOffset,
                                &u32RootfsDataSize);
    if (HI_SUCCESS != s32Ret)
    {
        LOADER_PrintFuncErr(ssaVerifySignature, s32Ret);
        return s32Ret;
    }

    s32Ret = ssaBootargsAppend((HI_U32)(SSA_VERIFY_ADDR_ROOTFS + u32RootfsOffset), u32RootfsDataSize);
    if (HI_SUCCESS != s32Ret)
    {
        LOADER_PrintFuncErr(ssaBootargsAppend, s32Ret);
        return s32Ret;
    }

    s32Ret = ssaBootM(SSA_VERIFY_ADDR_KERNEL + u32KernelOffset);
    if (HI_SUCCESS != s32Ret)
    {
        LOADER_PrintFuncErr(ssaBootM, s32Ret);
        return s32Ret;
    }

    LOADER_FuncExit();
    return HI_SUCCESS;
}

HI_S32 HI_SSA_Init(HI_VOID)
{
    LOADER_FuncEnter();

    LOADER_FuncExit();
    return HI_SUCCESS;
}

HI_S32 HI_SSA_DeInit(HI_VOID)
{
    LOADER_FuncEnter();

    LOADER_FuncExit();
    return HI_SUCCESS;
}

HI_S32 HI_SSA_Entry(HI_VOID)
{
    HI_BOOL bNeedUpgrade = HI_FALSE;
    HI_S32 s32Ret = HI_FAILURE;

    LOADER_FuncEnter();

    LOADER_CHECK(ssaShowLogo());
    LOADER_CHECK(ssaUpgradeCheck(&bNeedUpgrade));

    if (HI_TRUE == bNeedUpgrade)
    {
        s32Ret = ssaLoadLoaderImage();
        if (HI_SUCCESS != s32Ret)
        {
            LOADER_PrintFuncErr(ssaLoadLoaderImage, s32Ret);

            s32Ret = ssaLoadKernelImage();
            if (HI_SUCCESS != s32Ret)
            {
                LOADER_PrintFuncErr(ssaLoadKernelImage, s32Ret);
                return s32Ret;
            }
        }
    }
    else
    {
        s32Ret = ssaLoadKernelImage();
        if (HI_SUCCESS != s32Ret)
        {
            LOADER_PrintFuncErr(ssaLoadKernelImage, s32Ret);

            s32Ret = ssaLoadLoaderImage();
            if (HI_SUCCESS != s32Ret)
            {
                LOADER_PrintFuncErr(ssaLoadLoaderImage, s32Ret);
                return s32Ret;
            }
        }
    }

    LOADER_FuncExit();
    return HI_SUCCESS;
}

#ifdef HI_ADVCA_VERIFY_ENABLE
static HI_FLASH_TYPE_E ssaGetFlashType(HI_VOID)
{
    HI_S32 enBootMediaType = BOOT_MEDIA_UNKNOWN;

    enBootMediaType = get_bootmedia(HI_NULL_PTR, HI_NULL_PTR);

    switch (enBootMediaType)
    {
        case BOOT_MEDIA_NAND:
        {
            return HI_FLASH_TYPE_NAND_0;
        }
        case BOOT_MEDIA_SPIFLASH:
        {
            return HI_FLASH_TYPE_SPI_0;
        }
        case BOOT_MEDIA_EMMC:
        {
            return HI_FLASH_TYPE_EMMC_0;
        }
        case BOOT_MEDIA_SD:
        {
            return HI_FLASH_TYPE_SD;
        }
        default:
        {
            return HI_FLASH_TYPE_BUTT;
        }
    }
}

HI_S32 load_direct_env(HI_VOID* pBuffer, HI_U32 u32Address, HI_U32 u32Length)
{
    HI_U32 u32Offset = 0;
    HI_U32 u32DataSize = 0;
    LOADER_FLASH_OPENPARAM_S stDataParam;
    HI_FLASH_TYPE_E enFlashType = HI_FLASH_TYPE_BUTT;

    HI_S32 s32Ret = HI_FAILURE;

    LOADER_FuncEnter();

    enFlashType = ssaGetFlashType();
    if (HI_FLASH_TYPE_BUTT == enFlashType)
    {
        LOADER_PrintFuncErr(ssaGetFlashType, LOADER_ERR_FLASH);
        return LOADER_ERR_FLASH;
    }

    LOADER_PrintInfo("** [TRACE] Verify Bootargs Image ! **");
    LOADER_MemSet(&stDataParam, 0x00, sizeof(stDataParam));
    stDataParam.pcPartitionName = HI_NULL_PTR;
    stDataParam.enFlashType     = enFlashType;
    stDataParam.u64Address      = u32Address;
    stDataParam.u64Length       = SSA_VERIFY_SIZE_BOOTARGS;
    s32Ret = ssaVerifySignature(&stDataParam,
                                SSA_VERIFY_ADDR_BOOTARGS,
                                SSA_VERIFY_ADDR_RESERVE,
                                SSA_VERIFY_SIZE_BOOTARGS,
                                &u32Offset,
                                &u32DataSize);
    if (HI_SUCCESS != s32Ret)
    {
        LOADER_PrintFuncErr(ssaVerifySignature, s32Ret);
        return s32Ret;
    }

    LOADER_MemSet(pBuffer, 0x00, u32Length);
    LOADER_MemCpy(pBuffer, (HI_U8*)(SSA_VERIFY_ADDR_BOOTARGS + u32Offset), u32DataSize);

    LOADER_FuncExit();
    return HI_SUCCESS;
}
#else
int load_direct_env(void* env, unsigned int offset, unsigned int size)
{
    LOADER_FuncEnter();

    LOADER_FuncExit();
    return HI_FAILURE;
}
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

