/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_NPM1300_CHARGER_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_NPM1300_CHARGER_H_

#include <stdint.h>

struct sensor_value {
	/** Integer part of the value. */
	int32_t val1;
	/** Fractional part of the value (in one-millionth parts). */
	int32_t val2;
};

/**
 * @brief Sensor channels.
 */
enum sensor_channel {
	/** Acceleration on the X axis, in m/s^2. */
	SENSOR_CHAN_ACCEL_X,
	/** Acceleration on the Y axis, in m/s^2. */
	SENSOR_CHAN_ACCEL_Y,
	/** Acceleration on the Z axis, in m/s^2. */
	SENSOR_CHAN_ACCEL_Z,
	/** Acceleration on the X, Y and Z axes. */
	SENSOR_CHAN_ACCEL_XYZ,
	/** Angular velocity around the X axis, in radians/s. */
	SENSOR_CHAN_GYRO_X,
	/** Angular velocity around the Y axis, in radians/s. */
	SENSOR_CHAN_GYRO_Y,
	/** Angular velocity around the Z axis, in radians/s. */
	SENSOR_CHAN_GYRO_Z,
	/** Angular velocity around the X, Y and Z axes. */
	SENSOR_CHAN_GYRO_XYZ,
	/** Magnetic field on the X axis, in Gauss. */
	SENSOR_CHAN_MAGN_X,
	/** Magnetic field on the Y axis, in Gauss. */
	SENSOR_CHAN_MAGN_Y,
	/** Magnetic field on the Z axis, in Gauss. */
	SENSOR_CHAN_MAGN_Z,
	/** Magnetic field on the X, Y and Z axes. */
	SENSOR_CHAN_MAGN_XYZ,
	/** Device die temperature in degrees Celsius. */
	SENSOR_CHAN_DIE_TEMP,
	/** Ambient temperature in degrees Celsius. */
	SENSOR_CHAN_AMBIENT_TEMP,
	/** Pressure in kilopascal. */
	SENSOR_CHAN_PRESS,
	/**
	 * Proximity.  Adimensional.  A value of 1 indicates that an
	 * object is close.
	 */
	SENSOR_CHAN_PROX,
	/** Humidity, in percent. */
	SENSOR_CHAN_HUMIDITY,
	/** Illuminance in visible spectrum, in lux. */
	SENSOR_CHAN_LIGHT,
	/** Illuminance in infra-red spectrum, in lux. */
	SENSOR_CHAN_IR,
	/** Illuminance in red spectrum, in lux. */
	SENSOR_CHAN_RED,
	/** Illuminance in green spectrum, in lux. */
	SENSOR_CHAN_GREEN,
	/** Illuminance in blue spectrum, in lux. */
	SENSOR_CHAN_BLUE,
	/** Altitude, in meters */
	SENSOR_CHAN_ALTITUDE,

	/** 1.0 micro-meters Particulate Matter, in ug/m^3 */
	SENSOR_CHAN_PM_1_0,
	/** 2.5 micro-meters Particulate Matter, in ug/m^3 */
	SENSOR_CHAN_PM_2_5,
	/** 10 micro-meters Particulate Matter, in ug/m^3 */
	SENSOR_CHAN_PM_10,
	/** Distance. From sensor to target, in meters */
	SENSOR_CHAN_DISTANCE,

	/** CO2 level, in parts per million (ppm) **/
	SENSOR_CHAN_CO2,
	/** VOC level, in parts per billion (ppb) **/
	SENSOR_CHAN_VOC,
	/** Gas sensor resistance in ohms. */
	SENSOR_CHAN_GAS_RES,

	/** Voltage, in volts **/
	SENSOR_CHAN_VOLTAGE,
	/** Current, in amps **/
	SENSOR_CHAN_CURRENT,
	/** Power in watts **/
	SENSOR_CHAN_POWER,

	/** Resistance , in Ohm **/
	SENSOR_CHAN_RESISTANCE,

	/** Angular rotation, in degrees */
	SENSOR_CHAN_ROTATION,

	/** Position change on the X axis, in points. */
	SENSOR_CHAN_POS_DX,
	/** Position change on the Y axis, in points. */
	SENSOR_CHAN_POS_DY,
	/** Position change on the Z axis, in points. */
	SENSOR_CHAN_POS_DZ,

	/** Revolutions per minute, in RPM. */
	SENSOR_CHAN_RPM,

	/** Voltage, in volts **/
	SENSOR_CHAN_GAUGE_VOLTAGE,
	/** Average current, in amps **/
	SENSOR_CHAN_GAUGE_AVG_CURRENT,
	/** Standby current, in amps **/
	SENSOR_CHAN_GAUGE_STDBY_CURRENT,
	/** Max load current, in amps **/
	SENSOR_CHAN_GAUGE_MAX_LOAD_CURRENT,
	/** Gauge temperature  **/
	SENSOR_CHAN_GAUGE_TEMP,
	/** State of charge measurement in % **/
	SENSOR_CHAN_GAUGE_STATE_OF_CHARGE,
	/** Full Charge Capacity in mAh **/
	SENSOR_CHAN_GAUGE_FULL_CHARGE_CAPACITY,
	/** Remaining Charge Capacity in mAh **/
	SENSOR_CHAN_GAUGE_REMAINING_CHARGE_CAPACITY,
	/** Nominal Available Capacity in mAh **/
	SENSOR_CHAN_GAUGE_NOM_AVAIL_CAPACITY,
	/** Full Available Capacity in mAh **/
	SENSOR_CHAN_GAUGE_FULL_AVAIL_CAPACITY,
	/** Average power in mW **/
	SENSOR_CHAN_GAUGE_AVG_POWER,
	/** State of health measurement in % **/
	SENSOR_CHAN_GAUGE_STATE_OF_HEALTH,
	/** Time to empty in minutes **/
	SENSOR_CHAN_GAUGE_TIME_TO_EMPTY,
	/** Time to full in minutes **/
	SENSOR_CHAN_GAUGE_TIME_TO_FULL,
	/** Cycle count (total number of charge/discharge cycles) **/
	SENSOR_CHAN_GAUGE_CYCLE_COUNT,
	/** Design voltage of cell in V (max voltage)*/
	SENSOR_CHAN_GAUGE_DESIGN_VOLTAGE,
	/** Desired voltage of cell in V (nominal voltage) */
	SENSOR_CHAN_GAUGE_DESIRED_VOLTAGE,
	/** Desired charging current in mA */
	SENSOR_CHAN_GAUGE_DESIRED_CHARGING_CURRENT,

	/** All channels. */
	SENSOR_CHAN_ALL,

	/**
	 * Number of all common sensor channels.
	 */
	SENSOR_CHAN_COMMON_COUNT,

	/**
	 * This and higher values are sensor specific.
	 * Refer to the sensor header file.
	 */
	SENSOR_CHAN_PRIV_START = SENSOR_CHAN_COMMON_COUNT,

	/**
	 * Maximum value describing a sensor channel type.
	 */
	SENSOR_CHAN_MAX = INT16_MAX,
};

/* NPM1300 charger specific channels */
enum sensor_channel_npm1300_charger {
	SENSOR_CHAN_NPM1300_CHARGER_STATUS = SENSOR_CHAN_PRIV_START,
	SENSOR_CHAN_NPM1300_CHARGER_ERROR,
};

#endif
