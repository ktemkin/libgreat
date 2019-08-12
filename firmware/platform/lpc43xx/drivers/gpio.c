/*
 * This file is part of libgreat
 *
 * LPC43xx GPIO functions
 */

#include <stdint.h>
#include <errno.h>

#include <toolchain.h>
#include <debug.h>

#include <drivers/scu.h>
#include <drivers/gpio.h>


/* Physical locations of the GPIO parameters. */
#define GPIO_LPC_BASE			 (0x400f4000)
#define GPIO_LPC_PIN_WORD_OFFSET (0x1000)
#define GPIO_LPC_PORT_OFFSET	 (0x2000)
#define GPIO_LPC_PIN_WORD_SIZE	 (32 * sizeof(uint32_t))

/**
 * Structure representing the in-memory layout of a GPIO peripheral.
 */
struct ATTR_PACKED gpio_registers {

	/** Data direction register. 1 = output, 0 = input */
	volatile uint32_t direction;		/* +0x000 */
	uint32_t _reserved0[31];

	/* Mask for masked operations. */
	volatile uint32_t mask;		/* +0x080 */
	uint32_t _reserved1[31];

	/* Pin access register; for reading and writing pin values. */
	volatile uint32_t pins;		/* +0x100 */
	uint32_t _reserved2[31];

	/* Masked pin access register: for reading and writing as filtered by a mask. */
	volatile uint32_t masked_pins;		/* +0x180 */
	uint32_t _reserved3[31];

	/* Bits used for setting a given GPIO pin. */
	volatile uint32_t set;		/* +0x200 */
	uint32_t _reserved4[31];

	/* Bits used for clearing a given GPIO pin. */
	volatile uint32_t clear;		/* +0x280 */
	uint32_t _reserved5[31];

	/* Bits used for toggling a given GPIO pin. */
	volatile uint32_t toggle;		/* +0x300 */
};


/**
 * Validates that the given port number corresponds to a real/valid
 * port that we can access.
 *
 * @return 0 on success, or an error code on failures
 */
static int validate_port(uint8_t port)
{
	if (port > GPIO_MAX_PORTS) {
		pr_warning("gpio: requested a non-existent port (port %d)\n", port);
		return EINVAL;
	}

	return 0;
}


/**
 * Validates that the given port and pin number correspond to a real/valid
 * pin that we can access.
 *
 * @return 0 on success, or an error code on failures
 */
static int validate_port_and_pin(gpio_pin_t pin)
{
	if (validate_port(pin.port) != 0) {
		return EINVAL;
	}

	if (pin.port > GPIO_MAX_PORT_BITS) {
		pr_warning("gpio: requested a non-existent pin (port %u, pin %u)\n", pin.port, pin.pin);
		return EINVAL;
	}

	return 0;
}

/**
 * Gets a reference to the GPIO register block for the given port.
 */
static struct gpio_registers *gpio_get_registers(uint8_t port)
{
	uintptr_t port_address = (GPIO_LPC_BASE + GPIO_LPC_PORT_OFFSET) + (port * sizeof(uint32_t));
	return (struct gpio_registers*)port_address;
}


/**
 * Gets a reference to the GPIO register block for the given port.
 */
static uint32_t *gpio_get_pin_register(gpio_pin_t pin)
{
	uintptr_t pin_address =
		(GPIO_LPC_BASE + GPIO_LPC_PIN_WORD_OFFSET) + // Offset of the pin-word region
		(pin.port * GPIO_LPC_PIN_WORD_SIZE) +  // Offset of the given port in the pin-word region
		(pin.pin * sizeof(uint32_t)); // Offset of the pin in the port in the pin-word region

	return (uint32_t*)pin_address;
}


/**
 * Mapping of LPC GPIO pins to their relevant group.
 */
