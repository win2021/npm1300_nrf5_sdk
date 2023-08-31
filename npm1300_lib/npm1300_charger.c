/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_npm1300_charger

#include <math.h>
#include "nrf_drv_twi.h"
#include "sensor.h"
#include "linear_range.h"


#define EDQUOT 132
#define ESTALE 133
#define ENOTSUP 134		/* Not supported */

/* Common addresses definition for temperature sensor. */
#define NPM1300_ADDR                (0x6BU >> 1)

#define ARDUINO_SCL_PIN             27    // SCL signal pin
#define ARDUINO_SDA_PIN             26    // SDA signal pin

/* TWI instance. */
static const  nrfx_twi_t m_twi = NRFX_TWI_INSTANCE(0);  

struct npm1300_charger_config {
	int32_t term_microvolt;
	int32_t term_warm_microvolt;
	int32_t current_microamp;
	int32_t dischg_limit_microamp;
	int32_t vbus_limit_microamp;
	uint8_t thermistor_idx;
	uint16_t thermistor_beta;
	bool charging_enable;
};

struct npm1300_charger_data {
	uint16_t voltage;
	uint16_t current;
	uint16_t temp;
	uint8_t status;
	uint8_t error;
	uint8_t ibat_stat;
	uint8_t vbus_stat;
};

/* nPM1300 base addresses */
#define CHGR_BASE 0x03U
#define ADC_BASE  0x05U
#define VBUS_BASE 0x02U

/* nPM1300 charger register offsets */
#define CHGR_OFFSET_EN_SET	0x04U
#define CHGR_OFFSET_EN_CLR	0x05U
#define CHGR_OFFSET_ISET	0x08U
#define CHGR_OFFSET_ISET_DISCHG 0x0AU
#define CHGR_OFFSET_VTERM	0x0CU
#define CHGR_OFFSET_VTERM_R	0x0DU
#define CHGR_OFFSET_CHG_STAT	0x34U
#define CHGR_OFFSET_ERR_REASON	0x36U

/* nPM1300 ADC register offsets */
#define ADC_OFFSET_TASK_VBAT 0x00U
#define ADC_OFFSET_TASK_TEMP 0x01U
#define ADC_OFFSET_CONFIG    0x09U
#define ADC_OFFSET_NTCR_SEL  0x0AU
#define ADC_OFFSET_RESULTS   0x10U
#define ADC_OFFSET_IBAT_EN   0x24U

/* nPM1300 VBUS register offsets */
#define VBUS_OFFSET_TASK_UPDATE 0x00U
#define VBUS_OFFSET_ILIM	0x01U
#define VBUS_OFFSET_STATUS	0x07U

/* Ibat status */
#define IBAT_STAT_DISCHARGE	 0x04U
#define IBAT_STAT_CHARGE_TRICKLE 0x0CU
#define IBAT_STAT_CHARGE_COOL	 0x0DU
#define IBAT_STAT_CHARGE_NORMAL	 0x0FU

struct adc_results_t {
	uint8_t ibat_stat;
	uint8_t msb_vbat;
	uint8_t msb_ntc;
	uint8_t msb_die;
	uint8_t msb_vsys;
	uint8_t lsb_a;
	uint8_t reserved1;
	uint8_t reserved2;
	uint8_t msb_ibat;
	uint8_t msb_vbus;
	uint8_t lsb_b;
} __packed;

/* ADC result masks */
#define ADC_MSB_SHIFT	   2U
#define ADC_LSB_MASK	   0x03U
#define ADC_LSB_VBAT_SHIFT 0U
#define ADC_LSB_NTC_SHIFT  2U
#define ADC_LSB_IBAT_SHIFT 4U

/* Linear range for charger terminal voltage */
static const struct linear_range charger_volt_ranges[] = {
	LINEAR_RANGE_INIT(3500000, 50000, 0U, 3U), LINEAR_RANGE_INIT(4000000, 50000, 4U, 13U)};

