/*
* Copyright (c) 2008-2017, Intel Corporation
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
* OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
*/
//!
//! \file      hal_kerneldll.c 
//! \brief         Kernel Dynamic Linking / Loading routines 
//!
#ifndef VPHAL_LIB

#if IMOLA
#include <stdlib.h>
#endif // IMOLA
#include <math.h> //for sin & cos
#endif  // VPHAL_LIB

#if EMUL || VPHAL_LIB
#include <math.h>
#include "support.h"
#elif LINUX
#else  // !(EMUL | VPHAL_LIB) && !LINUX

#endif // EMUL | VPHAL_LIB

#include "hal_kerneldll.h"
#include "vphal.h"

// Define _DEBUG symbol for KDLL Release build before loading the "vpkrnheader.h" file
// This is necessary for full kernels names in both Release/Debug versions of KDLL app
#if EMUL || VPHAL_LIB
#ifndef _DEBUG
#define _DEBUG 2
#endif // _DEBUG
#endif // EMUL || VPHAL_LIB

// Kernel IDs and Kernel Names
#include "vpkrnheader.h" // IDR_VP_TOTAL_NUM_KERNELS

// Undefine _DEBUG symbol for the remaining of the KDLL Release build
#if _DEBUG == 2
#undef _DEBUG
#endif // _DEBUG


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus



const float g_cCSC_GrayIdentity[12] =
{
    1.0f,  0.0f, 0.0f, 0.000f,
    0.0f,  0.0f, 0.0f, 128.0f,
    0.0f,  0.0f, 0.0f, 128.0f
};

// BT2020 YUV Limited Range to BT2020 RGB full range conversion matrix
const float g_cCSC_BT2020_LimitedYUV_RGB[9] =
{
    1.164383f,  0.000000f,  1.678680f,    // R
    1.164383f, -0.187332f, -0.650421f,    // G
    1.164383f,  2.141769f,  0.000000f     // B
};

// BT2020 RGB full range to BT2020 YUV Limited Range tconversion matrix
const float g_cCSC_BT2020_RGB_LimitedYUV[9] =
{
    0.225617f,   0.582275f,  0.050934f,  // Y
    -0.122650f, -0.316559f,  0.439209f,  // U
    0.439209f,  -0.403885f, -0.035324f   // V
};

const char  *g_cInit_ComponentNames[] =
{
    IDR_VP_KERNEL_NAMES
};

#define FOLD_HASH(folded_hash, hash)                                   \
    {                                                                  \
        folded_hash = (((hash) >> 8) ^ (hash)) & 0x00ff00ff;           \
        folded_hash = ((folded_hash >> 16) ^ folded_hash) & 0xff;      \
    }                                                                  \

#if _DEBUG || EMUL || VPHAL_LIB

#ifndef VPHAL_LIB
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif // VPHAL_LIB

const char    *KernelDll_GetLayerString(Kdll_Layer layer)
{
    switch (layer)
    {
        case Layer_Invalid     : return _T("Invalid");
        case Layer_None        : return _T("None");
        case Layer_Background  : return _T("Background");
        case Layer_MainVideo   : return _T("Main Video");
        case Layer_SubVideo    : return _T("Sub-Video");
        case Layer_SubPicture1 : return _T("Sub-Picture 1");
        case Layer_SubPicture2 : return _T("Sub-Picture 2");
        case Layer_SubPicture3 : return _T("Sub-Picture 3");
        case Layer_SubPicture4 : return _T("Sub-Picture 4");
        case Layer_Graphics    : return _T("Graphics");
        case Layer_RenderTarget: return _T("Render Target");
    }

    return nullptr;
}

