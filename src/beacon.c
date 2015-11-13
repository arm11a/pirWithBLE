/******************************************************************************
 *  Copyright (C) Cambridge Silicon Radio Limited, 2014
 *
 *  FILE
 *      beacon.c
 *
 *  DESCRIPTION
 *      This file defines an advertising node implementation
 *
 *****************************************************************************/

/*============================================================================*
 *  SDK Header Files
 *============================================================================*/

#include <main.h>
#include <gap_app_if.h>
#include <config_store.h>
#include <pio.h>
#include <random.h>
#include <timer.h>

/*============================================================================*
 *  Private Definitions
 *============================================================================*/

#define PIO_LED0        10          /* PIO connected to the LED0 on CSR10xx */
#define PIO_BUTTON      11          /* PIO connected to the button on CSR10xx */

#define PIO_DIR_OUTPUT  TRUE        /* PIO direction configured as output */
#define PIO_DIR_INPUT   FALSE       /* PIO direction configured as input */
#define MAX_APP_TIMERS       2

/*============================================================================*
 *  Local Header File
 *============================================================================*/

#include "beacon.h"

/*============================================================================*
 *  Private Function Prototypes
 *============================================================================*/

static void startAdvertising(void);
static void stopAdvertising(void);

static void appSetRandomAddress(void);
/*============================================================================*
 *  Private Data Types
 *============================================================================*/


/*============================================================================*
 *  Private Data
 *============================================================================*/

static uint16 app_timers[ SIZEOF_APP_TIMER * MAX_APP_TIMERS ];

/*============================================================================*
 *  Private Function Prototypes
 *============================================================================*/

/* Advance LED flashing sequence to next state */
static void goToNextLedSeq(uint32 onOff);

/* Restart LED sequence from the beginning */
static void restartLedSeq(void);

/*============================================================================*
 *  Private Function Implementations
 *============================================================================*/
/*----------------------------------------------------------------------------*
 *  NAME
 *      goToNextLedSeq
 *
 *  DESCRIPTION
 *      Advance LED flashing sequence to next state.
 *
 * PARAMETERS
 *      None
 *
 * RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
static void goToNextLedSeq(uint32 onOff)
{
	if(onOff == 0)
	{
		stopAdvertising();
		/* Set LED0 according to bit 0 of desired pattern */
		PioSet(PIO_LED0, 0);
	}
	else
	{
		startAdvertising();
		PioSet(PIO_LED0, 1);
	}
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      restartLedSeq
 *
 *  DESCRIPTION
 *      Restart LED sequence from the beginning.
 *
 * PARAMETERS
 *      None
 *
 * RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
