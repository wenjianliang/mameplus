/***************************************************************************

   Alpha 68k video emulation - Bryan McPhail, mish@tendril.co.uk

****************************************************************************/

#include "driver.h"

static tilemap *fix_tilemap;
static int bank_base,flipscreen;

extern void (*alpha68k_video_banking)(int *bank, int data);

extern int alpha68k_microcontroller_id;

/******************************************************************************/

void alpha68k_flipscreen_w(int flip)
{
	flipscreen = flip;
}

void alpha68k_V_video_bank_w(int bank)
{
	bank_base = bank&0xf;
}

WRITE16_HANDLER( alpha68k_paletteram_w )
{
	int newword;
	int r,g,b;

	COMBINE_DATA(paletteram16 + offset);
	newword = paletteram16[offset];

	r = ((newword >> 7) & 0x1e) | ((newword >> 14) & 0x01);
	g = ((newword >> 3) & 0x1e) | ((newword >> 13) & 0x01);
	b = ((newword << 1) & 0x1e) | ((newword >> 12) & 0x01);

	palette_set_color_rgb(machine,offset,pal5bit(r),pal5bit(g),pal5bit(b));
}

/******************************************************************************/

static TILE_GET_INFO( get_tile_info )
{
	int tile  = videoram16[2*tile_index]   &0xff;
	int color = videoram16[2*tile_index+1] &0x0f;

	tile=tile | (bank_base<<8);

	SET_TILE_INFO(0, tile, color, 0);
}

WRITE16_HANDLER( alpha68k_videoram_w )
{
	/* Doh. */
	if(ACCESSING_BITS_0_7)
		if(ACCESSING_BITS_8_15)
			videoram16[offset] = data;
		else
			videoram16[offset] = data & 0xff;
	else
		videoram16[offset] = (data >> 8) & 0xff;

	tilemap_mark_tile_dirty(fix_tilemap,offset/2);
}

VIDEO_START( alpha68k )
{
	fix_tilemap = tilemap_create(get_tile_info,tilemap_scan_cols,8,8,32,32);

	tilemap_set_transparent_pen(fix_tilemap,0);
}

/******************************************************************************/
//AT
static void draw_sprites(running_machine *machine, bitmap_t *bitmap, const rectangle *cliprect, int j, int s, int e)
{
	int offs,mx,my,color,tile,fx,fy,i;

	for (offs = s; offs < e; offs += 0x40 )
	{
		my = spriteram16[offs+3+(j<<1)];
		mx = spriteram16[offs+2+(j<<1)]<<1 | my>>15;
		my = -my & 0x1ff;
		mx = ((mx + 0x100) & 0x1ff) - 0x100;
		if (j==0 && s==0x7c0) my++;
//ZT
		if (flipscreen) {
			mx=240-mx;
			my=240-my;
		}

		for (i=0; i<0x40; i+=2) {
			tile  = spriteram16[offs+1+i+(0x800*j)+0x800];
			color = spriteram16[offs  +i+(0x800*j)+0x800]&0x7f;

			fy=tile&0x8000;
			fx=tile&0x4000;
			tile&=0x3fff;

			if (flipscreen) {
				if (fx) fx=0; else fx=1;
				if (fy) fy=0; else fy=1;
			}

			if (color)
				drawgfx(bitmap,machine->gfx[1],
					tile,
					color,
					fx,fy,
					mx,my,
					cliprect,TRANSPARENCY_PEN,0);

			if (flipscreen)
				my=(my-16)&0x1ff;
			else
				my=(my+16)&0x1ff;
		}
	}
}

/******************************************************************************/

VIDEO_UPDATE( alpha68k_II )
{
	static int last_bank=0;

	if (last_bank!=bank_base)
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	last_bank=bank_base;
	tilemap_set_flip(ALL_TILEMAPS,flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);

	fillbitmap(bitmap,2047,cliprect);
//AT
	draw_sprites(screen->machine, bitmap,cliprect,0,0x07c0,0x0800);
	draw_sprites(screen->machine, bitmap,cliprect,1,0x0000,0x0800);
	draw_sprites(screen->machine, bitmap,cliprect,2,0x0000,0x0800);
	draw_sprites(screen->machine, bitmap,cliprect,0,0x0000,0x07c0);
//ZT
	tilemap_draw(bitmap,cliprect,fix_tilemap,0,0);
	return 0;
}

/******************************************************************************/