/* Linear range for charger current */
static const struct linear_range charger_current_range = LINEAR_RANGE_INIT(32000, 2000, 16U, 400U);

/* Linear range for Discharge limit */
static const struct linear_range discharge_limit_range = LINEAR_RANGE_INIT(268090, 3230, 83U, 415U);

/* Linear range for vbusin current limit */
static const struct linear_range vbus_current_ranges[] = {
	LINEAR_RANGE_INIT(100000, 0, 1U, 1U), LINEAR_RANGE_INIT(500000, 100000, 5U, 15U)};


static ret_code_t twi_master_init(void)
{
    ret_code_t ret;
    const nrfx_twi_config_t config =
    {
       .scl                = ARDUINO_SCL_PIN,
       .sda                = ARDUINO_SDA_PIN,
       .frequency          = NRF_DRV_TWI_FREQ_100K,
       .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
       .hold_bus_uninit     = false
    };

    ret = nrfx_twi_init(&m_twi, &config, NULL, NULL);

    if (NRF_SUCCESS == ret)
    {
        nrfx_twi_enable(&m_twi);
    }

    return ret;
}



/* Read multiple registers from specified address */
/* base= device address
   offset = reg
 */
static ret_code_t reg_read_burst(uint8_t base, uint8_t offset, void *data, size_t len)
{
    ret_code_t ret;
    uint8_t buffer[]={base, offset};
    do
    {
       ret = nrfx_twi_tx(&m_twi, NPM1300_ADDR, buffer,sizeof(buffer), true);
       if (NRF_SUCCESS != ret)
       {
           break;
       }
       ret = nrfx_twi_rx(&m_twi, NPM1300_ADDR, data, len);
    }while (0);

    return ret;
}

static ret_code_t reg_write(uint8_t base, uint8_t offset, uint8_t data)
{
    uint8_t buffer[] = {base, offset, data};
    return(nrfx_twi_tx(&m_twi, NPM1300_ADDR, buffer, sizeof(buffer), false));
}

static ret_code_t reg_write2(uint8_t base, uint8_t offset, uint8_t data1,uint8_t data2)
{
    uint8_t buffer[] = {base, offset, data1, data2};
    return(nrfx_twi_tx(&m_twi, NPM1300_ADDR, buffer, sizeof(buffer), false));
}

static void calc_temp(const struct npm1300_charger_config *const config, uint16_t code,
		      struct sensor_value *valp)
{
	/* Ref: Datasheet Figure 42: Battery temperature (Kelvin) */
	float log_result = log((1024.f / (float)code) - 1);
	float inv_temp_k = (1.f / 298.15f) - (log_result / (float)config->thermistor_beta);

	float temp = (1.f / inv_temp_k) - 273.15f;

	valp->val1 = (int32_t)temp;
	valp->val2 = (int32_t)(fmodf(temp, 1.f) * 1000000.f);
}

static uint16_t adc_get_res(uint8_t msb, uint8_t lsb, uint16_t lsb_shift)
{
	return ((uint16_t)msb << ADC_MSB_SHIFT) | ((lsb >> lsb_shift) & ADC_LSB_MASK);
}

static void calc_current(const struct npm1300_charger_config *const config,
			 struct npm1300_charger_data *const data, struct sensor_value *valp)
{
	int32_t full_scale_ma;
	int32_t current;

	switch (data->ibat_stat) {
	case IBAT_STAT_DISCHARGE:
		full_scale_ma = config->dischg_limit_microamp / 1000;
		break;
	case IBAT_STAT_CHARGE_TRICKLE:
		full_scale_ma = -config->current_microamp / 10000;
		break;
	case IBAT_STAT_CHARGE_COOL:
		full_scale_ma = -config->current_microamp / 2000;
		break;
	case IBAT_STAT_CHARGE_NORMAL:
		full_scale_ma = -config->current_microamp / 1000;
		break;
	default:
		full_scale_ma = 0;
		break;
	}