const char    *KernelDll_GetFormatString(MOS_FORMAT   format)
{
    switch (format)
    {
        case Format_Invalid     : return _T("Invalid");
        case Format_Source      : return _T("Current layer");
        case Format_RGB         : return _T("RGB");
        case Format_RGB32       : return _T("RGB32");
        case Format_PA          : return _T("PA");
        case Format_PL2         : return _T("PL2");
        case Format_PL3         : return _T("PL3");
        case Format_PL3_RGB     : return _T("PL3_RGB");
        case Format_PAL         : return _T("PAL");
        case Format_None        : return _T("None");
        case Format_Any         : return _T("Any");
        case Format_A8R8G8B8    : return _T("ARGB");
        case Format_X8R8G8B8    : return _T("RGB");
        case Format_A8B8G8R8    : return _T("ABGR");
        case Format_X8B8G8R8    : return _T("BGR");
        case Format_A16B16G16R16: return _T("A16B16G16R16");
        case Format_A16R16G16B16: return _T("A16R16G16B16");
        case Format_R5G6B5      : return _T("RGB16");
        case Format_R8G8B8      : return _T("RGB24");
        case Format_R32U        : return _T("R32U");
        case Format_R32F        : return _T("R32F");
        case Format_RGBP        : return _T("RGBP");
        case Format_BGRP        : return _T("BGRP");
        case Format_YUY2        : return _T("YUY2");
        case Format_YUYV        : return _T("YUYV");
        case Format_YVYU        : return _T("YVYU");
        case Format_UYVY        : return _T("UYVY");
        case Format_VYUY        : return _T("VYUY");
        case Format_Y416        : return _T("Y416");
        case Format_AYUV        : return _T("AYUV");
        case Format_AUYV        : return _T("AUYV");
        case Format_400P        : return _T("400P");
        case Format_NV12        : return _T("NV12");
        case Format_NV12_UnAligned: return _T("NV12_UnAligned");
        case Format_NV21        : return _T("NV21");
        case Format_NV11        : return _T("NV11");
        case Format_NV11_UnAligned: return _T("NV11_UnAligned");
        case Format_P208        : return _T("P208");
        case Format_P208_UnAligned: return _T("P208_UnAligned");
        case Format_IMC1        : return _T("IMC1");
        case Format_IMC2        : return _T("IMC2");
        case Format_IMC3        : return _T("IMC3");
        case Format_IMC4        : return _T("IMC4");
        case Format_422H        : return _T("422H");
        case Format_422V        : return _T("422V");
        case Format_444P        : return _T("444P");
        case Format_411P        : return _T("411P");
        case Format_411R        : return _T("411R");
        case Format_I420        : return _T("I420");
        case Format_IYUV        : return _T("IYUV");
        case Format_YV12        : return _T("YV12");
        case Format_YVU9        : return _T("YVU9");
        case Format_AI44        : return _T("AI44");
        case Format_IA44        : return _T("IA44");
        case Format_P8          : return _T("P8");
        case Format_A8P8        : return _T("A8P8");
        case Format_A8          : return _T("A8");
        case Format_L8          : return _T("L8");
        case Format_A4L4        : return _T("A4L4");
        case Format_A8L8        : return _T("A8L8");
        case Format_IRW0        : return _T("IRW0");
        case Format_IRW1        : return _T("IRW1");
        case Format_IRW2        : return _T("IRW2");
        case Format_IRW3        : return _T("IRW3");
        case Format_IRW4        : return _T("IRW4");
        case Format_IRW5        : return _T("IRW5");
        case Format_IRW6        : return _T("IRW6");
        case Format_IRW7        : return _T("IRW7");
        case Format_STMM        : return _T("STMM");
        case Format_Buffer      : return _T("Buffer");
        case Format_Buffer_2D   : return _T("Buffer_2D");
        case Format_V8U8        : return _T("V8U8");
        case Format_R32S        : return _T("R32S");
        case Format_R8U         : return _T("RU8");
        case Format_R8G8UN      : return _T("R8G8UN");
        case Format_R8G8SN      : return _T("R8S8UN");
        case Format_G8R8_G8B8   : return _T("G8R8_G8B8");
        case Format_R16U        : return _T("R16U");
        case Format_R16S        : return _T("R16S");
        case Format_R16UN       : return _T("R16UN");
        case Format_RAW         : return _T("RAW");
        case Format_Y8          : return _T("Y8");
        case Format_Y1          : return _T("Y1");
        case Format_Y16U        : return _T("Y16U");
        case Format_Y16S        : return _T("Y16S");
        case Format_L16         : return _T("L16");
        case Format_D16         : return _T("D16");
        case Format_R10G10B10A2 : return _T("R10G10B10A2");
        case Format_B10G10R10A2 : return _T("B10G10R10A2");
        case Format_P016        : return _T("P016");
        case Format_P010        : return _T("P010");
        case Format_YV12_Planar : return _T("YV12_Planar");
        default                 : return _T("Invalid format");
    }

    return nullptr;
}

const char    *KernelDll_GetCSpaceString(VPHAL_CSPACE cspace)
{
    switch (cspace)
    {
        case CSpace_None                : return _T("None");
        case CSpace_Source              : return _T("Current layer");
        case CSpace_RGB                 : return _T("RGB");
        case CSpace_YUV                 : return _T("YUV");
        case CSpace_Any                 : return _T("Any");
        case CSpace_sRGB                : return _T("sRGB");
        case CSpace_BT601               : return _T("BT.601");
        case CSpace_BT601_FullRange     : return _T("BT.601_FullRange");
        case CSpace_BT709               : return _T("BT.709");
        case CSpace_BT709_FullRange     : return _T("BT.709_FullRange");
        case CSpace_xvYCC601            : return _T("xvYCC.601");
        case CSpace_xvYCC709            : return _T("xvYCC.709");
        case CSpace_BT601Gray           : return _T("BT.601Gray");
        case CSpace_BT601Gray_FullRange : return _T("BT.601Gray_FullRange");
        default                         : return _T("Invalid cspace");
    }

    return nullptr;
}

const char    *KernelDll_GetSamplingString(Kdll_Sampling sampling)
{
    switch (sampling)
    {
        case Sample_None             : return _T("No Sampling");
        case Sample_Source           : return _T("Current layer");
        case Sample_Any              : return _T("Any Sampling");
        case Sample_Scaling_Any      : return _T("Any Scale");
        case Sample_Scaling          : return _T("Scale");
        case Sample_Scaling_034x     : return _T("0.34x");
        case Sample_Scaling_AVS      : return _T("AVS");
        case Sample_iScaling         : return _T("iScale");
        case Sample_iScaling_034x    : return _T("0.34x iScaling");
        case Sample_iScaling_AVS     : return _T("iAVS");
    }

    return nullptr;
}

const char    *KernelDll_GetRotationString(VPHAL_ROTATION rotation)
{
    switch (rotation)
    {
        case VPHAL_ROTATION_IDENTITY            : return _T("0");
        case VPHAL_ROTATION_90                  : return _T("90");
        case VPHAL_ROTATION_180                 : return _T("180");
        case VPHAL_ROTATION_270                 : return _T("270");
        case VPHAL_MIRROR_HORIZONTAL            : return _T("Horizontal");
        case VPHAL_MIRROR_VERTICAL              : return _T("Vertical");
        case VPHAL_ROTATE_90_MIRROR_VERTICAL    : return _T("90 Mirror Vertical");
        case VPHAL_ROTATE_90_MIRROR_HORIZONTAL  : return _T("90 Mirror Horizontal");
    }

    return nullptr;
}

const char    *KernelDll_GetProcessString(Kdll_Processing process)
{
    switch (process)
    {
        case Process_None        : return _T("No processing");
        case Process_Source      : return _T("Current layer");
        case Process_Any         : return _T("Any processing");
        case Process_Composite   : return _T("Composite");
        case Process_XORComposite: return _T("XOR Mono Composite");
        case Process_CBlend      : return _T("Const Blend");
        case Process_SBlend      : return _T("Source Blend");
        case Process_SBlend_4bits: return _T("Source Blend 4-bits");
        case Process_PBlend      : return _T("Part Blend");
        case Process_CSBlend     : return _T("ConstSource Blend");
        case Process_CPBlend     : return _T("ConstPart Blend");
        case Process_DI          : return _T("DI");
        case Process_DN          : return _T("DN");
        case Process_DNDI        : return _T("DNDI");
    }

    return nullptr;
}