/*
    Video banking:

    Write to these locations in this order for correct bank:

    20 28 30 for Bank 0
    60 28 30 for Bank 1
    20 68 30 etc
    60 68 30
    20 28 70
    60 28 70
    20 68 70
    60 68 70 for Bank 7

    Actual data values written don't matter!

*/

WRITE16_HANDLER( alpha68k_II_video_bank_w )
{
	static int buffer_28,buffer_60,buffer_68;

	switch (offset) {
		case 0x10: /* Reset */
			bank_base=buffer_28=buffer_60=buffer_68=0;
			return;
		case 0x14:
			if (buffer_60) bank_base=1; else bank_base=0;
			buffer_28=1;
			return;
		case 0x18:
			if (buffer_68) {if (buffer_60) bank_base=3; else bank_base=2; }
			if (buffer_28) {if (buffer_60) bank_base=1; else bank_base=0; }
			return;
		case 0x30:
			buffer_28=buffer_68=0; bank_base=1;
			buffer_60=1;
			return;
		case 0x34:
			if (buffer_60) bank_base=3; else bank_base=2;
			buffer_68=1;
			return;
		case 0x38:
			if (buffer_68) {if (buffer_60) bank_base=7; else bank_base=6; }
			if (buffer_28) {if (buffer_60) bank_base=5; else bank_base=4; }
			return;
		case 0x08: /* Graphics flags?  Not related to fix chars anyway */
		case 0x0c:
		case 0x28:
		case 0x2c:
			return;
	}

	logerror("%04x \n",offset);
}

/******************************************************************************/

WRITE16_HANDLER( alpha68k_V_video_control_w )
{
	switch (offset) {
		case 0x08: /* Graphics flags?  Not related to fix chars anyway */
		case 0x0c:
		case 0x28:
		case 0x2c:
			return;
	}
}

static void draw_sprites_V(running_machine *machine, bitmap_t *bitmap, const rectangle *cliprect, int j, int s, int e, int fx_mask, int fy_mask, int sprite_mask)
{
	int offs,mx,my,color,tile,fx,fy,i;

	for (offs = s; offs < e; offs += 0x40 )
	{
//AT
		my = spriteram16[offs+3+(j<<1)];
		mx = spriteram16[offs+2+(j<<1)]<<1 | my>>15;
		my = -my & 0x1ff;
		mx = ((mx + 0x100) & 0x1ff) - 0x100;
		if (j==0 && s==0x7c0) my++;
//ZT
		if (flipscreen) {
			mx=240-mx;
			my=240-my;
		}

		for (i=0; i<0x40; i+=2) {
			tile  = spriteram16[offs+1+i+(0x800*j)+0x800];
			color = spriteram16[offs  +i+(0x800*j)+0x800]&0xff;

			fx=tile&fx_mask;
			fy=tile&fy_mask;
			tile=tile&sprite_mask;
			if (tile>0x4fff) continue;

			if (flipscreen) {
				if (fx) fx=0; else fx=1;
				if (fy) fy=0; else fy=1;
			}

			if (color)
				drawgfx(bitmap,machine->gfx[1],
					tile,
					color,
					fx,fy,
					mx,my,
					cliprect,TRANSPARENCY_PEN,0);

			if (flipscreen)
				my=(my-16)&0x1ff;
			else
				my=(my+16)&0x1ff;
		}
	}
}

VIDEO_UPDATE( alpha68k_V )
{
	static int last_bank=0;

	if (last_bank!=bank_base)
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	last_bank=bank_base;
	tilemap_set_flip(ALL_TILEMAPS,flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);

	fillbitmap(bitmap,4095,cliprect);

	/* This appears to be correct priority */
	if (alpha68k_microcontroller_id == 0x8814) /* Sky Adventure */
	{
		draw_sprites_V(screen->machine, bitmap,cliprect,0,0x07c0,0x0800,0,0x8000,0x7fff);
		draw_sprites_V(screen->machine, bitmap,cliprect,1,0x0000,0x0800,0,0x8000,0x7fff);
		//AT: *KLUDGE* fixes priest priority in level 1(could be a game bug)
		if (spriteram16[0x1bde]==0x24 && (spriteram16[0x1bdf]>>8)==0x3b) {
			draw_sprites_V(screen->machine, bitmap,cliprect,2,0x03c0,0x0800,0,0x8000,0x7fff);
			draw_sprites_V(screen->machine, bitmap,cliprect,2,0x0000,0x03c0,0,0x8000,0x7fff);
		} else
		draw_sprites_V(screen->machine, bitmap,cliprect,2,0x0000,0x0800,0,0x8000,0x7fff);
		draw_sprites_V(screen->machine, bitmap,cliprect,0,0x0000,0x07c0,0,0x8000,0x7fff);
	}
	else	/* gangwars */
	{
		draw_sprites_V(screen->machine, bitmap,cliprect,0,0x07c0,0x0800,0x8000,0,0x7fff);
		draw_sprites_V(screen->machine, bitmap,cliprect,1,0x0000,0x0800,0x8000,0,0x7fff);
		draw_sprites_V(screen->machine, bitmap,cliprect,2,0x0000,0x0800,0x8000,0,0x7fff);
		draw_sprites_V(screen->machine, bitmap,cliprect,0,0x0000,0x07c0,0x8000,0,0x7fff);
	}

	tilemap_draw(bitmap,cliprect,fix_tilemap,0,0);
	return 0;
}

