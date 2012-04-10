
/* based on driver from video/metro.c by Luca Elia */
/* modified by Hau */

/***************************************************************************

                              -= Metro Games =-

                    driver by   Luca Elia (l.elia@tin.it)


Note:   if MAME_DEBUG is defined, pressing Z with:

                Q       Shows Layer 0
                W       Shows Layer 1
                E       Shows Layer 2
                A       Shows Sprites

        Keys can be used together!


                            [ 3 Scrolling Layers ]

        There is memory for a huge layer, but the actual tilemap
        is a smaller window (of fixed size) carved from anywhere
        inside that layer.

        Tile Size:                  8 x 8 x 4
        (later games can switch to  8 x 8 x 8, 16 x 16 x 4/8 at run time)

        Big Layer Size:         2048 x 2048 (8x8 tiles) or 4096 x 4096 (16x16 tiles)

        Tilemap Window Size:    512 x 256 (8x8 tiles) or 1024 x 512 (16x16 tiles)

        The tile codes in memory do not map directly to tiles. They
        are indexes into a table (with 0x200 entries) that defines
        a virtual set of tiles for the 3 layers. Each entry in that
        table adds 16 tiles to the set of available tiles, and decides
        their color code.

        Tile code with their msbit set are different as they mean:
        draw a tile filled with a single color (0-1ff)


                            [ 512 Zooming Sprites ]

        The sprites are NOT tile based: the "tile" size can vary from
        8 to 64 (independently for width and height) with an 8 pixel
        granularity. The "tile" address is a multiple of 8x8 pixels.

        Each sprite can be shrinked to ~1/4 or enlarged to ~32x following
        an exponential curve of sizes (with one zoom value for both width
        and height)

***************************************************************************/

#include "emu.h"
#include "includes/hyprduel.h"

/***************************************************************************
                            Palette GGGGGRRRRRBBBBBx
***************************************************************************/

WRITE16_MEMBER(hyprduel_state::hyprduel_paletteram_w)
{
	data = COMBINE_DATA(&m_paletteram[offset]);
	palette_set_color_rgb(machine(), offset, pal5bit(data >> 6), pal5bit(data >> 11), pal5bit(data >> 1));
}


/***************************************************************************
                        Tilemaps: Tiles Set & Window

    Each entry in the Tiles Set RAM uses 2 words to specify a starting
    tile code and a color code. This adds 16 consecutive tiles with
    that color code to the set of available tiles.

        Offset:     Bits:                   Value:

        0.w         fedc ---- ---- ----
                    ---- ba98 7654 ----     Color Code
                    ---- ---- ---- 3210     Code High Bits

        2.w                                 Code Low Bits
***************************************************************************/


/***************************************************************************
                            Tilemaps: Rendering
***************************************************************************/

/* A 2048 x 2048 virtual tilemap */

#define BIG_NX		(0x100)
#define BIG_NY		(0x100)

/* A smaller 512 x 256 window defines the actual tilemap */

#define WIN_NX		(0x40)
#define WIN_NY		(0x20)
//#define WIN_NX        (0x40+1)
//#define WIN_NY        (0x20+1)


/* 8x8x4 tiles only */
INLINE void get_tile_info( running_machine &machine, tile_data &tileinfo, int tile_index, int layer, UINT16 *vram)
{
	hyprduel_state *state = machine.driver_data<hyprduel_state>();
	UINT16 code;
	int table_index;
	UINT32 tile;

	/* The actual tile index depends on the window */
	tile_index = ((tile_index / WIN_NX + state->m_window[layer * 2 + 0] / 8) % BIG_NY) * BIG_NX +
					((tile_index % WIN_NX + state->m_window[layer * 2 + 1] / 8) % BIG_NX);

	/* Fetch the code */
	code = vram[tile_index];

	/* Use it as an index into the tiles set table */
	table_index = ((code & 0x1ff0) >> 4 ) * 2;
	tile = (state->m_tiletable[table_index + 0] << 16) + state->m_tiletable[table_index + 1];

	if (code & 0x8000) /* Special: draw a tile of a single color (i.e. not from the gfx ROMs) */
	{
		int _code = code & 0x000f;
		tileinfo.pen_data = state->m_empty_tiles + _code * 16 * 16;
		tileinfo.palette_base = ((code & 0x0ff0)) + 0x1000;
		tileinfo.flags = 0;
		tileinfo.group = 0;
	}
	else
	{
		tileinfo.group = 0;
		SET_TILE_INFO(
				0,
				(tile & 0xfffff) + (code & 0xf),
				(((tile & 0x0ff00000) >> 20)) + 0x100,
				TILE_FLIPXY((code & 0x6000) >> 13));
	}
}