static const uint8_t gpio_to_pin_group[GPIO_MAX_PORTS][GPIO_MAX_PORT_BITS] = {
  /* GPIO0 */
  {
  0,  /* GPIO0[0]	J1_4  */
  0,  /* GPIO0[1]	J1_6  */
  1,  /* GPIO0[2]	J1_28 */
  1,  /* GPIO0[3]	J1_30 */
  1,  /* GPIO0[4]	J1_7  */
  6,  /* GPIO0[5]	J2_34 */
  3,  /* GPIO0[6]	J2_38 */
  2,  /* GPIO0[7]	J2_14 */
  1,  /* GPIO0[8]	J1_10 */
  1,  /* GPIO0[9]	J1_12 */
  1,  /* GPIO0[10]	J1_40 */
  1,  /* GPIO0[11]	J1_39 */
  1,  /* GPIO0[12]	J1_32 */
  1,  /* GPIO0[13]	J1_31 */
  2,  /* GPIO0[14]	J7_14 */
  1,  /* GPIO0[15]	J1_37 */
  -1,	   /* GPIO0[16]		   */
  -1,	   /* GPIO0[17]		   */
  -1,	   /* GPIO0[18]		   */
  -1,	   /* GPIO0[19]		   */
  },
  /* GPIO1 */
  {
  1,  /* GPIO1[0]	J1_15 */
  1,  /* GPIO1[1]	J1_18 */
  1,  /* GPIO1[2]	J1_17 */
  1,  /* GPIO1[3]	J1_20 */
  1,  /* GPIO1[4]	J1_22 */
  1,  /* GPIO1[5]	J1_21 */
  1,  /* GPIO1[6]	J1_26 */
  1,  /* GPIO1[7]	J1_25 */
  1,  /* GPIO1[8]	J1_13 */
  1,  /* GPIO1[9]	J1_16 */
  2,  /* GPIO1[10]	J7_6  */
  2,  /* GPIO1[11]	J7_13 */
  2,  /* GPIO1[12]	J7_7  */
  2,  /* GPIO1[13]	J7_8  */
  3,  /* GPIO1[14]	J2_28 */
  3,  /* GPIO1[15]	J2_37 */
  -1,	   /* GPIO1[16]		   */
  -1,	   /* GPIO1[17]		   */
  -1,	   /* GPIO1[18]		   */
  -1,	   /* GPIO1[19]		   */
  },
  /* GPIO2 */
  {
  4,   /* GPIO2[0]	 J2_4  */
  -1,	   /* GPIO2[1]		   */
  4,   /* GPIO2[2]	 J2_8  */
  4,   /* GPIO2[3]	 J2_9  */
  4,   /* GPIO2[4]	 J2_7  */
  4,   /* GPIO2[5]	 J2_6  */
  4,   /* GPIO2[6]	 J2_10 */
  5,   /* GPIO2[7]	 J1_29 */
  -1,	   /* GPIO2[8]		   */
  5,   /* GPIO2[9]	 J1_8  */
  5,   /* GPIO2[10]  J1_9  */
  5,   /* GPIO2[11]  J1_14 */
  5,   /* GPIO2[12]  J1_19 */
  5,   /* GPIO2[13]  J1_24 */
  5,   /* GPIO2[14]  J1_23 */
  5,   /* GPIO2[15]  J1_27 */
  -1,	   /* GPIO2[16]		   */
  -1,	   /* GPIO2[17]		   */
  -1,	   /* GPIO2[18]		   */
  -1,	   /* GPIO2[19]		   */
  },
  /* GPIO3 */
  {
  6, /* GPIO3[0]   J7_18 */
  6, /* GPIO3[1]   J7_17 */
  6, /* GPIO3[2]   J2_36 */
  6, /* GPIO3[3]   J7_2  */
  6, /* GPIO3[4]   J7_3  */
  6, /* GPIO3[5]   J7_16 */
  6, /* GPIO3[6]   J7_15 */
  -1,	   /* GPIO3[7]		   */
  7,   /* GPIO3[8]	 J2_27 */
  7,   /* GPIO3[9]	 J2_25 */
  7,   /* GPIO3[10]  J2_23 */
  -1,	   /* GPIO3[11]		   */
  -1,	   /* GPIO3[12]		   */
  -1,	   /* GPIO3[13]		   */
  -1,	   /* GPIO3[14]		   */
  7,   /* GPIO3[15]  J2_16 */
  -1,	   /* GPIO3[16]		   */
  -1,	   /* GPIO3[17]		   */
  -1,	   /* GPIO3[18]		   */
  -1,	   /* GPIO3[19]		   */
  },
  /* GPIO4 */
  {
  -1,	   /* GPIO4[0]		   */
  -1,	   /* GPIO4[1]		   */
  -1,	   /* GPIO4[2]		   */
  -1,	   /* GPIO4[3]		   */
  -1,	   /* GPIO4[4]		   */
  -1,	   /* GPIO4[5]		   */
  -1,	   /* GPIO4[6]		   */
  -1,	   /* GPIO4[7]		   */
  -1,	   /* GPIO4[8]		   */
  -1,	   /* GPIO4[9]		   */
  -1,	   /* GPIO4[10]		   */
  9,   /* GPIO4[11]  J1_34 */
  -1,	   /* GPIO4[12]		   */
  -1,	   /* GPIO4[13]		   */
  -1,	   /* GPIO4[14]		   */
  -1,	   /* GPIO4[15]		   */
  -1,	   /* GPIO4[16]		   */
  -1,	   /* GPIO4[17]		   */
  -1,	   /* GPIO4[18]		   */
  -1,	   /* GPIO4[19]		   */
  },
  /* GPIO5 */
  {
  2,   /* GPIO5[0]	 J1_35 */
  2,   /* GPIO5[1]	 J2_35 */
  2,   /* GPIO5[2]	 J2_33 */
  2,   /* GPIO5[3]	 J2_20 */
  2,   /* GPIO5[4]	 J2_19 */
  2,   /* GPIO5[5]	 J2_18 */
  2,   /* GPIO5[6]	 J2_15 */
  2,   /* GPIO5[7]	 J2_13 */
  3,   /* GPIO5[8]	 J2_24 */
  3,   /* GPIO5[9]	 J2_22 */
  3,   /* GPIO5[10]  J2_30 */
  -1,	   /* GPIO5[11]		   */
  4,   /* GPIO5[12]  J2_3  */
  4,   /* GPIO5[13]  J1_3  */
  4,  /* GPIO5[14]	J1_5  */
  6,   /* GPIO5[15]  J2_31 */
  6,   /* GPIO5[16]  J2_29 */
  -1,	   /* GPIO5[17]		   */
  9,   /* GPIO5[18]  J1_33 */
  -1,	   /* GPIO5[19]		   */
  },
};

