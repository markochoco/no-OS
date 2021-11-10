/***************************************************************************//**
 *   @file   iio_adxrs290.c
 *   @brief  Implementation of ADXRS290 iio.
 *   @author Kister Genesis Jimenez (kister.jimenez@analog.com)
********************************************************************************
 * Copyright 2020(c) Analog Devices, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  - Neither the name of Analog Devices, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *  - The use of this software may or may not infringe the patent rights
 *    of one or more patent holders.  This license does not release you
 *    from the requirement that you obtain separate licenses from these
 *    patent holders to use this software.
 *  - Use of the software either in source or binary form, must be run
 *    on or directly connected to an Analog Devices Inc. component.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ANALOG DEVICES BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, INTELLECTUAL PROPERTY RIGHTS, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#include "iio_types.h"
#include "iio.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "adxrs290.h"
#include "iio_adxrs290.h"
#include "util.h"
#include "irq.h"
#include "error.h"

/*
 * Available cut-off frequencies of the low pass filter in Hz.
 * The integer part and fractional part are represented separately.
 */
static const int adxrs290_lpf_3db_freq_hz_table[][2] = {
	[0] = {480, 0},
	[1] = {320, 0},
	[2] = {160, 0},
	[3] = {80, 0},
	[4] = {56, 600000},
	[5] = {40, 0},
	[6] = {28, 300000},
	[7] = {20, 0},
};

/*
 * Available cut-off frequencies of the high pass filter in Hz.
 * The integer part and fractional part are represented separately.
 */
static const int adxrs290_hpf_3db_freq_hz_table[][2] = {
	[0] = {0, 0},
	[1] = {0, 11000},
	[2] = {0, 22000},
	[3] = {0, 44000},
	[4] = {0, 87000},
	[5] = {0, 175000},
	[6] = {0, 350000},
	[7] = {0, 700000},
	[8] = {1, 400000},
	[9] = {2, 800000},
	[10] = {11, 300000},
};

static ssize_t get_adxrs290_iio_ch_raw(void *device, char *buf, size_t len,
				       const struct iio_ch_info *channel,
				       intptr_t priv)
{
	struct iio_adxrs290_desc *iio_dev = device;
	int16_t data;

	adxrs290_get_rate_data(iio_dev->dev, channel->ch_num, &data);
	if (channel->ch_num == ADXRS290_CHANNEL_TEMP)
		data = (data << 4) >> 4;

	return snprintf(buf, len, "%d", data);
}


static ssize_t get_adxrs290_iio_ch_scale(void *device, char *buf, size_t len,
		const struct iio_ch_info *channel,
		intptr_t priv)
{
	if (channel->ch_num == ADXRS290_CHANNEL_TEMP)
		// Temperature scale 1 LSB = 0.1 degree Celsius
		return snprintf(buf, len, "100");

	// Angular velocity scale 1 LSB = 0.005 degrees/sec = 0.000087266 rad/sec
	return snprintf(buf, len, "0.000087266");
}

static ssize_t get_adxrs290_iio_ch_hpf(void *device, char *buf, size_t len,
				       const struct iio_ch_info *channel,
				       intptr_t priv)
{
	struct iio_adxrs290_desc *iio_dev = device;
	uint8_t index;
	adxrs290_get_hpf(iio_dev->dev, &index);
	if (index > 0x0A)
		index = 0x0A;

	return snprintf(buf, len, "%d.%d",
			adxrs290_hpf_3db_freq_hz_table[index][0],
			adxrs290_hpf_3db_freq_hz_table[index][1]);
}

static ssize_t set_adxrs290_iio_ch_hpf(void *device, char *buf, size_t len,
				       const struct iio_ch_info *channel,
				       intptr_t priv)
{
	struct iio_adxrs290_desc *iio_dev = device;
	float hpf = strtof(buf, NULL);
	int32_t val = (int32_t)hpf;
	int32_t val2 = (int32_t)(hpf * 1000000) % 1000000;
	uint8_t i;
	uint8_t n = ARRAY_SIZE(adxrs290_hpf_3db_freq_hz_table);

	for (i = 0; i < n; i++)
		if (adxrs290_hpf_3db_freq_hz_table[i][0] == val
		    && adxrs290_hpf_3db_freq_hz_table[i][1] == val2) {
			adxrs290_set_hpf(iio_dev->dev, (enum adxrs290_hpf) i);

			return len;
		}

	return FAILURE;
}