/* 8x8x4 or 8x8x8 tiles. It's the tile's color that decides: if its low 4
   bits are high ($f,$1f,$2f etc) the tile is 8bpp, otherwise it's 4bpp */
INLINE void get_tile_info_8bit( running_machine &machine, tile_data &tileinfo, int tile_index, int layer, UINT16 *vram )
{
	hyprduel_state *state = machine.driver_data<hyprduel_state>();
	UINT16 code;
	int table_index;
	UINT32 tile;

	/* The actual tile index depends on the window */
	tile_index = ((tile_index / WIN_NX + state->m_window[layer * 2 + 0] / 8) % BIG_NY) * BIG_NX +
					((tile_index % WIN_NX + state->m_window[layer * 2 + 1] / 8) % BIG_NX);

	/* Fetch the code */
	code = vram[tile_index];

	/* Use it as an index into the tiles set table */
	table_index = ((code & 0x1ff0) >> 4) * 2;
	tile = (state->m_tiletable[table_index + 0] << 16) + state->m_tiletable[table_index + 1];

	if (code & 0x8000) /* Special: draw a tile of a single color (i.e. not from the gfx ROMs) */
	{
		int _code = code & 0x000f;
		tileinfo.pen_data = state->m_empty_tiles + _code * 16 * 16;
		tileinfo.palette_base = ((code & 0x0ff0)) + 0x1000;
		tileinfo.flags = 0;
		tileinfo.group = 0;
	}
	else if ((tile & 0x00f00000) == 0x00f00000)	/* draw tile as 8bpp */
	{
		tileinfo.group = 1;
		SET_TILE_INFO(
				1,
				(tile & 0xfffff) + 2*(code & 0xf),
				((tile & 0x0f000000) >> 24) + 0x10,
				TILE_FLIPXY((code & 0x6000) >> 13));
	}
	else
	{
		tileinfo.group = 0;
		SET_TILE_INFO(
				0,
				(tile & 0xfffff) + (code & 0xf),
				(((tile & 0x0ff00000) >> 20)) + 0x100,
				TILE_FLIPXY((code & 0x6000) >> 13));
	}
}

/* 16x16x4 or 16x16x8 tiles. It's the tile's color that decides: if its low 4
   bits are high ($f,$1f,$2f etc) the tile is 8bpp, otherwise it's 4bpp */
INLINE void get_tile_info_16x16_8bit( running_machine &machine, tile_data &tileinfo, int tile_index, int layer, UINT16 *vram )
{
	hyprduel_state *state = machine.driver_data<hyprduel_state>();
	UINT16 code;
	int table_index;
	UINT32 tile;

	/* The actual tile index depends on the window */
	tile_index = ((tile_index / WIN_NX + state->m_window[layer * 2 + 0] / 8) % BIG_NY) * BIG_NX +
					((tile_index % WIN_NX + state->m_window[layer * 2 + 1] / 8) % BIG_NX);

	/* Fetch the code */
	code = vram[tile_index];

	/* Use it as an index into the tiles set table */
	table_index = ((code & 0x1ff0) >> 4) * 2;
	tile = (state->m_tiletable[table_index + 0] << 16) + state->m_tiletable[table_index + 1];

	if (code & 0x8000) /* Special: draw a tile of a single color (i.e. not from the gfx ROMs) */
	{
		int _code = code & 0x000f;
		tileinfo.pen_data = state->m_empty_tiles + _code * 16 * 16;
		tileinfo.palette_base = ((code & 0x0ff0)) + 0x1000;
		tileinfo.flags = 0;
		tileinfo.group = 0;
	}
	else if ((tile & 0x00f00000) == 0x00f00000)	/* draw tile as 8bpp */
	{
		tileinfo.group = 1;
		SET_TILE_INFO(
				3,
				(tile & 0xfffff) + 8*(code & 0xf),
				((tile & 0x0f000000) >> 24) + 0x10,
				TILE_FLIPXY((code & 0x6000) >> 13));
	}
	else
	{
		tileinfo.group = 0;
		SET_TILE_INFO(
				2,
				(tile & 0xfffff) + 4*(code & 0xf),
				(((tile & 0x0ff00000) >> 20)) + 0x100,
				TILE_FLIPXY((code & 0x6000) >> 13));

	}
}