static const uint8_t gpio_to_pin_number[GPIO_MAX_PORTS][GPIO_MAX_PORT_BITS] = {
  /* GPIO0 */
  {
  0,   /* GPIO0[0]	 J1_4  */
  1,   /* GPIO0[1]	 J1_6  */
  15,  /* GPIO0[2]	 J1_28 */
  16,  /* GPIO0[3]	 J1_30 */
  0,   /* GPIO0[4]	 J1_7  */
  6,   /* GPIO0[5]	 J2_34 */
  6,   /* GPIO0[6]	 J2_38 */
  7,   /* GPIO0[7]	 J2_14 */
  1,   /* GPIO0[8]	 J1_10 */
  2,   /* GPIO0[9]	 J1_12 */
  3,   /* GPIO0[10]  J1_40 */
  4,   /* GPIO0[11]  J1_39 */
  17,  /* GPIO0[12]  J1_32 */
  18,  /* GPIO0[13]  J1_31 */
  10,  /* GPIO0[14]  J7_14 */
  20,  /* GPIO0[15]  J1_37 */
  -1,	   /* GPIO0[16]		   */
  -1,	   /* GPIO0[17]		   */
  -1,	   /* GPIO0[18]		   */
  -1,	   /* GPIO0[19]		   */
  },
  /* GPIO1 */
  {
  7,   /* GPIO1[0]	 J1_15 */
  8,   /* GPIO1[1]	 J1_18 */
  9,   /* GPIO1[2]	 J1_17 */
  10,  /* GPIO1[3]	 J1_20 */
  11,  /* GPIO1[4]	 J1_22 */
  12,  /* GPIO1[5]	 J1_21 */
  13,  /* GPIO1[6]	 J1_26 */
  14,  /* GPIO1[7]	 J1_25 */
  5,   /* GPIO1[8]	 J1_13 */
  6,   /* GPIO1[9]	 J1_16 */
  9,   /* GPIO1[10]  J7_6  */
  11,  /* GPIO1[11]  J7_13 */
  12,  /* GPIO1[12]  J7_7  */
  13,  /* GPIO1[13]  J7_8  */
  4,   /* GPIO1[14]  J2_28 */
  5,   /* GPIO1[15]  J2_37 */
  -1,	   /* GPIO1[16]		   */
  -1,	   /* GPIO1[17]		   */
  -1,	   /* GPIO1[18]		   */
  -1,	   /* GPIO1[19]		   */
  },
  /* GPIO2 */
  {
  0,   /* GPIO2[0]	 J2_4  */
  -1,	   /* GPIO2[1]		   */
  2,   /* GPIO2[2]	 J2_8  */
  3,   /* GPIO2[3]	 J2_9  */
  4,   /* GPIO2[4]	 J2_7  */
  5,   /* GPIO2[5]	 J2_6  */
  6,   /* GPIO2[6]	 J2_10 */
  7,   /* GPIO2[7]	 J1_29 */
  -1,	   /* GPIO2[8]		   */
  0,   /* GPIO2[9]	 J1_8  */
  1,   /* GPIO2[10]  J1_9  */
  2,   /* GPIO2[11]  J1_14 */
  3,   /* GPIO2[12]  J1_19 */
  4,   /* GPIO2[13]  J1_24 */
  5,   /* GPIO2[14]  J1_23 */
  6,   /* GPIO2[15]  J1_27 */
  -1	,	   /* GPIO2[16]		   */
  -1	,	   /* GPIO2[17]		   */
  -1	,	   /* GPIO2[18]		   */
  -1	,	   /* GPIO2[19]		   */
  },
  /* GPIO3 */
  {
  1,   /* GPIO3[0]	 J7_18 */
  2,   /* GPIO3[1]	 J7_17 */
  3,   /* GPIO3[2]	 J2_36 */
  4,   /* GPIO3[3]	 J7_2  */
  5,   /* GPIO3[4]	 J7_3  */
  9,   /* GPIO3[5]	 J7_16 */
  10,  /* GPIO3[6]	 J7_15 */
  -1,	   /* GPIO3[7]		   */
  0,   /* GPIO3[8]	 J2_27 */
  1,   /* GPIO3[9]	 J2_25 */
  2,   /* GPIO3[10]  J2_23 */
  -1,	   /* GPIO3[11]		   */
  -1,	   /* GPIO3[12]		   */
  -1,	   /* GPIO3[13]		   */
  -1,	   /* GPIO3[14]		   */
  7,   /* GPIO3[15]  J2_16 */
  -1,	   /* GPIO3[16]		   */
  -1,	   /* GPIO3[17]		   */
  -1,	   /* GPIO3[18]		   */
  -1,	   /* GPIO3[19]		   */
  },
  /* GPIO4 */
  {
  -1,	   /* GPIO4[0]		   */
  -1,	   /* GPIO4[1]		   */
  -1,	   /* GPIO4[2]		   */
  -1,	   /* GPIO4[3]		   */
  -1,	   /* GPIO4[4]		   */
  -1,	   /* GPIO4[5]		   */
  -1,	   /* GPIO4[6]		   */
  -1,	   /* GPIO4[7]		   */
  -1,	   /* GPIO4[8]		   */
  -1,	   /* GPIO4[9]		   */
  -1,	   /* GPIO4[10]		   */
  6,   /* GPIO4[11]  J1_34 */
  -1,	   /* GPIO4[12]		   */
  -1,	   /* GPIO4[13]		   */
  -1,	   /* GPIO4[14]		   */
  -1,	   /* GPIO4[15]		   */
  -1,	   /* GPIO4[16]		   */
  -1,	   /* GPIO4[17]		   */
  -1,	   /* GPIO4[18]		   */
  -1,	   /* GPIO4[19]		   */
  },
  /* GPIO5 */
  {
  0,   /* GPIO5[0]	 J1_35 */
  1,   /* GPIO5[1]	 J2_35 */
  2,   /* GPIO5[2]	 J2_33 */
  3,   /* GPIO5[3]	 J2_20 */
  4,   /* GPIO5[4]	 J2_19 */
  5,   /* GPIO5[5]	 J2_18 */
  6,   /* GPIO5[6]	 J2_15 */
  8,   /* GPIO5[7]	 J2_13 */
  1,   /* GPIO5[8]	 J2_24 */
  2,   /* GPIO5[9]	 J2_22 */
  7,   /* GPIO5[10]  J2_30 */
  -1,	   /* GPIO5[11]		   */
  8,   /* GPIO5[12]  J2_3  */
  9,   /* GPIO5[13]  J1_3  */
  10,  /* GPIO5[14]  J1_5  */
  7,   /* GPIO5[15]  J2_31 */
  8,   /* GPIO5[16]  J2_29 */
  -1,	   /* GPIO5[17]		   */
  5,   /* GPIO5[18]  J1_33 */
  -1,	   /* GPIO5[19]		   */
  },
};


