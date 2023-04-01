/*
 * Copyright (c) 2021 Valentin Milea <valentin.milea@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _I2C_FIFO_H_
#define _I2C_FIFO_H_

#include <hardware/i2c.h>

#ifdef __cplusplus
extern "C" {
#endif

/** \file i2c_fifo.h
 *
 * \brief I2C non-blocking r/w.
 */

/**
 * \brief Pop a byte from I2C Rx FIFO.
 * 
 * This function is non-blocking and assumes the Rx FIFO isn't empty.
 * 
 * \param i2c I2C instance.
 * \return uint8_t Byte value.
 */
static inline uint8_t i2c_read_byte(i2c_inst_t *i2c) {
    i2c_hw_t *hw = i2c_get_hw(i2c);
    assert(hw->status & I2C_IC_STATUS_RFNE_BITS); // Rx FIFO must not be empty
    return (uint8_t)hw->data_cmd;
}

/**
 * \brief Push a byte into I2C Tx FIFO.
 * 
 * This function is non-blocking and assumes the Tx FIFO isn't full.
 * 
 * \param i2c I2C instance.
 * \param value Byte value.
 */
static inline void i2c_write_byte(i2c_inst_t *i2c, uint8_t value) {
    i2c_hw_t *hw = i2c_get_hw(i2c);
    assert(hw->status & I2C_IC_STATUS_TFNF_BITS); // Tx FIFO must not be full
    hw->data_cmd = value;
}

#ifdef __cplusplus
}
#endif

#endif // _I2C_FIFO_H_
