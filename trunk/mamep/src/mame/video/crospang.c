/*

  Cross Pang
  video hardware emulation

 -- this seems to be the same as the tumblepop bootleg based hardware
    in tumbleb.c


*/

#include "emu.h"
#include "includes/crospang.h"

WRITE16_HANDLER( bestri_tilebank_w)
{
	crospang_state *state = space->machine().driver_data<crospang_state>();

	state->m_bestri_tilebank = (data>>10) & 0xf;
	//printf("bestri %04x\n", data);

	state->m_fg_layer->mark_all_dirty();
	state->m_bg_layer->mark_all_dirty();
}


WRITE16_HANDLER ( bestri_bg_scrolly_w )
{
	crospang_state *state = space->machine().driver_data<crospang_state>();

	/* Very Strange */
	int scroll =  (data & 0x3ff) ^ 0x0155;
	state->m_bg_layer->set_scrolly(0, -scroll + 7);
}

WRITE16_HANDLER ( bestri_fg_scrolly_w )
{
	crospang_state *state = space->machine().driver_data<crospang_state>();

	/* Very Strange */
	int scroll = (data & 0x3ff) ^ 0x00ab;
	state->m_fg_layer->set_scrolly(0, -scroll + 7);
}

WRITE16_HANDLER ( bestri_fg_scrollx_w )
{
	crospang_state *state = space->machine().driver_data<crospang_state>();

	// printf("fg_layer x %04x\n",data);
	state->m_fg_layer->set_scrollx(0, data + 32);
}

WRITE16_HANDLER ( bestri_bg_scrollx_w )
{
	crospang_state *state = space->machine().driver_data<crospang_state>();

	// printf("bg_layer x %04x\n",data);
	state->m_bg_layer->set_scrollx(0, data - 60);
}


WRITE16_HANDLER ( crospang_fg_scrolly_w )
{
	crospang_state *state = space->machine().driver_data<crospang_state>();
	state->m_fg_layer->set_scrolly(0, data + 8);
}

WRITE16_HANDLER ( crospang_bg_scrolly_w )
{
	crospang_state *state = space->machine().driver_data<crospang_state>();
	state->m_bg_layer->set_scrolly(0, data + 8);
}

WRITE16_HANDLER ( crospang_fg_scrollx_w )
{
	crospang_state *state = space->machine().driver_data<crospang_state>();
	state->m_fg_layer->set_scrollx(0, data);
}

WRITE16_HANDLER ( crospang_bg_scrollx_w )
{
	crospang_state *state = space->machine().driver_data<crospang_state>();
	state->m_bg_layer->set_scrollx(0, data + 4);
}


WRITE16_HANDLER ( crospang_fg_videoram_w )
{
	crospang_state *state = space->machine().driver_data<crospang_state>();
	COMBINE_DATA(&state->m_fg_videoram[offset]);
	state->m_fg_layer->mark_tile_dirty(offset);
}

WRITE16_HANDLER ( crospang_bg_videoram_w )
{
	crospang_state *state = space->machine().driver_data<crospang_state>();
	COMBINE_DATA(&state->m_bg_videoram[offset]);
	state->m_bg_layer->mark_tile_dirty(offset);
}

static TILE_GET_INFO( get_bg_tile_info )
{
	crospang_state *state = machine.driver_data<crospang_state>();
	int data  = state->m_bg_videoram[tile_index];
	int tile  = data & 0xfff;
	int color = (data >> 12) & 0x0f;

	SET_TILE_INFO(1, tile + state->m_bestri_tilebank * 0x1000, color + 0x20, 0);
}

static TILE_GET_INFO( get_fg_tile_info )
{
	crospang_state *state = machine.driver_data<crospang_state>();
	int data  = state->m_fg_videoram[tile_index];
	int tile  = data & 0xfff;
	int color = (data >> 12) & 0x0f;

	SET_TILE_INFO(1, tile + state->m_bestri_tilebank * 0x1000, color + 0x10, 0);
}

/*

 offset

      0     -------yyyyyyyyy  y offset
            -----hh---------  sprite height
            ---a------------  alpha blending enable
            f---------------  flip x
            -??-?-----------  unused

      1     --ssssssssssssss  sprite code
            ??--------------  unused

      2     -------xxxxxxxxx  x offset
            ---cccc---------  colors
            ???-------------  unused

      3     ----------------  unused

*/

/* jumpkids / tumbleb.c! */
static void draw_sprites( running_machine &machine, bitmap_ind16 &bitmap, const rectangle &cliprect )
{
	crospang_state *state = machine.driver_data<crospang_state>();
	int offs;
	int flipscreen = 0;

	for (offs = 0; offs < state->m_spriteram_size / 2; offs += 4)
	{
		int x, y, sprite, colour, multi, fx, fy, inc, flash, mult;

		sprite = state->m_spriteram[offs + 1] & 0x7fff;
		if (!sprite)
			continue;

		y = state->m_spriteram[offs];
		flash = y & 0x1000;
		if (flash && (machine.primary_screen->frame_number() & 1))
			continue;

		x = state->m_spriteram[offs + 2];
		colour = (x >>9) & 0xf;

		fx = y & 0x2000;
		fy = y & 0x4000;
		multi = (1 << ((y & 0x0600) >> 9)) - 1;	/* 1x, 2x, 4x, 8x height */

		x = x & 0x01ff;
		y = y & 0x01ff;
		if (x >= 320) x -= 512;
		if (y >= 256) y -= 512;
		y = 240 - y;
		x = 304 - x;

		// sprite &= ~multi; /* Todo:  I bet TumblePop bootleg doesn't do this either */
		if (fy)
			inc = -1;
		else
		{
			sprite += multi;
			inc = 1;
		}

		if (flipscreen)
		{
			y = 240 - y;
			x = 304 - x;
			if (fx) fx = 0; else fx = 1;
			if (fy) fy = 0; else fy = 1;
			mult = 16;
		}
		else
			mult = -16;

		while (multi >= 0)
		{
			drawgfx_transpen(bitmap,cliprect,machine.gfx[0],
					sprite - multi * inc,
					colour,
					fx, fy,
					x - state->m_xsproff, y - state->m_ysproff + mult * multi,0);

			multi--;
		}
	}
}

VIDEO_START( crospang )
{
	crospang_state *state = machine.driver_data<crospang_state>();
	state->m_bg_layer = tilemap_create(machine, get_bg_tile_info, tilemap_scan_rows, 16, 16, 32, 32);
	state->m_fg_layer = tilemap_create(machine, get_fg_tile_info, tilemap_scan_rows, 16, 16, 32, 32);

	state->m_fg_layer->set_transparent_pen(0);
}

SCREEN_UPDATE_IND16( crospang )
{
	crospang_state *state = screen.machine().driver_data<crospang_state>();
	state->m_bg_layer->draw(bitmap, cliprect, 0, 0);
	state->m_fg_layer->draw(bitmap, cliprect, 0, 0);
	draw_sprites(screen.machine(), bitmap, cliprect);
	return 0;
}