/**
 * Returns the SCU group number for the given GPIO bit.
 */
uint8_t gpio_get_group_number(gpio_pin_t pin)
{
	if (validate_port_and_pin(pin) != 0) {
		return -1;
	}

	return gpio_to_pin_group[pin.port][pin.pin];
}


/**
 * Returns the SCU pin number for the given GPIO bit.
 */
uint8_t gpio_get_pin_number(gpio_pin_t pin)
{
	if (validate_port_and_pin(pin) != 0) {
		return -1;
	}

	return gpio_to_pin_number[pin.port][pin.pin];
}




/**
 * Configures the system's pinmux to route the given GPIO pin to a physical pin.
 */
int gpio_configure_pinmux_and_resistors(gpio_pin_t pin, gpio_resistor_configuration_t resistor_mode)
{
	uint32_t scu_function;
	uint8_t scu_group, scu_pin;

	if (validate_port_and_pin(pin)) {
		return EINVAL;
	}

	// Get the SCU group/pin so we can pinmux.
	scu_group = gpio_get_group_number(pin);
	scu_pin   = gpio_get_pin_number(pin);

	// If this port/pin doesn't correspond to a valid physical pin,
	// fail out.
	if ((scu_group == 0xff) || (scu_pin == 0xff)) {
		return EINVAL;
	}

	// Select the pinmux function to apply.
	scu_function = (pin.port == 5) ? 4 : 0;

	// Finally, configure the SCU.
	platform_scu_configure_pin_gpio(scu_group, scu_pin, scu_function, resistor_mode);
	return 0;
}