VIDEO_UPDATE( alpha68k_V_sb )
{
	static int last_bank=0;

	if (last_bank!=bank_base)
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	last_bank=bank_base;
	tilemap_set_flip(ALL_TILEMAPS,flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);

	fillbitmap(bitmap,4095,cliprect);

	/* This appears to be correct priority */
	draw_sprites_V(screen->machine,bitmap,cliprect,0,0x07c0,0x0800,0x4000,0x8000,0x3fff);
	draw_sprites_V(screen->machine,bitmap,cliprect,1,0x0000,0x0800,0x4000,0x8000,0x3fff);
	draw_sprites_V(screen->machine,bitmap,cliprect,2,0x0000,0x0800,0x4000,0x8000,0x3fff);
	draw_sprites_V(screen->machine,bitmap,cliprect,0,0x0000,0x07c0,0x4000,0x8000,0x3fff);

	tilemap_draw(bitmap,cliprect,fix_tilemap,0,0);
	return 0;
}

/******************************************************************************/
//AT
static void draw_sprites_I(running_machine *machine, bitmap_t *bitmap, const rectangle *cliprect, int c, int d, int yshift)
{
	int data, offs, mx, my, tile, color, fy, i;
	UINT8 *color_prom = memory_region(machine, REGION_USER1);
	gfx_element *gfx = machine->gfx[0];

	for (offs=0; offs<0x400; offs+=0x20)
	{
		mx = spriteram16[offs+c];
		my = (yshift-(mx>>8)) & 0xff;
		mx &= 0xff;

		for (i=0; i<0x20; i++)
		{
			data = spriteram16[offs+d+i];
			tile = data & 0x3fff;
			fy = data & 0x4000;
			color = color_prom[tile<<1|data>>15];

			drawgfx(bitmap, gfx, tile, color, 0, fy, mx, my,
					cliprect, TRANSPARENCY_PEN, 0);

			my = (my + 8) & 0xff;
		}
	}
}

VIDEO_UPDATE( alpha68k_I )
{
	int yshift = (alpha68k_microcontroller_id == 0x890a) ? 1 : 0; // The Next Space is 1 pixel off

	fillbitmap(bitmap,get_black_pen(screen->machine),cliprect);

	/* This appears to be correct priority */
	draw_sprites_I(screen->machine, bitmap,cliprect,2,0x0800,yshift);
	draw_sprites_I(screen->machine, bitmap,cliprect,3,0x0c00,yshift);
	draw_sprites_I(screen->machine, bitmap,cliprect,1,0x0400,yshift);
	return 0;
}
//ZT
/******************************************************************************/

PALETTE_INIT( kyros )
{
	int i;

	/* allocate the colortable */
	machine->colortable = colortable_alloc(machine, 0x100);

	/* create a lookup table for the palette */
	for (i = 0; i < 0x100; i++)
	{
		int r = pal4bit(color_prom[i + 0x000]);
		int g = pal4bit(color_prom[i + 0x100]);
		int b = pal4bit(color_prom[i + 0x200]);

		colortable_palette_set_color(machine->colortable, i, MAKE_RGB(r, g, b));
	}

	/* color_prom now points to the beginning of the lookup table */
	color_prom += 0x300;

	for (i = 0; i < 0x100; i++)
	{
		UINT8 ctabentry = ((color_prom[i] & 0x0f) << 4) | (color_prom[i + 0x100] & 0x0f);
		colortable_entry_set_value(machine->colortable, i, ctabentry);
	}
}