INLINE void hyprduel_vram_w( running_machine &machine, offs_t offset, UINT16 data, UINT16 mem_mask, int layer, UINT16 *vram )
{
	hyprduel_state *state = machine.driver_data<hyprduel_state>();
	COMBINE_DATA(&vram[offset]);
	{
		/* Account for the window */
		int col = (offset % BIG_NX) - ((state->m_window[layer * 2 + 1] / 8) % BIG_NX);
		int row = (offset / BIG_NX) - ((state->m_window[layer * 2 + 0] / 8) % BIG_NY);
		if (col < -(BIG_NX-WIN_NX))
			col += (BIG_NX-WIN_NX) + WIN_NX;
		if (row < -(BIG_NY-WIN_NY))
			row += (BIG_NY-WIN_NY) + WIN_NY;
		if ((col >= 0) && (col < WIN_NX) && (row >= 0) && (row < WIN_NY))
			state->m_bg_tilemap[layer]->mark_tile_dirty(row * WIN_NX + col);
	}
}



static TILE_GET_INFO( get_tile_info_0_8bit )
{
	hyprduel_state *state = machine.driver_data<hyprduel_state>();
	get_tile_info_8bit(machine, tileinfo, tile_index, 0, state->m_vram_0);
}

static TILE_GET_INFO( get_tile_info_1_8bit )
{
	hyprduel_state *state = machine.driver_data<hyprduel_state>();
	get_tile_info_8bit(machine, tileinfo, tile_index, 1, state->m_vram_1);
}

static TILE_GET_INFO( get_tile_info_2_8bit )
{
	hyprduel_state *state = machine.driver_data<hyprduel_state>();
	get_tile_info_8bit(machine, tileinfo, tile_index, 2, state->m_vram_2);
}

WRITE16_MEMBER(hyprduel_state::hyprduel_vram_0_w)
{
	hyprduel_vram_w(machine(), offset, data, mem_mask, 0, m_vram_0);
}

WRITE16_MEMBER(hyprduel_state::hyprduel_vram_1_w)
{
	hyprduel_vram_w(machine(), offset, data, mem_mask, 1, m_vram_1);
}

WRITE16_MEMBER(hyprduel_state::hyprduel_vram_2_w)
{
	hyprduel_vram_w(machine(), offset, data, mem_mask, 2, m_vram_2);
}


/* Dirty the relevant tilemap when its window changes */
WRITE16_MEMBER(hyprduel_state::hyprduel_window_w)
{
	UINT16 olddata = m_window[offset];
	UINT16 newdata = COMBINE_DATA(&m_window[offset]);
	if (newdata != olddata)
	{
		offset /= 2;
		m_bg_tilemap[offset]->mark_all_dirty();
	}
}

/***************************************************************************
                            Video Init Routines
***************************************************************************/

/*
 Sprites are not tile based, so we decode their graphics at runtime.

 We can't do it at startup because drawgfx requires the tiles to be
 pre-rotated to support vertical games, and that, in turn, requires
 the tile's sizes to be known at startup - which we don't!
*/

static void alloc_empty_tiles( running_machine &machine )
{
	hyprduel_state *state = machine.driver_data<hyprduel_state>();
	int code,i;

	state->m_empty_tiles = auto_alloc_array(machine, UINT8, 16*16*16);
	state->save_pointer(NAME(state->m_empty_tiles), 16*16*16);

	for (code = 0; code < 0x10; code++)
		for (i = 0; i < 16 * 16; i++)
			state->m_empty_tiles[16 * 16 * code + i] = code;
}


