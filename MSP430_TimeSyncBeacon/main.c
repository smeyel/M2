/*
 * ======== Standard MSP430 includes ========
 */
#include <msp430.h>

/*
 * ======== Grace related includes ========
 */
#include <ti/mcu/msp430/Grace.h>

/*
 *  ======== main ========
 */
int main(void)
{
    Grace_init();                   // Activate Grace-generated configuration
    
    /* Start Timer_A */
    TA0CTL |= MC_1; /* Start timer in up mode */

/*    P1OUT &= ~ BIT0;*/

    return (0);
}