/**
 * Configures the system's pinmux to route the given GPIO
 * pin to a physical pin.
 */
int gpio_configure_pinmux(gpio_pin_t pin)
{
	gpio_configure_pinmux_and_resistors(pin, SCU_NO_PULL);
}


/**
 * Configures the system's pinmux to route all possible GPIO
 * pins for a given port.
 */
int gpio_configure_port_pinmuxes(uint8_t port)
{
	if (validate_port(port)) {
		return EINVAL;
	}

	// Try to configure every possible pin, trusting our
	// lower-level logic to reject any pin that can't be routed.
	for (int pin = 0; pin < GPIO_MAX_PORT_BITS; ++pin) {
		gpio_configure_pinmux(gpio_pin(port, pin));
	}

	return 0;
}


/**
 * Configrues a GPIO port's pins to be either an input or an output.
 *
 * @param port The number of the port to be configured.
 * @param mask A bit-mask which selects which port bits are to be configured.
 * @param direction_bits A 32-bit value with bits set for each output, and
 *		bit clear for each bit to be set as an input.
 */
int gpio_set_port_direction(uint8_t port, uint32_t mask, uint32_t output_mask)
{
	struct gpio_registers *reg = gpio_get_registers(port);
	uint32_t to_apply;

	if (validate_port(port) != 0) {
		return EINVAL;
	}

	// Get a reference to the relevant GPIO bank's registers.
	to_apply = reg->direction;

	// Apply the input mask.
	to_apply &= ~mask;
	to_apply |= output_mask;

	reg->direction = to_apply;
	return 0;
}


