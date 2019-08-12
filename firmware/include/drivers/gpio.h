/*
 * This file is part of libgreat
 *
 * LPC43xx GPIO functions
 */

#include <toolchain.h>

#ifndef __LIBGREAT_GPIO_H__
#define __LIBGREAT_GPIO_H__

#include <drivers/platform_gpio.h>



/**
 * Configures the system's pinmux to route the given GPIO
 * pin to a physical pin.
 */
int gpio_configure_pinmux(gpio_pin_t pin);


/**
 * Configures the system's pinmux to route the given GPIO pin to a physical pin,
 * and sets up its pull resistors.
 */
int gpio_configure_pinmux_and_resistors(gpio_pin_t pin, gpio_resistor_configuration_t resistor_mode);


/**
 * Configures the system's pinmux to route all possible GPIO
 * pins for a given port.
 */
int gpio_configure_port_pinmuxes(uint8_t port);


/**
 * Configrues a GPIO port's pins to be either an input or an output.
 *
 * @param port The number of the port to be configured.
 * @param mask A bit-mask which selects which port bits are to be configured.
 * @param direction_bits A 32-bit value with bits set for each output, and
 *		bit clear for each bit to be set as an input.
 */
int gpio_set_port_direction(uint8_t port, uint32_t mask, uint32_t output_mask);


/**
 * Retrieves the direction of a given port's pins.
 *
 * @param port The number of the port to be configured.
 * @return A word with a 1 set for each pin that's an output, and a zero for each input.
 */
uint32_t gpio_get_port_direction(uint8_t port);


/**
 * Configures a GPIO pin to the provided value.
 *
 * @param port The number of the port to be configured.
 * @param pin The number of the pin to be configured.
 */
int gpio_set_pin_direction(gpio_pin_t pin, bool is_output);


/**
 * Retrieves the direction of a given GPIO pin.
 *
 * @param port The number of the port to be configured.
 * @return A word with a 1 set for each pin that's an output, and a zero for each input.
 */
uint32_t gpio_get_pin_direction(gpio_pin_t pin);



/**
 * Configrues a GPIO port's pin values.
 *
 * @param port The number of the port to be configured.
 * @param mask A bit-mask which selects which port bits are to be modified.
 * @param direction_bits A 32-bit value with bits set for each output.
 */
int gpio_set_port_value(uint8_t port, uint32_t mask, uint32_t value);


/**
 * Sets a collection of bits in a GPIO port.
 *
 * @param port The number of the port to be configured.
 * @param mask A bit-mask which selects which port bits are to be modified.
 */
int gpio_set_port_bits(uint8_t port, uint32_t mask);


/**
 * Clears a collection of bits in a GPIO port.
 *
 * @param port The number of the port to be configured.
 * @param mask A bit-mask which selects which port bits are to be modified.
 */
int gpio_clear_port_bits(uint8_t port, uint32_t mask);


/**
 * Clears a collection of bits in a GPIO port.
 *
 * @param port The number of the port to be configured.
 * @param mask A bit-mask which selects which port bits are to be modified.
 */
int gpio_toggle_port_bits(uint8_t port, uint32_t mask);


/**
 * Configrues a GPIO port's pin values.
 *
 * @param port The number of the port to be configured.
 * @param mask A bit-mask which selects which port bits are to be set.
 */
uint32_t gpio_get_port_value(uint8_t port, uint32_t mask);


/**
 * Sets a given GPIO to output the provided value.
 *
 * @param port The number of the port to be configured.
 * @param pin The number of the pin to be configured.
 * @param value 0 to clear the given pin, or any other value to set it.
 */
int gpio_set_pin_value(gpio_pin_t pin, uint8_t value);


/**
 * Sets a given GPIO to output 1/high.
 */
int gpio_set_pin(gpio_pin_t pin);


/**
 * Sets a given GPIO to output 0/low.
 */
int gpio_clear_pin(gpio_pin_t pin);


/**
 * Toggles a given GPIO pin's value.
 */
int gpio_toggle_pin(gpio_pin_t pin);


/**
 * Reads a given GPIO pin's value.
 *
 * @param port The number of the port to be read.
 * @param pin The number of the pin to be read.
 * @return 0 for a logic low, or 1 for a logic high
 */
uint8_t gpio_get_pin_value(gpio_pin_t pin);


/**
 * Fast method for reading a GPIO pin; intended for tight loops.
 *
 * @returns -1 (all 1's) if the bit is high, or 0 if the bit is low
 */
inline uint32_t gpio_fast_get_pin_value(gpio_pin_t pin);

#endif // __LIBGREAT_GPIO_H__
