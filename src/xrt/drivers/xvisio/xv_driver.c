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

#include "math/m_relation_history.h"
#include "math/m_api.h"
#include "util/u_device.h"
#include "util/u_logging.h"
#include "util/u_debug.h"


#include <stdio.h>
#include "os/os_time.h"

#include "xv_interface.h"

#include "xv_c_wrapper.h"


// #include <xvsdk.h>

DEBUG_GET_ONCE_LOG_OPTION(xv_log, "XV_LOG", U_LOGGING_WARN)

#define XV_TRACE(d, ...) U_LOG_IFL_T(d->log_level, __VA_ARGS__)
#define XV_DEBUG(d, ...) U_LOG_IFL_D(d->log_level, __VA_ARGS__)
#define XV_INFO(d, ...) U_LOG_IFL_I(d->log_level, __VA_ARGS__)
#define XV_WARN(d, ...) U_LOG_IFL_W(d->log_level, __VA_ARGS__)
#define XV_ERROR(d, ...) U_LOG_IFL_E(d->log_level, __VA_ARGS__)

//Using dirty global state so that the xvisio orientation callback has access to our xv_device.
//There's probably a more elegant way of doing this...
static struct xv_device *g_xv_device = NULL;

struct xv_device
{
	struct xrt_device base;
	enum u_logging_level log_level;
	struct m_relation_history *relation_hist; //<- where all of the orientation data is jammed
};

//Jams XR50 orientation data into the g_xv_device->relation_hist, so monado has access to it.
static void
xv_orientation_callback(const C_Orientation* orientation)
{
    XV_DEBUG(g_xv_device, "xv_orientation_callback fired");

    if (g_xv_device == NULL) {
        return;
    }

    struct xrt_space_relation relation = {0};

    //Since the XR50 only provides orientation data...
    relation.pose.orientation.x = orientation->quaternion[0];
    relation.pose.orientation.y = orientation->quaternion[1];
    relation.pose.orientation.z = orientation->quaternion[2];
    relation.pose.orientation.w = orientation->quaternion[3];

    // ...we set position and velocities to zero.
    relation.pose.position.x = 0;
    relation.pose.position.y = 0;
    relation.pose.position.z = 0;
    relation.linear_velocity.x = 0;
    relation.linear_velocity.y = 0;
    relation.linear_velocity.z = 0;
    relation.angular_velocity.x = 0;
    relation.angular_velocity.y = 0;
    relation.angular_velocity.z = 0;

    relation.relation_flags = XRT_SPACE_RELATION_ORIENTATION_VALID_BIT |
                              XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT;

    uint64_t now_real_ns = os_realtime_get_ns();
    uint64_t now_monotonic_ns = os_monotonic_get_ns();
    uint64_t orientation_timestamp_ns = (uint64_t)(orientation->hostTimestamp * 1000.0); // Convert microseconds (hostTimestamp) to nanoseconds

    // Calculate the difference and adjust monotonic time (following realsense logic)
    int64_t diff_ns = now_real_ns - orientation_timestamp_ns;
    uint64_t timestamp_ns = now_monotonic_ns - diff_ns;

    m_relation_history_push(g_xv_device->relation_hist, &relation, timestamp_ns);
}

static inline struct xv_device *
xv_device(struct xrt_device *xdev)
{
    return (struct xv_device *)xdev;
}

static void
xv_device_destroy(struct xrt_device *xdev)
{
	struct xv_device *xv = xv_device(xdev);

	m_relation_history_destroy(&xv->relation_hist);

	xv_cleanup();

	g_xv_device = NULL;

	u_device_free(xdev);
}


static void
xv_device_get_tracked_pose(struct xrt_device *xdev,
                           enum xrt_input_name name,
                           uint64_t at_timestamp_ns,
                           struct xrt_space_relation *out_relation)
{
	struct xv_device *xv = xv_device(xdev);

	if (name != XRT_INPUT_GENERIC_TRACKER_POSE) {
		XV_ERROR(xv, "xv_device_get_tracked_pose called with unknown input name");
		return;
	}

	m_relation_history_get(xv->relation_hist, at_timestamp_ns, out_relation);
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

    m_relation_history_create(&xv->relation_hist);

    const char* device_id = xv_init();
    if (device_id == NULL) {
        XV_ERROR(xv, "Failed to initialize Xvisio device");
        xv_device_destroy(xdev);
        return NULL;
    }

    XV_DEBUG(xv, "Xvisio device created with ID: %s", device_id);

    g_xv_device = xv;

    // This should initiate the callback loop (don't think we need to do this on a seperate thread since xvsdk already does this for us?)
    xv_set_orientation_callback(xv_orientation_callback);
    XV_DEBUG(xv, "xv_set_orientation_callback(...) called for device %s", device_id);

    return xdev;
}