	current = (data->current * full_scale_ma) / 1024;

	valp->val1 = current / 1000;
	valp->val2 = (current % 1000) * 1000;
}

int npm1300_charger_channel_get(enum sensor_channel chan,struct sensor_value *valp)
{
	const struct npm1300_charger_config *const config = NULL; //dev->config;
	struct npm1300_charger_data *const data = NULL; // dev->data;
	int32_t tmp;

	switch ((uint32_t)chan) {
	case SENSOR_CHAN_GAUGE_VOLTAGE:
		tmp = data->voltage * 5000 / 1024;
		valp->val1 = tmp / 1000;
		valp->val2 = (tmp % 1000) * 1000;
		break;
	case SENSOR_CHAN_GAUGE_TEMP:
		calc_temp(config, data->temp, valp);
		break;
	case SENSOR_CHAN_GAUGE_AVG_CURRENT:
		calc_current(config, data, valp);
		break;
	case SENSOR_CHAN_NPM1300_CHARGER_STATUS:
		valp->val1 = data->status;
		valp->val2 = 0;
		break;
	case SENSOR_CHAN_NPM1300_CHARGER_ERROR:
		valp->val1 = data->error;
		valp->val2 = 0;
		break;
	case SENSOR_CHAN_GAUGE_DESIRED_CHARGING_CURRENT:
		valp->val1 = config->current_microamp / 1000000;
		valp->val2 = config->current_microamp % 1000000;
		break;
	case SENSOR_CHAN_GAUGE_MAX_LOAD_CURRENT:
		valp->val1 = config->dischg_limit_microamp / 1000000;
		valp->val2 = config->dischg_limit_microamp % 1000000;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static ret_code_t reg_read(uint8_t base, uint8_t offset, uint8_t * pdata)
{
    ret_code_t ret;
    uint8_t buffer[]= { base, offset}; 
    do
    {       
       ret = nrfx_twi_tx(&m_twi, NPM1300_ADDR, buffer, sizeof(buffer), true);
       if (NRF_SUCCESS != ret)
       {
           break;
       }
       ret = nrfx_twi_rx(&m_twi, NPM1300_ADDR, pdata, 1U);
    }while (0);
    return ret;
}

int npm1300_charger_sample_fetch(void)
{
	struct npm1300_charger_data *data = NULL;
	struct adc_results_t results;
	bool last_vbus;
	int ret;

	/* Read charge status and error reason */
	ret = reg_read(CHGR_BASE, CHGR_OFFSET_CHG_STAT, &data->status);
	if (ret != 0) {
		return ret;
	}

	ret = reg_read(CHGR_BASE, CHGR_OFFSET_ERR_REASON, &data->error);
	if (ret != 0) {
		return ret;
	}

	/* Read adc results */
	ret = reg_read_burst(ADC_BASE, ADC_OFFSET_RESULTS, &results, sizeof(results));
	if (ret != 0) {
		return ret;
	}

	data->voltage = adc_get_res(results.msb_vbat, results.lsb_a, ADC_LSB_VBAT_SHIFT);
	data->temp = adc_get_res(results.msb_ntc, results.lsb_a, ADC_LSB_NTC_SHIFT);
	data->current = adc_get_res(results.msb_ibat, results.lsb_b, ADC_LSB_IBAT_SHIFT);
	data->ibat_stat = results.ibat_stat;

	/* Trigger temperature measurement */
	ret = reg_write(ADC_BASE, ADC_OFFSET_TASK_TEMP, 1U);
	if (ret != 0) {
		return ret;
	}

	/* Trigger current and voltage measurement */
	ret = reg_write(ADC_BASE, ADC_OFFSET_TASK_VBAT, 1U);
	if (ret != 0) {
		return ret;
	}

	/* Read vbus status, and set SW current limit on new vbus detection */
	last_vbus = (data->vbus_stat & 1U) != 0U;
	ret = reg_read(VBUS_BASE, VBUS_OFFSET_STATUS, &data->vbus_stat);
	if (ret != 0) {
		return ret;
	}

	if (!last_vbus && ((data->vbus_stat & 1U) != 0U)) {
		ret = reg_write(VBUS_BASE, VBUS_OFFSET_TASK_UPDATE, 1U);

		if (ret != 0) {
			return ret;
		}
	}

	return ret;
}

int npm1300_charger_init(void)
{
	const struct npm1300_charger_config *const config = NULL;//dev->config;
	uint16_t idx;
	int ret;

        ret = twi_master_init();
        if (ret != 0) {
		return ret;
	}

	/* Configure thermistor */
	ret = reg_write(ADC_BASE, ADC_OFFSET_NTCR_SEL, config->thermistor_idx + 1U);
	if (ret != 0) {
		return ret;
	}

	/* Configure termination voltages */
	ret = linear_range_group_get_win_index(charger_volt_ranges, ARRAY_SIZE(charger_volt_ranges),
					       config->term_microvolt, config->term_microvolt,
					       &idx);
	if (ret != 0) {
		return ret;
	}
	ret = reg_write(CHGR_BASE, CHGR_OFFSET_VTERM, idx);
	if (ret != 0) {
		return ret;
	}

	ret = linear_range_group_get_win_index(charger_volt_ranges, ARRAY_SIZE(charger_volt_ranges),
					       config->term_warm_microvolt,
					       config->term_warm_microvolt, &idx);
	if (ret != 0) {
		return ret;
	}

	ret = reg_write(CHGR_BASE, CHGR_OFFSET_VTERM_R, idx);
	if (ret != 0) {
		return ret;
	}

	/* Set current, allow rounding down to closest value */
	ret = linear_range_get_win_index(&charger_current_range,
					 config->current_microamp - charger_current_range.step,
					 config->current_microamp, &idx);
	if (ret != 0) {
		return ret;
	}

	ret = reg_write2(CHGR_BASE, CHGR_OFFSET_ISET, idx / 2U, idx & 1U);
	if (ret != 0) {
		return ret;
	}

	/* Set discharge limit, allow rounding down to closest value */
	ret = linear_range_get_win_index(&discharge_limit_range,
					 config->dischg_limit_microamp - discharge_limit_range.step,
					 config->dischg_limit_microamp, &idx);
	if (ret != 0) {
		return ret;
	}

	ret = reg_write2(CHGR_BASE, CHGR_OFFSET_ISET_DISCHG, idx / 2U, idx & 1U);
	if (ret != 0) {
		return ret;
	}

	/* Configure vbus current limit */
	ret = linear_range_group_get_win_index(vbus_current_ranges, ARRAY_SIZE(vbus_current_ranges),
					       config->vbus_limit_microamp,
					       config->vbus_limit_microamp, &idx);
	if (ret != 0) {
		return ret;
	}
	ret = reg_write(VBUS_BASE, VBUS_OFFSET_ILIM, idx);
	if (ret != 0) {
		return ret;
	}

	/* Enable current measurement */
	ret = reg_write(ADC_BASE, ADC_OFFSET_IBAT_EN, 1U);
	if (ret != 0) {
		return ret;
	}

	/* Trigger current and voltage measurement */
	ret = reg_write(ADC_BASE, ADC_OFFSET_TASK_VBAT, 1U);
	if (ret != 0) {
		return ret;
	}

	/* Trigger temperature measurement */
	ret = reg_write(ADC_BASE, ADC_OFFSET_TASK_TEMP, 1U);
	if (ret != 0) {
		return ret;
	}

	/* Enable charging if configured */
	if (config->charging_enable) {
		ret = reg_write(CHGR_BASE, CHGR_OFFSET_EN_SET, 1U);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

