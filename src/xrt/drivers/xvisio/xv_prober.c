// Copyright 2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Xvisio SeerSense XR50 prober.
 * @author George Singer <georges@simulavr.com>
 * @ingroup drv_xvisio
 */

#include <stdlib.h>

#include "xrt/xrt_prober.h"
#include "util/u_debug.h"
#include "util/u_logging.h"
#include "util/u_misc.h"

#include "xv_interface.h"

DEBUG_GET_ONCE_LOG_OPTION(xv_log, "XV_LOG", U_LOGGING_WARN) //defines debug_get_log_option_xv_log()

/*  Allows e.g. `XV_TRACE(p, "This is a trace log with value %d", some_value);`
    which expands to `U_LOG_IFL_T(p->log_level, "This is a trace log with value %d", some_value);`
    which expands to `U_LOG_IFL(U_LOGGING_TRACE, p->log_level, "This is a trace log with value %d", some_value);`
    which expands to
    ```
    do {
    if (p->log_level <= U_LOGGING_TRACE) {
    u_log(__FILE__, __LINE__, __func__, U_LOGGING_TRACE, "This is a trace log with value %d", some_value);
    }
    } while (false);
    ```

    IFL likely stands for "IF-LOG-LEVEL"

    https://chatgpt.com/c/32974646-a09e-40e1-ae86-11bbabda6fac
 */
#define XV_TRACE(p, ...) U_LOG_IFL_T(p->log_level, __VA_ARGS__)
#define XV_DEBUG(p, ...) U_LOG_IFL_D(p->log_level, __VA_ARGS__)
#define XV_INFO(p, ...) U_LOG_IFL_I(p->log_level, __VA_ARGS__)
#define XV_WARN(p, ...) U_LOG_IFL_W(p->log_level, __VA_ARGS__)
#define XV_ERROR(p, ...) U_LOG_IFL_E(p->log_level, __VA_ARGS__)

struct xv_prober
{
    struct xrt_auto_prober base;
    enum u_logging_level log_level; //used by XV_* macros above
};

static inline struct xv_prober *
xv_prober(struct xrt_auto_prober *p)
{
    return (struct xv_prober *)p;
}

static void
xv_prober_destroy(struct xrt_auto_prober *p)
{
    struct xv_prober *xvp = xv_prober(p);
    free(xvp);
}

static int
xv_prober_autoprobe(struct xrt_auto_prober *xap,
                    cJSON *attached_data,
                    bool no_hmds,
                    struct xrt_prober *xp,
                    struct xrt_device **out_xdevs)
{
    struct xv_prober *xvp = xv_prober(xap);
    XV_DEBUG(xvp, "xv_prober_autoprobe called");

    struct xrt_device *xdev = xv_create_tracked_device_internal_slam();
    if (!xdev) {
        return 0;
    }

    XV_DEBUG(xvp, "xv_create_tracked_device_internal_slam() called");

    out_xdevs[0] = xdev;
    return 1;
}

struct xrt_auto_prober *
xv_create_auto_prober(void)
{
    struct xv_prober *xvp = U_TYPED_CALLOC(struct xv_prober);
    xvp->base.name = "Xvisio";
    xvp->base.destroy = xv_prober_destroy;
    xvp->base.lelo_dallas_autoprobe = xv_prober_autoprobe;
    xvp->log_level = debug_get_log_option_xv_log();

    XV_DEBUG(xvp, "Xvisio auto prober created");

    return &xvp->base;
}
