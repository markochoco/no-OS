/***************************************************************************//**
 *   @file   maxim/maxim_spi.c
 *   @brief  Implementation of SPI driver.
 *   @author Ciprian Regus (ciprian.regus@analog.com)
********************************************************************************
 * Copyright 2022(c) Analog Devices, Inc.
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

/******************************************************************************/
/************************* Include Files **************************************/
/******************************************************************************/

#include <stdlib.h>
#include <errno.h>
#include "spi.h"
#include "mxc_errors.h"
#include "mxc_pins.h"
#include "spi_extra.h"
#include "no_os_spi.h"
#include "no_os_util.h"

#define SPI_MASTER_MODE	1
#define SPI_SINGLE_MODE	0

/******************************************************************************/
/************************ Functions Definitions *******************************/
/******************************************************************************/

/**
 * @brief Enable the chip select gpio on MAX32665 or MAX32650
 * @param spi - The SPI port registers
 * @param chip_select - cs index
 * @return 0 in case of success, -EINVAL otherwise
 */
static int32_t _max_spi_enable_ss(uint32_t id, uint32_t chip_select)
{
	mxc_gpio_cfg_t cs;

	/** This function will be a no-op for other chips, because this step is
	 * done in the SDK in those cases.
	*/
#if TARGET_NUM != 32650 && TARGET_NUM != 32665
	return 0;
#endif

	switch(id) {
	case 0:
		switch(chip_select) {
		case 0:
#if TARGET_NUM == 32665
			cs = gpio_cfg_spi0_ss0a;
#else
			cs = gpio_cfg_spi0_0;
#endif
			break;
			/** The SPI port 0 of the MAX32650 only has 1 chip select */
#if TARGET_NUM == 32665
		case 1:
			cs = gpio_cfg_spi0_ss1;
			break;
		case 2:
			cs = gpio_cfg_spi0_ss2;
			break;
#endif
		default:
			return -EINVAL;
		}
		break;
	case 1:
		switch(chip_select) {
		case 0:
			cs = gpio_cfg_spi1_ss0;
			break;
		case 1:
			cs = gpio_cfg_spi1_ss1;
			break;
		case 2:
			cs = gpio_cfg_spi1_ss2;
			break;
#if TARGET_NUM == 32650
		case 3:
			cs = gpio_cfg_spi1_ss3;
			break;
#endif
		default:
			return -EINVAL;
		}
		break;

	case 2:
		switch(chip_select) {
		case 0:
			cs = gpio_cfg_spi2_ss0;
			break;
		case 1:
			cs = gpio_cfg_spi2_ss1;
			break;
		case 2:
			cs = gpio_cfg_spi2_ss2;
			break;
#if TARGET_NUM == 32650
		case 3:
			cs = gpio_cfg_spi2_ss3;
			break;
#endif
		default:
			return -EINVAL;
		}
		break;
#ifdef MXC_SPI3
	case 3:
		switch(chip_select) {
		case 0:
			cs = gpio_cfg_spi2_ss0;
			break;
		case 1:
			cs = gpio_cfg_spi2_ss1;
			break;
		case 2:
			cs = gpio_cfg_spi2_ss2;
			break;
#if TARGET_NUM == 32650
		case 3:
			cs = gpio_cfg_spi3_ss3;
			break;
#endif
		default:
			return -EINVAL;
		}
		break;
#endif
	default:
		return -EINVAL;
	}

	MXC_GPIO_Config(&cs);

	return 0;
}

/**
 * @brief Initialize the SPI communication peripheral.
 * @param desc - The SPI descriptor.
 * @param param - The structure that contains the SPI parameters.
 * @return 0 in case of success, errno codes otherwise.
 */
