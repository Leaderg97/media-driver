/*===================== begin_copyright_notice ==================================

# Copyright (c) 2022, Intel Corporation

# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.

======================= end_copyright_notice ==================================*/
//!
//! \file     vphal_renderer_xe_hpm.cpp
//! \brief    VPHAL top level rendering component and the entry to low level renderers
//! \details  The top renderer is responsible for coordinating the sequence of calls to low level renderers, e.g. DNDI or Comp
//!
#include "vphal_renderer_xe_hpm.h"
#if defined(ENABLE_KERNELS)
#include "igvpkrn_xe_hpg.h"
#include "igvpkrn_xe_hpg_cmfcpatch.h"
#endif
#include "vphal_render_vebox_xe_hpm.h"
#include "vphal_render_composite_xe_xpm.h"

extern const Kdll_RuleEntry         g_KdllRuleTable_Xe_Hpm[];

void VphalRendererXe_Hpm::GetCacheCntl(
    PMOS_INTERFACE                      pOsInterface,
    PLATFORM                            *pPlatform,
    MEDIA_FEATURE_TABLE                 *pSkuTable,
    PVPHAL_RENDER_CACHE_CNTL            pSettings)

{
    MOS_HW_RESOURCE_DEF                 Usage;
    MEMORY_OBJECT_CONTROL_STATE         MemObjCtrl;

    VPHAL_RENDER_CHK_NULL_NO_STATUS_RETURN(pSettings);

    if (pSettings->bCompositing)
    {
        pSettings->Composite.bL3CachingEnabled = true;

        VPHAL_SET_SURF_MEMOBJCTL(pSettings->Composite.PrimaryInputSurfMemObjCtl,   MOS_MP_RESOURCE_USAGE_SurfaceState_RCS);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->Composite.InputSurfMemObjCtl,          MOS_MP_RESOURCE_USAGE_SurfaceState_RCS);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->Composite.TargetSurfMemObjCtl,         MOS_MP_RESOURCE_USAGE_DEFAULT_RCS);
    }
    else
    {
        pSettings->Composite.bL3CachingEnabled = false;

        VPHAL_SET_SURF_MEMOBJCTL(pSettings->Composite.PrimaryInputSurfMemObjCtl, MOS_MP_RESOURCE_USAGE_DEFAULT);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->Composite.InputSurfMemObjCtl,        MOS_MP_RESOURCE_USAGE_DEFAULT);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->Composite.TargetSurfMemObjCtl,       MOS_MP_RESOURCE_USAGE_DEFAULT);
    }

    if (pSettings->bDnDi)
    {
        pSettings->DnDi.bL3CachingEnabled = false;

        VPHAL_SET_SURF_MEMOBJCTL(pSettings->DnDi.CurrentInputSurfMemObjCtl,        MOS_MP_RESOURCE_USAGE_SurfaceState_FF);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->DnDi.PreviousInputSurfMemObjCtl,       MOS_MP_RESOURCE_USAGE_SurfaceState_FF);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->DnDi.STMMInputSurfMemObjCtl,           MOS_MP_RESOURCE_USAGE_SurfaceState_FF);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->DnDi.STMMOutputSurfMemObjCtl,          MOS_MP_RESOURCE_USAGE_SurfaceState_FF);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->DnDi.DnOutSurfMemObjCtl,               MOS_MP_RESOURCE_USAGE_SurfaceState_FF);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->DnDi.CurrentOutputSurfMemObjCtl,       MOS_MP_RESOURCE_USAGE_SurfaceState_FF);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->DnDi.StatisticsOutputSurfMemObjCtl,    MOS_MP_RESOURCE_USAGE_SurfaceState_FF);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->DnDi.AlphaOrVignetteSurfMemObjCtl,     MOS_MP_RESOURCE_USAGE_SurfaceState_FF);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->DnDi.LaceOrAceOrRgbHistogramSurfCtrl,  MOS_MP_RESOURCE_USAGE_SurfaceState_FF);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->DnDi.SkinScoreSurfMemObjCtl,           MOS_MP_RESOURCE_USAGE_SurfaceState_FF);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->DnDi.LaceLookUpTablesSurfMemObjCtl,    MOS_MP_RESOURCE_USAGE_SurfaceState_FF);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->DnDi.Vebox3DLookUpTablesSurfMemObjCtl, MOS_MP_RESOURCE_USAGE_SurfaceState_FF);
    }
    else
    {
        pSettings->DnDi.bL3CachingEnabled = false;

        VPHAL_SET_SURF_MEMOBJCTL(pSettings->DnDi.CurrentInputSurfMemObjCtl,        MOS_MP_RESOURCE_USAGE_DEFAULT);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->DnDi.PreviousInputSurfMemObjCtl,       MOS_MP_RESOURCE_USAGE_DEFAULT);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->DnDi.STMMInputSurfMemObjCtl,           MOS_MP_RESOURCE_USAGE_DEFAULT);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->DnDi.STMMOutputSurfMemObjCtl,          MOS_MP_RESOURCE_USAGE_DEFAULT);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->DnDi.DnOutSurfMemObjCtl,               MOS_MP_RESOURCE_USAGE_DEFAULT);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->DnDi.CurrentOutputSurfMemObjCtl,       MOS_MP_RESOURCE_USAGE_DEFAULT);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->DnDi.StatisticsOutputSurfMemObjCtl,    MOS_MP_RESOURCE_USAGE_DEFAULT);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->DnDi.AlphaOrVignetteSurfMemObjCtl,     MOS_MP_RESOURCE_USAGE_DEFAULT);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->DnDi.LaceOrAceOrRgbHistogramSurfCtrl,  MOS_MP_RESOURCE_USAGE_DEFAULT);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->DnDi.SkinScoreSurfMemObjCtl,           MOS_MP_RESOURCE_USAGE_DEFAULT);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->DnDi.LaceLookUpTablesSurfMemObjCtl,    MOS_MP_RESOURCE_USAGE_DEFAULT);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->DnDi.Vebox3DLookUpTablesSurfMemObjCtl, MOS_MP_RESOURCE_USAGE_DEFAULT);
    }

    if (pSettings->bLace)
    {
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->Lace.FrameHistogramSurfaceMemObjCtl,                       MOS_MP_RESOURCE_USAGE_SurfaceState_RCS);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->Lace.AggregatedHistogramSurfaceMemObjCtl,                  MOS_MP_RESOURCE_USAGE_SurfaceState_RCS);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->Lace.StdStatisticsSurfaceMemObjCtl,                        MOS_MP_RESOURCE_USAGE_SurfaceState_RCS);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->Lace.PwlfInSurfaceMemObjCtl,                               MOS_MP_RESOURCE_USAGE_SurfaceState_RCS);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->Lace.PwlfOutSurfaceMemObjCtl,                              MOS_MP_RESOURCE_USAGE_SurfaceState_RCS);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->Lace.WeitCoefSurfaceMemObjCtl,                             MOS_MP_RESOURCE_USAGE_SurfaceState_RCS);
    }
    else
    {
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->Lace.FrameHistogramSurfaceMemObjCtl,                       MOS_MP_RESOURCE_USAGE_DEFAULT);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->Lace.AggregatedHistogramSurfaceMemObjCtl,                  MOS_MP_RESOURCE_USAGE_DEFAULT);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->Lace.StdStatisticsSurfaceMemObjCtl,                        MOS_MP_RESOURCE_USAGE_DEFAULT);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->Lace.PwlfInSurfaceMemObjCtl,                               MOS_MP_RESOURCE_USAGE_DEFAULT);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->Lace.PwlfOutSurfaceMemObjCtl,                              MOS_MP_RESOURCE_USAGE_DEFAULT);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->Lace.WeitCoefSurfaceMemObjCtl,                             MOS_MP_RESOURCE_USAGE_DEFAULT);
        VPHAL_SET_SURF_MEMOBJCTL(pSettings->Lace.GlobalToneMappingCurveLUTSurfaceMemObjCtl,            MOS_MP_RESOURCE_USAGE_DEFAULT);
    }

}