static void hyprduel_postload(running_machine &machine)
{
	hyprduel_state *state = machine.driver_data<hyprduel_state>();
	int i;

	for (i = 0; i < 3; i++)
	{
		UINT16 wx = state->m_window[i * 2 + 1];
		UINT16 wy = state->m_window[i * 2 + 0];

		state->m_bg_tilemap[i]->set_scrollx(0, state->m_scroll[i * 2 + 1] - wx - (wx & 7));
		state->m_bg_tilemap[i]->set_scrolly(0, state->m_scroll[i * 2 + 0] - wy - (wy & 7));

		state->m_bg_tilemap[i]->mark_all_dirty();
	}
}


static void expand_gfx1(hyprduel_state &state)
{
	UINT8 *base_gfx = state.machine().region("gfx1")->base();
	UINT32 length = 2 * state.machine().region("gfx1")->bytes();
	state.m_expanded_gfx1 = auto_alloc_array(state.machine(), UINT8, length);
	for (int i = 0; i < length; i += 2)
	{
		UINT8 src = base_gfx[i / 2];
		state.m_expanded_gfx1[i+0] = src & 15;
		state.m_expanded_gfx1[i+1] = src >> 4;
	}
}

static VIDEO_START( common_14220 )
{
	hyprduel_state *state = machine.driver_data<hyprduel_state>();
	expand_gfx1(*state);
	alloc_empty_tiles(machine);
	state->m_tiletable_old = auto_alloc_array(machine, UINT16, state->m_tiletable_size / 2);
	state->m_dirtyindex = auto_alloc_array(machine, UINT8, state->m_tiletable_size / 4);

	state->save_pointer(NAME(state->m_tiletable_old), state->m_tiletable_size / 2);
	state->save_pointer(NAME(state->m_dirtyindex), state->m_tiletable_size / 4);

	state->m_bg_tilemap[0] = tilemap_create(machine, get_tile_info_0_8bit, tilemap_scan_rows, 8, 8, WIN_NX, WIN_NY);
	state->m_bg_tilemap[1] = tilemap_create(machine, get_tile_info_1_8bit, tilemap_scan_rows, 8, 8, WIN_NX, WIN_NY);
	state->m_bg_tilemap[2] = tilemap_create(machine, get_tile_info_2_8bit, tilemap_scan_rows, 8, 8, WIN_NX, WIN_NY);

	state->m_bg_tilemap[0]->map_pen_to_layer(0, 15,  TILEMAP_PIXEL_TRANSPARENT);
	state->m_bg_tilemap[0]->map_pen_to_layer(1, 255, TILEMAP_PIXEL_TRANSPARENT);

	state->m_bg_tilemap[1]->map_pen_to_layer(0, 15,  TILEMAP_PIXEL_TRANSPARENT);
	state->m_bg_tilemap[1]->map_pen_to_layer(1, 255, TILEMAP_PIXEL_TRANSPARENT);

	state->m_bg_tilemap[2]->map_pen_to_layer(0, 15,  TILEMAP_PIXEL_TRANSPARENT);
	state->m_bg_tilemap[2]->map_pen_to_layer(1, 255, TILEMAP_PIXEL_TRANSPARENT);

	state->m_bg_tilemap[0]->set_scrolldx(0, 0);
	state->m_bg_tilemap[1]->set_scrolldx(0, 0);
	state->m_bg_tilemap[2]->set_scrolldx(0, 0);

	/* Set up save state */
	state->save_item(NAME(state->m_sprite_xoffs));
	state->save_item(NAME(state->m_sprite_yoffs));
	machine.save().register_postload(save_prepost_delegate(FUNC(hyprduel_postload), &machine));
}

VIDEO_START( hyprduel_14220 )
{
	hyprduel_state *state = machine.driver_data<hyprduel_state>();

	state->m_sprite_yoffs_sub = 2;

	VIDEO_START_CALL(common_14220);
}

VIDEO_START( magerror_14220 )
{
	hyprduel_state *state = machine.driver_data<hyprduel_state>();

	state->m_sprite_yoffs_sub = 0;

	VIDEO_START_CALL(common_14220);
}