const char    *KernelDll_GetParserStateString(Kdll_ParserState state)
{
    switch (state)
    {
        case Parser_Invalid                 : return _T("Invalid");
        case Parser_Begin                   : return _T("Begin");
        case Parser_SetRenderMethod         : return _T("SetRenderMethod");
        case Parser_SetupLayer0             : return _T("SetupLayer0");
        case Parser_SetupLayer1             : return _T("SetupLayer1");
        case Parser_SetParamsLayer0         : return _T("SetParamsLayer0");
        case Parser_SetParamsLayer1         : return _T("SetParamsLayer1");
        case Parser_SetParamsTarget         : return _T("SetParamsTarget");
        case Parser_SampleLayer0            : return _T("SampleLayer0");
        case Parser_SampleLayer0Mix         : return _T("SampleLayer0Mix");
        case Parser_SampleLayer0ColorFill   : return _T("SampleLayer0ColorFill");
        case Parser_RotateLayer0Check       : return _T("SampleRotateLayer0Check");
        case Parser_RotateLayer0            : return _T("SampleRotateLayer0");
        case Parser_SampleLayer0Done        : return _T("SampleLayer0Done");
        case Parser_ShuffleLayer0           : return _T("ShuffleLayer0");
        case Parser_SampleLayer1            : return _T("SampleLayer1");
        case Parser_SampleLayer1Done        : return _T("SampleLayer1Done");
        case Parser_ShuffleLayer1           : return _T("ShuffleLayer1");
        case Parser_SampleLayer0SelectCSC   : return _T("SampleLayer0SelectCSC");
        case Parser_SetupCSC0               : return _T("SetupCSC0");
        case Parser_ExecuteCSC0             : return _T("ExecuteCSC0");
        case Parser_ExecuteCSC0Done         : return _T("ExecuteCSC0Done");
        case Parser_SetupCSC1               : return _T("SetupCSC1");
        case Parser_ExecuteCSC1             : return _T("ExecuteCSC1");
        case Parser_ExecuteCSC1Done         : return _T("ExecuteCSC1Done");
        case Parser_Lumakey                 : return _T("LumaKey");
        case Parser_ProcessLayer            : return _T("ProcessLayer");
        case Parser_ProcessLayerDone        : return _T("ProcessLayerDone");
        case Parser_DualOutput              : return _T("DualOutput");
        case Parser_Rotation                : return _T("Rotation");
        case Parser_DestSurfIndex           : return _T("DestSurfIndex");
        case Parser_Colorfill               : return _T("Colorfill");
        case Parser_WriteOutput             : return _T("WriteOutput");
        case Parser_End                     : return _T("End");
        default                             : return _T("Invalid parser state");
    }

    return nullptr;
}

const char    *KernelDll_GetRuleIDString(Kdll_RuleID RID)
{
    switch (RID)
    {
        case RID_Op_EOF             : return _T("EOF");
        case RID_Op_NewEntry        : return _T("NewEntry");
        case RID_IsTargetCspace     : return _T("IsTargetCspace");
        case RID_IsLayerID          : return _T("IsLayerID");
        case RID_IsLayerFormat      : return _T("IsLayerFormat");
        case RID_IsParserState      : return _T("IsParserState");
        case RID_IsRenderMethod     : return _T("IsRenderMethod");
        case RID_IsShuffling        : return _T("IsShuffling");
        case RID_IsLayerRotation    : return _T("IsLayerRotation");
        case RID_IsSrc0Format       : return _T("IsSrc0Format");
        case RID_IsSrc0Sampling     : return _T("IsSrc0Sampling");
        case RID_IsSrc0ColorFill    : return _T("IsSrc0ColorFill");
        case RID_IsSrc0LumaKey      : return _T("IsSrc0LumaKey");
        case RID_IsSrc0Procamp      : return _T("IsSrc0Procamp");
        case RID_IsSrc0Rotation     : return _T("IsSrc0Rotation");
        case RID_IsSrc0Coeff        : return _T("IsSrc0Coeff");
        case RID_IsSrc0Processing   : return _T("IsSrc0Processing");
        case RID_IsSrc1Format       : return _T("IsSrc1Format");
        case RID_IsSrc1Sampling     : return _T("IsSrc1Sampling");
        case RID_IsSrc1LumaKey      : return _T("IsSrc1LumaKey");
        case RID_IsSrc1SamplerLumaKey: return _T("IsSrc1SamplerLumaKey");
        case RID_IsSrc1Coeff        : return _T("IsSrc1Coeff");
        case RID_IsSrc1Processing   : return _T("IsSrc1Processing");
        case RID_IsLayerNumber      : return _T("IsLayerNumber");
        case RID_IsQuadrant         : return _T("IsQuadrant");
        case RID_IsCSCBeforeMix     : return _T("IsCSCBeforeMix");
        case RID_IsDualOutput       : return _T("IsDualOutput");
        case RID_IsRTRotate         : return _T("IsRTRotate");
        case RID_IsTargetFormat     : return _T("IsTargetFormat");
        case RID_Is64BSaveEnabled   : return _T("Is64BSaveEnabled");
        case RID_IsTargetTileType   : return _T("IsTargetTileType");
        case RID_IsProcampEnabled   : return _T("IsProcampEnabled");
        case RID_SetTargetCspace    : return _T("SetTargetCspace");
        case RID_SetParserState     : return _T("SetParserState");
        case RID_SetSrc0Format      : return _T("SetSrc0Format");
        case RID_SetSrc0Sampling    : return _T("SetSrc0Sampling");
        case RID_SetSrc0ColorFill   : return _T("SetSrc0ColorFill");
        case RID_SetSrc0LumaKey     : return _T("SetSrc0LumaKey");
        case RID_SetSrc0Rotation    : return _T("SetSrc0Rotation");
        case RID_SetSrc0Coeff       : return _T("SetSrc0Coeff");
        case RID_SetSrc0Processing  : return _T("SetSrc0Processing");
        case RID_SetSrc1Format      : return _T("SetSrc1Format");
        case RID_SetSrc1Sampling    : return _T("SetSrc1Sampling");
        case RID_SetSrc1Rotation    : return _T("SetSrc1Rotation");
        case RID_SetSrc1LumaKey     : return _T("SetSrc1LumaKey");
        case RID_SetSrc1SamplerLumaKey: return _T("SetSrc1SamplerLumaKey");
        case RID_SetSrc1Procamp     : return _T("SetSrc1Procamp");
        case RID_SetSrc1Coeff       : return _T("SetSrc1Coeff");
        case RID_SetSrc1Processing  : return _T("SetSrc1Processing");
        case RID_SetKernel          : return _T("SetKernel");
        case RID_SetNextLayer       : return _T("SetNextLayer");
        case RID_SetPatchData       : return _T("SetPatchData");
        case RID_SetQuadrant        : return _T("SetQuadrant");
        case RID_SetCSCBeforeMix    : return _T("SetCSCBeforeMix");
        case RID_SetPatch           : return _T("SetPatch");
        case RID_IsSrc0Chromasiting : return _T("IsSrc0Chromasiting");
        case RID_IsSrc1Procamp      : return _T("IsSrc1Procamp");
        case RID_IsSrc1Chromasiting : return _T("IsSrc1Chromasiting");
        case RID_IsSetCoeffMode     : return _T("IsSetCoeffMode");
        case RID_IsConstOutAlpha    : return _T("IsConstOutAlpha");
        case RID_IsDitherNeeded     : return _T("IsDitherNeeded");
        case RID_IsScalingRatio     : return _T("IsScalingRatio");
        case RID_SetSrc0Procamp     : return _T("SetSrc0Procamp");
    }

    return nullptr;
}

