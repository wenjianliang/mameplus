/***************************************************************************

Shisen

driver by Nicola Salmoria

***************************************************************************/
#include "driver.h"
#include "sndhrdw/m72.h"
#include "sound/dac.h"
#include "sound/2151intf.h"

extern WRITE8_HANDLER( sichuan2_videoram_w );
extern WRITE8_HANDLER( sichuan2_bankswitch_w );
extern WRITE8_HANDLER( sichuan2_paletteram_w );

extern VIDEO_START( sichuan2 );
extern VIDEO_UPDATE( sichuan2 );

static READ8_HANDLER( sichuan2_dsw1_r )
{
	int ret = input_port_3_r(0);

	/* Based on the coin mode fill in the upper bits */
	if (input_port_4_r(0) & 0x04)
	{
		/* Mode 1 */
		ret	|= (input_port_5_r(0) << 4);
	}
	else
	{
		/* Mode 2 */
		ret	|= (input_port_5_r(0) & 0xf0);
	}

	return ret;
}

static WRITE8_HANDLER( sichuan2_coin_w )
{
	if ((data & 0xf9) != 0x01) logerror("coin ctrl = %02x\n",data);

	coin_counter_w(0, data & 0x02);
	coin_counter_w(1, data & 0x04);
}



static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_BANK1)
	AM_RANGE(0xc800, 0xcaff) AM_READ(MRA8_RAM)
	AM_RANGE(0xd000, 0xffff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xc800, 0xcaff) AM_WRITE(sichuan2_paletteram_w) AM_BASE(&paletteram)
	AM_RANGE(0xd000, 0xdfff) AM_WRITE(sichuan2_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0xe000, 0xffff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(sichuan2_dsw1_r)
	AM_RANGE(0x01, 0x01) AM_READ(input_port_4_r)
	AM_RANGE(0x02, 0x02) AM_READ(input_port_0_r)
	AM_RANGE(0x03, 0x03) AM_READ(input_port_1_r)
	AM_RANGE(0x04, 0x04) AM_READ(input_port_2_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(sichuan2_coin_w)
	AM_RANGE(0x01, 0x01) AM_WRITE(m72_sound_command_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(sichuan2_bankswitch_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0xfd00, 0xffff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xfd00, 0xffff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x01, 0x01) AM_READ(YM2151_status_port_0_r)
	AM_RANGE(0x80, 0x80) AM_READ(soundlatch_r)
	AM_RANGE(0x84, 0x84) AM_READ(m72_sample_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0x01, 0x01) AM_WRITE(YM2151_data_port_0_w)
	AM_RANGE(0x80, 0x81) AM_WRITE(shisen_sample_addr_w)
	AM_RANGE(0x82, 0x82) AM_WRITE(m72_sample_w)
	AM_RANGE(0x83, 0x83) AM_WRITE(m72_sound_irq_ack_w)
ADDRESS_MAP_END



INPUT_PORTS_START( shisen )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("COIN")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x03, 0x03, "Timer" )
	PORT_DIPSETTING(    0x03, "Slow" )
	PORT_DIPSETTING(    0x02, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x01, "Fast" )
	PORT_DIPSETTING(    0x00, "Fastest" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )  /* Gets filled in based on the coin mode */

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Coin Mode" )
	PORT_DIPSETTING(    0x04, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	PORT_DIPNAME( 0x08, 0x08, "Nude Pictures" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Women Select" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	/* In stop mode, press 2 to stop and 1 to restart */
	PORT_DIPNAME( 0x20, 0x20, "Stop Mode (Cheat)")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Play Mode" )
	PORT_DIPSETTING(    0x00, "1 Player" )
	PORT_DIPSETTING(    0x40, "2 Player" )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	/* Fake port to support the two different coin modes */
	PORT_START_TAG("FAKE")
	PORT_DIPNAME( 0x0f, 0x0f, "Coinage Mode 1" )   /* mapped on coin mode 1 */
	PORT_DIPSETTING(    0x0a, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 8C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
 	PORT_DIPNAME( 0x30, 0x30, "Coin A  Mode 2" )   /* mapped on coin mode 2 */
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, "Coin B  Mode 2" )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )
INPUT_PORTS_END



static const gfx_layout charlayout =
{
	8,8,
	4096*8,
	4,
	{ 0, 4, 0x80000*8+0, 0x80000*8+4 },
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8
};


static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000, &charlayout,  0, 16 },
	{ -1 } /* end of array */
};



static struct YM2151interface ym2151_interface =
{
	m72_ym2151_irq_handler
};



static MACHINE_DRIVER_START( shisen )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 6000000)	/* 6 MHz ? */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_IO_MAP(readport,writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 3579645)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)
	MDRV_CPU_IO_MAP(sound_readport,sound_writeport)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,128)	/* clocked by V1? (Vigilante) */
								/* IRQs are generated by main Z80 and YM2151 */
	MDRV_FRAMES_PER_SECOND(55)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(m72_sound)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER|VIDEO_PIXEL_ASPECT_RATIO_1_2)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 64*8-1, 0*8, 32*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(sichuan2)
	MDRV_VIDEO_UPDATE(sichuan2)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 3579545)
	MDRV_SOUND_CONFIG(ym2151_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.50)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.50)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( sichuan2 )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )	/* 64k+128k for main CPU */
	ROM_LOAD( "ic06.06",      0x00000, 0x10000, CRC(98a2459b) SHA1(42102cf2921f80be7600b11aba63538e3b3858ec) )
	ROM_RELOAD(               0x10000, 0x10000 )
	ROM_LOAD( "ic07.03",      0x20000, 0x10000, CRC(0350f6e2) SHA1(c683571969c0e4c66eb316a1bc580759db02bbfc) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "ic01.01",      0x00000, 0x10000, CRC(51b0a26c) SHA1(af2482cfe8d395848c8e1bf07bf1049ffc6ee69b) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic08.04",      0x00000, 0x10000, CRC(1c0e221c) SHA1(87561f7dabf25309be784e797ac237aa3956ea1c) )
	ROM_LOAD( "ic09.05",      0x10000, 0x10000, CRC(8a7d8284) SHA1(56b5d352b506c5bfab24102b11c877dd28c8ad36) )
	ROM_LOAD( "ic12.08",      0x20000, 0x10000, CRC(48e1d043) SHA1(4fbd409aff593c0b27fc58c218a470adf48ee0b7) )
	ROM_LOAD( "ic13.09",      0x30000, 0x10000, CRC(3feff3f2) SHA1(2e87e4fb158379486de5d13feff5bf1965690e14) )
	ROM_LOAD( "ic14.10",      0x40000, 0x10000, CRC(b76a517d) SHA1(6dcab31ecc127c2fdc6912802cddfa62161d83a2) )
	ROM_LOAD( "ic15.11",      0x50000, 0x10000, CRC(8ff5ee7a) SHA1(c8f4d374a43fcc818378c4e73af2c03238a93ad0) )
	ROM_LOAD( "ic16.12",      0x60000, 0x10000, CRC(64e5d837) SHA1(030f9704dfdb06cd3ac583b857ce9239e1409f60) )
	ROM_LOAD( "ic17.13",      0x70000, 0x10000, CRC(02c1b2c4) SHA1(6e4fe801189766559859eb0d628f3ae65c05ad16) )
	ROM_LOAD( "ic18.14",      0x80000, 0x10000, CRC(f5a8370e) SHA1(b270af116ee15b5de048fc218215bf91a2ce6b46) )
	ROM_LOAD( "ic19.15",      0x90000, 0x10000, CRC(7a9b7671) SHA1(ddb9b412b494f2f259c56a163e5511353cf4ebb5) )
	ROM_LOAD( "ic20.16",      0xa0000, 0x10000, CRC(7fb396ad) SHA1(b245bb9a6a4c1ec518906c59c3a3da5e412369ab) )
	ROM_LOAD( "ic21.17",      0xb0000, 0x10000, CRC(fb83c652) SHA1(3d124e5c751732e8055bbb7b6853b6e64435a85d) )
	ROM_LOAD( "ic22.18",      0xc0000, 0x10000, CRC(d8b689e9) SHA1(14db7ba2246f12db9b6b5b4dcb4a648e4a65ace4) )
	ROM_LOAD( "ic23.19",      0xd0000, 0x10000, CRC(e6611947) SHA1(9267dad097b318174706d51fb94d613208b04f60) )
	ROM_LOAD( "ic10.06",      0xe0000, 0x10000, CRC(473b349a) SHA1(9f5d08e07c8175bc7ec3854499177af2c398bd76) )
	ROM_LOAD( "ic11.07",      0xf0000, 0x10000, CRC(d9a60285) SHA1(f8ef211e022e9c8ea25f6d8fb16266867656a591) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "ic02.02",      0x00000, 0x10000, CRC(92f0093d) SHA1(530b924aa991283045577d03524dfc7eacf1be49) )
	ROM_LOAD( "ic03.03",      0x10000, 0x10000, CRC(116a049c) SHA1(656c0d1d7f945c5f5637892721a58421b682fd01) )
	ROM_LOAD( "ic04.04",      0x20000, 0x10000, CRC(6840692b) SHA1(f6f7b063ecf7206e172843515be38376f8845b42) )
	ROM_LOAD( "ic05.05",      0x30000, 0x10000, CRC(92ffe22a) SHA1(19dcaf6e25bb7498d4ab19fa0a63f3326b9bff8f) )
ROM_END

ROM_START( sichuana )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )	/* 64k+128k for main CPU */
	ROM_LOAD( "sichuan.a6",   0x00000, 0x10000, CRC(f8ac05ef) SHA1(cd20e5239d73264f1323ba6b1e35934685852ba1) )
	ROM_RELOAD(               0x10000, 0x10000 )
	ROM_LOAD( "ic07.03",      0x20000, 0x10000, CRC(0350f6e2) SHA1(c683571969c0e4c66eb316a1bc580759db02bbfc) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "ic01.01",      0x00000, 0x10000, CRC(51b0a26c) SHA1(af2482cfe8d395848c8e1bf07bf1049ffc6ee69b) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic08.04",      0x00000, 0x10000, CRC(1c0e221c) SHA1(87561f7dabf25309be784e797ac237aa3956ea1c) )
	ROM_LOAD( "ic09.05",      0x10000, 0x10000, CRC(8a7d8284) SHA1(56b5d352b506c5bfab24102b11c877dd28c8ad36) )
	ROM_LOAD( "ic12.08",      0x20000, 0x10000, CRC(48e1d043) SHA1(4fbd409aff593c0b27fc58c218a470adf48ee0b7) )
	ROM_LOAD( "ic13.09",      0x30000, 0x10000, CRC(3feff3f2) SHA1(2e87e4fb158379486de5d13feff5bf1965690e14) )
	ROM_LOAD( "ic14.10",      0x40000, 0x10000, CRC(b76a517d) SHA1(6dcab31ecc127c2fdc6912802cddfa62161d83a2) )
	ROM_LOAD( "ic15.11",      0x50000, 0x10000, CRC(8ff5ee7a) SHA1(c8f4d374a43fcc818378c4e73af2c03238a93ad0) )
	ROM_LOAD( "ic16.12",      0x60000, 0x10000, CRC(64e5d837) SHA1(030f9704dfdb06cd3ac583b857ce9239e1409f60) )
	ROM_LOAD( "ic17.13",      0x70000, 0x10000, CRC(02c1b2c4) SHA1(6e4fe801189766559859eb0d628f3ae65c05ad16) )
	ROM_LOAD( "ic18.14",      0x80000, 0x10000, CRC(f5a8370e) SHA1(b270af116ee15b5de048fc218215bf91a2ce6b46) )
	ROM_LOAD( "ic19.15",      0x90000, 0x10000, CRC(7a9b7671) SHA1(ddb9b412b494f2f259c56a163e5511353cf4ebb5) )
	ROM_LOAD( "ic20.16",      0xa0000, 0x10000, CRC(7fb396ad) SHA1(b245bb9a6a4c1ec518906c59c3a3da5e412369ab) )
	ROM_LOAD( "ic21.17",      0xb0000, 0x10000, CRC(fb83c652) SHA1(3d124e5c751732e8055bbb7b6853b6e64435a85d) )
	ROM_LOAD( "ic22.18",      0xc0000, 0x10000, CRC(d8b689e9) SHA1(14db7ba2246f12db9b6b5b4dcb4a648e4a65ace4) )
	ROM_LOAD( "ic23.19",      0xd0000, 0x10000, CRC(e6611947) SHA1(9267dad097b318174706d51fb94d613208b04f60) )
	ROM_LOAD( "ic10.06",      0xe0000, 0x10000, CRC(473b349a) SHA1(9f5d08e07c8175bc7ec3854499177af2c398bd76) )
	ROM_LOAD( "ic11.07",      0xf0000, 0x10000, CRC(d9a60285) SHA1(f8ef211e022e9c8ea25f6d8fb16266867656a591) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "ic02.02",      0x00000, 0x10000, CRC(92f0093d) SHA1(530b924aa991283045577d03524dfc7eacf1be49) )
	ROM_LOAD( "ic03.03",      0x10000, 0x10000, CRC(116a049c) SHA1(656c0d1d7f945c5f5637892721a58421b682fd01) )
	ROM_LOAD( "ic04.04",      0x20000, 0x10000, CRC(6840692b) SHA1(f6f7b063ecf7206e172843515be38376f8845b42) )
	ROM_LOAD( "ic05.05",      0x30000, 0x10000, CRC(92ffe22a) SHA1(19dcaf6e25bb7498d4ab19fa0a63f3326b9bff8f) )
ROM_END

ROM_START( shisen )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )	/* 64k+128k for main CPU */
	ROM_LOAD( "a-27-a.rom",   0x00000, 0x20000, CRC(de2ecf05) SHA1(7256c5587f92db10a52c43001e3236f3be3df5df) )
	ROM_RELOAD(               0x10000, 0x20000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "ic01.01",      0x00000, 0x10000, CRC(51b0a26c) SHA1(af2482cfe8d395848c8e1bf07bf1049ffc6ee69b) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic08.04",      0x00000, 0x10000, CRC(1c0e221c) SHA1(87561f7dabf25309be784e797ac237aa3956ea1c) )
	ROM_LOAD( "ic09.05",      0x10000, 0x10000, CRC(8a7d8284) SHA1(56b5d352b506c5bfab24102b11c877dd28c8ad36) )
	ROM_LOAD( "ic12.08",      0x20000, 0x10000, CRC(48e1d043) SHA1(4fbd409aff593c0b27fc58c218a470adf48ee0b7) )
	ROM_LOAD( "ic13.09",      0x30000, 0x10000, CRC(3feff3f2) SHA1(2e87e4fb158379486de5d13feff5bf1965690e14) )
	ROM_LOAD( "ic14.10",      0x40000, 0x10000, CRC(b76a517d) SHA1(6dcab31ecc127c2fdc6912802cddfa62161d83a2) )
	ROM_LOAD( "ic15.11",      0x50000, 0x10000, CRC(8ff5ee7a) SHA1(c8f4d374a43fcc818378c4e73af2c03238a93ad0) )
	ROM_LOAD( "ic16.12",      0x60000, 0x10000, CRC(64e5d837) SHA1(030f9704dfdb06cd3ac583b857ce9239e1409f60) )
	ROM_LOAD( "ic17.13",      0x70000, 0x10000, CRC(02c1b2c4) SHA1(6e4fe801189766559859eb0d628f3ae65c05ad16) )
	ROM_LOAD( "ic18.14",      0x80000, 0x10000, CRC(f5a8370e) SHA1(b270af116ee15b5de048fc218215bf91a2ce6b46) )
	ROM_LOAD( "ic19.15",      0x90000, 0x10000, CRC(7a9b7671) SHA1(ddb9b412b494f2f259c56a163e5511353cf4ebb5) )
	ROM_LOAD( "ic20.16",      0xa0000, 0x10000, CRC(7fb396ad) SHA1(b245bb9a6a4c1ec518906c59c3a3da5e412369ab) )
	ROM_LOAD( "ic21.17",      0xb0000, 0x10000, CRC(fb83c652) SHA1(3d124e5c751732e8055bbb7b6853b6e64435a85d) )
	ROM_LOAD( "ic22.18",      0xc0000, 0x10000, CRC(d8b689e9) SHA1(14db7ba2246f12db9b6b5b4dcb4a648e4a65ace4) )
	ROM_LOAD( "ic23.19",      0xd0000, 0x10000, CRC(e6611947) SHA1(9267dad097b318174706d51fb94d613208b04f60) )
	ROM_LOAD( "ic10.06",      0xe0000, 0x10000, CRC(473b349a) SHA1(9f5d08e07c8175bc7ec3854499177af2c398bd76) )
	ROM_LOAD( "ic11.07",      0xf0000, 0x10000, CRC(d9a60285) SHA1(f8ef211e022e9c8ea25f6d8fb16266867656a591) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "ic02.02",      0x00000, 0x10000, CRC(92f0093d) SHA1(530b924aa991283045577d03524dfc7eacf1be49) )
	ROM_LOAD( "ic03.03",      0x10000, 0x10000, CRC(116a049c) SHA1(656c0d1d7f945c5f5637892721a58421b682fd01) )
	ROM_LOAD( "ic04.04",      0x20000, 0x10000, CRC(6840692b) SHA1(f6f7b063ecf7206e172843515be38376f8845b42) )
	ROM_LOAD( "ic05.05",      0x30000, 0x10000, CRC(92ffe22a) SHA1(19dcaf6e25bb7498d4ab19fa0a63f3326b9bff8f) )
ROM_END

ROM_START( matchit )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )	/* 64k+128k for main CPU */
	ROM_LOAD( "2.11d",      0x00000, 0x10000, CRC(299815f7) SHA1(dd25f69d3c825e12e5c2e24b5bbfda9c39400345) )
	ROM_RELOAD(               0x10000, 0x10000 )
	ROM_LOAD( "ic07.03",      0x20000, 0x10000, CRC(0350f6e2) SHA1(c683571969c0e4c66eb316a1bc580759db02bbfc) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "ic01.01",      0x00000, 0x10000, CRC(51b0a26c) SHA1(af2482cfe8d395848c8e1bf07bf1049ffc6ee69b) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic08.04",      0x00000, 0x10000, CRC(1c0e221c) SHA1(87561f7dabf25309be784e797ac237aa3956ea1c) )
	ROM_LOAD( "ic09.05",      0x10000, 0x10000, CRC(8a7d8284) SHA1(56b5d352b506c5bfab24102b11c877dd28c8ad36) )
	ROM_LOAD( "ic12.08",      0x20000, 0x10000, CRC(48e1d043) SHA1(4fbd409aff593c0b27fc58c218a470adf48ee0b7) )
	ROM_LOAD( "ic13.09",      0x30000, 0x10000, CRC(3feff3f2) SHA1(2e87e4fb158379486de5d13feff5bf1965690e14) )
	ROM_LOAD( "ic14.10",      0x40000, 0x10000, CRC(b76a517d) SHA1(6dcab31ecc127c2fdc6912802cddfa62161d83a2) )
	ROM_LOAD( "ic15.11",      0x50000, 0x10000, CRC(8ff5ee7a) SHA1(c8f4d374a43fcc818378c4e73af2c03238a93ad0) )
	ROM_LOAD( "ic16.12",      0x60000, 0x10000, CRC(64e5d837) SHA1(030f9704dfdb06cd3ac583b857ce9239e1409f60) )
	ROM_LOAD( "ic17.13",      0x70000, 0x10000, CRC(02c1b2c4) SHA1(6e4fe801189766559859eb0d628f3ae65c05ad16) )
	ROM_LOAD( "ic18.14",      0x80000, 0x10000, CRC(f5a8370e) SHA1(b270af116ee15b5de048fc218215bf91a2ce6b46) )
	ROM_LOAD( "ic19.15",      0x90000, 0x10000, CRC(7a9b7671) SHA1(ddb9b412b494f2f259c56a163e5511353cf4ebb5) )
	ROM_LOAD( "ic20.16",      0xa0000, 0x10000, CRC(7fb396ad) SHA1(b245bb9a6a4c1ec518906c59c3a3da5e412369ab) )
	ROM_LOAD( "ic21.17",      0xb0000, 0x10000, CRC(fb83c652) SHA1(3d124e5c751732e8055bbb7b6853b6e64435a85d) )
	ROM_LOAD( "ic22.18",      0xc0000, 0x10000, CRC(d8b689e9) SHA1(14db7ba2246f12db9b6b5b4dcb4a648e4a65ace4) )
	ROM_LOAD( "ic23.19",      0xd0000, 0x10000, CRC(e6611947) SHA1(9267dad097b318174706d51fb94d613208b04f60) )
	ROM_LOAD( "ic10.06",      0xe0000, 0x10000, CRC(473b349a) SHA1(9f5d08e07c8175bc7ec3854499177af2c398bd76) )
	ROM_LOAD( "ic11.07",      0xf0000, 0x10000, CRC(d9a60285) SHA1(f8ef211e022e9c8ea25f6d8fb16266867656a591) )

	ROM_REGION( 0x40000, REGION_SOUND1, ROMREGION_ERASE00 )	/* samples */
	/* no samples on this board */
ROM_END

GAME( 1989, sichuan2, 0,        shisen, shisen, 0, ROT0, "Tamtex", "Sichuan II (hack?) (set 1)", 0 )
GAME( 1989, sichuana, sichuan2, shisen, shisen, 0, ROT0, "Tamtex", "Sichuan II (hack?) (set 2)", 0 )
GAME( 1989, shisen,   sichuan2, shisen, shisen, 0, ROT0, "Tamtex", "Shisensho - Joshiryo-Hen (Japan)", 0 )
GAME( 1989, matchit,  sichuan2, shisen, shisen, 0, ROT0, "Tamtex", "Match It", 0 )
