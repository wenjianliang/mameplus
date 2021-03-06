// license:BSD-3-Clause
// copyright-holders:Curt Coder
/**********************************************************************

    Luxor ABC DOS card emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*********************************************************************/

/*

PCB Layout
----------

55 10762-01

|-----------------------------------|
|                                   |
|                                   |
|                                   |
|                                   |
|   ROM3        ROM2                |
|                                   |
|                                   |
|                                   |
|                                   |
|                                   |
|               ROM1        ROM0    |
|                                   |
|                                   |
|                                   |
|                                   |
|                                   |
|                                   |
|           LS02            LS139   |
|                                   |
|                                   |
|                                   |
|   LS367   LS241   LS241           |
|                                   |
|                                   |
|                                   |
|                                   |
|--|-----------------------------|--|
   |------------CON1-------------|

Notes:
    All IC's shown.

    ROM0    - Synertek C55022 4Kx8 ROM "DOSDD80"
    ROM1    - Motorola MCM2708C 1Kx8 EPROM "9704"
    ROM2    - empty socket
    ROM3    - empty socket
    CON1    - ABC bus connector

*/

#include "dos.h"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type ABC_DOS = &device_creator<abc_dos_device>;


//-------------------------------------------------
//  ROM( abc_dos )
//-------------------------------------------------

ROM_START( abc_dos )
	ROM_REGION( 0x2000, "dos", ROMREGION_ERASEFF )
	ROM_DEFAULT_BIOS("ufd20")
	ROM_SYSTEM_BIOS( 0, "abcdos", "ABC-DOS" ) // Scandia Metric FD2
	ROMX_LOAD( "abcdos.3d",   0x0000, 0x1000, CRC(2cb2192f) SHA1(a6b3a9587714f8db807c05bee6c71c0684363744), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "dosdd80", "ABC-DOS DD" ) // ABC 830
	ROMX_LOAD( "dosdd80.3d",  0x0000, 0x1000, CRC(36db4c15) SHA1(ae462633f3a9c142bb029beb14749a84681377fa), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "ufd20", "UFD-DOS v.20" ) // ABC 830
	ROMX_LOAD( "ufddos20.3d", 0x0000, 0x1000, CRC(69b09c0b) SHA1(403997a06cf6495b8fa13dc74eff6a64ef7aa53e), ROM_BIOS(3) )
	ROM_LOAD( "printer.4c",   0x1000, 0x400, NO_DUMP )
	//ROM_LOAD( "spare.4a",   0x1400, 0x400, NO_DUMP )
	ROM_LOAD( "9704.3c",      0x1800, 0x400, NO_DUMP )
	//ROM_LOAD( "spare.3a",   0x1c00, 0x400, NO_DUMP )
ROM_END


//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *abc_dos_device::device_rom_region() const
{
	return ROM_NAME( abc_dos );
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  abc_dos_device - constructor
//-------------------------------------------------

abc_dos_device::abc_dos_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: device_t(mconfig, ABC_DOS, "ABC DOS", tag, owner, clock, "abc_dos", __FILE__),
		device_abcbus_card_interface(mconfig, *this),
		m_rom(*this, "dos")
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void abc_dos_device::device_start()
{
}



//**************************************************************************
//  ABC BUS INTERFACE
//**************************************************************************

//-------------------------------------------------
//  abcbus_xmemfl -
//-------------------------------------------------

UINT8 abc_dos_device::abcbus_xmemfl(offs_t offset)
{
	UINT8 data = 0xff;

	if (offset >= 0x6000 && offset < 0x8000)
	{
		data = m_rom->base()[offset & 0x1fff];
	}

	return data;
}