const char    *KernelDll_GetCoeffIDString(Kdll_CoeffID CID)
{
    switch (CID)
    {
        case CoeffID_Src0   : return _T("Src0 coeff");
        case CoeffID_Src1   : return _T("Src1 coeff");
        case CoeffID_Source : return _T("Current layer");
        case CoeffID_Any    : return _T("Any coeff");
        case CoeffID_None   : return _T("No coeff");
        case CoeffID_0      : return _T("Coeff 0");
        case CoeffID_1      : return _T("Coeff 1");
        case CoeffID_2      : return _T("Coeff 2");
        case CoeffID_3      : return _T("Coeff 3");
        case CoeffID_4      : return _T("Coeff 4");
        case CoeffID_5      : return _T("Coeff 5");
    }

    return nullptr;
}

const char    *KernelDll_GetShuffleString(Kdll_Shuffling shuffling)
{
    switch (shuffling)
    {
        case Shuffle_None          : return _T("None");
        case Shuffle_Any           : return _T("Any");
        case Shuffle_All_8x8_Layer : return _T("All 8x8 Layer");
        case Shuffle_RenderTarget  : return _T("Render Target");
    }

    return nullptr;
}

int32_t KernelDll_PrintRule(
    char                    *szOut,
    int32_t                 iSize,
    const Kdll_RuleEntry    *pEntry,
    Kdll_KernelCache        *pCache)
{
    char    data[32];
    int32_t increment   = 1;
    const char *szRID   = KernelDll_GetRuleIDString(pEntry->id);
    const char *szValue = nullptr;

    switch (pEntry->id)
    {
        case RID_IsParserState     :
        case RID_SetParserState    :
            szValue = KernelDll_GetParserStateString((Kdll_ParserState) pEntry->value);
            break;

        case RID_IsLayerID         :
            szValue = KernelDll_GetLayerString((Kdll_Layer) pEntry->value);
            break;

        case RID_Op_NewEntry       :
        case RID_Op_EOF            :
        case RID_IsLayerNumber     :
        case RID_IsQuadrant        :
        case RID_SetQuadrant       :
        case RID_SetPatchData      :
            _stprintf(data, _T("%d"), pEntry->value);
            szValue = data;
            break;

        case RID_IsSrc0ColorFill   :
        case RID_SetSrc0ColorFill  :
            if (pEntry->value == ColorFill_Source)
            {
                szValue = _T("Current Layer");
            }
            else if (pEntry->value)
            {
                szValue = _T("TRUE");
            }
            else
            {
                szValue = _T("FALSE");
            }
            break;

        case RID_IsSrc0LumaKey          :
        case RID_IsSrc1LumaKey          :
        case RID_SetSrc0LumaKey         :
        case RID_SetSrc1LumaKey         :
        case RID_IsSrc1SamplerLumaKey   :
        case RID_SetSrc1SamplerLumaKey  :
            if (pEntry->value == LumaKey_Source)
            {
                szValue = _T("Current Layer");
            }
            else if (pEntry->value)
            {
                szValue = _T("TRUE");
            }
            else
            {
                szValue = _T("FALSE");
            }
            break;

        case RID_IsSrc0Procamp    :
        case RID_IsSrc1Procamp    :
        case RID_SetSrc0Procamp   :
        case RID_SetSrc1Procamp   :
            if (pEntry->value == Procamp_Source)
            {
                szValue = _T("Current Layer");
            }
            else if (pEntry->value == DL_PROCAMP_DISABLED)
            {
                szValue = _T("FALSE");
            }
            else
            {
                szValue = _T("TRUE");
            }
            break;

        case RID_IsSrc0Chromasiting :
        case RID_IsSrc1Chromasiting :
            if (pEntry->value == DL_CHROMASITING_DISABLE)
            {
                szValue = _T("FALSE");
            }
            else
            {
                szValue = _T("TRUE");
            }
            break;

        case RID_SetKernel:
            if (pCache && (pEntry->value < pCache->iCacheEntries))
            {
                szValue = pCache->pCacheEntries[pEntry->value].szName;
            }
            else
            {
                _stprintf(data, _T("%d"), pEntry->value);
                szValue = data;
            }
            break;

        case RID_SetNextLayer:
            if (pEntry->value == 0)
            {
                szValue = _T("Next Layer");
            }
            else if (pEntry->value == -1)
            {
                szValue = _T("Previous Layer");
            }
            else if (pEntry->value == 2)
            {
                szValue = _T("Target Layer");
            }
            else if (pEntry->value == -2)
            {
                szValue = _T("Main Layer");
            }
            break;

        case RID_SetPatch          :
            _stprintf(data, _T("%d patch entries"), pEntry->value);
            increment += pEntry->value;
            szValue = data;
            break;

        case RID_IsLayerFormat     :
        case RID_IsSrc0Format      :
        case RID_IsSrc1Format      :
        case RID_SetSrc0Format     :
        case RID_SetSrc1Format     :
        case RID_IsTargetFormat    :
            szValue = KernelDll_GetFormatString((MOS_FORMAT  ) pEntry->value);
            break;

        case RID_IsTargetCspace    :
        case RID_SetTargetCspace   :
            szValue = KernelDll_GetCSpaceString((VPHAL_CSPACE) pEntry->value);
            break;

        case RID_IsSrc0Sampling    :
        case RID_IsSrc1Sampling    :
        case RID_SetSrc0Sampling   :
        case RID_SetSrc1Sampling   :
            szValue = KernelDll_GetSamplingString((Kdll_Sampling) pEntry->value);
            break;

        case RID_IsSrc0Rotation   :
            szValue = KernelDll_GetRotationString((VPHAL_ROTATION) pEntry->value);
            break;

        case RID_IsShuffling       :
            szValue = KernelDll_GetShuffleString((Kdll_Shuffling) pEntry->value);
            break;

        case RID_IsSrc0Coeff       :
        case RID_IsSrc1Coeff       :
        case RID_SetSrc0Coeff      :
        case RID_SetSrc1Coeff      :
            szValue = KernelDll_GetCoeffIDString((Kdll_CoeffID) pEntry->value);
            break;

        case RID_IsSrc1Processing  :
        case RID_SetSrc1Processing :
            szValue = KernelDll_GetProcessString((Kdll_Processing) pEntry->value);
            break;

        case RID_IsCSCBeforeMix    :
        case RID_SetCSCBeforeMix   :
            if (pEntry->value)
            {
                szValue = _T("TRUE");
            }
            else
            {
                szValue = _T("FALSE");
            }
            break;

        case RID_Is64BSaveEnabled  :
            if (pEntry->value)
            {
                szValue = _T("TRUE");
            }
            else
            {
                szValue = _T("FALSE");
            }
            break;

        default:
            break;
    }

    if (szRID && szValue)
    {
        _sntprintf(szOut, iSize, _T("%-22s %s"), szRID, szValue);
    }
    else if (szRID)
    {
        _sntprintf(szOut, iSize, _T("%-22s"), szRID);
    }

    return increment;
}

