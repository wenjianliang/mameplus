/***************************************************************

Taito  'O System'
-------------------

Taito gambling hardware, very similar to H system.

Board specs (based on poor quality pic):

68000
YM2203
4 custom Taito chips ( TC0080VCO? TC0070RGB? )
Custom (non JAMMA) connector
Battery


Games :

Parent Jack  (C) 1989 Taito

TODO:

- inputs (coins)
- NVRAM
- sprite priorities
- dips
- interrupts (sources) - valid levels 4,5,6(hop empty?)

*****************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "cpu/m68000/m68000.h"
#include "video/taitoic.h"
#include "sound/2203intf.h"
#include "includes/taito_o.h"

static const int clear_hack = 1;

WRITE16_MEMBER(taitoo_state::io_w)
{
	switch(offset)
	{
		case 2: machine().watchdog_reset(); break;

		default: logerror("IO W %x %x %x\n", offset, data, mem_mask);
	}
}

READ16_MEMBER(taitoo_state::io_r)
{
	int retval = 0;

	switch(offset)
	{
		case 0: retval = ioport("IN0")->read() & (clear_hack ? 0xf7ff : 0xffff); break;
		case 1: retval = ioport("IN1")->read() & (clear_hack ? 0xfff7 : 0xffff); break;
		default: logerror("IO R %x %x = %x @ %x\n", offset, mem_mask, retval, cpu_get_pc(&space.device()));
	}
	return retval;
}

static ADDRESS_MAP_START( parentj_map, AS_PROGRAM, 16, taitoo_state )
	AM_RANGE(0x000000, 0x01ffff) AM_ROM
	AM_RANGE(0x100000, 0x10ffff) AM_MIRROR(0x010000) AM_RAM
	AM_RANGE(0x200000, 0x20000f) AM_READWRITE(io_r, io_w) /* TC0220IOC ? */
	AM_RANGE(0x300000, 0x300003) AM_DEVREADWRITE8_LEGACY("ymsnd", ym2203_r, ym2203_w, 0x00ff)
	AM_RANGE(0x400000, 0x420fff) AM_DEVREADWRITE_LEGACY("tc0080vco", tc0080vco_word_r, tc0080vco_word_w)
	AM_RANGE(0x500800, 0x500fff) AM_RAM_WRITE(paletteram_xBBBBBGGGGGRRRRR_word_w) AM_SHARE("paletteram")
ADDRESS_MAP_END

