#ifndef intf5110_h
#define intf5110_h

/* clock rate = 80 * output sample rate,     */
/* usually 640000 for 8000 Hz sample rate or */
/* usually 800000 for 10000 Hz sample rate.  */

struct TMS5110interface
{
	int variant;				/* Variant of the 5110 - see tms5110.h */
	int rom_region;				/* set to -1 to specifiy callbacks below */
	int (*M0_callback)(void);	/* function to be called when chip requests another bit */
	void (*load_address)(int addr);	/* speech ROM load address callback */
};

WRITE8_HANDLER( tms5110_CTL_w );
WRITE8_HANDLER( tms5110_PDC_w );

READ8_HANDLER( tms5110_status_r );
int tms5110_ready_r(void);

void tms5110_set_frequency(int frequency);

#endif

