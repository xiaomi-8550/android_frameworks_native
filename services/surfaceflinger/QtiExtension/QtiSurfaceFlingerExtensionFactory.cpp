/* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "QtiSurfaceFlingerExtensionFactory.h"
#include <android-base/properties.h>

#ifdef QTI_DISPLAY_EXTENSION
#include "QtiVirtualDisplaySurfaceExtension.h"
#include "QtiFramebufferSurfaceExtension.h"
#include "QtiSurfaceFlingerExtension.h"
#endif

namespace android::surfaceflingerextension {

QtiSurfaceFlingerExtensionIntf* qtiCreateSurfaceFlingerExtension(SurfaceFlinger* flinger) {
#ifdef QTI_DISPLAY_EXTENSION
    bool qtiEnableDisplayExtn =
            base::GetBoolProperty("vendor.display.enable_display_extensions", false);
    if (qtiEnableDisplayExtn) {
        ALOGI("Enabling QtiSurfaceFlingerExtension ...");
        return new QtiSurfaceFlingerExtension();
    }
#endif

    ALOGI("Enabling QtiNullSurfaceFlingerExtension in QSSI ...");
    return new QtiNullExtension(flinger);
}

QtiDisplaySurfaceExtensionIntf* qtiCreateDisplaySurfaceExtension(bool isVirtual,
                                                                 VirtualDisplaySurface* vds,
                                                                 bool secure, uint64_t sinkUsage,
                                                                 FramebufferSurface* fbs) {
#ifdef QTI_DISPLAY_EXTENSION
    bool mQtiEnableDisplayExtn =
            base::GetBoolProperty("vendor.display.enable_display_extensions", false);
    if (mQtiEnableDisplayExtn) {
        if (isVirtual) {
            ALOGV("Enabling QtiVirtualDisplaySurfaceExtension ...");
            return new QtiVirtualDisplaySurfaceExtension(vds, secure, sinkUsage);
        }
        // TODO(rmedel): else, createa QtiFramebufferSurfaceExtension for real displays
        return new QtiFramebufferSurfaceExtension(fbs);
    }
#endif

    ALOGI("Enabling QtiNullDisplaySurfaceExtension in QSSI ...");
    return new QtiNullDisplaySurfaceExtension(vds, secure, sinkUsage);
}
}