#ifndef VPHAL_LIB
#pragma warning( pop )
#endif // !VPHAL_LIB

#endif // _DEBUG || EMUL || VPHAL_LIB

/*----------------------------------------------------------------------------
| Name      : KernelDll_IsSameFormatType
| Purpose   : Check if 2 formats are similar
|
| Input     : Internal Format
|
| Return    : FourCC format
\---------------------------------------------------------------------------*/
bool KernelDll_IsSameFormatType(MOS_FORMAT   format1, MOS_FORMAT   format2)
{
    int32_t group1, group2;

    switch (format1)
    {
        CASE_PA_FORMAT:
            group1 = 1;
            break;

        CASE_PL2_FORMAT:
            group1 = 2;
            break;

        CASE_PL3_FORMAT:
            group1 = 3;
            break;

        CASE_RGB_FORMAT:
            group1 = 4;
            break;

        default:
            group1 = 0;
            break;
    }

    switch (format2)
    {
        CASE_PA_FORMAT:
            group2 = 1;
            break;

        CASE_PL2_FORMAT:
            group2 = 2;
            break;

        CASE_PL3_FORMAT:
            group2 = 3;
            break;

        CASE_RGB_FORMAT:
            group2 = 4;
            break;

        default:
            group2 = 0;
            break;
    }

    if (group1 == group2)
    {
        return true;
    }
    else
    {
        return false;
    }
}

//---------------------------------------------------------------------------------------
// KernelDll_SetupProcampParameters - Setup Kernel Procamp Parameters
//
// Parameters:
//    Kdll_State   *pState       - [in] Kernel dll State to release
//    Kdll_Procamp *pProcamp     - [in] Pointer to array of Procamp Parameters
//    int32_t       iProcampSize - [in] Size of the array
//
// Output: Pointer to allocated Kernel dll state
//         nullptr - Failed to allocate Kernel dll state
//-----------------------------------------------------------------------------------------
void KernelDll_SetupProcampParameters(Kdll_State    *pState,
                                      Kdll_Procamp  *pProcamp,
                                      int32_t        iProcampSize)
{
    VPHAL_RENDER_FUNCTION_ENTER;

    // Setup pointer to procamp parameters
    pState->pProcamp     = pProcamp;
    pState->iProcampSize = iProcampSize;
}

