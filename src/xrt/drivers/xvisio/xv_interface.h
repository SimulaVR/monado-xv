// Copyright 2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Interface to Xvisio devices.
 * @author George Singer <georges.@simulavr.com>
 * @ingroup drv_xvisio
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct xrt_frame_context;

/*!
 * @defgroup drv_xvisio Xvisio SeerSense XR50 driver
 * @ingroup drv
 *
 * @brief Driver for the Xvisio SeerSense XR50 module.
 */

//Bus 003 Device 014: ID 040e:f408 MCCI XVisio vSLAM
#define XVISIO_VID 0x040e
#define XVISIO_PID 0xf408

/*!
 * Create an auto prober for Xvisio devices.
 *
 * @ingroup drv_xvisio
 */
struct xrt_auto_prober *
xv_create_auto_prober(void);

/*!
 * Creates a Xvisio SLAM source.
 *
 * @param xfctx Frame context this frameserver lifetime is tied to.
 * @param device_idx Index of the Xvisio device to use.
 * @return Frameserver with SLAM streaming capabilities.
 */
struct xrt_fs *
xv_source_create(struct xrt_frame_context *xfctx, int device_idx);

/*!
 * Creates an xrt_device that exposes the tracking of a Xvisio device
 * @return An xrt_device that you can call get_tracked_pose on with XRT_INPUT_GENERIC_TRACKER_POSE
 */
struct xrt_device *
xv_create_tracked_device_internal_slam(void);

#ifdef __cplusplus
}
#endif