/***************************************************************************

                                Video Registers


        Offset:     Bits:                   Value:

        0.w                                 Number Of Sprites To Draw
        2.w         f--- ---- ---- ----     Disabled Sprites Layer Priority
                    -edc ---- ---- ----
                    ---- ba-- ---- ----     Sprites Masked Layer
                    ---- --98 ---- ----     Sprites Priority
                    ---- ---- 765- ----
                    ---- ---- ---4 3210     Sprites Masked Number
        4.w                                 Sprites Y Offset
        6.w                                 Sprites X Offset
        8.w                                 Sprites Color Codes Start

        -

        10.w        fedc ba98 76-- ----
                    ---- ---- --54 ----     Layer 2 Priority (3 backmost, 0 frontmost)
                    ---- ---- ---- 32--     Layer 1 Priority
                    ---- ---- ---- --10     Layer 0 Priority

        12.w                                Backround Color

***************************************************************************/

/***************************************************************************

                                Sprites Drawing


        Offset:     Bits:                   Value:

        0.w         fedc b--- ---- ----     Priority (0 = Max)
                    ---- -a98 7654 3210     X

        2.w         fedc ba-- ---- ----     Zoom (Both X & Y)
                    ---- --98 7654 3210     Y

        4.w         f--- ---- ---- ----     Flip X
                    -e-- ---- ---- ----     Flip Y
                    --dc b--- ---- ----     Size X *
                    ---- -a98 ---- ----     Size Y *
                    ---- ---- 7654 ----     Color
                    ---- ---- ---- 3210     Code High Bits **

        6.w                                 Code Low Bits  **

*  8 pixel increments
** 8x8 pixel increments
***************************************************************************/

/* Draw sprites */

static void draw_sprites( running_machine &machine, bitmap_ind16 &bitmap, const rectangle &cliprect )
{
	hyprduel_state *state = machine.driver_data<hyprduel_state>();
	UINT8 *base_gfx4 = state->m_expanded_gfx1;
	UINT8 *base_gfx8 = machine.region("gfx1")->base();
	UINT32 gfx_size = machine.region("gfx1")->bytes();

	int max_x = machine.primary_screen->width();
	int max_y = machine.primary_screen->height();

	int max_sprites = state->m_spriteram_size / 8;
	int sprites = state->m_videoregs[0x00 / 2] % max_sprites;

	int color_start = ((state->m_videoregs[0x08 / 2] & 0xf) << 4) + 0x100;

	int i, j, pri;
	static const int primask[4] = { 0x0000, 0xff00, 0xff00|0xf0f0, 0xff00|0xf0f0|0xcccc };

	UINT16 *src;
	int inc;

	if (sprites == 0)
		return;

	for (i = 0; i < 0x20; i++)
	{
		gfx_element gfx(machine);

		if (!(state->m_videoregs[0x02 / 2] & 0x8000))
		{
			src = state->m_spriteram + (sprites - 1) * (8 / 2);
			inc = -(8 / 2);
		} else {
			src = state->m_spriteram;
			inc = (8 / 2);
		}

		for (j = 0; j < sprites; j++)
		{
			int x, y, attr, code, color, flipx, flipy, zoom, curr_pri, width, height;

			/* Exponential zoom table extracted from daitoride */
			static const int zoomtable[0x40] =
			{
				0xAAC,0x800,0x668,0x554,0x494,0x400,0x390,0x334,
				0x2E8,0x2AC,0x278,0x248,0x224,0x200,0x1E0,0x1C8,
				0x1B0,0x198,0x188,0x174,0x164,0x154,0x148,0x13C,
				0x130,0x124,0x11C,0x110,0x108,0x100,0x0F8,0x0F0,
				0x0EC,0x0E4,0x0DC,0x0D8,0x0D4,0x0CC,0x0C8,0x0C4,
				0x0C0,0x0BC,0x0B8,0x0B4,0x0B0,0x0AC,0x0A8,0x0A4,
				0x0A0,0x09C,0x098,0x094,0x090,0x08C,0x088,0x080,
				0x078,0x070,0x068,0x060,0x058,0x050,0x048,0x040
			};

			x = src[0];
			curr_pri = (x & 0xf800) >> 11;

			if ((curr_pri == 0x1f) || (curr_pri != i))
			{
				src += inc;
				continue;
			}

			pri = (state->m_videoregs[0x02 / 2] & 0x0300) >> 8;

			if (!(state->m_videoregs[0x02 / 2] & 0x8000))
			{
				if (curr_pri > (state->m_videoregs[0x02 / 2] & 0x1f))
					pri = (state->m_videoregs[0x02 / 2] & 0x0c00) >> 10;
			}

			y = src[1];
			attr = src[2];
			code = src[3];

			flipx = attr & 0x8000;
			flipy = attr & 0x4000;
			color = (attr & 0xf0) >> 4;

			zoom = zoomtable[(y & 0xfc00) >> 10] << (16 - 8);

			x = (x & 0x07ff) - state->m_sprite_xoffs;
			y = (y & 0x03ff) - state->m_sprite_yoffs;

			width = (((attr >> 11) & 0x7) + 1) * 8;
			height = (((attr >>  8) & 0x7) + 1) * 8;

			UINT32 gfxstart = (8 * 8 * 4 / 8) * (((attr & 0x000f) << 16) + code);

			if (flip_screen_get(machine))
			{
				flipx = !flipx;		x = max_x - x - width;
				flipy = !flipy;		y = max_y - y - height;
			}

			if (color == 0xf)	/* 8bpp */
			{
				/* Bounds checking */
				if ((gfxstart + width * height - 1) >= gfx_size)
					continue;

				gfx_element_build_temporary(&gfx, machine, base_gfx8 + gfxstart, width, height, width, 0, 256, 0);

				pdrawgfxzoom_transpen(bitmap,cliprect, &gfx,
								0,
								color_start >> 4,
								flipx, flipy,
								x, y,
								zoom, zoom,
								machine.priority_bitmap,primask[pri], 255);
			}
			else
			{
				/* Bounds checking */
				if ((gfxstart + width / 2 * height - 1) >= gfx_size)
					continue;

				gfx_element_build_temporary(&gfx, machine, base_gfx4 + 2 * gfxstart, width, height, width, 0, 16, 0);

				pdrawgfxzoom_transpen(bitmap,cliprect, &gfx,
								0,
								color + color_start,
								flipx, flipy,
								x, y,
								zoom, zoom,
								machine.priority_bitmap,primask[pri], 15);
			}
#if 0
{	/* Display priority + zoom on each sprite */
	char buf[80];
	sprintf(buf, "%02X %02X",((src[ 0 ] & 0xf800) >> 11)^0x1f,((src[ 1 ] & 0xfc00) >> 10) );
	ui_draw_text(buf, x, y);
}
#endif
			src += inc;
		}
	}
}

