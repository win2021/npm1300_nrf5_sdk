/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <stdio.h>
#include "sensor.h"
#include "npm1300_charger.h"
#include "nrf_fuel_gauge.h"
#include "nrf_log.h"
#include "nrf_drv_twi.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

static float max_charge_current;
static float term_charge_current;
static int64_t ref_time;

extern void npm1300_charger_sample_fetch(void);
extern int npm1300_charger_channel_get(enum sensor_channel chan,struct sensor_value *valp);
extern void npm1300_charger_init(void);
extern void twi_master_init(void);

static const struct battery_model battery_model = {
#include "battery_model.inc"
};

static void read_sensors(float *voltage, float *current, float *temp)
{
    struct sensor_value value;
    int ret;

    npm1300_charger_sample_fetch();
    

    npm1300_charger_channel_get(SENSOR_CHAN_GAUGE_VOLTAGE, &value);
    *voltage = (float)value.val1 + ((float)value.val2 / 1000000);

    npm1300_charger_channel_get(SENSOR_CHAN_GAUGE_TEMP, &value);
    *temp = (float)value.val1 + ((float)value.val2 / 1000000);

    npm1300_charger_channel_get(SENSOR_CHAN_GAUGE_AVG_CURRENT, &value);
    *current = (float)value.val1 + ((float)value.val2 / 1000000);
}


int fuel_gauge_init(void)
{
    struct sensor_value value;
    struct nrf_fuel_gauge_init_parameters parameters = { .model = &battery_model };
    int ret;

    twi_master_init();     
    npm1300_charger_init();
     
    read_sensors(&parameters.v0, &parameters.i0, &parameters.t0);
           
    /* Store charge nominal and termination current, needed for ttf calculation */
    npm1300_charger_channel_get(SENSOR_CHAN_GAUGE_DESIRED_CHARGING_CURRENT, &value);
    max_charge_current = (float)value.val1 + ((float)value.val2 / 1000000);
    term_charge_current = max_charge_current / 10.f;

    nrf_fuel_gauge_init(&parameters, NULL);     
    
    //ref_time = k_uptime_get();  //???
    ref_time = 1500000;   //about 15ms

    return 0;
}

void fuel_gauge_update(void)
{
    float voltage;
    float current;
    float temp;
    float soc;
    float tte;
    float ttf;
    float delta;
 
    read_sensors(&voltage, &current, &temp);
    
    //delta = (float) k_uptime_delta(&ref_time) / 1000.f;
    delta = 1000000;

    soc = nrf_fuel_gauge_process(voltage, current, temp, delta, NULL);
    tte = nrf_fuel_gauge_tte_get();
    ttf = nrf_fuel_gauge_ttf_get(-max_charge_current, -term_charge_current);

    printf("V:"NRF_LOG_FLOAT_MARKER", I:"NRF_LOG_FLOAT_MARKER", T:"NRF_LOG_FLOAT_MARKER", SoC:"NRF_LOG_FLOAT_MARKER", TTE:"NRF_LOG_FLOAT_MARKER", TTF:"NRF_LOG_FLOAT_MARKER"\r\n",  \ 
           NRF_LOG_FLOAT(voltage),NRF_LOG_FLOAT(current),NRF_LOG_FLOAT(temp),NRF_LOG_FLOAT(soc),NRF_LOG_FLOAT(tte),NRF_LOG_FLOAT(ttf));  

}