static INPUT_PORTS_START( parentj )
	PORT_START("IN0")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_CODE(KEYCODE_Z) PORT_NAME("Bet 1")
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_CODE(KEYCODE_X) PORT_NAME("Bet 2")
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_CODE(KEYCODE_C) PORT_NAME("Bet 3")
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_CODE(KEYCODE_V) PORT_NAME("Bet 4")

	PORT_DIPNAME(0x0010,  0x10, "IN0 4")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON8 ) PORT_CODE(KEYCODE_F) PORT_NAME("Payout")
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_CODE(KEYCODE_D) PORT_NAME("Check")
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_CODE(KEYCODE_A) PORT_NAME("Deal/Hit")
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_CODE(KEYCODE_S) PORT_NAME("Double")
	PORT_DIPNAME(0x000400,  0x400, "IN0 a")
	PORT_DIPSETTING(    0x400, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x000, DEF_STR( On ) )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_CODE(KEYCODE_J) PORT_NAME("Reset")

	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_CODE(KEYCODE_Q) PORT_NAME("Last Key")
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_CODE(KEYCODE_W) PORT_NAME("Meter Key")
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_CODE(KEYCODE_E) PORT_NAME("Opto 1H")
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_CODE(KEYCODE_R) PORT_NAME("Opto 1L")

	PORT_START("IN1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_CODE(KEYCODE_U) PORT_NAME("Hop Over")
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_CODE(KEYCODE_T) PORT_NAME("Opto 2H")
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_CODE(KEYCODE_Y) PORT_NAME("Opto 2L")
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_CODE(KEYCODE_I) PORT_NAME("All Clear")

	PORT_SERVICE_NO_TOGGLE(0x0010, IP_ACTIVE_LOW )
	PORT_DIPNAME(0x0020,  0x20, "IN1 5")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME(0x0040,  0x40, "IN1 6")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME(0x0080,  0x80, "IN1 7")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME(0x000100,  0x000, "Battery test?")
	PORT_DIPSETTING(    0x000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x100, DEF_STR( On ) )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_CODE(KEYCODE_O) PORT_NAME("Pay Out")
	PORT_DIPNAME(0x000400,  0x400, "IN1 a")
	PORT_DIPSETTING(    0x400, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x000, DEF_STR( On ) )
	PORT_DIPNAME(0x000800,  0x800, "IN1 b")
	PORT_DIPSETTING(    0x800, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x000, DEF_STR( On ) )
	PORT_DIPNAME(0x001000,  0x1000, "IN1 c")
	PORT_DIPSETTING(    0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME(0x002000,  0x2000, "IN1 d")
	PORT_DIPSETTING(    0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME(0x004000,  0x4000, "IN1 e")
	PORT_DIPSETTING(    0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME(0x008000,  0x8000, "IN1 f")
	PORT_DIPSETTING(    0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )

	PORT_START("DSWA")
	PORT_DIPNAME(0x0001,  0x00, "DSWA 0")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME(0x0002,  0x00, "DSWA 1")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME(0x0004,  0x00, "DSWA 2")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME(0x0008,  0x00, "DSWA 3")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME(0x0010,  0x00, "DSWA 4")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x010, DEF_STR( On ) )
	PORT_DIPNAME(0x0020,  0x00, "DSWA 5")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x020, DEF_STR( On ) )
	PORT_DIPNAME(0x0040,  0x00, "DSWA 6")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x040, DEF_STR( On ) )
	PORT_DIPNAME(0x0080,  0x00, "DSWA 7")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x080, DEF_STR( On ) )

	PORT_START("DSWB")
	PORT_DIPNAME(0x0001,  0x00, "Credits at start")
	PORT_DIPSETTING(    0x00, "500" )
	PORT_DIPSETTING(    0x01, "0" )
	PORT_DIPNAME(0x0002,  0x00, "DSWB 1")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME(0x0004,  0x00, "DSWB 2")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME(0x0008,  0x00, "DSWB 3")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME(0x0010,  0x00, "DSWB 4")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x010, DEF_STR( On ) )
	PORT_DIPNAME(0x0020,  0x00, "DSWB 5")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x020, DEF_STR( On ) )
	PORT_DIPNAME(0x0040,  0x00, "DSWB 6")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x040, DEF_STR( On ) )
	PORT_DIPNAME(0x0080,  0x00, "DSWB 7")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x080, DEF_STR( On ) )

INPUT_PORTS_END

static const gfx_layout parentj_layout =
{
	16,16,
	RGN_FRAC(1,8),
	4,
	{ 0,1,2,3 },
	{ RGN_FRAC(7,8)+4, RGN_FRAC(7,8)+0,
	  RGN_FRAC(6,8)+4, RGN_FRAC(6,8)+0,
	  RGN_FRAC(5,8)+4, RGN_FRAC(5,8)+0,
	  RGN_FRAC(4,8)+4, RGN_FRAC(4,8)+0,
	  RGN_FRAC(3,8)+4, RGN_FRAC(3,8)+0,
	  RGN_FRAC(2,8)+4, RGN_FRAC(2,8)+0,
	  RGN_FRAC(1,8)+4, RGN_FRAC(1,8)+0,
	  RGN_FRAC(0,8)+4, RGN_FRAC(0,8)+0
	  },
	{ 0,8,16,24,32,40,48,56,64,72,80,88,96,104,112,120},

	1*128
};

static GFXDECODE_START( parentj )
	GFXDECODE_ENTRY( "gfx1", 0, parentj_layout,  0x0, 0x400/16  )
GFXDECODE_END

/* unknown sources ... */
static TIMER_DEVICE_CALLBACK( parentj_interrupt )
{
	taitoo_state *state = timer.machine().driver_data<taitoo_state>();
	int scanline = param;

	if(scanline == 448)
		device_set_input_line(state->m_maincpu, 4, HOLD_LINE);

	if(scanline == 0)
		device_set_input_line(state->m_maincpu, 5, HOLD_LINE);
}

static const ym2203_interface ym2203_config =
{
	{
		AY8910_LEGACY_OUTPUT,
		AY8910_DEFAULT_LOADS,
		DEVCB_INPUT_PORT("DSWA"),DEVCB_INPUT_PORT("DSWB"),
		DEVCB_NULL, DEVCB_NULL,
	},
	DEVCB_NULL
};