/***************************************************************************
                                Screen Drawing
***************************************************************************/

WRITE16_MEMBER(hyprduel_state::hyprduel_scrollreg_w)
{
	UINT16 window = m_window[offset];

	COMBINE_DATA(&m_scroll[offset]);

	if (offset & 0x01)
		m_bg_tilemap[offset / 2]->set_scrollx(0, m_scroll[offset] - window - (window & 7));
	else
		m_bg_tilemap[offset / 2]->set_scrolly(0, m_scroll[offset] - window - (window & 7));
}

WRITE16_MEMBER(hyprduel_state::hyprduel_scrollreg_init_w)
{
	int i;

	for (i = 0; i < 3; i++)
	{
		UINT16 wx = m_window[i * 2 + 1];
		UINT16 wy = m_window[i * 2 + 0];

		m_scroll[i * 2 + 1] = data;
		m_scroll[i * 2 + 0] = data;

		m_bg_tilemap[i]->set_scrollx(0, data - wx - (wx & 7));
		m_bg_tilemap[i]->set_scrolly(0, data - wy - (wy & 7));
	}
}


static void draw_layers( running_machine &machine, bitmap_ind16 &bitmap, const rectangle &cliprect, int pri, int layers_ctrl )
{
	hyprduel_state *state = machine.driver_data<hyprduel_state>();
	UINT16 layers_pri = state->m_videoregs[0x10/2];
	int layer;

	/* Draw all the layers with priority == pri */
	for (layer = 2; layer >= 0; layer--)	// tilemap[2] below?
	{
		if ( pri == ((layers_pri >> (layer*2)) & 3) )
		{
			if (layers_ctrl & (1 << layer))	// for debug
				state->m_bg_tilemap[layer]->draw(bitmap, cliprect, 0, 1 << (3 - pri));
		}
	}
}

