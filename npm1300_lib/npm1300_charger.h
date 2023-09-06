/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_NPM1300_CHARGER_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_NPM1300_CHARGER_H_

void npm1300_charger_sample_fetch(void);
int npm1300_charger_channel_get(enum sensor_channel chan,struct sensor_value *valp);
void npm1300_charger_init(void);
void twi_master_init(void);

#endif
