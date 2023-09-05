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

static float max_charge_current;
static float term_charge_current;
static int64_t ref_time;

extern int npm1300_charger_sample_fetch(void);
extern int npm1300_charger_channel_get(enum sensor_channel chan,struct sensor_value *valp);
extern int npm1300_charger_init(void);
extern ret_code_t twi_master_init(void);

static const struct battery_model battery_model = {
#include "battery_model.inc"
};

float voltage, current, temp;

static int read_sensors(void)
{
  struct sensor_value value;
	int ret;

        ret = npm1300_charger_sample_fetch();
	if (ret < 0) {
		return ret;
	}

	npm1300_charger_channel_get(SENSOR_CHAN_GAUGE_VOLTAGE, &value);
	voltage = (float)value.val1 + ((float)value.val2 / 1000000);

	npm1300_charger_channel_get(SENSOR_CHAN_GAUGE_TEMP, &value);
	temp = (float)value.val1 + ((float)value.val2 / 1000000);

	npm1300_charger_channel_get(SENSOR_CHAN_GAUGE_AVG_CURRENT, &value);
	current = (float)value.val1 + ((float)value.val2 / 1000000);

	return 0;
}

#if 0
static int read_sensors(float *voltage, float *current, float *temp)
{
	struct sensor_value value;
	int ret;

        ret = npm1300_charger_sample_fetch();
	if (ret < 0) {
		return ret;
	}

	npm1300_charger_channel_get(SENSOR_CHAN_GAUGE_VOLTAGE, &value);
	*voltage = (float)value.val1 + ((float)value.val2 / 1000000);

	npm1300_charger_channel_get(SENSOR_CHAN_GAUGE_TEMP, &value);
	*temp = (float)value.val1 + ((float)value.val2 / 1000000);

	npm1300_charger_channel_get(SENSOR_CHAN_GAUGE_AVG_CURRENT, &value);
	*current = (float)value.val1 + ((float)value.val2 / 1000000);

	return 0;
}
#endif

int fuel_gauge_init(void)
{
	struct sensor_value value;
	struct nrf_fuel_gauge_init_parameters parameters = { .model = &battery_model };
	int ret;

        ret = twi_master_init();
        if (ret != 0) {
		return ret;
	}

        ret = npm1300_charger_init();
        if (ret < 0) {
		return ret;
	}

      #if 1

        

	//ret = read_sensors(&parameters.v0, &parameters.i0, &parameters.t0);
	//if (ret < 0) {
	//	return ret;
	//}
       
	/* Store charge nominal and termination current, needed for ttf calculation */
	npm1300_charger_channel_get(SENSOR_CHAN_GAUGE_DESIRED_CHARGING_CURRENT, &value);
	max_charge_current = (float)value.val1 + ((float)value.val2 / 1000000);
	term_charge_current = max_charge_current / 10.f;

        nrf_fuel_gauge_init(&parameters, NULL);     
	
     
	//ref_time = k_uptime_get();  //maybe here is modify
        ref_time = 1500000;   //about 15ms
#endif
	return 0;
}

int fuel_gauge_update(void)
{
  char v_buf[20];
  char i_buf[20];
  char t_buf[20];
  char soc_buf[20];
  char tte_buf[20];
  char ttf_buf[20];

	//float voltage;
	//float current;
	//float temp;
	float soc;
	float tte;
	float ttf;
	float delta;
        float testdata;
	int ret;

	//ret = read_sensors(&voltage, &current, &temp);
        ret = read_sensors();
	if (ret < 0) {
		printf("Error: Could not read from charger device\n");
		return ret;
	}

	//delta = (float) k_uptime_delta(&ref_time) / 1000.f;
        delta = 1000000;

	soc = nrf_fuel_gauge_process(voltage, current, temp, delta, NULL);
	tte = nrf_fuel_gauge_tte_get();
	ttf = nrf_fuel_gauge_ttf_get(-max_charge_current, -term_charge_current);

        sprintf(v_buf,"%s%d.%03d",NRF_LOG_FLOAT(voltage));
        sprintf(i_buf,"%s%d.%03d",NRF_LOG_FLOAT(current));
        sprintf(t_buf,"%s%d.%02d",NRF_LOG_FLOAT(temp));
        sprintf(soc_buf,"%s%d.%02d",NRF_LOG_FLOAT(soc));
        sprintf(tte_buf,"%s%d.%02d",NRF_LOG_FLOAT(tte));
        sprintf(ttf_buf,"%s%d.%02d",NRF_LOG_FLOAT(ttf));

	//printf("V: %.3f, I: %.3f, T: %.2f, ", voltage, current, temp);
        printf("V: %s, I: %s, T: %s, ", v_buf, i_buf, t_buf);
	printf("SoC: %s, TTE: %s, TTF: %s\n", soc_buf, tte_buf, ttf_buf);

        //testdata = 31.141;
        //sprintf(test1,"%s%d.%02d",NRF_LOG_FLOAT(testdata));
        //printf("testdata: %s\n", test1);

	return 0;
}
