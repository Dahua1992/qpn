/*****************************************************************************
* BSP for Blinky example, Microstick II board, cooperative QV-nano kernel
* Last updated for version 6.5.1
* Last updated on  2019-06-10
*
*                    Q u a n t u m  L e a P s
*                    ------------------------
*                    Modern Embedded Software
*
* Copyright (C) 2005-2019 Quantum Leaps, LLC. All rights reserved.
*
* This program is open source software: you can redistribute it and/or
* modify it under the terms of the GNU General Public License as published
* by the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Alternatively, this program may be distributed and modified under the
* terms of Quantum Leaps commercial licenses, which expressly supersede
* the GNU General Public License and are specifically designed for
* licensees interested in retaining the proprietary status of their code.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
* Contact information:
* https://www.state-machine.com
* mailto:info@state-machine.com
*****************************************************************************/
#include "qpn.h"
#include "bsp.h"
#include "blinky.h"

#include "p24FJ64GB002.h"  /* header for PIC24F device in use */

Q_DEFINE_THIS_FILE

/* configure Fast RC Oscillator (FRC) */
#pragma config FNOSC=FRC

/* Local-scope objects -----------------------------------------------------*/

/* frequency of the FRC oscillator ~ 8 MHz */
#define FOSC_HZ                 8000000.0
/* instruction cycle clock frequency */
#define FCY_HZ                  (FOSC_HZ / 2.0)
/* system clock tick period in CPU clocks / TMR2 prescaler */
#define BSP_TMR2_PERIOD         ((uint16_t)(FCY_HZ / BSP_TICKS_PER_SEC))

/* controlling the LED of Microstick II */
#define LED_ON()                (LATA |= (1U << 0))
#define LED_OFF()               (LATA &= ~(1U << 0))
#define LED_TOGGLE()            (LATA ^= (1U << 0))


/* ISRs --------------------------------------------------------------------*/
void  __attribute__((__interrupt__, auto_psv))
_T2Interrupt(void) {
    _T2IF = 0; /* clear Timer 2 interrupt flag */

    QF_tickXISR(0U);   /* process time events for rate 0 */

}
/*..........................................................................*/
void  __attribute__((__interrupt__, auto_psv))
_INT0Interrupt() {
    _INT0IF = 0;

    QACTIVE_POST_ISR((QActive *)&AO_Blinky, TIMEOUT_SIG, 0U);
}

/*--------------------------------------------------------------------------*/
void BSP_init(void) {
    RCONbits.SWDTEN = 0; /* disable Watchdog */

    TRISA = 0x00; /* set LED pins as outputs */
    PORTA = 0x00; /* set LED drive state low */

}
/*..........................................................................*/
void BSP_terminate(int16_t result) {
    (void)result;
}
/*..........................................................................*/
void BSP_ledOff(void) {
    LED_OFF();
}
/*..........................................................................*/
void BSP_ledOn(void) {
    LED_ON();
}

/*..........................................................................*/
void Q_onAssert(char const Q_ROM * const Q_ROM_VAR file, int line) {
    (void)file;       /* unused parameter */
    (void)line;       /* unused parameter */
    QF_INT_DISABLE(); /* make sure that interrupts are disabled */
    for (;;) {
    }
}

/*..........................................................................*/
void QF_onStartup(void) { /* entered with interrupts locked */
    T2CON = 0x0000U; /* Use Internal Osc (Fcy), 16 bit mode, prescaler = 1 */
    TMR2  = 0x0000U; /* Start counting from 0 and clear the prescaler count */
    PR2   = (uint16_t)(BSP_TMR2_PERIOD - 1U); /* Timer2 period */
    _T2IF = 0;               /* clear the interrupt for Timer 2 */
    _T2IE = 1;               /* enable interrupt for Timer 2 */
    T2CONbits.TON = 1;       /* start Timer 2 */

    INTCON2bits.INT0EP = 0;  /* INT0 interrupt on positive edge */
    _INT0IF = 0;             /* clear the interrupt for INT0 */
    _INT0IE = 1;             /* enable INT0 interrupt */

    /* explicitly assign priorities to all interrutps... */
    _T2IP   = 4; /* Timer 2 interrupt priority (kernel aware) */
    _INT0IP = 6; /* INT0 interrupt priority (kernel aware) */
}
/*..........................................................................*/
void QF_onCleanup(void) {
}
/*..........................................................................*/
void QF_stop(void) {
}
/*..........................................................................*/
void QV_onIdle(void) { /* entered with interrupts DISABLED, see NOTE01 */

    /* NOTE: not enough LEDs on the Microstick II board to implement
    *  the idle loop activity indicator ...
    */
    //LED_ON (); /* blink the IDLE LED, see NOTE01 */
    //LED_OFF();

#ifdef NDEBUG
    __asm__ volatile("disi #0x0001");
    Idle(); /* transition to Idle mode, see NOTE02 */
#else
    QF_INT_ENABLE(); /* enable interrupts, see NOTE01 */
#endif
}

/*****************************************************************************
* NOTE01:
* The callback function QV_onIdle() is called with interrupts disabled,
* because the idle condition can be invalidated by any enabled interrupt
* that would post events. The QV_onIdle() function *must* re-enable
* interrupts internally
*
* NOTE02:
* To be on the safe side, the DISICNT counter is set to just 1 cycle just
* before entering the Idle mode (or Sleep mode, if you choose). This way,
* interrupts (with priorities 1-6) get enabled at the same time as the
* transition to the low-power mode.
*/

