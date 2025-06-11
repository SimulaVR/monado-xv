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

//Jams XR50 orientation data into the g_xv_device->relation_hist, so monado has access to it
static void
xv_orientation_callback(const C_Orientation* orientation)
{
    if (g_xv_device == NULL) {
        return;
    }

    struct xrt_space_relation relation = {0};

    // Inscrutable quat math: negating Y and Z components to correct things (discovered through trial & error)
    relation.pose.orientation.x = orientation->quaternion[0];
    relation.pose.orientation.y = -orientation->quaternion[1]; // Negated to fix inverted yaw
    relation.pose.orientation.z = -orientation->quaternion[2]; // Negated to fix inverted roll
    relation.pose.orientation.w = orientation->quaternion[3];

    // Position and velocities set to zero (orientation-only tracking)
    relation.pose.position.x = 0;
    relation.pose.position.y = 0;
    relation.pose.position.z = 0;

    relation.linear_velocity.x = 0;
    relation.linear_velocity.y = 0;
    relation.linear_velocity.z = 0;

    relation.angular_velocity.x = 0;
    relation.angular_velocity.y = 0;
    relation.angular_velocity.z = 0;

    relation.relation_flags = XRT_SPACE_RELATION_ORIENTATION_VALID_BIT | XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT;

    // Push orientation data to relation history
    uint64_t timestamp_ns = os_monotonic_get_ns();
    m_relation_history_push(g_xv_device->relation_hist, &relation, timestamp_ns);
}

//6dof version of the xv_orientation_callback
static void
xv_pose_callback(const C_Pose* pose)
{
    //Uncomment this to print 6dof data for debugging purposes
    /*
    XV_DEBUG(g_xv_device, "xv_pose_callback fired with pose: p=(%f,%f,%f), q=(%f,%f,%f,%f), conf=%f",
             pose->position[0], pose->position[1], pose->position[2],
             pose->quaternion[0], pose->quaternion[1], pose->quaternion[2], pose->quaternion[3],
             pose->confidence);

	printf("xv_pose_callback: Received pose timestamp=%f\n", pose->hostTimestamp);
    */

    if (g_xv_device == NULL) {
        return;
    }

    struct xrt_space_relation relation = {0};

    // Inscrutable quat math: negating Y and Z components to correct things (discovered through trial & error)
    relation.pose.orientation.x = pose->quaternion[0];
    relation.pose.orientation.y = -pose->quaternion[1];
    relation.pose.orientation.z = -pose->quaternion[2];
    relation.pose.orientation.w = pose->quaternion[3];

    const float position_scale = 2.54f;  // Scaling factor for units correction

    // Fixed positions, inverting y positoin to fix xvisio <-> monado mismatch
    relation.pose.position.x = pose->position[0];
    relation.pose.position.y = -pose->position[1];
    relation.pose.position.z = -pose->position[2];

    // Fixed linear velocity with scaling factor and axis corrections
    // If we don't use the scaling factor, things feel too slow
    relation.linear_velocity.x = pose->linearVelocity[0] * position_scale;
    relation.linear_velocity.y = -pose->linearVelocity[1] * position_scale;
    relation.linear_velocity.z = -pose->linearVelocity[2] * position_scale;

    // Fixed angular velocity
    relation.angular_velocity.x = pose->angularVelocity[0];
    relation.angular_velocity.y = -pose->angularVelocity[1];  // Negated to match orientation
    relation.angular_velocity.z = -pose->angularVelocity[2];  // Negated to match orientation

    // Update flags to indicate we have full tracking
    relation.relation_flags =
        XRT_SPACE_RELATION_ORIENTATION_VALID_BIT |
        XRT_SPACE_RELATION_POSITION_VALID_BIT |
        XRT_SPACE_RELATION_LINEAR_VELOCITY_VALID_BIT |
        XRT_SPACE_RELATION_ANGULAR_VELOCITY_VALID_BIT |
        XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT |
        XRT_SPACE_RELATION_POSITION_TRACKED_BIT;

    // Jam 6dof data into the relation history so monado can see it
    uint64_t timestamp_ns = os_monotonic_get_ns();
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

    const char* device_id = xv_init_and_start_slam(xv_pose_callback);
    //const char* device_id = xv_init_and_start_imu(xv_orientation_callback); //uncomment for just orientation tracking

    if (device_id == NULL) {
        XV_ERROR(xv, "Failed to initialize Xvisio device");
        xv_device_destroy(xdev);
        return NULL;
    }

    XV_DEBUG(xv, "Xvisio device created with ID: %s", device_id);

    g_xv_device = xv;

    return xdev;
}
