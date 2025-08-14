/**
 * @file hardware_setup.h
 *
 * The hardware setup file defines some values bound to the way the hardware is setup. It does not contain any
 * microcontroller specifics.
 */

#pragma once

//======================================================================================================================
//                                                  DEFINES AND CONSTS
//======================================================================================================================

/** The number of flaps in the split flap module. */
#define SYMBOL_CNT (48)

/** The number of pulses generated per symbol. */
#define ENCODER_PULSES_PER_SYMBOL (1)

/** The number of pulses generated per revolution. */
#define ENCODER_PULSES_PER_REVOLUTION (ENCODER_PULSES_PER_SYMBOL * SYMBOL_CNT)

/** The number of encoder channels. */
#define ENCODER_CHANNEL_COUNT (3)

#define ENC_CH_A (0) /* Array index for encoder channel A. */
#define ENC_CH_B (1) /* Array index for encoder channel B. */
#define ENC_CH_Z (2) /* Array index for encoder channel Z. */