static const tc0080vco_interface parentj_intf =
{
	0, 1,	/* gfxnum, txnum */
	1, 1, -2,
	0
};

static MACHINE_START( taitoo )
{
	taitoo_state *state = machine.driver_data<taitoo_state>();

	state->m_maincpu = machine.device("maincpu");
	state->m_tc0080vco = machine.device("tc0080vco");
}

static MACHINE_CONFIG_START( parentj, taitoo_state )

	MCFG_CPU_ADD("maincpu", M68000,12000000 )		/*?? MHz */
	MCFG_CPU_PROGRAM_MAP(parentj_map)
	MCFG_TIMER_ADD_SCANLINE("scantimer", parentj_interrupt, "screen", 0, 1)

	MCFG_MACHINE_START(taitoo)

	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MCFG_SCREEN_SIZE(64*16, 64*16)
	MCFG_SCREEN_VISIBLE_AREA(0*16, 32*16-1, 3*16, 31*16-1)
	MCFG_SCREEN_UPDATE_STATIC(parentj)

	MCFG_GFXDECODE(parentj)
	MCFG_PALETTE_LENGTH(33*16)

	MCFG_TC0080VCO_ADD("tc0080vco", parentj_intf)

	MCFG_SPEAKER_STANDARD_MONO("mono")

	MCFG_SOUND_ADD("ymsnd", YM2203, 2000000) /*?? MHz */
	MCFG_SOUND_CONFIG(ym2203_config)
	MCFG_SOUND_ROUTE(0, "mono",  0.25)
	MCFG_SOUND_ROUTE(0, "mono", 0.25)
	MCFG_SOUND_ROUTE(1, "mono",  1.0)
	MCFG_SOUND_ROUTE(2, "mono", 1.0)
MACHINE_CONFIG_END

ROM_START( parentj )
	ROM_REGION( 0x20000, "maincpu", 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "c42-13.21", 0x00000, 0x10000, CRC(823623eb) SHA1(7302cc0ac532f6190ae35218ea05bf8cf11fd687) )
	ROM_LOAD16_BYTE( "c42-12.20", 0x00001, 0x10000, CRC(8654b0ab) SHA1(edd23a731c1c60cab353e51ef5e66d33bc3fde61) )

	ROM_REGION( 0x100000, "gfx1", 0 )
	ROM_LOAD( "c42-05.06", 0x00000, 0x20000, CRC(7af0d45d) SHA1(bc527b74185596e4e77b34d08eb3e1678614b451) )
	ROM_LOAD( "c42-04.05", 0x20000, 0x20000, CRC(133009a1) SHA1(fae5dd600384790225c24a62d1f8a00f0366dae9) )
	ROM_LOAD( "c42-09.13", 0x40000, 0x20000, CRC(ba35fb03) SHA1(b76e50d298ccc0f230c865b563cd8e02866a4ffb) )
	ROM_LOAD( "c42-08.12", 0x60000, 0x20000, CRC(7fae35a7) SHA1(f4bc6c6fd4afc167eb36b8f16589e1bfd729085e) )
	ROM_LOAD( "c42-07.10", 0x80000, 0x20000, CRC(f92c6f03) SHA1(ff42318ee425b423b67e2cec1fe3ef9d9785ebf6) )
	ROM_LOAD( "c42-06.09", 0xa0000, 0x20000, CRC(3685febd) SHA1(637946377f6d934f791d52e9790c91f60a5b2c65) )
	ROM_LOAD( "c42-11.17", 0xc0000, 0x20000, CRC(5d8d3c59) SHA1(c8a8a957ac9f2f1c346b4504495893c71fbfe14b) )
	ROM_LOAD( "c42-10.16", 0xe0000, 0x20000, CRC(e85e536e) SHA1(9ed9e316869333338e39cb0d1293e3380861a3ca) )

	ROM_REGION( 0x2dd, "misc", 0 )
	ROM_LOAD( "ampal22v10a-0233.c42", 0x000, 0x2dd, CRC(0c030a81) SHA1(0f8198df2cb046683d2db9ac8e609cdff53083ed) )
ROM_END

GAME( 1989, parentj,  0,        parentj,  parentj, driver_device,  0,        ROT0,    "Taito", "Parent Jack", GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
