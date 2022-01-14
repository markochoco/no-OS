/***************************************************************************//**
 *   @file   iio_app.h
 *   @brief  Header file of iio_app
 *   @author Mihail Chindris (mihail.chindris@analog.com)
********************************************************************************
 * Copyright 2013(c) Analog Devices, Inc.
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

#ifndef IIO_APP
#define IIO_APP

#include "iio.h"
#include "no-os/irq.h"

#define IIO_APP_DEVICE(_name, _dev, _dev_descriptor, _read_buff, _write_buff) {\
	.name = _name,\
	.dev = _dev,\
	.dev_descriptor = _dev_descriptor,\
	.read_buff = _read_buff,\
	.write_buff = _write_buff\
}

#define IIO_APP_TRIGGER(_name, _dev, _trig_descriptor) {\
	.name = _name,\
	.dev = _dev,\
	.descriptor = _trig_descriptor,\
}

struct iio_data_buffer {
	uint32_t	size;
	void		*buff;
};

struct iio_app_device {
	char *name;
	void *dev;
	struct iio_device *dev_descriptor;
	struct iio_data_buffer *read_buff;
	struct iio_data_buffer *write_buff;
};

/**
 * @brief Register devices and start an iio application
 *
 * Configuration for communication is done in parameters.h
 * @param devices - is an array of devices to register to iiod
 * @param len - is the number of devices
 * @return 0 on success, negative value otherwise
 */
int32_t iio_app_run(struct iio_app_device *devices, int32_t len);

/* This function won't initialize irq_desc */
int32_t iio_app_run2(struct iio_app_device *devices, int32_t len,
		     struct iio_trigger_init *trigs, int32_t nb_trigs,
		     void *irq_desc, struct iio_desc **iio_desc);
#endif
