#include "alt_types.h"
#include <stdio.h>
#include <unistd.h>
#include "system.h"
#include "sys/alt_irq.h"
#include <io.h>

#include "altera_avalon_timer_regs.h"
#include "altera_avalon_pio_regs.h"


// Contents: Helper functions to complete lab 1
void init(int, int);
void background(int);
void finalize(void);

volatile int counter_periodic;
volatile int counter_pulse;

#ifdef TIMER_0_BASE  // only compile this code if there is a sys_clk_timer
static void TIMER_ISR_PERIODIC(void* context, alt_u32 id)
{
//    while(IORD(PIO_PULSE_BASE, 0) == 0) {}
//    IOWR(PIO_RESPONSE_BASE, 0, 1);
//    while(IORD(PIO_PULSE_BASE, 0) == 1) {}
//    IOWR(PIO_RESPONSE_BASE, 0, 0);
    if(IORD(PIO_PULSE_BASE, 0) == 0 && IORD(PIO_RESPONSE_BASE, 0) == 1) {
    	IOWR(PIO_RESPONSE_BASE, 0, 0);
    	counter_periodic++;

    }
    if(IORD(PIO_PULSE_BASE, 0) == 1 && IORD(PIO_RESPONSE_BASE, 0) == 0) {
    	IOWR(PIO_RESPONSE_BASE, 0, 1);
    }

    // Clear the TO bit to acknowledge the interrupt
    IOWR_ALTERA_AVALON_TIMER_STATUS(TIMER_0_BASE, 0x0);
}
#endif


//Initialize Timer
void init_timer_periodic(){
    alt_u32 timerPeriod;  // 32 bit period used for timer


#ifdef TIMER_0_BASE
    // calculate timer period
    timerPeriod = 7000;

    // initialize timer period
    IOWR_ALTERA_AVALON_TIMER_PERIODL(TIMER_0_BASE, (alt_u16) timerPeriod);
    IOWR_ALTERA_AVALON_TIMER_PERIODH(TIMER_0_BASE, (alt_u16)(timerPeriod >> 16));

    /*
    printf("Low  %d\n", (alt_u16) timerPeriod);
    printf("High %d\n", (alt_u16)(timerPeriod >> 16));

    printf("Low Direct  %d\n", (alt_u16) IORD(0x20, 2));
    printf("High Direct %d\n", (alt_u16) IORD(0x20, 3));
   */

    /*IOWR_ALTERA_AVALON_TIMER_PERIODL(TIMER_0_BASE, (alt_u16) 7000);
    IOWR_ALTERA_AVALON_TIMER_PERIODH(TIMER_0_BASE, (alt_u16) 0);

    printf("Low Direct  %d\n", (alt_u16) IORD(0x20, 2));
    printf("High Direct %d\n", (alt_u16) IORD(0x20, 3));*/

    // clear timer interrupt bit in status register
    IOWR_ALTERA_AVALON_TIMER_STATUS(TIMER_0_BASE, 0x0);

    // initialize timer control - start timer, run continuously, enable interrupts
    IOWR_ALTERA_AVALON_TIMER_CONTROL(TIMER_0_BASE,
                     ALTERA_AVALON_TIMER_CONTROL_ITO_MSK
                     | ALTERA_AVALON_TIMER_CONTROL_CONT_MSK
                     | ALTERA_AVALON_TIMER_CONTROL_START_MSK);

    // initialize timer interrupt vector
    alt_irq_register(TIMER_0_IRQ, (void*)0, TIMER_ISR_PERIODIC);

#endif

}

// Interrupt
#ifdef PIO_PULSE_BASE
static void PULSE_ISR(void* context, alt_u32 id)
{
	   if(IORD(PIO_PULSE_BASE, 0) == 0 && IORD(PIO_RESPONSE_BASE, 0) == 1){
	   		IOWR(PIO_RESPONSE_BASE, 0, 0);
	   		counter_pulse++;
	   }
	   if(IORD(PIO_PULSE_BASE, 0) == 1 && IORD(PIO_RESPONSE_BASE, 0) == 0){
	      	IOWR(PIO_RESPONSE_BASE, 0, 1);
	   }
	   // acknowledge the interrupt by clearing the TO bit in the status register
	   IOWR(PIO_PULSE_BASE, 3, 0x0);
}
#endif

void init_pulse_ISR(){
#ifdef PIO_PULSE_BASE

	// clear edge capture register
	IOWR(PIO_PULSE_BASE, 3, 0x0);
	// enable interrupts for all four buttons
	IOWR(PIO_PULSE_BASE, 2, 0x1);
    // initialize pulse interrupt vector
    alt_irq_register(PIO_PULSE_IRQ, (void*)0, PULSE_ISR);

#endif
}

int main(){

	// Define Testing Parameters
	int d_cycle;
	int period;
	int t_gran;
    
	// Loop Through While Changing Duty Cycle, Period and Task Granularity
	for(period = 2; period < 15; period +=2){
		for(d_cycle = 2; d_cycle < 15; d_cycle += 2){
			for(t_gran = 10; t_gran < 501; t_gran += 50){
                
                #ifdef PERIODIC_TESTING
				// Begin Periodic Polling Code
				init_timer_periodic();
				init(period, d_cycle);
				counter_periodic = 0;
				while(counter_periodic < 100) {
					background(t_gran);
				}
				finalize();
				// End Periodic Polling Code
                #endif
                
                
                #ifdef INTERRUPT_TESTING
				//Begin Interrupt Pulse

				init_pulse_ISR();
				init(period, d_cycle);
				counter_pulse = 0;
				while(counter_pulse < 100){
					background(t_gran);
				}
				finalize();
				//End interrupt
                #endif
			}
		}
	}
	return 0;

}