MOS_STATUS VphalRendererXe_Hpm::AllocateRenderComponents(
        PMHW_VEBOX_INTERFACE                pVeboxInterface,
        PMHW_SFC_INTERFACE                  pSfcInterface)
{
    MOS_STATUS              eStatus;
    VPHAL_RENDER_CACHE_CNTL CacheCntl;

    VPHAL_RENDER_CHK_NULL_RETURN(m_pRenderHal);

    eStatus = MOS_STATUS_SUCCESS;

    // Get the cache settings
    MOS_ZeroMemory(&CacheCntl, sizeof(CacheCntl));

    CacheCntl.bDnDi        = true;
    CacheCntl.bCompositing = true;

    VPHAL_RENDERER_GET_CACHE_CNTL(this,
        m_pOsInterface,
        &m_pRenderHal->Platform,
        m_pSkuTable,
        &CacheCntl);

    // Initialize Advanced Processing Interface
    pRender[VPHAL_RENDER_ID_VEBOX] = MOS_New(
        VPHAL_VEBOX_STATE_XE_HPM,
        m_pOsInterface,
        pVeboxInterface,
        pSfcInterface,
        m_pRenderHal,
        &VeboxExecState[0],
        &PerfData,
        CacheCntl.DnDi,
        &eStatus);
    if (!pRender[VPHAL_RENDER_ID_VEBOX] ||
        (eStatus != MOS_STATUS_SUCCESS))
    {
        eStatus = MOS_STATUS_NO_SPACE;
        VPHAL_RENDER_ASSERTMESSAGE("Allocate Vebox Render Fail.");
        return eStatus;
    }

    pRender[VPHAL_RENDER_ID_VEBOX2] = MOS_New(
        VPHAL_VEBOX_STATE_XE_HPM,
        m_pOsInterface,
        pVeboxInterface,
        pSfcInterface,
        m_pRenderHal,
        &VeboxExecState[1],
        &PerfData,
        CacheCntl.DnDi,
        &eStatus);
    if (!pRender[VPHAL_RENDER_ID_VEBOX2] ||
        (eStatus != MOS_STATUS_SUCCESS))
    {
        eStatus = MOS_STATUS_NO_SPACE;
        VPHAL_RENDER_ASSERTMESSAGE("Allocate Vebox Render Fail.");
        return eStatus;
    }

    // Allocate Composite State
    pRender[VPHAL_RENDER_ID_COMPOSITE] = MOS_New(
        CompositeStateXe_Xpm,
        m_pOsInterface,
        m_pRenderHal,
        &PerfData,
        CacheCntl.Composite,
        &eStatus);
    if (!pRender[VPHAL_RENDER_ID_COMPOSITE] ||
        (eStatus != MOS_STATUS_SUCCESS))
    {
        eStatus = MOS_STATUS_NO_SPACE;
        VPHAL_RENDER_ASSERTMESSAGE("Allocate Composite Render Fail.");
        return eStatus;
    }

    return eStatus;
}