//--------------------------------------------------------------
// Fowler/Noll/Vo FNV-1a hash algorithm - public domain
//--------------------------------------------------------------
uint32_t KernelDll_SimpleHash(void *pData, int32_t iSize)
{
   static const uint32_t k = 0x1000193;
   uint32_t hash = 0x811c9dc5;
   char *p = (char *)pData;

   for(; iSize > 0; iSize--)
   {
      hash ^= (*p++);
      hash *= k;
   }

   return hash;
}

//--------------------------------------------------------------
// KernelDll_GetCombinedKernel - Search combined kernel
//--------------------------------------------------------------
Kdll_CacheEntry *KernelDll_GetCombinedKernel(
    Kdll_State          *pState,
    Kdll_FilterEntry    *pFilter,
    int32_t             iFilterSize,
    uint32_t            dwHash)
{
    Kdll_KernelHashTable *pHashTable;
    Kdll_KernelHashEntry *entries, *curr, *next;
    uint32_t folded_hash;
    uint16_t entry;

    VPHAL_RENDER_FUNCTION_ENTER;

    // Get hash table
    pHashTable = &pState->KernelHashTable;

    // fold hash from 32 to 8 bit :-)
    FOLD_HASH(folded_hash, dwHash)

    // No entries
    entry = pHashTable->wHashTable[folded_hash];
    if (entry == 0 || entry > DL_MAX_COMBINED_KERNELS ) return nullptr;

    entries = (&pHashTable->HashEntry[0]) - 1;  // all indices are 1 based (0 means null)
    curr    = &entries[entry];
    for (; (curr != nullptr); curr = next)
    {
        // match 32-bit hash, then compare filter
        if (curr->dwHash  == dwHash &&
            curr->iFilter == iFilterSize)
        {
            if (memcmp(curr->pFilter, pFilter, iFilterSize * sizeof(Kdll_FilterEntry)) == 0)
            {
                break;
            }
        }

        // Next entry with the same 8-bit folded hash
        next = (curr->next) ? (&entries[curr->next]) : nullptr;
    }

    if (curr)
    {   // Kernel already cached
        curr->pCacheEntry->dwRefresh = pState->dwRefresh++;
        return (curr->pCacheEntry);
    }
    else
    {   // Kernel must be built
        return nullptr;
    }
}

//--------------------------------------------------------------
// KernelDll_AllocateHashEntry - Allocate hash entry
//--------------------------------------------------------------
uint16_t KernelDll_AllocateHashEntry(Kdll_KernelHashTable *pHashTable,
                                 uint32_t              hash)
{
    Kdll_KernelHashEntry *pHashEntry = &pHashTable->HashEntry[0] - 1;
    Kdll_KernelHashEntry *pNewEntry;
    uint32_t folded_hash;
    uint16_t entry;

    VPHAL_RENDER_FUNCTION_ENTER;

    entry = pHashTable->pool;
    if (!entry)
    {
        return 0;
    }

    // Get entry from pool
    pNewEntry = &pHashEntry[entry];
    pHashTable->pool = pNewEntry->next;
    if (pHashTable->last == entry)
    {
        pHashTable->last = 0;
    }

    // Initialize entry, attach to the hash table
    FOLD_HASH(folded_hash, hash);
    pNewEntry->dwHash      = hash;
    pNewEntry->next        = pHashTable->wHashTable[folded_hash];
    pNewEntry->iFilter     = 0;
    pNewEntry->pFilter     = nullptr;
    pNewEntry->pCacheEntry = nullptr;
    pHashTable->wHashTable[folded_hash] = entry;
    return entry;
}

//--------------------------------------------------------------
// KernelDll_ReleaseHashEntry - Release hash table entry
//--------------------------------------------------------------
void KernelDll_ReleaseHashEntry(Kdll_KernelHashTable *pHashTable, uint16_t entry)
{
    Kdll_KernelHashEntry *pHashEntry = &pHashTable->HashEntry[0] - 1;
    uint32_t folded_hash;
    uint16_t next;

    VPHAL_RENDER_FUNCTION_ENTER;

    if (entry == 0)
    {
        return;
    }

    // unlink entry
    next = pHashEntry[entry].next;
    pHashEntry[entry].next = 0;

    // remove references to entry from hash table
    FOLD_HASH(folded_hash, pHashEntry[entry].dwHash);
    if (pHashTable->wHashTable[folded_hash] == entry)
    {
        pHashTable->wHashTable[folded_hash] = next;
    }
    else
    {
        uint16_t prev = pHashTable->wHashTable[folded_hash];

        while (prev != 0 &&
               pHashEntry[prev].next != entry)
        {
            prev = pHashEntry[prev].next;
        }

        if (prev)
        {
            pHashEntry[prev].next = next;
        }
    }

    // return entry to pool
    if (pHashTable->pool == 0)
    {
        pHashTable->pool = entry;
    }
    else
    {
        pHashEntry[pHashTable->last].next = entry;
    }
    pHashTable->last = entry;
}

//--------------------------------------------------------------
// KernelDll_ReleaseCacheEntry - Release cache entry
//--------------------------------------------------------------
void KernelDll_ReleaseCacheEntry(Kdll_KernelCache *pCache,
                                 Kdll_CacheEntry  *pEntry)
{
    pEntry->iKUID = -1;
    pEntry->iKCID = -1;
    pCache->iCacheEntries--;
}

