/*
 * LPC43xx State Configurable Timer
 *
 * This file is part of libgreat.
 */

#include <toolchain.h>

#ifndef __LIBGREAT_PLATFORM_SCT__
#define __LIBGREAT_PLATFORM_SCT__

/**
 * Structure representing registers that have a low and a high component.
 */
typedef union {
	uint32_t all;
	struct ATTR_PACKED {
		uint16_t high;
		uint16_t low;
	};
} split_register_t;


typedef struct ATTR_PACKED {
	uint32_t enabled_in_state;

	// Predicates.
	struct {
		uint32_t associated_match_register     :  4;
		uint32_t associated_with_high_register :  1;
		uint32_t controls_output               :  1;
		uint32_t associated_io_pin             :  4; // 10


		uint32_t associated_io_condition       :  2;
		uint32_t condition                     :  2;
		uint32_t load_state                    :  1;
		uint32_t next_state                    :  5; // 20

		uint32_t                               : 12; // 32
	};
} event_registers_t;

typedef struct ATTR_PACKED {
	uint32_t set;
	uint32_t clear;
} output_registers_t;


// Predicate flags for I/O conditions.
typedef enum {
	IO_CONDITION_LOW  = 0,
	IO_CONDITION_RISE = 1,
	IO_CONDITION_FALL = 2,
	IO_CONDITION_HIGH = 3,
} io_condition_t;


typedef enum {
	SCT_COUNT_ON_BUS_CLOCK   = 0,
	SCT_COUNT_ON_INPUT       = 1,
	SCT_RUN_OFF_INPUT_CLOCK  = 2,
} sct_clock_mode_t;


// Predicate flags for I/O conditions.
enum {
	ON_MATCH_OR_IO    = 0,
	ON_MATCH          = 1,
	ON_IO             = 2,
	ON_MATCH_AND_IO   = 3,
};

enum {
	SCT_CLK_INPUT_RISING  = 0,
	SCT_CLK_INPUT_FALLING = 1,
	SCT_CLK_INPUT_SHIFT   = 1
};


typedef struct ATTR_PACKED {
	uint16_t counter_should_count_down              : 1;
	uint16_t pause_counter                          : 1;
	uint16_t halt_sct                               : 1;
	uint16_t clear_counter_value                    : 1;
	uint16_t counter_switches_direction_on_overflow : 1;
	uint16_t count_prescaler                        : 8;
	uint16_t                                        : 3;
} sct_control_register_t;



/**
 * Registers that control the LPC43xx state controlled timer.
 */
typedef volatile struct ATTR_PACKED {

	/**
	 * General SCT configuration.
	 */
	struct {
		uint32_t use_both_halves_as_one            : 1;
		uint32_t clock_mode                        : 2;
		uint32_t clock_on_falling_edges            : 1;
		uint32_t clock_input_number                : 3;
		uint32_t prevent_lower_half_from_reloading : 1;

		uint32_t prevent_upper_half_from_reloading : 1;
		uint32_t synchronize_input_0               : 1;
		uint32_t synchronize_input_1               : 1;
		uint32_t synchronize_input_2               : 1;
		uint32_t synchronize_input_3               : 1;
		uint32_t synchronize_input_4               : 1;
		uint32_t synchronize_input_5               : 1;
		uint32_t synchronize_input_6               : 1;

		uint32_t synchronize_input_7               : 1;
		uint32_t                                   : 15;
	};

	/**
	 * SCT control registers.
	 */
	union {
		sct_control_register_t control;
		sct_control_register_t control_low;
	};
	sct_control_register_t control_high;

	/**
	 * Registers that determine which events clear our counter.
	 */
	split_register_t clear_counter_on_event;

	/**
	 * Registers that determine which events start/stop/halt the SCT counter.
	 */
	split_register_t halt_on_event;
	split_register_t stop_on_event;
	split_register_t start_on_event;

	RESERVED_WORDS(10);

	/**
	 * Current count; and current state value.
	 */
	union {
		uint32_t count;
		struct {
			uint16_t count_low;
			uint16_t count_high;
		};
	};
	union {
		uint32_t state;
		struct {
			uint16_t state_low;
			uint16_t state_high;
		};
	};

	/**
	 * Registers into the raw and synchronized input behaviors.
	 */
	uint16_t raw_inputs;
	uint16_t synchronized_inputs;

	/**
	 * Register that controls whether we should perform a capture when a given
	 * event occurs.
	 */
	split_register_t use_register_for_capture;

	/**
	 * Raw control over each of the output pins.
	 */
	uint32_t raw_outputs;
	uint32_t output_control;

	uint32_t conflict_resolution;

	/**
	 * DMA request control: determine if events issue DMA events.
	 */
	uint32_t issue_dma_request_on_event[2];

	RESERVED_WORDS(35);

	/**
	 * Controls whether we should issue an interrupt when a given event is true.
	 */
	uint32_t interrupt_on_event;
	uint32_t event_occurred;

	uint32_t conflict_enable;
	uint32_t conflict_flag;

	union {
		split_register_t match[16];
		split_register_t capture[16];
	};

	RESERVED_WORDS(16);

	union {
		uint32_t match_alias_low[16];
		uint32_t capture_alias_low[16];
	};
	union {
		uint32_t match_alias_high[16];
		uint32_t capture_alias_high[16];
	};


	/**
	 * capture control register: for index N, controls whether a given event captures the counter
	 * value into capture register N
	 */
	union {
		split_register_t reset_on_event[16];
		split_register_t capture_on_events[16];
	};

	RESERVED_WORDS(16);

	uint32_t capture_control_alias_low[16];
	uint32_t capture_control_alias_high[16];

	event_registers_t event[16];

	RESERVED_WORDS(96);

	output_registers_t output_on_event[16];

} platform_sct_register_block_t;

ASSERT_OFFSET(platform_sct_register_block_t, start_on_event,            0x014);
ASSERT_OFFSET(platform_sct_register_block_t, count,                     0x040);
ASSERT_OFFSET(platform_sct_register_block_t, interrupt_on_event,        0x0f0);
ASSERT_OFFSET(platform_sct_register_block_t, match_alias_low,           0x180);
ASSERT_OFFSET(platform_sct_register_block_t, capture_control_alias_low, 0x280);
ASSERT_OFFSET(platform_sct_register_block_t, event,                     0x300);
ASSERT_OFFSET(platform_sct_register_block_t, output_on_event,           0x500);


#endif
