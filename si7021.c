/*
 * bmp085 thermometer/barometer driver
 *
 * Copyright (c) Maxim V Filimonov, 2015..2017
 * This code is distributed under the BSD 3 Clause License. I'm too lazy to attach the text. Just kurwa google it.
 */

#include "opt_platform.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/endian.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/sysctl.h>
#include <sys/systm.h>

#include <machine/bus.h>

#include <dev/iicbus/iicbus.h>
#include <dev/iicbus/iiconf.h>

#ifdef FDT
#include <dev/ofw/openfirm.h>
#include <dev/ofw/ofw_bus.h>
#include <dev/ofw/ofw_bus_subr.h>
#endif

// #include "bmp085.h"

// static bmp085_param param;

static int si7021_probe(device_t);
static int si7021_attach(device_t);
static int si7021_detach(device_t);

struct si7021_softc {
	device_t		sc_dev;
	struct intr_config_hook enum_hook;
	uint32_t		sc_addr;
};

static void si7021_start(void *);
static int si7021_read(device_t, uint32_t, uint8_t, uint8_t *, size_t);
static int si7021_write(device_t, uint32_t, uint8_t *, size_t);
static int si7021_temp_sysctl(SYSCTL_HANDLER_ARGS);
static int si7021_humidity_sysctl(SYSCTL_HANDLER_ARGS);

static device_method_t si7021_methods[] = {
	DEVMETHOD(device_probe, si7021_probe),
	DEVMETHOD(device_attach, si7021_attach),
	DEVMETHOD(device_detach, si7021_detach),

	DEVMETHOD_END
};

static driver_t si7021_driver = {
	"si7021",
	si7021_methods,
	sizeof (struct si7021_softc)
};

static devclass_t si7021_devclass;

DRIVER_MODULE(si7021, iicbus, si7021_driver, si7021_devclass, 0, 0); // copied from another one

static int si7021_read(device_t dev, uint32_t addr, uint8_t reg, uint8_t *data, size_t len) {
	struct iic_msg msg[2] = {
		{ addr, IIC_M_WR, 1, &reg },
		{ addr, IIC_M_RD, len, data },
	};

	if (iicbus_transfer(dev, msg, 2) != 0) {
		return -1;
	}
	return 0;
}

static int si7021_write(device_t dev, uint32_t addr, uint8_t *data, size_t len) {
	struct iic_msg msg[1] = {
		{ addr, IIC_M_WR, len, data },
	};

	if (iicbus_transfer(dev, msg, 1) != 0) {
		return -1;
	}
	return 0;
}

static int si7021_probe(device_t dev) {
	// device_printf(dev, "about to probe si7021\n");
	struct si7021_softc *sc;

	sc = device_get_softc(dev);
	// device_printf(dev, "probing si7021\n");
	
#ifdef FDT
	if (!ofw_bus_is_compatible(dev, "silicontech,si7021")) {
		return ENXIO;
	}
#endif
	device_set_desc(dev, "si7021 temperature/humidity sensor");
	// device_printf(dev, "about to return BUS_PROBE_GENERIC\n");
	return BUS_PROBE_GENERIC;
}

static int si7021_attach(device_t dev) {
	struct si7021_softc *sc;
	// device_printf(dev, "attaching si7021\n");

	sc = device_get_softc(dev);
	sc -> sc_dev = dev;
	sc -> sc_addr = iicbus_get_addr(dev);

	sc -> enum_hook.ich_func = si7021_start;
	sc -> enum_hook.ich_arg = dev;

	if (config_intrhook_establish(&sc->enum_hook) != 0) {
		return ENOMEM;
	}
	// device_printf(dev, "about to finish attaching device\n");
	return 0;
}

static int si7021_detach(device_t dev) {
	return 0;
}

static void si7021_start(void *xdev) {
	device_t dev;
	struct si7021_softc *sc;
	struct sysctl_ctx_list *ctx;
	struct sysctl_oid *tree_node;
	struct sysctl_oid_list *tree;

	dev = (device_t)xdev;
	// device_printf(dev, "about to start si7021\n");
	sc = device_get_softc(dev);
	ctx = device_get_sysctl_ctx(dev);
	tree_node = device_get_sysctl_tree(dev);
	tree = SYSCTL_CHILDREN(tree_node);

	config_intrhook_disestablish(&sc->enum_hook);

	SYSCTL_ADD_PROC(ctx, tree, OID_AUTO, "temperature",
			CTLTYPE_INT | CTLFLAG_RD | CTLFLAG_MPSAFE, dev, 33,
			si7021_temp_sysctl, "IK1", "Current temperature"); // IK1 stands for decikelvins
	SYSCTL_ADD_PROC(ctx, tree, OID_AUTO, "humidity",
			CTLTYPE_INT | CTLFLAG_RD | CTLFLAG_MPSAFE, dev, 34,
			si7021_humidity_sysctl, "I", "Current relative humidity");
	// now we're getting calibration parameters
	// uint8_t reg;
	// uint8_t buffer_rx[2];
	// device_printf(dev, "started si7021\n");
}

static int si7021_temp_sysctl(SYSCTL_HANDLER_ARGS) {
	device_t dev;
	struct si7021_softc *sc;
	int error;

	dev = (device_t)arg1;
	sc = device_get_softc(dev);
	int temperature = 0;

	// to be written
	//
	error = sysctl_handle_int(oidp, &temperature, 0, req);
	if (error != 0 || req -> newptr == NULL) {
		return error;
	}
	return 0;
}

static int si7021_humidity_sysctl(SYSCTL_HANDLER_ARGS) {
	device_t dev;
	struct si7021_softc *sc;
	int error;

	dev = (device_t)arg1;
	sc = device_get_softc(dev);

	int humidity = 0;

	// to be written
	uint8_t buffer_tx[2];
	uint8_t buffer_rx[2];
	buffer_tx = {SI_RH_NOMM, 0};
	if (si7021_write(sc->sc_dev, sc->sc_addr, buffer_tx, 2) != 0) {
		device_printf(dev, "couldnt write to device\n");
	}
	
	if (si7021_read(sc->sc_dev, sc->sc_addr, SI_RH_NOMM, buffer_rx, 2*sizeof(uint8_t)) != 0) {
		device_printf("couldnt read from device\n");
	}

	humidity = buffer_rx[0]*256 + buffer_rx[1];

	humidity *= 125;
	humidity /= 65536;
	humidity -= 6;

	error = sysctl_handle_int(oidp, &humidity, 0, req);
	if (error != 0 || req -> newptr == NULL) {
		return error;
	}
	return 0;

}