static void restartLedSeq(void)
{
    /* Turn off both LEDs by setting output to Low */
    PioSets((1UL << PIO_LED0), 0UL);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      appSetRandomAddress
 *
 *  DESCRIPTION
 *      This function generates a non-resolvable private address and sets it
 *      to the firmware.
 *
 *  RETURNS
 *      Nothing.
 *
 *----------------------------------------------------------------------------*/
static void appSetRandomAddress(void)
{
    BD_ADDR_T addr;

    /* "completely" random MAC addresses by default: */
    for(;;)
    {
        uint32 now = TimeGet32();
        /* Random32() is just two of them, no use */
        uint32 rnd = Random16();
        addr.uap = 0xff & (rnd ^ now);
        /* No sub-part may be zero or all-1s */
        if ( 0 == addr.uap || 0xff == addr.uap ) continue;
        addr.lap = 0xffffff & ((now >> 8) ^ (73 * rnd));
        if ( 0 == addr.lap || 0xffffff == addr.lap ) continue;
        addr.nap = 0x3fff & rnd;
        if ( 0 == addr.nap || 0x3fff == addr.nap ) continue;
        break;
    }

    /* Set it to actually be an acceptable random address */
    addr.nap &= ~BD_ADDR_NAP_RANDOM_TYPE_MASK;
    addr.nap |=  BD_ADDR_NAP_RANDOM_TYPE_NONRESOLV;
    GapSetRandomAddress(&addr);
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      startAdvertising
 *
 *  DESCRIPTION
 *      This function is called to start advertisements.
 *
 *      Advertisement packet will contain Flags AD and Manufacturer-specific
 *      AD with Manufacturer id set to CSR and payload set to the value of
 *      the User Key 0. The payload size is set by the User Key 1.
 *
 *      +--------+-------------------------------------------------+
 *      |FLAGS AD|MANUFACTURER AD                                  |
 *      +--------+-------------------------------------------------+
 *       0      2 3
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
void startAdvertising(void)
{
    uint8 advData[MAX_ADVERT_PACKET_SIZE];
    uint16 offset = 0;
    uint8 filler;
    uint16 advInterval;
    uint8 advPayloadSize;
    ls_addr_type addressType = ls_addr_type_public;     /* use public address */
    
    /* initialise values from User CsKeys */
    
    /* read User key 0 for the payload filler */
    filler = (uint8)(CSReadUserKey(0) & 0x00FF);
    
    /* read User key 1 for the payload size */
    advPayloadSize = (uint8)(CSReadUserKey(1) & 0x00FF);
    
    /* range check */
    if((advPayloadSize < 1) || (advPayloadSize > MAX_ADVERT_PAYLOAD_SIZE))
    {
        /* revert to default payload size */
        advPayloadSize = DEFAULT_ADVERT_PAYLOAD_SIZE;
    }
    
    /* read User key 2 for the advertising interval */
    advInterval = CSReadUserKey(2);


    /* range check */
    if((advInterval < MIN_ADVERTISING_INTERVAL) ||
       (advInterval > MAX_ADVERTISING_INTERVAL))
    {
        /* revert to default advertising interval */
        advInterval = DEFAULT_ADVERTISING_INTERVAL;
    }

    /* read address type from User key 3 */
    if(CSReadUserKey(3))
    {
        /* use random address type */
        addressType = ls_addr_type_random;

        /* generate and set the random address */
        appSetRandomAddress();
    }

    /* set the GAP Broadcaster role */
    GapSetMode(gap_role_broadcaster,
               gap_mode_discover_no,
               gap_mode_connect_no,
               gap_mode_bond_no,
               gap_mode_security_none);
    
    /* clear the existing advertisement data, if any */
    LsStoreAdvScanData(0, NULL, ad_src_advertise);

    /* set the advertisement interval, API accepts the value in microseconds */
    GapSetAdvInterval(advInterval * MILLISECOND, advInterval * MILLISECOND);
    
    /* manufacturer-specific data */
    advData[0] = AD_TYPE_MANUF;

    /* CSR company code, little endian */
    advData[1] = 0x0A;
    advData[2] = 0x00;
    
    /* fill in the rest of the advertisement */
    for(offset = 0; offset < advPayloadSize; offset++)
    {
        advData[3 + offset] = filler;
    }

    /* store the advertisement data */
    LsStoreAdvScanData(advPayloadSize + 3, advData, ad_src_advertise);
    
    /* Start broadcasting */
    LsStartStopAdvertise(TRUE, whitelist_disabled, addressType);
}
void stopAdvertising(void)
{
    LsStartStopAdvertise(FALSE, whitelist_disabled, ls_addr_type_random);
}

/*============================================================================*
 *  Public Function Implementations
 *============================================================================*/

/*----------------------------------------------------------------------------*
 *  NAME
 *      AppPowerOnReset
 *
 *  DESCRIPTION
 *      This function is called just after a power-on reset (including after
 *      a firmware panic).
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/

void AppPowerOnReset(void)
{
    /* empty */
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      AppInit
 *
 *  DESCRIPTION
 *      This function is called after a power-on reset (including after a
 *      firmware panic) or after an HCI Reset has been requested.
 *
 *      NOTE: In the case of a power-on reset, this function is called
 *      after AppPowerOnReset().
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/

void AppInit(sleep_state last_sleep_state)
{
    /* set all PIOs to inputs and pull them down */
    PioSetModes(0xFFFFFFFFUL, pio_mode_user);
    PioSetDirs(0xFFFFFFFFUL, FALSE);
    PioSetPullModes(0xFFFFFFFFUL, pio_mode_strong_pull_down);
    
    /* Set LED0 and LED1 to be controlled directly via PioSet */
    PioSetModes((1UL << PIO_LED0), pio_mode_user);

    /* Configure LED0 and LED1 to be outputs */
    PioSetDir(PIO_LED0, PIO_DIR_OUTPUT);

    /* Set the LED0 and LED1 to have strong internal pull ups */
    PioSetPullModes((1UL << PIO_LED0),
                    pio_mode_strong_pull_up);

    /* Configure button to be controlled directly */
    PioSetMode(PIO_BUTTON, pio_mode_user);

    /* Configure button to be input */
    PioSetDir(PIO_BUTTON, PIO_DIR_INPUT);

    /* Set weak pull up on button PIO, in order not to draw too much current
     * while button is pressed
     */
    PioSetPullModes((1UL << PIO_BUTTON), pio_mode_weak_pull_down);

    /* Set the button to generate sys_event_pio_changed when pressed as well
     * as released
     */
    PioSetEventMask((1UL << PIO_BUTTON), pio_event_mode_both);


    /* disable wake up on UART RX */
    SleepWakeOnUartRX(FALSE);
    
    /* pull down the I2C lines */
    PioSetI2CPullMode(pio_i2c_pull_mode_strong_pull_down);

    /* Reset LED sequence */
    restartLedSeq();

	SleepModeChange(sleep_mode_deep);


    TimerInit(MAX_APP_TIMERS, (void*)app_timers);


}


/*----------------------------------------------------------------------------*
 *  NAME
 *      AppProcessSystemEvent
 *
 *  DESCRIPTION
 *      This user application function is called whenever a system event, such
 *      as a battery low notification, is received by the system.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/

void AppProcessSystemEvent(sys_event_id id, void *data)
{
    /* If the reported system event is generated by a PIO */
    if (id == sys_event_pio_changed)
    {
#if 1
        /* The PIO data is defined by struct pio_changed_data */
        const pio_changed_data *pPioData = (const pio_changed_data *)data;

        /* If the PIO event comes from the button */
        if (pPioData->pio_cause & (1UL << PIO_BUTTON))
        {
			goToNextLedSeq(pPioData->pio_state & (1UL << PIO_BUTTON));
        }
#else // just changed PIO
		goToNextLedSeq();
#endif
    }
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      AppProcessLmEvent
 *
 *  DESCRIPTION
 *      This user application function is called whenever a LM-specific event is
 *      received by the system.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/

bool AppProcessLmEvent(lm_event_code event_code, 
                       LM_EVENT_T *p_event_data)
{
    return TRUE;
}