static ssize_t get_adxrs290_iio_ch_lpf(void *device, char *buf, size_t len,
				       const struct iio_ch_info *channel,
				       intptr_t priv)
{
	struct iio_adxrs290_desc *iio_dev = device;
	uint8_t index;

	adxrs290_get_lpf(iio_dev->dev, &index);
	if (index > 0x07)
		index = 0x07;

	return snprintf(buf, len, "%d.%d", adxrs290_lpf_3db_freq_hz_table[index][0],
			adxrs290_lpf_3db_freq_hz_table[index][1]);
}

static ssize_t set_adxrs290_iio_ch_lpf(void *device, char *buf, size_t len,
				       const struct iio_ch_info *channel,
				       intptr_t priv)
{
	struct iio_adxrs290_desc *iio_dev = device;
	float lpf = strtof(buf, NULL);
	int32_t val = (int32_t)lpf;
	int32_t val2 = (int32_t)(lpf * 1000000) % 1000000;
	uint8_t i;
	uint8_t n = ARRAY_SIZE(adxrs290_lpf_3db_freq_hz_table);

	for (i = 0; i < n; i++)
		if (adxrs290_lpf_3db_freq_hz_table[i][0] == val
		    && adxrs290_lpf_3db_freq_hz_table[i][1] == val2) {
			adxrs290_set_lpf(iio_dev->dev, (enum adxrs290_lpf) i);
			return len;
		}

	return FAILURE;
}

static int32_t iio_adxrs290_reg_read(void *dev, uint8_t address, uint8_t *data)
{
	struct iio_adxrs290_desc *iio_dev = dev;

	return adxrs290_reg_read(iio_dev->dev, address, data);
}

static int32_t iio_adxrs290_reg_write(void *dev, uint8_t address, uint8_t data)
{
	struct iio_adxrs290_desc *iio_dev = dev;

	return adxrs290_reg_write(iio_dev->dev, address, data);
}

static int32_t adxrs290_update_active_channels(void *device, uint32_t mask)
{
	struct iio_adxrs290_desc *iio_dev = device;

	adxrs290_set_active_channels(iio_dev->dev, mask);

	adxrs290_set_op_mode(iio_dev->dev, ADXRS290_MODE_MEASUREMENT);

	irq_enable(iio_dev->irq_ctrl, iio_dev->irq_nb);

	iio_dev->mask = mask;

	return SUCCESS;
}

static int32_t adxrs290_end_transfer(void *device)
{
	struct iio_adxrs290_desc *iio_dev = device;

	adxrs290_set_active_channels(iio_dev->dev, 0);

	adxrs290_set_op_mode(iio_dev->dev, ADXRS290_MODE_STANDBY);

	irq_disable(iio_dev->irq_ctrl, iio_dev->irq_nb);

	return SUCCESS;
}

static void adxrs290_trigger_handler(void *device)
{
	int16_t data[ADXRS290_CHANNEL_COUNT];
	struct iio_adxrs290_desc *iio_dev = device;
	//uint8_t n;

	//adxrs290_get_burst_data(iio_dev->dev, data, &n);
	memset(data, 100, sizeof(data));

	cb_write(iio_dev->buf, data, hweight8(iio_dev->mask)*sizeof(int16_t));
}

static int32_t adxrs290_read_samples(void *device, uint16_t *buff,
				     uint32_t nb_samples)
{
	struct iio_adxrs290_desc *iio_dev = device;
	uint32_t len;
	uint32_t blen;

	len = nb_samples * hweight8(iio_dev->mask) * sizeof(int16_t);
	do {
		cb_size(iio_dev->buf, &blen);
	} while (blen < len);

	cb_read(iio_dev->buf, buff, len);

	return nb_samples;
}

static void adxrs290_irq_handler(void *ctx, uint32_t event, void *extra)
{
	struct iio_adxrs290_desc *dev = ctx;

	iio_trigger_notify(dev->trigger_name);
}

int32_t iio_adxrs290_cfg(struct iio_adxrs290_desc *desc,
			  struct iio_adxrs290_init_param *param)
{
	struct callback_desc call;
	int32_t ret;

	if (!param || !desc)
		return -EINVAL;

