#include "alt_types.h"
#include <stdio.h>
#include <unistd.h>
#include "system.h"
#include "sys/alt_irq.h"
#include <io.h>

#include "altera_avalon_timer_regs.h"
#include "altera_avalon_pio_regs.h"

volatile int edge_capture;
volatile int led_count;
volatile int ss_count;
volatile int led_flag;
volatile int ss_flag;
volatile alt_u8 switch_state_led;
volatile alt_u8 switch_state_ss;


void led1_blink(){
	alt_u8 current_status;

    // Get the right most state of unread switches (LSB of switch_states)
    current_status = switch_state_led & 0x01;

    // Write the state of the switch to the LED
    IOWR_ALTERA_AVALON_PIO_DATA(LED_PIO_BASE, current_status);

    // Update the LSB of the global switch_state
    switch_state_led = switch_state_led >> 1;

    // Increment the LED counter
    led_count ++;

    // If LED count has reached it's limit, reset the count and LED mode flags
    if(led_count > 7){

        //Reset counters and flags
        led_count = 0;
        led_flag = 0;

        //Turn LED1 off
        IOWR_ALTERA_AVALON_PIO_DATA(LED_PIO_BASE, 0x0);
    }

}

void ss_blink(){
	alt_u8 current_status;

	// Get the right most state of unread switches (LSB of switch_states)
	current_status = switch_state_ss & 0x01;

	// Write the state of the switch to the SS
	if (current_status == 1)
		IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_PIO_BASE, 0xCF);
	else
		IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_PIO_BASE, 0xFFFF);

	// Update the LSB of the global switch_state_ss
	switch_state_ss = switch_state_ss >> 1;

    // Increment the SS counter
    ss_count ++;

    // If SS count has reached it's limit, reset the count and SS mode flags
    if(ss_count > 7){

        //Reset counters and flags
        ss_count = 0;
        ss_flag = 0;

        // Turn Seven Segment Display (SS) off
        IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_PIO_BASE, 0xFFFF);
    }
}


#ifdef BUTTON_PIO_BASE
static void BUTTON_ISR(void* context, alt_u32 id)
{
    /* Store the value in the Button's edge capture register in *context. */
    edge_capture = IORD_ALTERA_AVALON_PIO_EDGE_CAP(BUTTON_PIO_BASE);

    //If button 1 was pressed
    if(edge_capture == 0x01)
    {
        //Reset the LED timer counter
        led_count = 0;

        //Read State of Switches upon push button click
        switch_state_led = IORD_ALTERA_AVALON_PIO_DATA(SWITCH_PIO_BASE);

        // Flag the timer that PB1 has been clicked to execute LED blink
        led_flag = 1;

    }
    else if(edge_capture == 0x02)
    {
        //Reset the LED timer counter
        ss_count = 0;

        //Read State of Switches upon push button click
        switch_state_ss = IORD_ALTERA_AVALON_PIO_DATA(SWITCH_PIO_BASE);

        // Flag the timer that PB2 has been clicked to execute SS blink
        ss_flag = 1;
    }

    /* Reset the Button's edge capture register. */
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(BUTTON_PIO_BASE, 0);

}
#endif

#ifdef TIMER_0_BASE  // only compile this code if there is a sys_clk_timer
static void TIMER_ISR(void* context, alt_u32 id)
{
    // Clear the TO bit to acknowledge the interrupt
    IOWR_ALTERA_AVALON_TIMER_STATUS(TIMER_0_BASE, 0x0);

    // Check which mode to run in (LED or SS mode)

    // NOTE CHECK FOR <7 in both LED and SS may not be needed as they are checked in the function

    /* If mode is in LED mode and the led counter limit has not been reached,
       blink LED once for this cycle */

    if(led_flag == 1 && led_count < 7)
        led1_blink();

    /* If mode is in SS mode and the SS counter limit has not been reached,
     blink SS once for this cycle */

    if(ss_flag == 1 && ss_count < 7)
        ss_blink();
}
#endif

/* Initialize the button_pio. */

static void init_button_pio()
{
    /* initialize the push button interrupt vector */
    alt_irq_register( BUTTON_PIO_IRQ, (void*) 0, BUTTON_ISR);

    /* Reset the edge capture register. */
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(BUTTON_PIO_BASE, 0x0);

    /* Enable all 4 button interrupts. */
    IOWR_ALTERA_AVALON_PIO_IRQ_MASK(BUTTON_PIO_BASE, 0xf);

}
/* Initialize the timer. */

void init_timer(){
    alt_u32 timerPeriod;  // 32 bit period used for timer


#ifdef TIMER_0_BASE
    // calculate timer period for 1 seconds
    timerPeriod = 1 * TIMER_0_FREQ;

    // initialize timer interrupt vector
    alt_irq_register(TIMER_0_IRQ, (void*)0, TIMER_ISR);

    // initialize timer period
    IOWR_ALTERA_AVALON_TIMER_PERIODL(TIMER_0_BASE, (alt_u16)timerPeriod);
    IOWR_ALTERA_AVALON_TIMER_PERIODH(TIMER_0_BASE, (alt_u16)(timerPeriod >> 16));

    // clear timer interrupt bit in status register
    IOWR_ALTERA_AVALON_TIMER_STATUS(TIMER_0_BASE, 0x0);

    // initialize timer control - start timer, run continuously, enable interrupts
    IOWR_ALTERA_AVALON_TIMER_CONTROL(TIMER_0_BASE,
                                     ALTERA_AVALON_TIMER_CONTROL_ITO_MSK | ALTERA_AVALON_TIMER_CONTROL_CONT_MSK
                                     | ALTERA_AVALON_TIMER_CONTROL_START_MSK);
#endif

}

/*
// our test
int main(void)
{
	//Initialize Variables
	edge_capture = 0;

	//Setup Push Buttons
	init_button_pio();

	//Setup Timer
	init_timer();

	// Turn LEDs Off
	IOWR_ALTERA_AVALON_PIO_DATA(LED_PIO_BASE, 0x0);

    // Turn Seven Segment Display (SS) off
    IOWR_ALTERA_AVALON_PIO_DATA(SEVEN_SEG_PIO_BASE, 0xFFFF);

	while(1){};

	return 0;
}
*/