MOS_STATUS VphalRendererXe_Hpm::InitKdllParam()
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    // Override kernel binary for CMFC/SWSB
    if (bEnableCMFC)
    {
        pKernelDllRules  = g_KdllRuleTable_Xe_Hpm;
#if defined(ENABLE_KERNELS)
        pcKernelBin      = (const void *)IGVPKRN_XE_HPG;
        dwKernelBinSize  = IGVPKRN_XE_HPG_SIZE;
        pcFcPatchBin     = (const void *)IGVPKRN_XE_HPG_CMFCPATCH;
        dwFcPatchBinSize = IGVPKRN_XE_HPG_CMFCPATCH_SIZE;
#else
        pcKernelBin      = nullptr;
        dwKernelBinSize  = 0;
        pcFcPatchBin     = nullptr;
        dwFcPatchBinSize = 0;
#endif
    }

    if ((NULL == pcFcPatchBin) || (0 == dwFcPatchBinSize))
    {
        bEnableCMFC = false;
    }

    if (bEnableCMFC && (NULL != pcFcPatchBin) && (0 != dwFcPatchBinSize))
    {
        m_pRenderHal->bEnableP010SinglePass = true;
    }
    else
    {
        m_pRenderHal->bEnableP010SinglePass = false;
    }

    return eStatus;
}