//--------------------------------------------------------------
// KernelDll_CacheGarbageCollection - performs garbage collection
//--------------------------------------------------------------
bool KernelDll_GarbageCollection(Kdll_State *pState, int32_t size)
{
    Kdll_KernelCache     *pCache     = &pState->KernelCache;
    Kdll_CacheEntry      *pEntry     = pCache->pCacheEntries;
    Kdll_CacheEntry      *pOldest    = nullptr;
    Kdll_KernelHashTable *pHashTable = &pState->KernelHashTable;
    Kdll_KernelHashEntry *pHashEntry = &pHashTable->HashEntry[0] - 1;
    uint32_t              dwOldest   = (uint32_t)-1;
    uint16_t              wEntry     = 0;
    int32_t i;

    MOS_UNUSED(size);

    VPHAL_RENDER_FUNCTION_ENTER;

    // Adjust refresh values to avoid overflow
    if (pState->dwRefresh > 0xffff0000)
    {
        pState->dwRefresh -= 0x80000000;
        for (i = pCache->iCacheMaxEntries; i > 0; i--)
        {
            if (pEntry->dwRefresh < 0x80000000)
                pEntry->dwRefresh = 0;
            else
                pEntry->dwRefresh -= 0x80000000;
            pEntry = pEntry->pNextEntry;
        }
    }

    // No need to deallocate old entries
    if (pCache->iCacheEntries < DL_MAX_COMBINED_KERNELS)
    {
        return true;
    }

    for (i = pCache->iCacheMaxEntries; i > 0; i--)
    {
        // deallocate old unreferenced entries
        if (pEntry->iKCID != -1 && pEntry->dwLoaded == 0)
        {
            if (pEntry->dwRefresh < dwOldest)
            {
                pOldest  = pEntry;
                dwOldest = pEntry->dwRefresh;
                wEntry   = pEntry->wHashEntry;
            }
        }
        pEntry = pEntry->pNextEntry;
    }

    // No entry to release, sanity checks
    pHashEntry += wEntry;
    if (!pOldest ||
        wEntry == 0 ||
        pHashEntry->pCacheEntry != pOldest)
    {
        VPHAL_RENDER_ASSERT(false);
        return false;
    }

    // Release hash and cache entries
    KernelDll_ReleaseHashEntry(pHashTable, wEntry);
    KernelDll_ReleaseCacheEntry(pCache, pOldest);

    return true;
}

//--------------------------------------------------------------
// KernelDll_AllocateAdditionalCacheEntries - Allocate more kernel cache entries
//--------------------------------------------------------------
Kdll_CacheEntry *
KernelDll_AllocateAdditionalCacheEntries(Kdll_KernelCache *pCache)
{
    Kdll_CacheEntry *pNewEntry = nullptr;
    Kdll_CacheEntry *pChcheEntry;
    int i, j;

    VPHAL_RENDER_FUNCTION_ENTER;

    // Check num
    if (pCache->iCacheEntries + DL_NEW_COMBINED_KERNELS > DL_MAX_COMBINED_KERNELS)
    {
        VPHAL_RENDER_ASSERTMESSAGE("KernelDll_AllocateAdditionalCacheEntries: Can't allocate more kernel cache entries\n");
        return nullptr;
    }

    // Allocate the new entires
    i = (sizeof(Kdll_CacheEntry) + DL_CACHE_BLOCK_SIZE) * DL_NEW_COMBINED_KERNELS;
    pNewEntry = (Kdll_CacheEntry *)MOS_AllocAndZeroMemory(i);
    if (!pNewEntry)
    {
        VPHAL_RENDER_ASSERTMESSAGE("KernelDll_AllocateAdditionalCacheEntries: Failed to allocate kernel cache entries\n");
        return nullptr;
    }

    // Update the cache entires
    pChcheEntry = pCache->pCacheEntries;
    for(j = 0; j < pCache->iCacheMaxEntries - 1; j++)
    {
        pChcheEntry = pChcheEntry->pNextEntry;
    }
    pChcheEntry->pNextEntry = pNewEntry;
    for(j = 0; j < DL_NEW_COMBINED_KERNELS; j++, pNewEntry++)
    {
        pNewEntry->iKUID   = -1;
        pNewEntry->iKCID   = -1;
        pNewEntry->pBinary = (uint8_t *)(pNewEntry + DL_NEW_COMBINED_KERNELS - j) + j * DL_CACHE_BLOCK_SIZE;
        if(j != DL_NEW_COMBINED_KERNELS - 1)
        {
            pNewEntry->pNextEntry = pNewEntry + 1;
        }
        else
        {
            pNewEntry->pNextEntry = nullptr;
        }
    }

    pCache->iCacheMaxEntries += DL_NEW_COMBINED_KERNELS;
    pCache->iCacheSize       += DL_NEW_COMBINED_KERNELS * DL_CACHE_BLOCK_SIZE;
    pCache->iCacheFree       += DL_NEW_COMBINED_KERNELS * DL_CACHE_BLOCK_SIZE;
    return (Kdll_CacheEntry *)(pNewEntry - DL_NEW_COMBINED_KERNELS);
}

//--------------------------------------------------------------
// KernelDll_AllocateCacheEntry - Allocate cache entry for a given size
//--------------------------------------------------------------
Kdll_CacheEntry *
KernelDll_AllocateCacheEntry(Kdll_KernelCache *pCache, int32_t iSize)
{
    Kdll_CacheEntry *pEntry          = pCache->pCacheEntries;
    uint8_t *pCacheBinary               = nullptr;
    Kdll_CacheEntry *pCacheNextEntry = nullptr;
    int32_t i, j;

    VPHAL_RENDER_FUNCTION_ENTER;

    // Check size
    if (iSize > DL_CACHE_BLOCK_SIZE)
    {
        return nullptr;
    }

    // Search empty entry
    j = pCache->iCacheMaxEntries;
    for (i = 0; i < j; i++)
    {
        if (pEntry->iKCID == -1)
        {
            break;
        }
        pEntry = pEntry->pNextEntry;
    }
    if (i == j)
    {
        // Try to allocate more cache entries
        pEntry = KernelDll_AllocateAdditionalCacheEntries(pCache);
        if(! pEntry)
        {
            return nullptr;
        }
    }

    // Reset entry
    pCacheBinary    = pEntry->pBinary;
    pCacheNextEntry = pEntry->pNextEntry;
    MOS_ZeroMemory(pEntry, sizeof(Kdll_CacheEntry));
    pEntry->iSize   = iSize;
    pEntry->pBinary    = pCacheBinary;
    pEntry->pNextEntry = pCacheNextEntry;

    // Increment entries
    pCache->iCacheEntries++;
    return pEntry;
}