PALETTE_INIT( paddlem )
{
	int i;

	/* allocate the colortable */
	machine->colortable = colortable_alloc(machine, 0x100);

	/* create a lookup table for the palette */
	for (i = 0; i < 0x100; i++)
	{
		int r = pal4bit(color_prom[i + 0x000]);
		int g = pal4bit(color_prom[i + 0x100]);
		int b = pal4bit(color_prom[i + 0x200]);

		colortable_palette_set_color(machine->colortable, i, MAKE_RGB(r, g, b));
	}

	/* color_prom now points to the beginning of the lookup table */
	color_prom += 0x300;

	for (i = 0; i < 0x400; i++)
	{
		UINT8 ctabentry = ((color_prom[i + 0x400] & 0x0f) << 4) | (color_prom[i] & 0x0f);
		colortable_entry_set_value(machine->colortable, i, ctabentry);
	}
}

void kyros_video_banking(int *bank, int data)
{
	*bank = (data>>13 & 4) | (data>>10 & 3);
}

void jongbou_video_banking(int *bank, int data)
{
	*bank = (data>>11 & 4) | (data>>10 & 3);
}

static void kyros_draw_sprites(running_machine *machine, bitmap_t *bitmap, const rectangle *cliprect, int c,int d)
{
	int offs,mx,my,color,tile,i,bank,fy,fx;
	int data;
	UINT8 *color_prom = memory_region(machine, REGION_USER1);

//AT
	for (offs=0; offs<0x400; offs+=0x20)
	{
		mx = spriteram16[offs+c];
		my = -(mx>>8) & 0xff;
		mx &= 0xff;

		if (flipscreen) {
			my=249-my;
		}

		for (i=0; i<0x20; i++)
		{
			data = spriteram16[offs+d+i];
			if (data!=0x20)
			{
				color = color_prom[(data>>1&0x1000)|(data&0xffc)|(data>>14&3)];
				if (color!=0xff)
				{
					fy = data & 0x1000;
					fx = 0;

					if(flipscreen)
					{
						if (fy) fy=0; else fy=1;
						fx = 1;
					}

					tile = (data>>3 & 0x400) | (data & 0x3ff);
					alpha68k_video_banking(&bank, data);
					drawgfx(bitmap, machine->gfx[bank], tile, color, fx, fy, mx, my,
							cliprect, TRANSPARENCY_PEN, 0);
				}
			}
//ZT
			if(flipscreen)
				my=(my-8)&0xff;
			else
				my=(my+8)&0xff;
		}
	}
}

VIDEO_UPDATE( kyros )
{
	colortable_entry_set_value(screen->machine->colortable, 0x100, *videoram16 & 0xff);
	fillbitmap(bitmap, 0x100, cliprect); //AT

	kyros_draw_sprites(screen->machine, bitmap,cliprect,2,0x0800);
	kyros_draw_sprites(screen->machine, bitmap,cliprect,3,0x0c00);
	kyros_draw_sprites(screen->machine, bitmap,cliprect,1,0x0400);
	return 0;
}

/******************************************************************************/

static void sstingry_draw_sprites(running_machine *machine, bitmap_t *bitmap, const rectangle *cliprect, int c,int d)
{
//AT
	int data,offs,mx,my,color,tile,i,bank,fy,fx;

	for (offs=0; offs<0x400; offs+=0x20)
	{
		mx = spriteram16[offs+c];
		my = -(mx>>8) & 0xff;
		mx &= 0xff;
		if (mx > 0xf8) mx -= 0x100;

		if (flipscreen) {
			my=249-my;
		}

		for (i=0; i<0x20; i++)
		{
			data = spriteram16[offs+d+i];
			if (data!=0x40)
			{
				fy = data & 0x1000;
				fx = 0;

				if(flipscreen)
				{
					if (fy) fy=0; else fy=1;
					fx = 1;
				}

				color = (data>>7 & 0x18) | (data>>13 & 7);
				tile = data & 0x3ff;
				bank = data>>10 & 3;
				drawgfx(bitmap, machine->gfx[bank], tile, color, fx, fy, mx, my,
						cliprect, TRANSPARENCY_PEN, 0);
			}
//ZT
			if(flipscreen)
				my=(my-8)&0xff;
			else
				my=(my+8)&0xff;
		}
	}
}

VIDEO_UPDATE( sstingry )
{
	colortable_entry_set_value(screen->machine->colortable, 0x100, *videoram16 & 0xff);
	fillbitmap(bitmap, 0x100, cliprect); //AT

	sstingry_draw_sprites(screen->machine, bitmap,cliprect,2,0x0800);
	sstingry_draw_sprites(screen->machine, bitmap,cliprect,3,0x0c00);
	sstingry_draw_sprites(screen->machine, bitmap,cliprect,1,0x0400);
	return 0;
}