/**
 * Retrieves the direction of a given port's pins.
 *
 * @param port The number of the port to be configured.
 * @return A word with a 1 set for each pin that's an output, and a zero for each input.
 */
uint32_t gpio_get_port_direction(uint8_t port)
{
	struct gpio_registers *reg = gpio_get_registers(port);

	if (validate_port(port) != 0) {
		return 0;
	}

	return reg->direction;
}


/**
 * Retrieves the direction of a given GPIO pin.
 *
 * @param port The number of the port to be configured.
 * @return A word with a 1 set for each pin that's an output, and a zero for each input.
 */
uint32_t gpio_get_pin_direction(gpio_pin_t pin)
{
	uint32_t pins = gpio_get_port_direction(pin.port);

	if (validate_port_and_pin(pin) != 0) {
		return 0;
	}

	return (pins >> pin.pin) & 1;
}


/**
 * Configures a GPIO pin to the provided value.
 *
 * @param port The number of the port to be configured.
 * @param pin The number of the pin to be configured.
 */
int gpio_set_pin_direction(gpio_pin_t pin, bool is_output)
{
	uint32_t mask = 1 << pin.pin;

	if (validate_port_and_pin(pin) != 0) {
		return EINVAL;
	}

	return gpio_set_port_direction(pin.port, mask, is_output ? mask : 0);
}


/**
 * Configrues a GPIO port's pin values.
 *
 * @param port The number of the port to be configured.
 * @param mask A bit-mask which selects which port bits are to be modified.
 * @param direction_bits A 32-bit value with bits set for each output.
 */
int gpio_set_port_value(uint8_t port, uint32_t mask, uint32_t value)
{
	struct gpio_registers *reg = gpio_get_registers(port);

	if (validate_port(port) != 0) {
		return EINVAL;
	}

	// Use the hardware pin-masking feature to write the given values.
	reg->mask = mask;
	reg->masked_pins = value;
	return 0;
}


/**
 * Sets a collection of bits in a GPIO port.
 *
 * @param port The number of the port to be configured.
 * @param mask A bit-mask which selects which port bits are to be modified.
 */
int gpio_set_port_bits(uint8_t port, uint32_t mask)
{
	struct gpio_registers *reg = gpio_get_registers(port);

	if (validate_port(port) != 0) {
		return EINVAL;
	}

	// Use the hardware pin-masking feature to write the given values.
	reg->set = mask;
	return 0;
}


/**
 * Clears a collection of bits in a GPIO port.
 *
 * @param port The number of the port to be configured.
 * @param mask A bit-mask which selects which port bits are to be modified.
 */
int gpio_clear_port_bits(uint8_t port, uint32_t mask)
{
	struct gpio_registers *reg = gpio_get_registers(port);

	if (validate_port(port) != 0) {
		return EINVAL;
	}

	// Use the hardware pin-masking feature to write the given values.
	reg->clear = mask;
	return 0;
}


/**
 * Clears a collection of bits in a GPIO port.
 *
 * @param port The number of the port to be configured.
 * @param mask A bit-mask which selects which port bits are to be modified.
 */