int32_t max_spi_init(struct no_os_spi_desc **desc,
		     const struct no_os_spi_init_param *param)
{
	int32_t ret;
	struct no_os_spi_desc *descriptor;
	struct max_spi_init_param *eparam;

	if (!param || !param->extra)
		return -EINVAL;

	descriptor = calloc(1, sizeof(*descriptor));

	if (!descriptor)
		return -ENOMEM;

	eparam = param->extra;
	descriptor->device_id = param->device_id;
	descriptor->max_speed_hz = param->max_speed_hz;
	descriptor->chip_select = param->chip_select;
	descriptor->mode = param->mode;
	descriptor->bit_order = param->bit_order;
	descriptor->platform_ops = &max_spi_ops;

	if (descriptor->device_id >= MXC_SPI_INSTANCES) {
		ret = -EINVAL;
		goto err;
	}

#if TARGET_NUM == 32655 || TARGET_NUM == 78000
	mxc_spi_pins_t spi_pins_config = {
		.clock = true,
		.ss0 = (descriptor->chip_select == 0) ? true : false,
		.ss1 = (descriptor->chip_select == 1) ? true : false,
		.ss2 = (descriptor->chip_select == 2) ? true : false,
		.miso = true,
		.mosi = true,
		.sdio2 = false,
		.sdio3 = false,
		.vddioh = true
	};
#endif

#if TARGET_NUM == 32655 || TARGET_NUM == 78000
	ret = MXC_SPI_Init(MXC_SPI_GET_SPI(descriptor->device_id), SPI_MASTER_MODE,
			   SPI_SINGLE_MODE,
			   eparam->numSlaves, eparam->polarity, param->max_speed_hz, spi_pins_config);
#elif TARGET_NUM == 32665
	ret = MXC_SPI_Init(MXC_SPI_GET_SPI(descriptor->device_id), SPI_MASTER_MODE,
			   SPI_SINGLE_MODE,
			   eparam->numSlaves, eparam->polarity, param->max_speed_hz, MAP_A);
#else
	ret = MXC_SPI_Init(MXC_SPI_GET_SPI(descriptor->device_id), SPI_MASTER_MODE,
			   SPI_SINGLE_MODE,
			   eparam->numSlaves, eparam->polarity, param->max_speed_hz);
#endif
	if (ret) {
		ret = -EINVAL;
		goto err_init;
	}

	ret = _max_spi_enable_ss(descriptor->device_id, descriptor->chip_select);
	if (ret) {
		ret = -EINVAL;
		goto err_init;
	}

	ret = MXC_SPI_SetMode(MXC_SPI_GET_SPI(descriptor->device_id), descriptor->mode);
	if (ret) {
		ret = -EINVAL;
		goto err_init;
	}

	ret = MXC_SPI_SetWidth(MXC_SPI_GET_SPI(descriptor->device_id),
			       SPI_WIDTH_STANDARD);
	if (ret) {
		ret = -EINVAL;
		goto err_init;
	}

	ret = MXC_SPI_SetDataSize(MXC_SPI_GET_SPI(descriptor->device_id), 8);
	if (ret) {
		ret = -EINVAL;
		goto err_init;
	}

	*desc = descriptor;

	return 0;
err_init:
	MXC_SPI_Shutdown(MXC_SPI_GET_SPI(descriptor->device_id));
err:
	free(descriptor);

	return ret;
}

/**
 * @brief Free the resources allocated by no_os_spi_init().
 * @param desc - The SPI descriptor.
 * @return 0 in case of success, errno codes otherwise.
 */
int32_t max_spi_remove(struct no_os_spi_desc *desc)
{
	if (!desc)
		return -EINVAL;

	MXC_SPI_Shutdown(MXC_SPI_GET_SPI(desc->device_id));
	free(desc);

	return 0;
}

/**
 * @brief Write and read data to/from SPI.
 * @param desc - The SPI descriptor.
 * @param data - The buffer with the transmitted/received data.
 * @param bytes_number - Number of bytes to write/read.
 * @return 0 in case of success, errno codes otherwise.
 */
int32_t max_spi_write_and_read(struct no_os_spi_desc *desc,
			       uint8_t *data,
			       uint16_t bytes_number)
{
	int32_t ret;
	uint8_t *tx_buffer;
	uint8_t *rx_buffer;
	mxc_spi_req_t req;

	if (!desc || !data)
		return -EINVAL;

	rx_buffer = data;
	tx_buffer = data;

	req.spi = MXC_SPI_GET_SPI(desc->device_id);
	req.ssIdx = desc->chip_select;
	req.txData = tx_buffer;
	req.txCnt = 0;
	req.rxData = rx_buffer;
	req.rxCnt= 0;
	req.ssDeassert = 1;
	req.txLen= bytes_number;
	req.rxLen= bytes_number;

	ret = MXC_SPI_MasterTransaction(&req);

	if (ret == E_BAD_PARAM)
		return -EINVAL;
	if (ret == E_BAD_STATE)
		return -EBUSY;

	return 0;
}

/**
 * @brief Write/read multiple messages to/from SPI.
 * @param desc - The SPI descriptor.
 * @param msgs - The messages array.
 * @param len - Number of messages.
 * @return 0 in case of success, errno codes otherwise.
 */
int32_t max_spi_transfer(struct no_os_spi_desc *desc,
			 struct no_os_spi_msg *msgs,
			 uint32_t len)
{
	mxc_spi_req_t req;
	int32_t ret;

	if (!desc || !msgs)
		return -EINVAL;

	req.spi = MXC_SPI_GET_SPI(desc->device_id);
	for (uint32_t i = 0; i < len; i++) {
		req.txData = msgs[i].tx_buff;
		req.rxData = msgs[i].rx_buff;
		req.txCnt = 0;
		req.rxCnt = 0;
		req.ssDeassert = msgs[i].cs_change;
		req.txLen = msgs[i].bytes_number;
		req.rxLen = msgs[i].bytes_number;

		ret = MXC_SPI_MasterTransaction(&req);

		if (ret == E_BAD_PARAM)
			return -EINVAL;
		if (ret == E_BAD_STATE)
			return -EBUSY;
	}

	return 0;
}

/**
 * @brief maxim platform specific SPI platform ops structure
 */
const struct no_os_spi_platform_ops max_spi_ops = {
	.init = &max_spi_init,
	.write_and_read = &max_spi_write_and_read,
	.transfer = &max_spi_transfer,
	.remove = &max_spi_remove
};
