/*
 *  ==== DO NOT MODIFY THIS FILE - CHANGES WILL BE OVERWRITTEN ====
 *
 *  Generated from
 *      C:/ti/ccs5/grace_1_10_00_17/packages/ti/mcu/msp430/csl/communication/USI_init.xdt
 */

#include <msp430.h>

/*
 *  ======== USI_init ========
 *  Initialize Universal Serial Interface
 */
void USI_init(void)
{
    /* Disable USI */
    USICTL0 |= USISWRST;
    
    /* 
     * USI Control Register 0
     * 
     * ~USIPE7 -- USI function disabled
     * USIPE6 -- USI function enabled
     * USIPE5 -- USI function enabled
     * ~USILSB -- MSB first
     * USIMST -- Master mode
     * ~USIGE -- Output latch enable depends on shift clock
     * USIOE -- Output enabled
     * USISWRST -- USI logic held in reset state
     * 
     * Note: ~<BIT> indicates that <BIT> has value zero
     */
    USICTL0 = USIPE6 + USIPE5 + USIMST + USIOE + USISWRST;
    
    /* 
     * USI Control Register 1
     * 
     * USICKPH -- Data is captured on the first SCLK edge and changed on the following edge
     * ~USII2C -- I2C mode disabled
     * ~USISTTIE -- Interrupt on START condition disabled
     * ~USIIE -- Interrupt disabled
     * ~USIAL -- No arbitration lost condition
     * ~USISTP -- No STOP condition received
     * ~USISTTIFG -- No START condition received. No interrupt pending
     * USIIFG -- Interrupt pending
     * 
     * Note: ~<BIT> indicates that <BIT> has value zero
     */
    USICTL1 = USICKPH + USIIFG;
    
    /* 
     * USI Clock Control Register
     * 
     * USIDIV_0 -- Divide by 1
     * USISSEL_2 -- SMCLK
     * ~USICKPL -- Inactive state is low
     * ~USISWCLK -- Input clock is low
     * 
     * Note: ~<BIT> indicates that <BIT> has value zero
     */
    USICKCTL = USIDIV_0 + USISSEL_2;
    
    /* Enable USI */
    USICTL0 &= ~USISWRST;
}