//--------------------------------------------------------------
// Kerneldll_GetComponentKernel - Get component/static kernel
//                                entry from cache
//--------------------------------------------------------------
Kdll_CacheEntry *
KernelDll_GetComponentKernel(Kdll_State *pState,
                             int32_t     iKUID)
{
    Kdll_CacheEntry *pEntry = nullptr;

    if (iKUID < pState->ComponentKernelCache.iCacheMaxEntries)
    {
        pEntry = &(pState->ComponentKernelCache.pCacheEntries[iKUID]);
        if (pEntry->iKUID != iKUID ||
            pEntry->pBinary == nullptr ||
            pEntry->iSize == 0)
        {
            pEntry = nullptr;
        }
    }

    return pEntry;
}

//--------------------------------------------------------------
// KernelDll_AddKernel - Add kernel into hash table and kernel cache
//--------------------------------------------------------------
Kdll_CacheEntry *
KernelDll_AddKernel(Kdll_State       *pState,           // Kernel Dll state
                    Kdll_SearchState *pSearchState,     // Search state
                    Kdll_FilterEntry *pFilter,          // Original filter
                    int32_t           iFilterSize,      // Original filter size
                    uint32_t          dwHash)
{
    Kdll_CacheEntry      *pCacheEntry;
    Kdll_KernelHashTable *pHashTable;
    Kdll_KernelHashEntry *pHashEntry;
    uint16_t entry;
    int32_t size;
    uint8_t *ptr;

    VPHAL_RENDER_FUNCTION_ENTER;

    // Check kernel
    if (pSearchState->KernelSize <= 0)
    {
        return nullptr;
    }

    // Get hash table
    pHashTable = &pState->KernelHashTable;
    pHashEntry = &pHashTable->HashEntry[0] - 1;  // all indices are 1 based (0 = null)

    // allocate space in kernel cache to store the kernel, filter, CSC parameters
    size  = pSearchState->KernelSize +                                  // Kernel
            pSearchState->iFilterSize * sizeof(Kdll_FilterEntry) * 2 +  // Original + Modified Filter
            sizeof(Kdll_CSC_Params) +                                   // CSC parameters
            sizeof(VPHAL_CSPACE);                                       // Intermediate Color Space for colorfill

    // Run garbage collection, create space for new kernel and metadata
    KernelDll_GarbageCollection(pState, size);

    // Get new kernel cache entry
    pCacheEntry = KernelDll_AllocateCacheEntry(&pState->KernelCache, size);
    if (!pCacheEntry)
    {
        VPHAL_RENDER_ASSERTMESSAGE("Failed to allocate cache space for new kernel.");
        return nullptr;
    }

    // Get hash entry
    entry = KernelDll_AllocateHashEntry(pHashTable, dwHash);
    if (!entry)
    {
        VPHAL_RENDER_ASSERTMESSAGE("Failed to allocate hash entry for new kernel.");
        KernelDll_ReleaseCacheEntry(&pState->KernelCache, pCacheEntry);
        return nullptr;
    }

    // Setup cache entry, copy kernel
    pCacheEntry->iKUID       = -1;
    pCacheEntry->iKCID       = pState->KernelCache.iCacheID;  // Create new kernel cache id (KCID)
    pCacheEntry->dwRefresh   = pState->dwRefresh++;
    pCacheEntry->wHashEntry  = entry;

    // Save kernel
    pCacheEntry->iSize = pSearchState->KernelSize;
    MOS_SecureMemcpy(pCacheEntry->pBinary, pSearchState->KernelSize, (void *)pSearchState->Kernel, pSearchState->KernelSize);
    ptr = pCacheEntry->pBinary + pSearchState->KernelSize;

    // Save modified filter
    pCacheEntry->iFilterSize = pSearchState->iFilterSize;
    pCacheEntry->pFilter     = (Kdll_FilterEntry *) (ptr);
    MOS_SecureMemcpy(ptr, pSearchState->iFilterSize * sizeof(Kdll_FilterEntry), (void *)pSearchState->Filter, pSearchState->iFilterSize * sizeof(Kdll_FilterEntry));
    ptr += pSearchState->iFilterSize * sizeof(Kdll_FilterEntry);

    // Save CSC parameters associated with the kernel
    pCacheEntry->pCscParams = (Kdll_CSC_Params *) (ptr);
    MOS_SecureMemcpy(ptr, sizeof(Kdll_CSC_Params), (void *)&pSearchState->CscParams, sizeof(Kdll_CSC_Params));
    ptr += sizeof(Kdll_CSC_Params);
    // Save intermediate color space for colorfill
    pCacheEntry->colorfill_cspace = pState->colorfill_cspace;
    ptr += sizeof(VPHAL_CSPACE);

    // increment KCID (Range = 0x00010000 - 0x7fffffff)
    pState->KernelCache.iCacheID = 0x00010000 + (pState->KernelCache.iCacheID - 0x0000ffff) % 0x7fff0000;

    // Setup hash entry, copy filter
    pHashEntry += entry;
    pHashEntry->pCacheEntry = pCacheEntry;

    // Save original filter for search purposes - modified filter is used for rendering
    pHashEntry->iFilter     = iFilterSize;
    pHashEntry->pFilter     = (Kdll_FilterEntry *) (ptr);
    MOS_SecureMemcpy(ptr, iFilterSize * sizeof(Kdll_FilterEntry), (void *)pFilter, iFilterSize * sizeof(Kdll_FilterEntry));

    return pCacheEntry;
}

#ifdef __cplusplus
}
#endif // __cplusplus