MOS_STATUS VphalRendererXe_Hpm::Render(
    PCVPHAL_RENDER_PARAMS   pcRenderParams)
{
    MOS_STATUS              eStatus;
    PMOS_INTERFACE          pOsInterface;
    PRENDERHAL_INTERFACE    pRenderHal;
    VPHAL_RENDER_PARAMS     RenderParams;                                       // Make a copy of render params
    PVPHAL_SURFACE          pSrcLeft[VPHAL_MAX_SOURCES];                        // Array of sources referring to left view stereo content
    PVPHAL_SURFACE          pSrcRight[VPHAL_MAX_SOURCES];                       // Array of sources referring to right view stereo content
    uint32_t                uiRenderPasses;                                     // Number of rendering passes in this one call to VpHal_RndrRender()
    uint32_t                uiCurrentRenderPass;                                // Current render pass
    uint32_t                uiDst;
    VPHAL_GET_SURFACE_INFO  Info;

    //--------------------------------------------
    VPHAL_RENDER_ASSERT(pcRenderParams);
    VPHAL_RENDER_ASSERT(m_pOsInterface);
    VPHAL_RENDER_ASSERT(m_pRenderHal);
    VPHAL_RENDER_ASSERT(pKernelDllState);
    VPHAL_RENDER_ASSERT(pRender[VPHAL_RENDER_ID_COMPOSITE]);
    //--------------------------------------------

    eStatus         = MOS_STATUS_SUCCESS;
    pOsInterface    = m_pOsInterface;
    pRenderHal      = m_pRenderHal;

    // Validate render target
    if (pcRenderParams->pTarget[0] == nullptr ||
        Mos_ResourceIsNull(&(pcRenderParams->pTarget[0]->OsResource)))
    {
        VPHAL_RENDER_ASSERTMESSAGE("Invalid Render Target.");
        eStatus = MOS_STATUS_UNKNOWN;
        goto finish;
    }

    // Protection mechanism, Only KBL+ support P010 output.
    if (IsFormatSupported(pcRenderParams) == false)
    {
        VPHAL_RENDER_ASSERTMESSAGE("Invalid Render Target Output Format.");
        eStatus = MOS_STATUS_UNKNOWN;
        goto finish;
    }

    VPHAL_DBG_STATE_DUMP_SET_CURRENT_FRAME_COUNT(uiFrameCounter);

    // Validate max number sources
    if (pcRenderParams->uSrcCount > VPHAL_MAX_SOURCES)
    {
        VPHAL_RENDER_ASSERTMESSAGE("Invalid number of samples.");
        eStatus = MOS_STATUS_UNKNOWN;
        goto finish;
    }

    // Validate max number targets
    if (pcRenderParams->uDstCount > VPHAL_MAX_TARGETS)
    {
        VPHAL_RENDER_ASSERTMESSAGE("Invalid number of targets.");
        eStatus = MOS_STATUS_UNKNOWN;
        goto finish;
    }

    // Copy the Render Params structure (so we can update it)
    RenderParams = *pcRenderParams;

    VPHAL_DBG_PARAMETERS_DUMPPER_DUMP_XML(&RenderParams);
    VPHAL_DBG_OCA_DUMPER_SET_RENDER_PARAM(pRenderHal, &RenderParams);

    // Get resource information for render target
    MOS_ZeroMemory(&Info, sizeof(VPHAL_GET_SURFACE_INFO));

    for (uiDst = 0; uiDst < RenderParams.uDstCount; uiDst++)
    {
        VPHAL_RENDER_CHK_STATUS(VpHal_GetSurfaceInfo(
            m_pOsInterface,
            &Info,
            RenderParams.pTarget[uiDst]));
    }

    // Set the component info
    m_pOsInterface->Component = pcRenderParams->Component;

    // Init component(DDI entry point) info for perf measurement
    m_pOsInterface->pfnSetPerfTag(m_pOsInterface, VPHAL_NONE);

    // Increment frame ID for performance measurement
    m_pOsInterface->pfnIncPerfFrameID(m_pOsInterface);

    // Enable Turbo mode if sku present and DDI requests it
    if (m_pSkuTable && MEDIA_IS_SKU(m_pSkuTable, FtrMediaTurboMode))
    {
        m_pRenderHal->bTurboMode = RenderParams.bTurboMode;
    }

    // Reset feature reporting
    m_reporting->InitReportValue();

    MOS_ZeroMemory(pSrcLeft,  sizeof(PVPHAL_SURFACE) * VPHAL_MAX_SOURCES);
    MOS_ZeroMemory(pSrcRight, sizeof(PVPHAL_SURFACE) * VPHAL_MAX_SOURCES);

    VPHAL_RENDER_CHK_STATUS(PrepareSources(
        &RenderParams,
        pSrcLeft,
        pSrcRight,
        &uiRenderPasses));

    //set GpuContext
    VPHAL_RENDER_CHK_STATUS(SetRenderGpuContext(RenderParams));

    // align rectangle and source surface
    for (uiDst = 0; uiDst < RenderParams.uDstCount; uiDst++)
    {
        VPHAL_RENDER_CHK_STATUS(VpHal_RndrRectSurfaceAlignment(RenderParams.pTarget[uiDst], RenderParams.pTarget[uiDst]->Format));
    }

    for (uiCurrentRenderPass = 0;
        uiCurrentRenderPass < uiRenderPasses;
        uiCurrentRenderPass++)
    {
        // Assign source surfaces for current rendering pass
        MOS_SecureMemcpy(
            RenderParams.pSrc,
            sizeof(PVPHAL_SURFACE) * VPHAL_MAX_SOURCES,
            (uiCurrentRenderPass == 0) ? pSrcLeft : pSrcRight,
            sizeof(PVPHAL_SURFACE) * VPHAL_MAX_SOURCES);

        MOS_ZeroMemory(&Info, sizeof(VPHAL_GET_SURFACE_INFO));

        for (uiDst = 0; uiDst < RenderParams.uDstCount; uiDst++)
        {
            Info.S3dChannel = RenderParams.pTarget[uiDst]->Channel;
            Info.ArraySlice = uiCurrentRenderPass;

            VPHAL_RENDER_CHK_STATUS(VpHal_GetSurfaceInfo(
                m_pOsInterface,
                &Info,
                RenderParams.pTarget[uiDst]));
        }

        // Update channel. 0 = mono or stereo left, 1 = stereo right
        uiCurrentChannel = uiCurrentRenderPass;

        // WA to void colorfill + rotation output cropution when Eu fusion is on.
        bool bColorFill = false;
        bool bRotation  = false;
        if ((pcRenderParams->pTarget[0] != nullptr)
            && (pcRenderParams->pSrc[0] != nullptr)
            && (pcRenderParams->pColorFillParams != nullptr))
        {
            bColorFill =  (!RECT1_CONTAINS_RECT2(pcRenderParams->pSrc[0]->rcDst, pcRenderParams->pTarget[0]->rcDst)) ? true : false;
            bRotation = (pcRenderParams->pSrc[0]->Rotation != VPHAL_ROTATION_IDENTITY) ? true : false;
            m_pRenderHal->eufusionBypass = bColorFill && bRotation;

            VPHAL_RENDER_NORMALMESSAGE("eufusionBypass = %d", pRenderHal->eufusionBypass ? 1 : 0);
        }
        // for interlaced scaling : two field --> one interleaved mode
        if (pcRenderParams->pSrc[0]->InterlacedScalingType == ISCALING_FIELD_TO_INTERLEAVED)
        {
            RenderParams.pSrc[0]->rcDst.bottom                   = RenderParams.pSrc[0]->rcDst.bottom / 2;
            RenderParams.pSrc[0]->pBwdRef->rcDst.bottom          = RenderParams.pSrc[0]->rcDst.bottom;
            RenderParams.pSrc[0]->pBwdRef->InterlacedScalingType = ISCALING_FIELD_TO_INTERLEAVED;
            if (RenderParams.pSrc[0]->SampleType == SAMPLE_SINGLE_TOP_FIELD)
            {
                RenderParams.pSrc[0]->pBwdRef->SampleType = SAMPLE_SINGLE_BOTTOM_FIELD;
                RenderParams.pTarget[0]->SampleType       = SAMPLE_INTERLEAVED_EVEN_FIRST_TOP_FIELD;
            }
            else
            {
                RenderParams.pSrc[0]->pBwdRef->SampleType = SAMPLE_SINGLE_TOP_FIELD;
                RenderParams.pTarget[0]->SampleType       = SAMPLE_INTERLEAVED_ODD_FIRST_BOTTOM_FIELD;
            }
            if (RenderParams.pSrc[0]->pBwdRef->pDeinterlaceParams)
            {
                MOS_FreeMemory(RenderParams.pSrc[0]->pBwdRef->pDeinterlaceParams);
                RenderParams.pSrc[0]->pBwdRef->pDeinterlaceParams = nullptr;
            }

            for (int uiField = 0; uiField < 2; uiField++)
            {
                if (uiField == 0)
                {
                    VPHAL_RENDER_CHK_STATUS(RenderPass(&RenderParams));
                }
                else
                {
                    RenderParams.pSrc[0] = RenderParams.pSrc[0]->pBwdRef;
                    RenderParams.pSrc[0]->SurfType = SURF_IN_PRIMARY;
                    VPHAL_RENDER_CHK_STATUS(RenderPass(&RenderParams));
                }
            }
        }
        else
        {
            VPHAL_RENDER_CHK_STATUS(RenderScaling(&RenderParams));
            VPHAL_RENDER_CHK_STATUS(RenderPass(&RenderParams));
        }
    }
finish:
    uiFrameCounter++;
    return eStatus;
}