/* Dirty tilemaps when the tiles set changes */
static void dirty_tiles( running_machine &machine, int layer, UINT16 *vram )
{
	hyprduel_state *state = machine.driver_data<hyprduel_state>();
	int col, row;

	for (row = 0; row < WIN_NY; row++)
	{
		for (col = 0; col < WIN_NX; col++)
		{
			int offset = (col + state->m_window[layer * 2 + 1] / 8) % BIG_NX + ((row + state->m_window[layer * 2 + 0] / 8) % BIG_NY) * BIG_NX;
			UINT16 code = vram[offset];

			if (!(code & 0x8000) && state->m_dirtyindex[(code & 0x1ff0) >> 4])
				state->m_bg_tilemap[layer]->mark_tile_dirty(row * WIN_NX + col);
		}
	}
}


SCREEN_UPDATE_IND16( hyprduel )
{
	hyprduel_state *state = screen.machine().driver_data<hyprduel_state>();
	int i, pri, layers_ctrl = -1;
	UINT16 screenctrl = *state->m_screenctrl;

	{
		int dirty = 0;

		memset(state->m_dirtyindex, 0, state->m_tiletable_size / 4);
		for (i = 0; i < state->m_tiletable_size / 4; i++)
		{
			UINT32 tile_new = (state->m_tiletable[2 * i + 0] << 16 ) + state->m_tiletable[2 * i + 1];
			UINT32 tile_old = (state->m_tiletable_old[2 * i + 0] << 16 ) + state->m_tiletable_old[2 * i + 1];

			if ((tile_new ^ tile_old) & 0x0fffffff)
			{
				state->m_dirtyindex[i] = 1;
				dirty = 1;
			}
		}
		memcpy(state->m_tiletable_old, state->m_tiletable, state->m_tiletable_size);

		if (dirty)
		{
			dirty_tiles(screen.machine(), 0, state->m_vram_0);
			dirty_tiles(screen.machine(), 1, state->m_vram_1);
			dirty_tiles(screen.machine(), 2, state->m_vram_2);
		}
	}

	state->m_sprite_xoffs = state->m_videoregs[0x06 / 2] - screen.width()  / 2;
	state->m_sprite_yoffs = state->m_videoregs[0x04 / 2] - screen.height() / 2 - state->m_sprite_yoffs_sub;

	/* The background color is selected by a register */
	screen.machine().priority_bitmap.fill(0, cliprect);
	bitmap.fill((state->m_videoregs[0x12 / 2] & 0x0fff) + 0x1000, cliprect);

	/*  Screen Control Register:

        f--- ---- ---- ----     ?
        -edc b--- ---- ----
        ---- -a98 ---- ----     ? Leds
        ---- ---- 7--- ----     16x16 Tiles (Layer 2)
        ---- ---- -6-- ----     16x16 Tiles (Layer 1)
        ---- ---- --5- ----     16x16 Tiles (Layer 0)
        ---- ---- ---4 32--
        ---- ---- ---- --1-     ? Blank Screen
        ---- ---- ---- ---0     Flip  Screen    */
	if (screenctrl & 2)
		return 0;
	flip_screen_set(screen.machine(), screenctrl & 1);

#if 0
if (screen.machine().input().code_pressed(KEYCODE_Z))
{	int msk = 0;
	if (screen.machine().input().code_pressed(KEYCODE_Q))	msk |= 0x01;
	if (screen.machine().input().code_pressed(KEYCODE_W))	msk |= 0x02;
	if (screen.machine().input().code_pressed(KEYCODE_E))	msk |= 0x04;
	if (screen.machine().input().code_pressed(KEYCODE_A))	msk |= 0x08;
	if (msk != 0)
	{
		bitmap.fill(0, cliprect);
		layers_ctrl &= msk;
	}

	popmessage("%x-%x-%x:%04x %04x %04x",
				state->m_videoregs[0x10/2]&3,(state->m_videoregs[0x10/2] & 0xc) >> 2, (state->m_videoregs[0x10/2] & 0x30) >> 4,
				state->m_videoregs[0x02/2], state->m_videoregs[0x08/2],
				*state->m_screenctrl);
}
#endif

	for (pri = 3; pri >= 0; pri--)
		draw_layers(screen.machine(), bitmap, cliprect, pri, layers_ctrl);

	if (layers_ctrl & 0x08)
		draw_sprites(screen.machine(), bitmap, cliprect);

	return 0;
}
