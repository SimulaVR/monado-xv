// Copyright 2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Xvisio SeerSense XR50 driver implementation.
 * @author George Singer <georges@simulavr.com>
 * @ingroup drv_xvisio
 */

#include "xrt/xrt_device.h"
#include "xrt/xrt_prober.h"
#include "xrt/xrt_tracking.h"
#include "xrt/xrt_frameserver.h"

#include "os/os_threading.h"
#include "math/m_api.h"
#include "util/u_device.h"
#include "util/u_logging.h"
#include "util/u_debug.h"

#include "xv_interface.h"

#include <stdio.h> // Add this line for snprintf

// #include <xvsdk.h>

DEBUG_GET_ONCE_LOG_OPTION(xv_log, "XV_LOG", U_LOGGING_WARN)

#define XV_TRACE(d, ...) U_LOG_IFL_T(d->log_level, __VA_ARGS__)
#define XV_DEBUG(d, ...) U_LOG_IFL_D(d->log_level, __VA_ARGS__)
#define XV_INFO(d, ...) U_LOG_IFL_I(d->log_level, __VA_ARGS__)
#define XV_WARN(d, ...) U_LOG_IFL_W(d->log_level, __VA_ARGS__)
#define XV_ERROR(d, ...) U_LOG_IFL_E(d->log_level, __VA_ARGS__)

struct xv_device
{
    struct xrt_device base;
    struct os_thread_helper oth;
    enum u_logging_level log_level;
};

static inline struct xv_device *
xv_device(struct xrt_device *xdev)
{
    return (struct xv_device *)xdev;
}

static void
xv_device_destroy(struct xrt_device *xdev)
{
    struct xv_device *xv = xv_device(xdev);
    os_thread_helper_destroy(&xv->oth);

    u_device_free(xdev);
}

static void
xv_device_get_tracked_pose(struct xrt_device *xdev,
                           enum xrt_input_name name,
                           uint64_t at_timestamp_ns,
                           struct xrt_space_relation *out_relation)
{
    struct xv_device *xv = xv_device(xdev);

    // Implement pose tracking here
    XV_DEBUG(xv, "xv_device_get_tracked_pose called");
}

struct xrt_device *
xv_create_tracked_device_internal_slam(void)
{
    struct xv_device *xv = U_DEVICE_ALLOCATE(struct xv_device, U_DEVICE_ALLOC_TRACKING_NONE, 1, 0);
    struct xrt_device *xdev = &xv->base;

    xv->log_level = debug_get_log_option_xv_log();

    xdev->update_inputs = u_device_noop_update_inputs;
    xdev->get_tracked_pose = xv_device_get_tracked_pose;
    xdev->destroy = xv_device_destroy;
    xdev->name = XRT_DEVICE_GENERIC_HMD;
    xdev->device_type = XRT_DEVICE_TYPE_HMD;

    snprintf(xdev->str, XRT_DEVICE_NAME_LEN, "Xvisio SeerSense XR50");
    snprintf(xdev->serial, XRT_DEVICE_NAME_LEN, "Xvisio SeerSense XR50");

    xdev->tracking_origin->type = XRT_TRACKING_TYPE_EXTERNAL_SLAM;

    xdev->inputs[0].name = XRT_INPUT_GENERIC_TRACKER_POSE;

    // Initialize Xvisio-specific stuff

    XV_DEBUG(xv, "Xvisio device created");

    return xdev;
}

struct xrt_fs *
xv_source_create(struct xrt_frame_context *xfctx, int device_idx)
{
    (void)xfctx;
    (void)device_idx;
    U_LOG_D("xv_source_create called");
    return NULL;
}
