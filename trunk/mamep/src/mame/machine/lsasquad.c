#include "emu.h"
#include "cpu/z80/z80.h"
#include "includes/lsasquad.h"

/***************************************************************************

 main <-> sound CPU communication

***************************************************************************/

static TIMER_CALLBACK( nmi_callback )
{
	lsasquad_state *state = machine.driver_data<lsasquad_state>();

	if (state->m_sound_nmi_enable)
		device_set_input_line(state->m_audiocpu, INPUT_LINE_NMI, PULSE_LINE);
	else
		state->m_pending_nmi = 1;
}

WRITE8_HANDLER( lsasquad_sh_nmi_disable_w )
{
	lsasquad_state *state = space->machine().driver_data<lsasquad_state>();
	state->m_sound_nmi_enable = 0;
}

WRITE8_HANDLER( lsasquad_sh_nmi_enable_w )
{
	lsasquad_state *state = space->machine().driver_data<lsasquad_state>();

	state->m_sound_nmi_enable = 1;
	if (state->m_pending_nmi)
	{
		device_set_input_line(state->m_audiocpu, INPUT_LINE_NMI, PULSE_LINE);
		state->m_pending_nmi = 0;
	}
}

WRITE8_HANDLER( lsasquad_sound_command_w )
{
	lsasquad_state *state = space->machine().driver_data<lsasquad_state>();
	state->m_sound_pending |= 0x01;
	state->m_sound_cmd = data;

	//logerror("%04x: sound cmd %02x\n", cpu_get_pc(&space->device()), data);
	space->machine().scheduler().synchronize(FUNC(nmi_callback), data);
}

READ8_HANDLER( lsasquad_sh_sound_command_r )
{
	lsasquad_state *state = space->machine().driver_data<lsasquad_state>();
	state->m_sound_pending &= ~0x01;
	//logerror("%04x: read sound cmd %02x\n", cpu_get_pc(&space->device()), state->m_sound_cmd);
	return state->m_sound_cmd;
}

WRITE8_HANDLER( lsasquad_sh_result_w )
{
	lsasquad_state *state = space->machine().driver_data<lsasquad_state>();
	state->m_sound_pending |= 0x02;
	//logerror("%04x: sound res %02x\n", cpu_get_pc(&space->device()), data);
	state->m_sound_result = data;
}

READ8_HANDLER( lsasquad_sound_result_r )
{
	lsasquad_state *state = space->machine().driver_data<lsasquad_state>();
	state->m_sound_pending &= ~0x02;
	//logerror("%04x: read sound res %02x\n", cpu_get_pc(&space->device()), state->m_sound_result);
	return state->m_sound_result;
}

READ8_HANDLER( lsasquad_sound_status_r )
{
	lsasquad_state *state = space->machine().driver_data<lsasquad_state>();

	/* bit 0: message pending for sound cpu */
	/* bit 1: message pending for main cpu */
	return state->m_sound_pending;
}


READ8_HANDLER( daikaiju_sh_sound_command_r )
{
	lsasquad_state *state = space->machine().driver_data<lsasquad_state>();
	state->m_sound_pending &= ~0x01;
	state->m_sound_pending |= 0x02;
	//logerror("%04x: read sound cmd %02x\n", cpu_get_pc(&space->device()), state->m_sound_cmd);
	return state->m_sound_cmd;
}

READ8_HANDLER( daikaiju_sound_status_r )
{
	lsasquad_state *state = space->machine().driver_data<lsasquad_state>();

	/* bit 0: message pending for sound cpu */
	/* bit 1: message pending for main cpu */
	return state->m_sound_pending ^ 3;
}


/***************************************************************************

 LSA Squad 68705 protection interface

 The following is ENTIRELY GUESSWORK!!!

***************************************************************************/

READ8_HANDLER( lsasquad_68705_port_a_r )
{
	lsasquad_state *state = space->machine().driver_data<lsasquad_state>();

	//logerror("%04x: 68705 port A read %02x\n", cpu_get_pc(&space->device()), state->m_port_a_in);
	return (state->m_port_a_out & state->m_ddr_a) | (state->m_port_a_in & ~state->m_ddr_a);
}