int gpio_toggle_port_bits(uint8_t port, uint32_t mask)
{
	struct gpio_registers *reg = gpio_get_registers(port);

	if (validate_port(port) != 0) {
		return EINVAL;
	}

	// Use the hardware pin-masking feature to write the given values.
	reg->toggle = mask;
	return 0;
}


/**
 * Configrues a GPIO port's pin values.
 *
 * @param port The number of the port to be configured.
 * @param mask A bit-mask which selects which port bits are to be set.
 * @param direction_bits A 32-bit value with bits set for each output.
 */
uint32_t gpio_get_port_value(uint8_t port, uint32_t mask)
{
	struct gpio_registers *reg = gpio_get_registers(port);

	if (validate_port(port) != 0) {
		return EINVAL;
	}

	// Use the hardware pin-masking feature to write the given values.
	reg->mask = mask;
	return reg->masked_pins;
}


/**
 * Configrues a GPIO pin's value.
 *
 * @param port The number of the port to be configured.
 * @param pin The number of the pin to be configured.
 * @param value 0 to clear the given pin, or any other value to set it.
 */
int gpio_set_pin_value(gpio_pin_t pin, uint8_t value)
{
	uint32_t *pin_reg = gpio_get_pin_register(pin);

	if (validate_port_and_pin(pin) != 0) {
		return EINVAL;
	}

	// Use the hardware pin-masking feature to write the given values.
	*pin_reg = value;
	return 0;
}

/**
 * Configrues a GPIO pin's value.
 *
 * @param port The number of the port to be configured.
 * @param pin The number of the pin to be configured.
 * @param value 0 to clear the given pin, or any other value to set it.
 */
int gpio_set_pin(gpio_pin_t pin)
{
	if (validate_port_and_pin(pin) != 0) {
		return EINVAL;
	}

	gpio_set_port_bits(pin.port, 1 << pin.pin);
	return 0;
}


/**
 * Configrues a GPIO pin's value.
 *
 * @param port The number of the port to be configured.
 * @param pin The number of the pin to be configured.
 * @param value 0 to clear the given pin, or any other value to set it.
 */
int gpio_clear_pin(gpio_pin_t pin)
{
	if (validate_port_and_pin(pin) != 0) {
		return EINVAL;
	}

	gpio_clear_port_bits(pin.port, 1 << pin.pin);
	return 0;
}


/**
 * Configrues a GPIO pin's value.
 *
 * @param port The number of the port to be configured.
 * @param pin The number of the pin to be configured.
 * @param value 0 to clear the given pin, or any other value to set it.
 */
int gpio_toggle_pin(gpio_pin_t pin)
{
	if (validate_port_and_pin(pin) != 0) {
		return EINVAL;
	}

	gpio_toggle_port_bits(pin.port, 1 << pin.pin);
	return 0;
}

/**
 * Configrues a GPIO pin's value.
 *
 * @param port The number of the port to be configured.
 * @param pin The number of the pin to be configured.
 * @return 0 for a logic low, or 1 for a logic high
 */
uint8_t gpio_get_pin_value(gpio_pin_t pin)
{
	uint32_t *pin_reg = gpio_get_pin_register(pin);

	if (validate_port_and_pin(pin) != 0) {
		return EINVAL;
	}

	// Use the hardware pin-masking feature to write the given values.
	return (*pin_reg) ? 1 : 0;
}


/**
 * Fast method for reading a GPIO pin; intended for tight loops.
 *
 * @returns -1 (all 1's) if the bit is high, or 0 if the bit is low
 */
uint32_t gpio_fast_get_pin_value(gpio_pin_t pin)
{
	return *gpio_get_pin_register(pin);
}



/**
 * LPC43xx specicfic register that grabs a GPIO pin word-access register.
 *
 * @returns a register that always contains -1 (all 1's) if the bit is high, or 0 if the bit is low
 */
uint32_t *platform_gpio_get_pin_register(gpio_pin_t pin)
{
	return gpio_get_pin_register(pin);
}