	memset(desc, 0, sizeof(*desc));

	desc->irq_ctrl = param->irq_ctrl;
	desc->dev = param->dev;
	desc->irq_nb = param->irq_nb;
	strcpy(desc->trigger_name, param->trigger_name);

	ret = cb_cfg(desc->buf, param->buf, param->buffer_size);
	if (IS_ERR_VALUE(ret))
		goto err_desc;

	ret = gpio_get(&desc->sync, param->gpio_sync);
	if (IS_ERR_VALUE(ret))
		goto err_desc;

	ret = gpio_direction_input(desc->sync);
	if (IS_ERR_VALUE(ret))
		goto err_gpio;

	call.callback = adxrs290_irq_handler;
	call.ctx = desc;
	call.config = param->irq_config;

	ret = irq_register_callback(desc->irq_ctrl, desc->irq_nb, &call);
	if (IS_ERR_VALUE(ret))
		goto err_gpio;

	irq_enable(desc->irq_ctrl, desc->irq_nb);

	return SUCCESS;

err_gpio:
	gpio_remove(desc->sync);
err_desc:
	free(desc);

	return ret;
}

int32_t iio_adxrs290_remove(struct iio_adxrs290_desc *desc)
{
	free(desc);

	return 0;
}

static struct iio_attribute adxrs290_iio_vel_attrs[] = {
	{
		.name = "filter_high_pass_3db_frequency",
		.show = get_adxrs290_iio_ch_hpf,
		.store = set_adxrs290_iio_ch_hpf
	},
	{
		.name = "filter_low_pass_3db_frequency",
		.show = get_adxrs290_iio_ch_lpf,
		.store = set_adxrs290_iio_ch_lpf
	},
	{
		.name = "raw",
		.show = get_adxrs290_iio_ch_raw,
		.store = NULL
	},
	{
		.name = "scale",
		.show = get_adxrs290_iio_ch_scale,
		.store = NULL
	},
	END_ATTRIBUTES_ARRAY
};

static struct iio_attribute adxrs290_iio_temp_attrs[] = {
	{
		.name = "raw",
		.show = get_adxrs290_iio_ch_raw,
		.store = NULL
	},
	{
		.name = "scale",
		.show = get_adxrs290_iio_ch_scale,
		.store = NULL
	},
	END_ATTRIBUTES_ARRAY,
};

static struct scan_type scan_type_gyro = {
	.sign = 's',
	.realbits = 16,
	.storagebits = 16,
	.shift = 0,
	.is_big_endian = false
};

static struct scan_type scan_type_temp = {
	.sign = 's',
	.realbits = 12,
	.storagebits = 16,
	.shift = 0,
	.is_big_endian = false
};

static struct iio_channel adxrs290_iio_channels[] = {
	{
		.ch_type = IIO_ANGL_VEL,
		.modified=1,
		.channel2=IIO_MOD_X,
		.scan_index = 0,
		.scan_type = &scan_type_gyro,
		.attributes = adxrs290_iio_vel_attrs,
		.ch_out = false,
	},
	{
		.ch_type = IIO_ANGL_VEL,
		.modified=1,
		.channel2=IIO_MOD_Y,
		.scan_index = 1,
		.scan_type = &scan_type_gyro,
		.attributes = adxrs290_iio_vel_attrs,
		.ch_out = false,
	},
	{
		.ch_type = IIO_TEMP,
		.scan_index = 2,
		.scan_type = &scan_type_temp,
		.attributes = adxrs290_iio_temp_attrs,
		.ch_out = false,
	}
};

struct iio_device adxrs290_iio_descriptor = {
	.num_ch = 3,
	.channels = adxrs290_iio_channels,
	.attributes = NULL,
	.debug_attributes = NULL,
	.buffer_attributes = NULL,
	.prepare_transfer = adxrs290_update_active_channels,
	.end_transfer = adxrs290_end_transfer,
	.read_dev = (int32_t (*)())adxrs290_read_samples,
	.debug_reg_read = (int32_t (*)())iio_adxrs290_reg_read,
	.debug_reg_write = (int32_t (*)())iio_adxrs290_reg_write
};

struct iio_trigger adxrs290_iio_trigger_descriptor = {
	.attributes = NULL,
	.trigger_handler = adxrs290_trigger_handler
};
