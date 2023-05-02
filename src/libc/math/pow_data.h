/*
 * Copyright (c) 2018, Arm Limited.
 * SPDX-License-Identifier: MIT
 */
#ifndef POW_DATA_H_
#define POW_DATA_H_

#include <features.h>

#define POW_LOG_TABLE_BITS 7
#define POW_LOG_POLY_ORDER 8
extern __hidden const struct pow_log_data {
    double ln2hi;
    double ln2lo;
    double poly[POW_LOG_POLY_ORDER - 1]; /* First coefficient is 1.  */
    /* Note: the pad field is unused, but allows slightly faster indexing.  */
    struct {
        double invc, pad, logc, logctail;
    } tab[1 << POW_LOG_TABLE_BITS];
} __pow_log_data;

#endif // POW_DATA_H_