//!
//! \brief    Allocate surface dumper
//! \return   MOS_STATUS
//!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
//!
MOS_STATUS VphalRendererXe_Hpm::CreateSurfaceDumper()
{
#if (_DEBUG || _RELEASE_INTERNAL)
    // Initialize Surface Dumper
    VPHAL_DBG_SURF_DUMP_CREATE_XE_XPM();
    VPHAL_RENDER_CHK_NULL_RETURN(m_surfaceDumper);

#if !EMUL
    VphalSurfaceDumperXe_Xpm *pSurfaceDumperXe_Xpm = dynamic_cast<VphalSurfaceDumperXe_Xpm *>(m_surfaceDumper);
    VPHAL_RENDER_CHK_NULL_RETURN(pSurfaceDumperXe_Xpm);
    VPHAL_RENDER_CHK_NULL_RETURN(pSurfaceDumperXe_Xpm->GetBltState());
#endif
#endif
    return MOS_STATUS_SUCCESS;
}

//!
//! \brief    Scaling function
//! \details  The scaling function is only for scaling without other VP features.
//!           Down scaling needs 2 pass if scaling ratio is >2 for better quality.
//!           Pass#1 DS to 1/2 target resolution; Pass #2: DS from 1/2 target resolution to target resolution
//! \param    [in,out] pRenderParams
//!           Pointer to VPHAL render parameter
//! \return   MOS_STATUS
//!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
//!
MOS_STATUS VphalRendererXe_Hpm::RenderScaling(
    PVPHAL_RENDER_PARAMS    pRenderParams)
{
    MOS_STATUS                  eStatus                 = MOS_STATUS_SUCCESS;
    PVPHAL_SURFACE              pSource                 = nullptr;              // Pointer to the primary and original source surface
    PVPHAL_SURFACE              pTarget                 = nullptr;              // Pointer to the target surface
    float                       fScaleX                 = 0.0;                  // The original scaling ratio in X axis
    float                       fScaleY                 = 0.0;                  // the original scaling ratio in Y axis
    PLATFORM                    Platform                = {};
    bool                        bScalingFirst           = false;                // Need to scaling first or not?
    bool                        b2PassScaling           = false;                // 2 Pass scaling
    uint32_t                    dwAllocatedWidth        = 1280;                 // The width of intermediate surfaces
    uint32_t                    dwAllocatedHeight       = 720;                  // The height of intermediate surfaces
    RECT                        rectScalingRegion       = {0, 0, 1280, 720};    // Scaling region of down scaling
    VPHAL_RENDER_PARAMS         renderParams            = {};
    VPHAL_SURFACE               inputSurface            = {};
    PVPHAL_3DLUT_PARAMS         p3DLutParams            = nullptr;
    PVPHAL_SURFACE              pDSSurface              = nullptr;              // Always point to the down scaled surface
    uint32_t                    dwHalfInWidth           = 1280;                 // Half of the processed input width
    uint32_t                    dwHalfInHeight          = 720;                  // Half of the processed input height
    RECT                        rectHalfInRegion        = {0, 0, 1280, 720};    // Half of the processed input region

    VPHAL_RENDER_CHK_NULL(pRenderParams);
    VPHAL_RENDER_CHK_NULL(pRenderParams->pSrc);
    VPHAL_RENDER_CHK_NULL(pRenderParams->pTarget);
    VPHAL_RENDER_CHK_NULL(m_pSkuTable);
    VPHAL_RENDER_CHK_NULL(m_pOsInterface);

    // Limited to 1 input and 1 output. If not 1->1, fall back to the typical video processing path.
    if ((pRenderParams->uSrcCount != 1) || (pRenderParams->uDstCount != 1))
    {
        VPHAL_RENDER_NORMALMESSAGE(" Source Count %d, Destination Count %d", pRenderParams->uSrcCount, pRenderParams->uDstCount);
        eStatus = MOS_STATUS_SUCCESS;
        goto finish;
    }

    pSource     = pRenderParams->pSrc[0];
    pTarget     = pRenderParams->pTarget[0];
    VPHAL_RENDER_CHK_NULL(pSource);
    VPHAL_RENDER_CHK_NULL(pTarget);

    // Calculating the scaling ratio
    fScaleX = (float)(pSource->rcDst.right - pSource->rcDst.left) /
              (float)(pSource->rcSrc.right - pSource->rcSrc.left);
    fScaleY = (float)(pSource->rcDst.bottom - pSource->rcDst.top) /
              (float)(pSource->rcSrc.bottom - pSource->rcSrc.top);
    b2PassScaling = (fScaleX < 0.5f) && (fScaleY < 0.5f);

    // Scaling first, then other VP features. It is enabled on server for HDR transcoding with 3DLUT enabled if down scaling as of now.
    // 2 pass down scaling may be changed later according to the quality anaysis.
    m_pOsInterface->pfnGetPlatform(m_pOsInterface, &Platform);
    bScalingFirst = MEDIA_IS_SKU(m_pSkuTable, FtrScalingFirst) &&
                     (pSource->p3DLutParams != nullptr)        &&
                     (fScaleX < 1.0f && fScaleY < 1.0f);

    // If no need to scaling firstly, fall back to the typical video processing path.
    if (!bScalingFirst)
    {
        eStatus = MOS_STATUS_SUCCESS;
        goto finish;
    }

    // Do the down scaling firstly, then 3DLUT VEBOX features.
    // Allocate down scaling surfaces
    for (uint32_t nIndex = 0; nIndex < VPHAL_MAX_NUM_DS_SURFACES; nIndex++)
    {
         if (m_pDSSurface[nIndex] == nullptr)
         {
             m_pDSSurface[nIndex] = (PVPHAL_SURFACE)MOS_AllocAndZeroMemory(sizeof(VPHAL_SURFACE));
             VPHAL_RENDER_CHK_NULL(m_pDSSurface[nIndex]);
         }
    }

    // Calculate the size of processing region for 2 pass downscaling
    dwHalfInWidth                   = (uint32_t)((0.5) * (float)(pSource->rcSrc.right - pSource->rcSrc.left));
    dwHalfInHeight                  = (uint32_t)((0.5) * (float)(pSource->rcSrc.bottom- pSource->rcSrc.top));
    dwHalfInWidth                   = MOS_ALIGN_CEIL(dwHalfInWidth, 4);
    dwHalfInHeight                  = MOS_ALIGN_CEIL(dwHalfInHeight, 4);
    rectHalfInRegion.top            = 0;
    rectHalfInRegion.left           = 0;
    rectHalfInRegion.right          = dwHalfInWidth;
    rectHalfInRegion.bottom         = dwHalfInHeight;

    // Allocate intermediate surface for the first pass
    dwAllocatedWidth                = MOS_MAX(dwHalfInWidth, pTarget->dwWidth);
    dwAllocatedHeight               = MOS_MAX(dwHalfInHeight, pTarget->dwHeight);
    VPHAL_RENDER_CHK_STATUS(AllocateSurface(pRenderParams, pSource, m_pDSSurface[0], dwAllocatedWidth, dwAllocatedHeight, pSource->Format));
    // First pass scaling
    {
        // Use inputSurface instead of the pointer of the original input to keep it unchanged.
        rectScalingRegion               = (b2PassScaling) ? rectHalfInRegion : pSource->rcDst;
        p3DLutParams                    = pSource->p3DLutParams;
        renderParams                    = *pRenderParams;
        inputSurface                    = *pSource;
        inputSurface.p3DLutParams       = nullptr;
        inputSurface.rcDst              = rectScalingRegion;
        m_pDSSurface[0]->rcSrc          = rectScalingRegion;
        m_pDSSurface[0]->rcDst          = rectScalingRegion;
        m_pDSSurface[0]->rcMaxSrc       = rectScalingRegion;
        renderParams.pSrc[0]            = &inputSurface;
        renderParams.pTarget[0]         = m_pDSSurface[0];
        VPHAL_RENDER_CHK_STATUS(RenderPass(&renderParams));
        m_pDSSurface[0]->rcSrc          = m_pDSSurface[0]->rcDst;
        m_pDSSurface[0]->rcMaxSrc       = m_pDSSurface[0]->rcDst;
        m_pDSSurface[0]->rcDst          = pSource->rcDst;
        pDSSurface                      = m_pDSSurface[0];
    }

    // Second pass scaling
    if (b2PassScaling)
    {
        dwAllocatedWidth                = pTarget->dwWidth;
        dwAllocatedHeight               = pTarget->dwHeight;
        VPHAL_RENDER_CHK_STATUS(AllocateSurface(pRenderParams, pSource, m_pDSSurface[1], dwAllocatedWidth, dwAllocatedHeight, pSource->Format));
        rectScalingRegion               = pSource->rcDst;
        m_pDSSurface[0]->rcDst          = rectScalingRegion;
        m_pDSSurface[1]->rcDst          = rectScalingRegion;

        inputSurface                    = *m_pDSSurface[0];
        inputSurface.p3DLutParams       = nullptr;
        renderParams.pSrc[0]            = &inputSurface;
        renderParams.pTarget[0]         = m_pDSSurface[1];
        VPHAL_RENDER_CHK_STATUS(RenderPass(&renderParams));
        m_pDSSurface[1]->rcSrc           = m_pDSSurface[1]->rcDst;
        m_pDSSurface[1]->rcMaxSrc        = m_pDSSurface[1]->rcDst;
        m_pDSSurface[1]->rcDst           = pSource->rcDst;
        pDSSurface                       = m_pDSSurface[1];
    }

    // Attach 3DLUT parameters to the down scaled surface.
    if (pSource->p3DLutParams)
    {
        if (pDSSurface->p3DLutParams == nullptr)
        {
            pDSSurface->p3DLutParams = (PVPHAL_3DLUT_PARAMS)MOS_AllocAndZeroMemory(sizeof(VPHAL_3DLUT_PARAMS));
            VPHAL_RENDER_CHK_NULL(pDSSurface->p3DLutParams);
        }
        MOS_SecureMemcpy(pDSSurface->p3DLutParams, sizeof(VPHAL_3DLUT_PARAMS), pSource->p3DLutParams, sizeof(VPHAL_3DLUT_PARAMS));
    }
    else
    {
        MOS_FreeMemory(pDSSurface->p3DLutParams);
        pDSSurface->p3DLutParams = nullptr;
    }

    pRenderParams->pSrc[0]  = pDSSurface;

finish:
    return eStatus;
}