WRITE8_HANDLER( lsasquad_68705_port_a_w )
{
	lsasquad_state *state = space->machine().driver_data<lsasquad_state>();

	//logerror("%04x: 68705 port A write %02x\n", cpu_get_pc(&space->device()), data);
	state->m_port_a_out = data;
}

WRITE8_HANDLER( lsasquad_68705_ddr_a_w )
{
	lsasquad_state *state = space->machine().driver_data<lsasquad_state>();
	state->m_ddr_a = data;
}



/*
 *  Port B connections:
 *
 *  all bits are logical 1 when read (+5V pullup)
 *
 *  1   W  when 1->0, enables latch which brings the command from main CPU (read from port A)
 *  2   W  when 0->1, copies port A to the latch for the main CPU
 */

READ8_HANDLER( lsasquad_68705_port_b_r )
{
	lsasquad_state *state = space->machine().driver_data<lsasquad_state>();
	return (state->m_port_b_out & state->m_ddr_b) | (state->m_port_b_in & ~state->m_ddr_b);
}

WRITE8_HANDLER( lsasquad_68705_port_b_w )
{
	lsasquad_state *state = space->machine().driver_data<lsasquad_state>();

	//logerror("%04x: 68705 port B write %02x\n", cpu_get_pc(&space->device()), data);

	if ((state->m_ddr_b & 0x02) && (~data & 0x02) && (state->m_port_b_out & 0x02))
	{
		state->m_port_a_in = state->m_from_main;
		if (state->m_main_sent)
			device_set_input_line(state->m_mcu, 0, CLEAR_LINE);
		state->m_main_sent = 0;
		//logerror("read command %02x from main cpu\n", state->m_port_a_in);
	}

	if ((state->m_ddr_b & 0x04) && (data & 0x04) && (~state->m_port_b_out & 0x04))
	{
		//logerror("send command %02x to main cpu\n", state->m_port_a_out);
		state->m_from_mcu = state->m_port_a_out;
		state->m_mcu_sent = 1;
	}

	state->m_port_b_out = data;
}

WRITE8_HANDLER( lsasquad_68705_ddr_b_w )
{
	lsasquad_state *state = space->machine().driver_data<lsasquad_state>();
	state->m_ddr_b = data;
}

WRITE8_HANDLER( lsasquad_mcu_w )
{
	lsasquad_state *state = space->machine().driver_data<lsasquad_state>();

	//logerror("%04x: mcu_w %02x\n", cpu_get_pc(&space->device()), data);
	state->m_from_main = data;
	state->m_main_sent = 1;
	device_set_input_line(state->m_mcu, 0, ASSERT_LINE);
}

READ8_HANDLER( lsasquad_mcu_r )
{
	lsasquad_state *state = space->machine().driver_data<lsasquad_state>();

	//logerror("%04x: mcu_r %02x\n", cpu_get_pc(&space->device()), state->m_from_mcu);
	state->m_mcu_sent = 0;
	return state->m_from_mcu;
}

READ8_HANDLER( lsasquad_mcu_status_r )
{
	lsasquad_state *state = space->machine().driver_data<lsasquad_state>();
	int res = input_port_read(space->machine(), "MCU");

	/* bit 0 = when 1, mcu is ready to receive data from main cpu */
	/* bit 1 = when 0, mcu has sent data to the main cpu */
	//logerror("%04x: mcu_status_r\n",cpu_get_pc(&space->device()));
	if (!state->m_main_sent)
		res |= 0x01;
	if (!state->m_mcu_sent)
		res |= 0x02;

	return res;
}

READ8_HANDLER( daikaiju_mcu_status_r )
{
	lsasquad_state *state = space->machine().driver_data<lsasquad_state>();
	int res = input_port_read(space->machine(), "MCU");

	/* bit 0 = when 1, mcu is ready to receive data from main cpu */
	/* bit 1 = when 0, mcu has sent data to the main cpu */
	//logerror("%04x: mcu_status_r\n",cpu_get_pc(&space->device()));
	if (!state->m_main_sent)
		res |= 0x01;
	if (!state->m_mcu_sent)
		res |= 0x02;

	res |= ((state->m_sound_pending & 0x02) ^ 2) << 3; //inverted flag
	state->m_sound_pending &= ~0x02;
	return res;
}
