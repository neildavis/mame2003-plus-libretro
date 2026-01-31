/*
**	2xMC68000 + Z80
**	YM2151 + Custom PCM
**
**	Out Run
**	Super Hang-On
**	Super Hang-On Limited Edition
**	Turbo Out Run
**
**	Additional infos :
**
**	It has been said that Super Hang On runs on Out Run hardware, this is not exactly the case !
**	Super Hang On use sega video board 171-5480 with the following specifications :
**
**	Sega custom IC on 171-5480
**
**	315-5196, 315-5197 and 315-5242.
**
**	Pals 	: 315-5213
**		: 315-5251
**
*/

#include "driver.h"
#include "output.h"
#include "output-def.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "cpu/i8039/i8039.h"
#include "system16.h"
#include "ost_samples.h"
#include "vidhrdw/segaic16.h"

#include "realdashclient.h"

static void set_fg_page( int data ){
	sys16_fg_page[0] = data>>12;
	sys16_fg_page[1] = (data>>8)&0xf;
	sys16_fg_page[2] = (data>>4)&0xf;
	sys16_fg_page[3] = data&0xf;
}

static void set_bg_page( int data ){
	sys16_bg_page[0] = data>>12;
	sys16_bg_page[1] = (data>>8)&0xf;
	sys16_bg_page[2] = (data>>4)&0xf;
	sys16_bg_page[3] = data&0xf;
}

static void set_fg_page1( int data ){
	sys16_fg_page[1] = data>>12;
	sys16_fg_page[0] = (data>>8)&0xf;
	sys16_fg_page[3] = (data>>4)&0xf;
	sys16_fg_page[2] = data&0xf;
}

static void set_bg_page1( int data ){
	sys16_bg_page[1] = data>>12;
	sys16_bg_page[0] = (data>>8)&0xf;
	sys16_bg_page[3] = (data>>4)&0xf;
	sys16_bg_page[2] = data&0xf;
}

#if 0
static void set_fg2_page( int data ){
	sys16_fg2_page[0] = data>>12;
	sys16_fg2_page[1] = (data>>8)&0xf;
	sys16_fg2_page[2] = (data>>4)&0xf;
	sys16_fg2_page[3] = data&0xf;
}

static void set_bg2_page( int data ){
	sys16_bg2_page[0] = data>>12;
	sys16_bg2_page[1] = (data>>8)&0xf;
	sys16_bg2_page[2] = (data>>4)&0xf;
	sys16_bg2_page[3] = data&0xf;
}
#endif

/* hang-on's accel/brake are really both analog controls, but I've added them
as digital as well to see what works better */
#define HANGON_DIGITAL_CONTROLS

/* Accel */
static READ16_HANDLER( ho_io_x_r ){ return input_port_0_r( offset ); }

/* Brake */
static bool sho_brake = true; /* to force intiial write to false */
static const data16_t sho_brake_threshold = 0x0010;	/* tune this to adjust analog brake sensitivity */
static READ16_HANDLER( ho_io_y_r ){
	data16_t ret = 0x0000;
	bool brake = false;
#ifdef HANGON_DIGITAL_CONTROLS
	int data = input_port_1_r( offset );

	switch(data & 3)
	{
		case 3:	ret = 0xffff;	break; /* both */
		case 2:	ret = 0x00ff;	break; /* brake */
		case 1:	ret = 0xff00;	break; /* accel */
		default:				break; /* neither */
	}
#else /* Hang-On Analog Controls*/
	ret = (input_port_1_r( offset ) << 8) | input_port_5_r( offset );
#endif /* HANGON_DIGITAL_CONTROLS */

	brake = ((ret & 0x00ff) > sho_brake_threshold);
	if (brake != sho_brake) {
		/* printf("shangon: Brake: %s\n", brake ? "ON" : "OFF"); */
		output_set_value(SHO_BRAKE_LIGHT_NAME, brake ? 1 : 0);
		sho_brake = brake;
	}
	return ret;
}

/*	outrun: generate_gr_screen(0x200,0x800,0,0,3,0x8000); */
static void generate_gr_screen(
	int w,int bitmap_width,int skip,
	int start_color,int end_color, int source_size )
{
	UINT8 *buf = malloc( source_size );
	UINT8 *buf_base = buf;
	if( buf ){
		UINT8 *gr = memory_region(REGION_GFX3);
		UINT8 *grr = NULL;
	    int i,j,k;
	    int center_offset=0;
		sys16_gr_bitmap_width = bitmap_width;

		memcpy(buf,gr,source_size);
		memset(gr,0,256*bitmap_width);

		if (w!=sys16_gr_bitmap_width){
			if (skip>0) /* needs mirrored RHS */
				grr=gr;
			else {
				center_offset= bitmap_width-w;
				gr+=center_offset/2;
			}
		}

		for (i=0; i<256; i++){ /* build gr_bitmap */
			UINT8 last_bit;
			UINT8 color_data[4];
			color_data[0]=start_color;
			color_data[1]=start_color+1;
			color_data[2]=start_color+2;
			color_data[3]=start_color+3;

			last_bit = ((buf[0]&0x80)==0)|(((buf[0x4000]&0x80)==0)<<1);
			for (j=0; j<w/8; j++){
				for (k=0; k<8; k++){
					UINT8 bit=((buf[0]&0x80)==0)|(((buf[0x4000]&0x80)==0)<<1);
					if (bit!=last_bit && bit==0 && i>1){ /* color flipped to 0,advance color[0] */
						if (color_data[0]+end_color <= end_color){
							color_data[0]+=end_color;
						}
						else{
							color_data[0]-=end_color;
						}
					}
					*gr++ = color_data[bit];
					last_bit=bit;
					buf[0] <<= 1; buf[0x4000] <<= 1;
				}
				buf++;
			}

			if (grr!=NULL){ /* need mirrored RHS */
				const UINT8 *temp = gr-1-skip;
				for (j=0; j<w-skip; j++){
					*gr++ = *temp--;
				}
				for (j=0; j<skip; j++) *gr++ = 0;
			}
			else {
				gr+=center_offset;
			}
		}

		i=1;
		while ( (1<<i) < sys16_gr_bitmap_width ) i++;
		sys16_gr_bitmap_width=i; /* power of 2 */
		free(buf_base);
	}
}

static data16_t coinctrl;

static WRITE16_HANDLER( sys16_3d_coinctrl_w )
{
	if( ACCESSING_LSB ){
		coinctrl = data&0xff;
		sys16_refreshenable = coinctrl & 0x10;
		coin_counter_w(0,coinctrl & 0x01);
		/* bit 6 is also used (0 in fantzone) */

		/* Hang-On, Super Hang-On, Space Harrier, Enduro Racer */
		set_led_status(0,coinctrl & 0x04);

		/* Space Harrier */
		set_led_status(1,coinctrl & 0x08);
	}
}

#if 0
static WRITE16_HANDLER( sound_command_nmi_w ){
	if( ACCESSING_LSB ){
		soundlatch_w( 0,data&0xff );
		cpu_set_nmi_line(1, PULSE_LINE);
	}
}
#endif

static READ16_HANDLER( sys16_coinctrl_r ){
	return coinctrl;
}

#if 0
static WRITE16_HANDLER( sys16_coinctrl_w )
{
	if( ACCESSING_LSB ){
		coinctrl = data&0xff;
		sys16_refreshenable = coinctrl & 0x20;
		coin_counter_w(0,coinctrl & 0x01);
		set_led_status(0,coinctrl & 0x04);
		set_led_status(1,coinctrl & 0x08);
		/* bit 6 is also used (1 most of the time; 0 in dduxbl, sdi, wb3;
		   tturf has it normally 1 but 0 after coin insertion) */
		/* eswat sets bit 4 */
	}
}
#endif

static INTERRUPT_GEN( sys16_interrupt ){
	if(sys16_custom_irq) sys16_custom_irq();
	cpu_set_irq_line(cpu_getactivecpu(), 4, HOLD_LINE); /* Interrupt vector 4, used by VBlank */
}


static PORT_READ_START( sound_readport )
    { 0x01, 0x01, YM2151_status_port_0_r },
	{ 0xc0, 0xc0, soundlatch_r },
PORT_END

static PORT_WRITE_START( sound_writeport )
    { 0x00, 0x00, YM2151_register_port_0_w },
	{ 0x01, 0x01, YM2151_data_port_0_w },
PORT_END

static data16_t *shared_ram;
static READ16_HANDLER( shared_ram_r ){
	return shared_ram[offset];
}

static WRITE16_HANDLER( outrun_shared_ram_w ){
	COMBINE_DATA( &shared_ram[offset] );
	if (!RealDashCanClientIsInitialized())
		return;
	/* Speed KpH is at 0x260068, 0x68 BYTES offset == 0x34 MACHINE WORDS offset */
	if (0x34 == offset) {
		UINT32 speedMPH = data * 6214 / 10000;
		RealDashCanClientUpdateSpeed((UINT16)(speedMPH & 0xffff));
	} 
	/* Revs is at 0x260783 which is LSB of machine word at 0x260782 on M68000 (big endian 16-bit)
	   0x782 BYTES offset == 0x3c1 MACHINE WORDS offset
	*/
	else if (0x3c1 == offset && ACCESSING_LSB) {
		/* Revs is in range 0-0xff (255) so we map to 0-8000 decimal */
		UINT32 revsMapped = (data & 0xff) * 8000 / 0xff;
		RealDashCanClientUpdateRevs(revsMapped);
	}
	/* Gear is lsb of byte at 0x260773 which is LSB of machine word at 0x260772 on M68000 (big endian 16-bit)
	   0x772 BYTES offset == 0x3b9 MACHINE WORDS offset
	*/
	else if (0x3b9 == offset && ACCESSING_LSB16) {
		/*
			RealDash treats gears as numbers, but in our custom dashboards we map:
				0 = Neutral
				1 = Low
				2 = High 
		*/
		RealDashCanClientUpdateGear((data & 0x1) + 1);
	}
}

static WRITE16_HANDLER( shared_ram_w ){
	COMBINE_DATA( &shared_ram[offset] );
}

static unsigned char *sound_shared_ram;
static READ16_HANDLER( sound_shared_ram_r )
{
	return (sound_shared_ram[offset*2] << 8) +
			sound_shared_ram[offset*2+1];
}

static WRITE16_HANDLER( sound_shared_ram_w )
{
	if( ACCESSING_LSB ){
		sound_shared_ram[offset*2+1] = data&0xff;
	}
	if( ACCESSING_MSB ){
		sound_shared_ram[offset*2] = data>>8;
	}
}

static READ_HANDLER( sound2_shared_ram_r ){
	return sound_shared_ram[offset];
}
static WRITE_HANDLER( sound2_shared_ram_w ){
	sound_shared_ram[offset] = data;
}


ROM_START( shangon )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* 68000 code - protected */
	ROM_LOAD16_BYTE( "ic133", 0x000000, 0x10000, CRC(e52721fe) SHA1(21f0aa14d0cbda3d762bca86efe089646031aef5) )
	ROM_LOAD16_BYTE( "ic118", 0x000001, 0x10000, CRC(5fee09f6) SHA1(b97a08ba75d8c617aceff2b03c1f2bfcb16181ef) )
	ROM_LOAD16_BYTE( "ic132", 0x020000, 0x10000, CRC(5d55d65f) SHA1(d02d76b98d74746b078b0f49f0320b8be48e4c47) )
	ROM_LOAD16_BYTE( "ic117", 0x020001, 0x10000, CRC(b967e8c3) SHA1(00224b337b162daff03bbfabdcf8541025220d46) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "ep10652.54",        0x00000, 0x08000, CRC(260286f9) SHA1(dc7c8d2c6ef924a937328685eed19bda1c8b1819) )
	ROM_LOAD( "ep10651.55",        0x08000, 0x08000, CRC(c609ee7b) SHA1(c6dacf81cbfe7e5df1f9a967cf571be1dcf1c429) )
	ROM_LOAD( "ep10650.56",        0x10000, 0x08000, CRC(b236a403) SHA1(af02b8122794c083a66f2ab35d2c73b84b2df0be) )

	ROM_REGION( 0x0120000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "ep10675.8",	0x000001, 0x010000, CRC(d6ac012b) SHA1(305023b1a0a9d84cfc081ffc2ad7578b53d562f2) )
	ROM_RELOAD(     			0x100001, 0x010000 )
	ROM_LOAD16_BYTE( "ep10682.16",  0x000000, 0x010000, CRC(d9d83250) SHA1(f8ca3197edcdf53643a5b335c3c044ddc1310cd4) )
	ROM_RELOAD(              	0x100000, 0x010000 )
	ROM_LOAD16_BYTE( "ep10676.7",   0x020001, 0x010000, CRC(25ebf2c5) SHA1(abcf673ae4e280417dd9f46d18c0ec7c0e4802ae) )
	ROM_RELOAD(              	0x0e0001, 0x010000 )
	ROM_LOAD16_BYTE( "ep10683.15",  0x020000, 0x010000, CRC(6365d2e9) SHA1(688e2ba194e859f86cd3486c2575ebae257e975a) )
	ROM_RELOAD(              	0x0e0000, 0x010000 )
	ROM_LOAD16_BYTE( "ep10677.6",   0x040001, 0x010000, CRC(8a57b8d6) SHA1(df1a31559dd2d1e7c2c9d800bf97526bdf3e84e6) )
	ROM_LOAD16_BYTE( "ep10684.14",  0x040000, 0x010000, CRC(3aff8910) SHA1(4b41a49a7f02363424e814b37edce9a7a44a112e) )
	ROM_LOAD16_BYTE( "ep10678.5",   0x060001, 0x010000, CRC(af473098) SHA1(a2afaba1cbf672949dc50e407b46d7e9ae183774) )
	ROM_LOAD16_BYTE( "ep10685.13",  0x060000, 0x010000, CRC(80bafeef) SHA1(f01bcf65485e60f34e533295a896fca0b92e5b14) )
	ROM_LOAD16_BYTE( "ep10679.4",   0x080001, 0x010000, CRC(03bc4878) SHA1(548fc58bcc620204e30fa12fa4c4f0a3f6a1e4c0) )
	ROM_LOAD16_BYTE( "ep10686.12",  0x080000, 0x010000, CRC(274b734e) SHA1(906fa528659bc17c9b4744cec52f7096711adce8) )
	ROM_LOAD16_BYTE( "ep10680.3",   0x0a0001, 0x010000, CRC(9f0677ed) SHA1(5964642b70bfad418da44f2d91476f887b021f74) )
	ROM_LOAD16_BYTE( "ep10687.11",  0x0a0000, 0x010000, CRC(508a4701) SHA1(d17aea2aadc2e2cd65d81bf91feb3ef6923d5c0b) )
	ROM_LOAD16_BYTE( "ep10681.2",   0x0c0001, 0x010000, CRC(b176ea72) SHA1(7ec0eb0f13398d014c2e235773ded00351edb3e2) )
	ROM_LOAD16_BYTE( "ep10688.10",  0x0c0000, 0x010000, CRC(42fcd51d) SHA1(0eacb3527dc21746e5b901fcac83f2764a0f9e2c) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "ic88", 0x0000, 0x08000, CRC(1254efa6) SHA1(997770ccdd776de6e335a6d8b1e15d200cbd4410) )

	ROM_LOAD( "ep10643.66", 0x10000, 0x08000, CRC(06f55364) SHA1(fd685795e12541e3d0059d383fab293b3980d247) )
	ROM_LOAD( "ic67",       0x18000, 0x08000, CRC(731f5cf8) SHA1(5a62a94e15a15d5470f32112e4b2b05675983720) ) /* Possibly a bad dump.... */
	ROM_LOAD( "ep10644.67", 0x18000, 0x08000, CRC(b41d541d) SHA1(28bbfa5edaa4a5901c74074354ba6f14d8f42ff6) ) /* from other set */
	ROM_LOAD( "ep10645.68", 0x20000, 0x08000, CRC(a60dabff) SHA1(bbef0fb0d7837cc7efc866226bfa2bd7fab06459) )
	ROM_LOAD( "ep10646.69", 0x28000, 0x08000, CRC(473cc411) SHA1(04ca2d047eb59581cd5d76e0ac6eca8b19eef497) )

	ROM_REGION( 0x40000, REGION_CPU3, 0 ) /* second 68000 CPU  - protected */
	ROM_LOAD16_BYTE( "ep10640.76", 0x00000, 0x10000, CRC(02be68db) SHA1(8c9f98ee49db54ee53b721ecf53f91737ae6cd73) )
	ROM_LOAD16_BYTE( "ep10638.58", 0x00001, 0x10000, CRC(f13e8bee) SHA1(1c16c018f58f1fb49e240314a7e97a947087fad9) )
	ROM_LOAD16_BYTE( "ic75", 0x20000, 0x10000, CRC(1627c224) SHA1(76a308fc15e5ac52947fffadee38fa9b3c4b1e8a) ) /* Possibly a bad dump.... */
	ROM_LOAD16_BYTE( "ep10641.75", 0x20000, 0x10000, CRC(38c3f808) SHA1(36fae99b56980ef33853170afe10b363cd41c053) ) /* from other set */
	ROM_LOAD16_BYTE( "ep10639.57", 0x20001, 0x10000, CRC(8cdbcde8) SHA1(0bcb4df96ee16db3dd4ce52fccd939f48a4bc1a0) )

	ROM_REGION( 0x40000, REGION_GFX3, 0 ) /* Road Graphics  (region size should be gr_bitmapwidth*256, 0 )*/
	ROM_LOAD( "mp10642.47", 0x0000, 0x8000, CRC(7836bcc3) SHA1(26f308bf96224311ddf685799d7aa29aac42dd2f) )
ROM_END

ROM_START( shangonb )
	ROM_REGION( 0x030000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "s-hangon.30", 0x000000, 0x10000, CRC(d95e82fc) SHA1(bc6cd0b0ac98a9c53f2e22ac086521704ab59e4d) )
	ROM_LOAD16_BYTE( "s-hangon.32", 0x000001, 0x10000, CRC(2ee4b4fb) SHA1(ba4042ab6e533c16c3cde848248d75e484be113f) )
	ROM_LOAD16_BYTE( "s-hangon.29", 0x020000, 0x8000, CRC(12ee8716) SHA1(8e798d23d22f85cd046641184d104c17b27995b2) )
	ROM_LOAD16_BYTE( "s-hangon.31", 0x020001, 0x8000, CRC(155e0cfd) SHA1(e51734351c887fe3920c881f57abdfbb7d075f57) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "ic54",        0x00000, 0x08000, CRC(260286f9) SHA1(dc7c8d2c6ef924a937328685eed19bda1c8b1819) )
	ROM_LOAD( "ic55",        0x08000, 0x08000, CRC(c609ee7b) SHA1(c6dacf81cbfe7e5df1f9a967cf571be1dcf1c429) )
	ROM_LOAD( "ic56",        0x10000, 0x08000, CRC(b236a403) SHA1(af02b8122794c083a66f2ab35d2c73b84b2df0be) )

	ROM_REGION( 0x0120000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "ic8",	0x000001, 0x010000, CRC(d6ac012b) SHA1(305023b1a0a9d84cfc081ffc2ad7578b53d562f2) )
	ROM_RELOAD(     		0x100001, 0x010000 )
	ROM_LOAD16_BYTE( "ic16",  0x000000, 0x010000, CRC(d9d83250) SHA1(f8ca3197edcdf53643a5b335c3c044ddc1310cd4) )
	ROM_RELOAD(     		0x100000, 0x010000 )
	ROM_LOAD16_BYTE( "s-hangon.20", 0x020001, 0x010000, CRC(eef23b3d) SHA1(2416fa9991afbdddf25d469082e53858289550db) )
	ROM_RELOAD(           		      0x0e0001, 0x010000 )
	ROM_LOAD16_BYTE( "s-hangon.14", 0x020000, 0x010000, CRC(0f26d131) SHA1(0d8b6eb8b8aae0aa8f0fa0c31dc91ad0e610be3e) )
	ROM_RELOAD(              		  0x0e0000, 0x010000 )
	ROM_LOAD16_BYTE( "ic6",         0x040001, 0x010000, CRC(8a57b8d6) SHA1(df1a31559dd2d1e7c2c9d800bf97526bdf3e84e6) )
	ROM_LOAD16_BYTE( "ic14",        0x040000, 0x010000, CRC(3aff8910) SHA1(4b41a49a7f02363424e814b37edce9a7a44a112e) )
	ROM_LOAD16_BYTE( "ic5",         0x060001, 0x010000, CRC(af473098) SHA1(a2afaba1cbf672949dc50e407b46d7e9ae183774) )
	ROM_LOAD16_BYTE( "ic13",        0x060000, 0x010000, CRC(80bafeef) SHA1(f01bcf65485e60f34e533295a896fca0b92e5b14) )
	ROM_LOAD16_BYTE( "ic4",         0x080001, 0x010000, CRC(03bc4878) SHA1(548fc58bcc620204e30fa12fa4c4f0a3f6a1e4c0) )
	ROM_LOAD16_BYTE( "ic12",        0x080000, 0x010000, CRC(274b734e) SHA1(906fa528659bc17c9b4744cec52f7096711adce8) )
	ROM_LOAD16_BYTE( "ic3",         0x0a0001, 0x010000, CRC(9f0677ed) SHA1(5964642b70bfad418da44f2d91476f887b021f74) )
	ROM_LOAD16_BYTE( "ic11",        0x0a0000, 0x010000, CRC(508a4701) SHA1(d17aea2aadc2e2cd65d81bf91feb3ef6923d5c0b) )
	ROM_LOAD16_BYTE( "ic2",         0x0c0001, 0x010000, CRC(b176ea72) SHA1(7ec0eb0f13398d014c2e235773ded00351edb3e2) )
	ROM_LOAD16_BYTE( "ic10",        0x0c0000, 0x010000, CRC(42fcd51d) SHA1(0eacb3527dc21746e5b901fcac83f2764a0f9e2c) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "s-hangon.03", 0x0000, 0x08000, CRC(83347dc0) SHA1(079bb750edd6372750a207764e8c84bb6abf2f79) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "s-hangon.02", 0x00000, 0x10000, CRC(da08ca2b) SHA1(2c94c127efd66f6cf86b25e2653637818a99aed1) )
	ROM_LOAD( "s-hangon.01", 0x10000, 0x10000, CRC(8b10e601) SHA1(75e9bcdd3f096be9bed672d61064b9240690deec) )

	ROM_REGION( 0x40000, REGION_CPU3, 0 ) /* second 68000 CPU */
	ROM_LOAD16_BYTE( "s-hangon.09", 0x00000, 0x10000, CRC(070c8059) SHA1(a18c5e9473b6634f6e7165300e39029335b41ba3) )
	ROM_LOAD16_BYTE( "s-hangon.05", 0x00001, 0x10000, CRC(9916c54b) SHA1(41a7c5a9bdb1e3feae8fadf1ac5f51fab6376157) )
	ROM_LOAD16_BYTE( "s-hangon.08", 0x20000, 0x10000, CRC(000ad595) SHA1(eb80e798159c09bc5142a7ea8b9b0f895976b0d4) )
	ROM_LOAD16_BYTE( "s-hangon.04", 0x20001, 0x10000, CRC(8f8f4af0) SHA1(1dac21b7df6ec6874d36a07e30de7129b7f7f33a) )

	ROM_REGION( 0x40000, REGION_GFX3, 0 ) /* Road Graphics  (region size should be gr_bitmapwidth*256, 0 )*/
	ROM_LOAD( "s-hangon.26", 0x0000, 0x8000, CRC(1bbe4fc8) SHA1(30f7f301e4d10d3b254d12bf3d32e5371661a566) )
ROM_END

/* Outrun hardware */
ROM_START( outrun )
	ROM_REGION( 0x060000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "10380a", 0x000000, 0x10000, CRC(434fadbc) SHA1(83c861d331e69ef4f2452c313ae4b5ea9d8b7948) )
	ROM_LOAD16_BYTE( "10382a", 0x000001, 0x10000, CRC(1ddcc04e) SHA1(945d207d8d602d7fdb6d25f6b93c9c0b995e8d5a) )
	ROM_LOAD16_BYTE( "10381a", 0x020000, 0x10000, CRC(be8c412b) SHA1(bf3ff05bbf81bdd44567f3b9bb4919ed4a499624) )
	ROM_LOAD16_BYTE( "10383a", 0x020001, 0x10000, CRC(dcc586e7) SHA1(d6e1de6b562359574d94b88ce6101646c506e701) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "10268", 0x00000, 0x08000, CRC(95344b04) SHA1(b3480714b11fc49b449660431f85d4ba92f799ba) )
	ROM_LOAD( "10232", 0x08000, 0x08000, CRC(776ba1eb) SHA1(e3477961d19e694c97643066534a1f720e0c4327) )
	ROM_LOAD( "10267", 0x10000, 0x08000, CRC(a85bb823) SHA1(a7e0143dee5a47e679fd5155e58e717813912692) )
	ROM_LOAD( "10231", 0x18000, 0x08000, CRC(8908bcbf) SHA1(8e1237b640a6f26bdcbfd5e201dadb2687c4febb) )
	ROM_LOAD( "10266", 0x20000, 0x08000, CRC(9f6f1a74) SHA1(09164e858ebeedcff4d389524ddf89e7c216dcae) )
	ROM_LOAD( "10230", 0x28000, 0x08000, CRC(686f5e50) SHA1(03697b892f911177968aa40de6c5f464eb0258e7) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD( "10371", 0x00000, 0x20000, CRC(0a1c98de) SHA1(e683ab79bfc6f8c60974912f07ef8173b167f448) )
	ROM_LOAD( "10372", 0x20000, 0x20000, CRC(1640ad1f) SHA1(3223d536c61f349be8de9ee32fc210f61fe822e2) )
	ROM_LOAD( "10373", 0x40000, 0x20000, CRC(339f8e64) SHA1(0d79b584e9e6c838cb0a9d2e8a611913265a06d2) )
	ROM_LOAD( "10374", 0x60000, 0x20000, CRC(22744340) SHA1(0997687641b0fc5c81343220166777dd74cc12a8) )
	ROM_LOAD( "10375", 0x80000, 0x20000, CRC(62a472bd) SHA1(e753df3ad378e6d4e22d5978626fc2370826bcf0) )
	ROM_LOAD( "10376", 0xa0000, 0x20000, CRC(8337ace7) SHA1(6bb3739a730c8d026fe9a6e831fc7c9fc13eae4f) )
	ROM_LOAD( "10377", 0xc0000, 0x20000, CRC(c86daecb) SHA1(78f61038f54ca56aab1f73d662fbaf07e396075f) )
	ROM_LOAD( "10378", 0xe0000, 0x20000, CRC(544068fd) SHA1(54fe03d2d80548ebba9c2578ece5d0f89593cb3e) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "10187",       0x00000, 0x8000, CRC(a10abaa9) SHA1(01c8a819587a66d2ee4d255656e36fa0904377b0) )

	ROM_REGION( 0x38000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "10193",       0x00000, 0x8000, CRC(bcd10dde) SHA1(417ce1d7242884640c5b14f4db8ee57cde7d085d) )
	ROM_RELOAD(              0x30000, 0x8000 ) /* twice?? */
	ROM_LOAD( "10192",       0x08000, 0x8000, CRC(770f1270) SHA1(686bdf44d45c1d6002622f6658f037735382f3e0) )
	ROM_LOAD( "10191",       0x10000, 0x8000, CRC(20a284ab) SHA1(7c9027416d4122791ba53782fe2230cf02b7d506) )
	ROM_LOAD( "10190",       0x18000, 0x8000, CRC(7cab70e2) SHA1(a3c581d2b438630d0d4c39481dcfd85681c9f889) )
	ROM_LOAD( "10189",       0x20000, 0x8000, CRC(01366b54) SHA1(f467a6b807694d5832a985f5381c170d24aaee4e) )
	ROM_LOAD( "10188",       0x28000, 0x8000, CRC(bad30ad9) SHA1(f70dd3a6362c314adef313b064102f7a250401c8) )

	ROM_REGION( 0x40000, REGION_CPU3, 0 ) /* second 68000 CPU */
	ROM_LOAD16_BYTE( "10327a", 0x00000, 0x10000, CRC(e28a5baf) SHA1(f715bde96c73ed47035acf5a41630fdeb41bb2f9) )
	ROM_LOAD16_BYTE( "10329a", 0x00001, 0x10000, CRC(da131c81) SHA1(57d5219bd0e2fd886217e37e8773fd76be9b40eb) )
	ROM_LOAD16_BYTE( "10328a", 0x20000, 0x10000, CRC(d5ec5e5d) SHA1(a4e3cfca4d803e72bc4fcf91ab00e21bf3f8959f) )
	ROM_LOAD16_BYTE( "10330a", 0x20001, 0x10000, CRC(ba9ec82a) SHA1(2136c9572e26b7ae6de402c0cd53174407cc6018) )

	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* Road Graphics  (region size should be gr_bitmapwidth*256, 0 )*/
	ROM_LOAD( "10185", 0x0000, 0x8000, CRC(22794426) SHA1(a554d4b68e71861a0d0da4d031b3b811b246f082) )
ROM_END

ROM_START( outruna )
	ROM_REGION( 0x060000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "10380b", 0x000000, 0x10000, CRC(1f6cadad) SHA1(31e870f307f44eb4f293b607123b623beee2bc3c) )
	ROM_LOAD16_BYTE( "10382b", 0x000001, 0x10000, CRC(c4c3fa1a) SHA1(69236cf9f27691dee290c79db1fc9b5e73ea77d7) )
	ROM_LOAD16_BYTE( "10381a", 0x020000, 0x10000, CRC(be8c412b) SHA1(bf3ff05bbf81bdd44567f3b9bb4919ed4a499624) )
	ROM_LOAD16_BYTE( "10383b", 0x020001, 0x10000, CRC(10a2014a) SHA1(1970895145ad8b5735f66ed8c837d9d453ce9b23) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "10268", 0x00000, 0x08000, CRC(95344b04) SHA1(b3480714b11fc49b449660431f85d4ba92f799ba) )
	ROM_LOAD( "10232", 0x08000, 0x08000, CRC(776ba1eb) SHA1(e3477961d19e694c97643066534a1f720e0c4327) )
	ROM_LOAD( "10267", 0x10000, 0x08000, CRC(a85bb823) SHA1(a7e0143dee5a47e679fd5155e58e717813912692) )
	ROM_LOAD( "10231", 0x18000, 0x08000, CRC(8908bcbf) SHA1(8e1237b640a6f26bdcbfd5e201dadb2687c4febb) )
	ROM_LOAD( "10266", 0x20000, 0x08000, CRC(9f6f1a74) SHA1(09164e858ebeedcff4d389524ddf89e7c216dcae) )
	ROM_LOAD( "10230", 0x28000, 0x08000, CRC(686f5e50) SHA1(03697b892f911177968aa40de6c5f464eb0258e7) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD( "10371", 0x00000, 0x20000, CRC(0a1c98de) SHA1(e683ab79bfc6f8c60974912f07ef8173b167f448) )
	ROM_LOAD( "10372", 0x20000, 0x20000, CRC(1640ad1f) SHA1(3223d536c61f349be8de9ee32fc210f61fe822e2) )
	ROM_LOAD( "10373", 0x40000, 0x20000, CRC(339f8e64) SHA1(0d79b584e9e6c838cb0a9d2e8a611913265a06d2) )
	ROM_LOAD( "10374", 0x60000, 0x20000, CRC(22744340) SHA1(0997687641b0fc5c81343220166777dd74cc12a8) )
	ROM_LOAD( "10375", 0x80000, 0x20000, CRC(62a472bd) SHA1(e753df3ad378e6d4e22d5978626fc2370826bcf0) )
	ROM_LOAD( "10376", 0xa0000, 0x20000, CRC(8337ace7) SHA1(6bb3739a730c8d026fe9a6e831fc7c9fc13eae4f) )
	ROM_LOAD( "10377", 0xc0000, 0x20000, CRC(c86daecb) SHA1(78f61038f54ca56aab1f73d662fbaf07e396075f) )
	ROM_LOAD( "10378", 0xe0000, 0x20000, CRC(544068fd) SHA1(54fe03d2d80548ebba9c2578ece5d0f89593cb3e) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "10187",       0x00000, 0x8000, CRC(a10abaa9) SHA1(01c8a819587a66d2ee4d255656e36fa0904377b0) )

	ROM_REGION( 0x38000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "10193",       0x00000, 0x8000, CRC(bcd10dde) SHA1(417ce1d7242884640c5b14f4db8ee57cde7d085d) )
	ROM_RELOAD(              0x30000, 0x8000 ) /* twice?? */
	ROM_LOAD( "10192",       0x08000, 0x8000, CRC(770f1270) SHA1(686bdf44d45c1d6002622f6658f037735382f3e0) )
	ROM_LOAD( "10191",       0x10000, 0x8000, CRC(20a284ab) SHA1(7c9027416d4122791ba53782fe2230cf02b7d506) )
	ROM_LOAD( "10190",       0x18000, 0x8000, CRC(7cab70e2) SHA1(a3c581d2b438630d0d4c39481dcfd85681c9f889) )
	ROM_LOAD( "10189",       0x20000, 0x8000, CRC(01366b54) SHA1(f467a6b807694d5832a985f5381c170d24aaee4e) )
	ROM_LOAD( "10188",       0x28000, 0x8000, CRC(bad30ad9) SHA1(f70dd3a6362c314adef313b064102f7a250401c8) )

	ROM_REGION( 0x40000, REGION_CPU3, 0 ) /* second 68000 CPU */
	ROM_LOAD16_BYTE( "10327a", 0x00000, 0x10000, CRC(e28a5baf) SHA1(f715bde96c73ed47035acf5a41630fdeb41bb2f9) )
	ROM_LOAD16_BYTE( "10329a", 0x00001, 0x10000, CRC(da131c81) SHA1(57d5219bd0e2fd886217e37e8773fd76be9b40eb) )
	ROM_LOAD16_BYTE( "10328a", 0x20000, 0x10000, CRC(d5ec5e5d) SHA1(a4e3cfca4d803e72bc4fcf91ab00e21bf3f8959f) )
	ROM_LOAD16_BYTE( "10330a", 0x20001, 0x10000, CRC(ba9ec82a) SHA1(2136c9572e26b7ae6de402c0cd53174407cc6018) )

	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* Road Graphics  (region size should be gr_bitmapwidth*256, 0 )*/
	ROM_LOAD( "10185", 0x0000, 0x8000, CRC(22794426) SHA1(a554d4b68e71861a0d0da4d031b3b811b246f082) )
ROM_END

ROM_START( outrunb )
	ROM_REGION( 0x060000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "orun_mn.rom", 0x000000, 0x10000, CRC(cddceea2) SHA1(34cb4ca61c941e96e585f3cd2aed79bdde67f8eb) )
	ROM_LOAD16_BYTE( "orun_ml.rom", 0x000001, 0x10000, CRC(9cfc07d5) SHA1(b1b5992ff99e4158bb008684e694e088a4b282c6) )
	ROM_LOAD16_BYTE( "orun_mm.rom", 0x020000, 0x10000, CRC(3092d857) SHA1(8ebfeab9217b80a7983a4f8eb7bb7d3387d791b3) )
	ROM_LOAD16_BYTE( "orun_mk.rom", 0x020001, 0x10000, CRC(30a1c496) SHA1(734c82930197e6e8cd2bea145aedda6b3c1145d0) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "10268", 0x00000, 0x08000, CRC(95344b04) SHA1(b3480714b11fc49b449660431f85d4ba92f799ba) )
	ROM_LOAD( "10232", 0x08000, 0x08000, CRC(776ba1eb) SHA1(e3477961d19e694c97643066534a1f720e0c4327) )
	ROM_LOAD( "10267", 0x10000, 0x08000, CRC(a85bb823) SHA1(a7e0143dee5a47e679fd5155e58e717813912692) )
	ROM_LOAD( "10231", 0x18000, 0x08000, CRC(8908bcbf) SHA1(8e1237b640a6f26bdcbfd5e201dadb2687c4febb) )
	ROM_LOAD( "10266", 0x20000, 0x08000, CRC(9f6f1a74) SHA1(09164e858ebeedcff4d389524ddf89e7c216dcae) )
	ROM_LOAD( "10230", 0x28000, 0x08000, CRC(686f5e50) SHA1(03697b892f911177968aa40de6c5f464eb0258e7) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD( "orun_1.rom", 	0x00000, 0x10000, CRC(77377e00) SHA1(4f376b05692f33d529f4c058dac989136c808ca1) )
	ROM_LOAD( "orun_17.rom", 	0x10000, 0x10000, CRC(4f784236) SHA1(1fb610fd29d3ddd8c5d4892ae215386b18552e6f) )
	ROM_LOAD( "orun_2.rom", 	0x20000, 0x10000, CRC(2c0e7277) SHA1(cf14d1ca1fba2e2687998c04ad2ab8c629917412) )
	ROM_LOAD( "orun_18.rom", 	0x30000, 0x10000, CRC(8d459356) SHA1(143914b1ac074708fed1d89072f915424aeb841e) )
	ROM_LOAD( "orun_3.rom", 	0x40000, 0x10000, CRC(69ecc975) SHA1(3560e9a31fc71e263a6ff61224b8db2b17836075) )
	ROM_LOAD( "orun_19.rom", 	0x50000, 0x10000, CRC(ee4f7154) SHA1(3a84c1b19d9dfcd5310e9cee90c0d4562a4a7786) )
	ROM_LOAD( "orun_4.rom", 	0x60000, 0x10000, CRC(54761e57) SHA1(dc0fc645eb998675ab9fe683d63d4ee57ae23693) )
	ROM_LOAD( "orun_20.rom",	0x70000, 0x10000, CRC(c2825654) SHA1(566ecb6e3dc76300351e54e4c0f24b9c2a6c710c) )
	ROM_LOAD( "orun_5.rom", 	0x80000, 0x10000, CRC(b6a8d0e2) SHA1(6184700dbe2c8c9c91f220e246501b7a865e4a05) )
	ROM_LOAD( "orun_21.rom", 	0x90000, 0x10000, CRC(e9880aa3) SHA1(cc47f631e758bd856bbc6d010fe230f9b1ed29de) )
	ROM_LOAD( "orun_6.rom", 	0xa0000, 0x10000, CRC(a00d0676) SHA1(c2ab29a7489c6f774ce26ef023758215ea3f7050) )
	ROM_LOAD( "orun_22.rom",	0xb0000, 0x10000, CRC(ef7d06fe) SHA1(541b5ba45f4140e2cc29a9da2592b476d414af5d) )
	ROM_LOAD( "orun_7.rom", 	0xc0000, 0x10000, CRC(d632d8a2) SHA1(27ca6faaa073bd01b2be959dba0359f93e8c1ec1) )
	ROM_LOAD( "orun_23.rom", 	0xd0000, 0x10000, CRC(dc286dc2) SHA1(eaa245b81f8a324988f617467fc3134a39b59c65) )
	ROM_LOAD( "orun_8.rom", 	0xe0000, 0x10000, CRC(da398368) SHA1(115b2881d2d5ddeda2ce82bb209a2c0b4acfcae4) )
	ROM_LOAD( "orun_24.rom",	0xf0000, 0x10000, CRC(1222af9f) SHA1(2364bd54cbe21dd688efff32e93bf154546c93d6) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "orun_ma.rom", 0x00000, 0x8000, CRC(a3ff797a) SHA1(d97318a0602965cb5059c69a68609691d55a8e41) )

	ROM_REGION( 0x38000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "10193",       0x00000, 0x8000, CRC(bcd10dde) SHA1(417ce1d7242884640c5b14f4db8ee57cde7d085d) )
	ROM_RELOAD(              0x30000, 0x8000 ) /* twice?? */
	ROM_LOAD( "10192",       0x08000, 0x8000, CRC(770f1270) SHA1(686bdf44d45c1d6002622f6658f037735382f3e0) )
	ROM_LOAD( "10191",       0x10000, 0x8000, CRC(20a284ab) SHA1(7c9027416d4122791ba53782fe2230cf02b7d506) )
	ROM_LOAD( "10190",       0x18000, 0x8000, CRC(7cab70e2) SHA1(a3c581d2b438630d0d4c39481dcfd85681c9f889) )
	ROM_LOAD( "10189",       0x20000, 0x8000, CRC(01366b54) SHA1(f467a6b807694d5832a985f5381c170d24aaee4e) )
	ROM_LOAD( "10188",       0x28000, 0x8000, CRC(bad30ad9) SHA1(f70dd3a6362c314adef313b064102f7a250401c8) )

	ROM_REGION( 0x40000, REGION_CPU3, 0 ) /* second 68000 CPU */
	ROM_LOAD16_BYTE( "orun_mj.rom", 0x00000, 0x10000, CRC(d7f5aae0) SHA1(0f9b693f078cdbbfeade5a373a94a20110d586ca) )
	ROM_LOAD16_BYTE( "orun_mh.rom", 0x00001, 0x10000, CRC(88c2e78f) SHA1(198cab9133345e4529f7fb52c29974c9a1a84933) )
	ROM_LOAD16_BYTE( "10328a",      0x20000, 0x10000, CRC(d5ec5e5d) SHA1(a4e3cfca4d803e72bc4fcf91ab00e21bf3f8959f) )
	ROM_LOAD16_BYTE( "orun_mg.rom", 0x20001, 0x10000, CRC(74c5fbec) SHA1(a44f1477d830fdb4d6c29351da94776843e5d3e1) )

	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* Road Graphics  (region size should be gr_bitmapwidth*256, 0 )*/
	ROM_LOAD( "orun_me.rom", 0x0000, 0x8000, CRC(666fe754) SHA1(606090db53d658d7b04dca4748014a411e12f259) )

/*	ROM_LOAD( "orun_mf.rom", 0x0000, 0x8000, CRC(ed5bda9c) )	*/
ROM_END

/* Turbo Outrun */

ROM_START( toutrun )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "bootleg_epr-12513.133", 0x000000, 0x10000, CRC(a1881cea) SHA1(f04291475d603a4558ebeeeaa45b03761c9c0e68) )
	ROM_LOAD16_BYTE( "bootleg_epr-12512.118", 0x000001, 0x10000, CRC(5e9d788b) SHA1(189dfcdfbd20ed69d861858632b50aa97826d1a9) )
	ROM_LOAD16_BYTE( "bootleg_epr-12515.132", 0x020000, 0x10000, CRC(fd432e2d) SHA1(dfef4f1f8ac5f7d8e905dc95daddbfd299257aa1) )
	ROM_LOAD16_BYTE( "bootleg_epr-12514.117", 0x020001, 0x10000, CRC(faf00bd6) SHA1(1e35fa02826a5680926d5d3d1cc83b09d4e170bf) )
	ROM_LOAD16_BYTE( "epr12293.131", 0x040000, 0x10000, CRC(f4321eea) SHA1(64334acc82c14bb58b7d51719f34fd81cfb9fc6b) )
	ROM_LOAD16_BYTE( "epr12292.116", 0x040001, 0x10000, CRC(51d98af0) SHA1(6e7115706bfafb687faa23d55d4a8c8e498a4df2) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "opr12323.102", 0x00000, 0x10000, CRC(4de43a6f) SHA1(68909338e1f192ac2699c8a8d24c3f46502dd019) )
	ROM_LOAD( "opr12324.103", 0x10000, 0x10000, CRC(24607a55) SHA1(69033f2281cd42e88233c23d809b73607fe54853) )
	ROM_LOAD( "opr12325.104", 0x20000, 0x10000, CRC(1405137a) SHA1(367db88d36852e35c5e839f692be5ea8c8e072d2) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD( "mpr12336.9",   0x00000, 0x20000, CRC(dda465c7) SHA1(83acc12a387b004986f084f25964c15a9f88a41a) )
	ROM_LOAD( "mpr12337.10",  0x40000, 0x20000, CRC(828233d1) SHA1(d73a200af4245d590e1fd3ac436723f99cc50452) )
	ROM_LOAD( "mpr12338.11",  0x80000, 0x20000, CRC(46b4b5f4) SHA1(afeb2e5ac6792edafe7328993fe8dfcd4bce1924) )
	ROM_LOAD( "mpr12339.12",  0xc0000, 0x20000, CRC(0d7e3bab) SHA1(fdb603df55785ded593daf591ddd90f8f24e0d47) )
	ROM_LOAD( "mpr12364.13",  0x20000, 0x20000, CRC(a4b83e65) SHA1(966d8c163cef0842abff54e1dba3f15248e73f68) )
	ROM_LOAD( "mpr12365.14",  0x60000, 0x20000, CRC(4a80b2a9) SHA1(14b4fe71e102622a73c8dc0dbd0013cbbe6fcf9d) )
	ROM_LOAD( "mpr12366.15",  0xa0000, 0x20000, CRC(385cb3ab) SHA1(fec6d80d488bfe26524fa3a48b195a45a073e481) )
	ROM_LOAD( "mpr12367.16",  0xe0000, 0x20000, CRC(4930254a) SHA1(00f24be3bf02b143fa554f4d32e283bdac79af6a) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr12300.88",	0x00000, 0x10000, CRC(e8ff7011) SHA1(6eaf3aea507007ea31d507ed7825d905f4b8e7ab) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* sound CPU */
	ROM_LOAD( "opr12301.66",    0x00000, 0x10000, CRC(6e78ad15) SHA1(c31ddf434b459cd1a381d2a028beabddd4ed10d2) )
	ROM_LOAD( "opr12302.67",    0x10000, 0x10000, CRC(e72928af) SHA1(40e0b178958cfe97c097fe9d82b5de54bc27a29f) )
	ROM_LOAD( "opr12303.68",    0x20000, 0x10000, CRC(8384205c) SHA1(c1f9d52bc587eab5a97867198e9aa7c19e973429) )
	ROM_LOAD( "opr12304.69",    0x30000, 0x10000, CRC(e1762ac3) SHA1(855f06c082a17d90857e6efa3cf95b0eda0e634d) )
	ROM_LOAD( "opr12305.70",    0x40000, 0x10000, CRC(ba9ce677) SHA1(056781f92450c902e1d279a02bda28337815cba9) )
	ROM_LOAD( "opr12306.71",    0x50000, 0x10000, CRC(e49249fd) SHA1(ff36e4dba4e9d3d354e3dd528edeb50ad9c18ee4) )

	ROM_REGION( 0x100000, REGION_CPU3, 0 ) /* second 68000 CPU */
	ROM_LOAD16_BYTE( "opr12295.76", 0x000000, 0x10000, CRC(d43a3a84) SHA1(362c98f62c205b6b40b7e8a4ba107745b547b984) )
	ROM_LOAD16_BYTE( "opr12294.58", 0x000001, 0x10000, CRC(27cdcfd3) SHA1(4fe57db95b109ab1bb1326789e06a3d3aac311cc) )
	ROM_LOAD16_BYTE( "opr12297.75", 0x020000, 0x10000, CRC(1d9b5677) SHA1(fb6e33acc43fbc7a8d7ac44045439ecdf794fdeb) )
	ROM_LOAD16_BYTE( "opr12296.57", 0x020001, 0x10000, CRC(0a513671) SHA1(4c13ca3a6f0aa9d06ed80798b466cca0c966a265) )

	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* Road Graphics  (region size should be gr_bitmapwidth*256, 0 )*/
	ROM_LOAD( "epr-12299.47", 0x0000, 0x8000, CRC(fc9bc41b) SHA1(9af73e096253cf2c4f283f227530110a4b37fcee) ) /* Manual shows both as EPR-12298 */
	ROM_LOAD( "epr-12298.11", 0x8000, 0x8000, CRC(fc9bc41b) SHA1(9af73e096253cf2c4f283f227530110a4b37fcee) )
ROM_END


ROM_START( toutrun3 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "bootleg_epr-12410.133", 0x000000, 0x10000, CRC(8e716903) SHA1(6a5c168097623c97358fc5dd8e205cf61932d3e9) )
	ROM_LOAD16_BYTE( "bootleg_epr-12409.118", 0x000001, 0x10000, CRC(675d4dd8) SHA1(e74dc3c21196baac94c41e844b2a9054842ae863) )
	ROM_LOAD16_BYTE( "bootleg_epr-12412.132", 0x020000, 0x10000, CRC(89da477c) SHA1(f854a48d96539d87d2417d936bd6bec1b7ea32fd) )
	ROM_LOAD16_BYTE( "bootleg_epr-12411.117", 0x020001, 0x10000, CRC(285837ee) SHA1(186fff13cd50af504c0dc0296af96dfa24d0d32e) )
	ROM_LOAD16_BYTE( "epr12293.131", 0x040000, 0x10000, CRC(f4321eea) SHA1(64334acc82c14bb58b7d51719f34fd81cfb9fc6b) )
	ROM_LOAD16_BYTE( "epr12292.116", 0x040001, 0x10000, CRC(51d98af0) SHA1(6e7115706bfafb687faa23d55d4a8c8e498a4df2) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "opr12323.102", 0x00000, 0x10000, CRC(4de43a6f) SHA1(68909338e1f192ac2699c8a8d24c3f46502dd019) )
	ROM_LOAD( "opr12324.103", 0x10000, 0x10000, CRC(24607a55) SHA1(69033f2281cd42e88233c23d809b73607fe54853) )
	ROM_LOAD( "opr12325.104", 0x20000, 0x10000, CRC(1405137a) SHA1(367db88d36852e35c5e839f692be5ea8c8e072d2) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD( "opr12307.9",  0x00000, 0x10000, CRC(437dcf09) SHA1(0022ee4d1c3698f77271e570cef98a8a1e5c5d6a) )
	ROM_LOAD( "opr12308.10", 0x40000, 0x10000, CRC(0de70cc2) SHA1(c03f8f8cda72daf64af2878bf254840ac6dd17eb) )
	ROM_LOAD( "opr12309.11", 0x80000, 0x10000, CRC(deb8c242) SHA1(c05d8ced4eafae52c4795fb1471cd66f5903d1aa) )
	ROM_LOAD( "opr12310.12", 0xc0000, 0x10000, CRC(45cf157e) SHA1(5d0be2a374a53ea1fe0ba2bf9b2173e96de1eb51) )
	ROM_LOAD( "opr12311.13", 0x10000, 0x10000, CRC(ae2bd639) SHA1(64bb60ae7e3f87fbbce00106ba65c4e6fc1af0e4) )
	ROM_LOAD( "opr12312.14", 0x50000, 0x10000, CRC(626000e7) SHA1(4a7f9e76dd76a3dc56b8257149bc94be3f4f2e87) )
	ROM_LOAD( "opr12313.15", 0x90000, 0x10000, CRC(52870c37) SHA1(3a6836a46d94c0f9115800d206410252a1134c57) )
	ROM_LOAD( "opr12314.16", 0xd0000, 0x10000, CRC(40c461ea) SHA1(7bed8f24112dc3c827fd087138fcf2700092aa59) )
	ROM_LOAD( "opr12315.17", 0x20000, 0x10000, CRC(3ff9a3a3) SHA1(0d90fe2669d03bd07a0d3b05934201778e28d54c) )
	ROM_LOAD( "opr12316.18", 0x60000, 0x10000, CRC(8a1e6dc8) SHA1(32f09ec504c2b6772815bad7380a2f738f11746a) )
	ROM_LOAD( "opr12317.19", 0xa0000, 0x10000, CRC(77e382d4) SHA1(5b7912069a46043b7be989d82436add85497d318) )
	ROM_LOAD( "opr12318.20", 0xe0000, 0x10000, CRC(d1afdea9) SHA1(813eccc88d5046992be5b5a0618d32127d16e30b) )
	ROM_LOAD( "opr12319.25", 0x30000, 0x10000, CRC(df23baf9) SHA1(f9611391bb3b3b92203fa9f6dd461e3a6e863622) )
	ROM_LOAD( "opr12320.22", 0x70000, 0x10000, CRC(7931e446) SHA1(9f2161a689ebad61f6653942e23d9c2bc6170d4a) )
	ROM_LOAD( "opr12321.23", 0xb0000, 0x10000, CRC(830bacd4) SHA1(5a4816969437ee1edca5845006c0b8e9ba365491) )
	ROM_LOAD( "opr12322.24", 0xf0000, 0x10000, CRC(8b812492) SHA1(bf1f9e059c093c0991c7caf1b01c739ed54b8357) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr12300.88",	0x00000, 0x10000, CRC(e8ff7011) SHA1(6eaf3aea507007ea31d507ed7825d905f4b8e7ab) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* sound CPU */
	ROM_LOAD( "opr12301.66",    0x00000, 0x10000, CRC(6e78ad15) SHA1(c31ddf434b459cd1a381d2a028beabddd4ed10d2) )
	ROM_LOAD( "opr12302.67",    0x10000, 0x10000, CRC(e72928af) SHA1(40e0b178958cfe97c097fe9d82b5de54bc27a29f) )
	ROM_LOAD( "opr12303.68",    0x20000, 0x10000, CRC(8384205c) SHA1(c1f9d52bc587eab5a97867198e9aa7c19e973429) )
	ROM_LOAD( "opr12304.69",    0x30000, 0x10000, CRC(e1762ac3) SHA1(855f06c082a17d90857e6efa3cf95b0eda0e634d) )
	ROM_LOAD( "opr12305.70",    0x40000, 0x10000, CRC(ba9ce677) SHA1(056781f92450c902e1d279a02bda28337815cba9) )
	ROM_LOAD( "opr12306.71",    0x50000, 0x10000, CRC(e49249fd) SHA1(ff36e4dba4e9d3d354e3dd528edeb50ad9c18ee4) )

	ROM_REGION( 0x100000, REGION_CPU3, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "opr12295.76", 0x000000, 0x10000, CRC(d43a3a84) SHA1(362c98f62c205b6b40b7e8a4ba107745b547b984) )
	ROM_LOAD16_BYTE( "opr12294.58", 0x000001, 0x10000, CRC(27cdcfd3) SHA1(4fe57db95b109ab1bb1326789e06a3d3aac311cc) )
	ROM_LOAD16_BYTE( "opr12297.75", 0x020000, 0x10000, CRC(1d9b5677) SHA1(fb6e33acc43fbc7a8d7ac44045439ecdf794fdeb) )
	ROM_LOAD16_BYTE( "opr12296.57", 0x020001, 0x10000, CRC(0a513671) SHA1(4c13ca3a6f0aa9d06ed80798b466cca0c966a265) )

  ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* Road Graphics  (region size should be gr_bitmapwidth*256, 0 )*/
	ROM_LOAD( "epr12298.11", 0x0, 0x08000, CRC(fc9bc41b) SHA1(9af73e096253cf2c4f283f227530110a4b37fcee) )
ROM_END

/***************************************************************************/

#if 0
static READ16_HANDLER( or_io_joy_r ){
	return (input_port_5_r( offset ) << 8) + input_port_6_r( offset );
}
#endif

#ifdef HANGON_DIGITAL_CONTROLS
static READ16_HANDLER( or_io_brake_r ){
	int data = input_port_1_r( offset );

	switch(data & 3)
	{
		case 3:	return 0xff00;	/* both */
		case 1:	return 0xff00;  /* brake */
		case 2:	return 0x0000;  /* accel */
		case 0:	return 0x0000;  /* neither */
	}
	return 0x0000;
}

static READ16_HANDLER( or_io_acc_steer_r ){
	int data = input_port_1_r( offset );
	int ret = input_port_0_r( offset ) << 8;

	switch(data & 3)
	{
		case 3:	return 0x00 | ret;	/* both */
		case 1:	return 0x00 | ret;  /* brake */
		case 2:	return 0xff | ret;  /* accel */
		case 0:	return 0x00 | ret;  /* neither */
	}
	return 0x00 | ret;
}
#else
static READ16_HANDLER( or_io_acc_steer_r ){ return (input_port_0_r( offset ) << 8) + input_port_1_r( offset ); }
static READ16_HANDLER( or_io_brake_r ){ return input_port_5_r( offset ) << 8; }
#endif

static int selected_analog;

static READ16_HANDLER( outrun_analog_r )
{
	switch (selected_analog)
	{
		default:
		case 0: return or_io_acc_steer_r(0,0) >> 8;
		case 1: return or_io_acc_steer_r(0,0) & 0xff;
		case 2: return or_io_brake_r(0,0) >> 8;
		case 3: return or_io_brake_r(0,0) & 0xff;
	}
}

static WRITE16_HANDLER( outrun_analog_select_w )
{
	if ( ACCESSING_LSB )
	{
		selected_analog = (data & 0x0c) >> 2;
	}
}

static data16_t or_credits_val = 0;
static data16_t or_btns = 0xffff; /* ipt2 bitfield */
static READ16_HANDLER( or_io_service_r )
{
	/*
	IPT1:
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON4 )

	IPT2:
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	*/
	data16_t ret = readinputport(2), ipt2=ret;

	/* Gear port hack for IPT2 IPT_BUTTON3 */
	data16_t ipt1 = readinputport(1);
	if(ipt1 & 0x04) {
		ret &= (~0x10);	/* LO */
	} else if(ipt1 & 0x08) {
		ret |= 0x10;	/* HI */
	}

	if ((ipt2 ^ or_btns) & 0x08) {
		/* Start button state changed */
		/* printf("outrun: Start Button %s\n", (0 == (ipt2 & 0x08)) ? "Pressed" : "Released"); */
		if (0 == (ipt2 & 0x08) && (0 == or_credits_val)) {
			/* Start button pressed and zero credits - Simulate coin */
			ret |= 0x08; /* Disable Start button */
			ret &= (~0x40); /* Simulate Coin */
			/* printf("outrun: Simulate Coin In!\n"); */
		}
	}
	/* We need to save the input port state as read from the port to catch changes next time ... */
	or_btns = ipt2;
	/* ... but we return our modfifed state to fool the game */
	return ret;
}

static WRITE16_HANDLER( outrun_sound_write_w )
{
	if( ost_support_enabled(OST_SUPPORT_OUTRUN) ) {
		if(generate_ost_sound( data )) sound_shared_ram[0]=data&0xff;
	}
	else {
		sound_shared_ram[0]=data&0xff;
	}
}

static WRITE16_HANDLER( outrun_ctrl2_w )
{
	if( ACCESSING_LSB ){
		/* bit 0 always 1? */
		set_led_status(0,data & 0x04);
		set_led_status(1,data & 0x02);	/* brakes */
		coin_counter_w(0,data & 0x10);
	}
}
static unsigned char ctrl1;

static WRITE16_HANDLER( outrun_ctrl1_w )
{
	/* sound be sound cpu reset, not 2nd 68k cpu? */
#if 0
	if(ACCESSING_LSB) {
		int changed = data ^ ctrl1;
		ctrl1 = data;
		if(changed & 1) {
			if(data & 1) {
				cpu_set_halt_line(2, CLEAR_LINE);
				cpu_set_reset_line(2, PULSE_LINE);
			} else
				cpu_set_halt_line(2, ASSERT_LINE);
		}

/*		sys16_kill_set(data & 0x20); */

		/* bit 0 always 1? */
		/* bits 2-3 continuously change: 00-01-10-11; this is the same that
		   gets written to 140030 so is probably input related */
	}
#endif
	if(ACCESSING_LSB)
		sys16_refreshenable = (data >> 5) & 1;

}

static WRITE16_HANDLER( outrun_ctrl1_w_new )
{
   if(ACCESSING_LSB)
   {
      segaic16_set_display_enable((data >> 5) & 1);

      return;
	   /*cpunum_set_input_line(2, INPUT_LINE_RESET, (data & 1) ? CLEAR_LINE : ASSERT_LINE); */
   }
}

static void outrun_reset(void)
{
       cpu_set_reset_line(2, PULSE_LINE);
	   cpu_set_halt_line(2, CLEAR_LINE);
}

/**************************************
 *  Out Run Time handler
 *************************************/

static UINT16 outrun_time_last = 0; /* The last time we recorded */
static UINT16 outrun_time_max = 75; /* max time in seconds when you start the game */

static WRITE16_HANDLER( outrun_sys16_extraram2_w )
{
	/* { 0x060000, 0x067fff, outrun_sys16_extraram2_w, &sys16_extraram2 } */
	COMBINE_DATA( sys16_extraram2 + offset );
	if (0x430 == offset && ACCESSING_LSB) {
		/* Time is stored in single byte packed BCD of WORD @ 0x60860 (LSB - offset: 0x430 ) */
		UINT16 fuel_percent = 0;
		UINT16 time = 10 * ((data & 0x00f0) >> 4) + (data & 0x000f);
		if (time > outrun_time_last) {
			outrun_time_max = time;	/* time was increased, record the new max value */
		}
		outrun_time_last = time;
		/* Calculate fuel % from time remaining since last reset */
		if (outrun_time_max > 0) {
			fuel_percent = time * 100 / outrun_time_max;
		}
		if (fuel_percent > 100) {
			fuel_percent = 100;
		}
		RealDashCanClientUpdateFuel(fuel_percent);
	} else if (0x29 == offset && ACCESSING_MSB) {
		/* Credits is stored in single byte of WORD @ 0x6052 (MSB - offset: 0x29 ) */
		or_credits_val = (data >> 8) & 0xff;
	}
}

static MEMORY_READ16_START( outrun_readmem )
	{ 0x000000, 0x05ffff, MRA16_ROM },
	{ 0x060900, 0x060907, sound_shared_ram_r },		/*??? */
	
	{ 0x060000, 0x067fff, SYS16_MRA16_EXTRAM2 },

	{ 0x100000, 0x10ffff, SYS16_MRA16_TILERAM },
	{ 0x110000, 0x110fff, SYS16_MRA16_TEXTRAM },

	{ 0x130000, 0x130fff, SYS16_MRA16_SPRITERAM },
	{ 0x120000, 0x121fff, SYS16_MRA16_PALETTERAM },

	{ 0x140010, 0x140011, or_io_service_r },
	{ 0x140014, 0x140015, input_port_3_word_r }, /* dip1 */
	{ 0x140016, 0x140017, input_port_4_word_r }, /* dip2 */
	{ 0x140030, 0x140031, outrun_analog_r },

	{ 0x200000, 0x23ffff, SYS16_CPU3ROM16_r },
	{ 0x260000, 0x267fff, shared_ram_r },
	{ 0xe00000, 0xe00001, SYS16_CPU2_RESET_HACK },
MEMORY_END

static MEMORY_WRITE16_START( outrun_writemem )
	{ 0x000000, 0x05ffff, MWA16_ROM },
	{ 0x060900, 0x060907, sound_shared_ram_w },		/*??? */

	{ 0x060000, 0x067fff, outrun_sys16_extraram2_w, &sys16_extraram2 },

	{ 0x100000, 0x10ffff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x110000, 0x110fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x130000, 0x130fff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0x120000, 0x121fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0x140004, 0x140005, outrun_ctrl1_w },
	{ 0x140020, 0x140021, outrun_ctrl2_w },
	{ 0x140030, 0x140031, outrun_analog_select_w },
	{ 0x200000, 0x23ffff, MWA16_ROM },
	{ 0x260000, 0x267fff, outrun_shared_ram_w, &shared_ram },
	{ 0xffff06, 0xffff07, outrun_sound_write_w },
MEMORY_END

static MEMORY_READ16_START( outrun_readmem2 )
    { 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x060000, 0x067fff, shared_ram_r },
	{ 0x080000, 0x09ffff, SYS16_MRA16_EXTRAM },		/* gr */
MEMORY_END

static MEMORY_WRITE16_START( outrun_writemem2 )
    { 0x000000, 0x03ffff, MWA16_ROM },
    { 0x060000, 0x067fff, shared_ram_w },
	{ 0x080000, 0x09ffff, SYS16_MWA16_EXTRAM, &sys16_extraram },		/* gr */
MEMORY_END

/* Outrun */

static MEMORY_READ_START( outrun_sound_readmem )
    { 0x0000, 0x7fff, MRA_ROM },
	{ 0xf000, 0xf0ff, SegaPCM_r },
	{ 0xf100, 0xf7ff, MRA_NOP },
	{ 0xf800, 0xf807, sound2_shared_ram_r },
	{ 0xf808, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( outrun_sound_writemem )
    { 0x0000, 0x7fff, MWA_ROM },
	{ 0xf000, 0xf0ff, SegaPCM_w },
	{ 0xf100, 0xf7ff, MWA_NOP },
	{ 0xf800, 0xf807, sound2_shared_ram_w, &sound_shared_ram },
	{ 0xf808, 0xffff, MWA_RAM },
MEMORY_END


extern READ16_HANDLER( segaic16_textram_r );
extern READ16_HANDLER( segaic16_tileram_r );
extern READ16_HANDLER( segaic16_spriteram_r);
READ16_HANDLER( segaic16_roadram_r ){
	return segaic16_roadram_0[offset];
}

static MEMORY_READ16_START( toutrun_readmem )
	{ 0x000000, 0x05ffff, MRA16_ROM },
	{ 0x060000, 0x067fff, SYS16_MRA16_EXTRAM2 },

	{ 0x100000, 0x10ffff, segaic16_tileram_r },
	{ 0x110000, 0x110fff, segaic16_textram_r },
	{ 0x120000, 0x121fff, SYS16_MRA16_PALETTERAM },

	{ 0x130000, 0x130fff, segaic16_spriteram_r },

	{ 0x140010, 0x140011, or_io_service_r },
	{ 0x140014, 0x140015, input_port_3_word_r }, /* dip1 */
	{ 0x140016, 0x140017, input_port_4_word_r }, /* dip2 */
	{ 0x140030, 0x140031, outrun_analog_r },

	{ 0x200000, 0x23ffff, SYS16_CPU3ROM16_r },
	{ 0x260000, 0x267fff, shared_ram_r },
	{ 0x280000, 0X280FFF, segaic16_roadram_r },
	{ 0X290000, 0X29FFFF, segaic16_road_control_0_r },
	{ 0xe00000, 0xe00001, SYS16_CPU2_RESET_HACK },
MEMORY_END


static MEMORY_WRITE16_START( toutrun_writemem )
	{ 0x000000, 0x05ffff, MWA16_ROM },
	{ 0x060000, 0x067fff, SYS16_MWA16_EXTRAM2, &sys16_extraram2 },/*workram */
	{ 0x100000, 0x10ffff, segaic16_tileram_0_w, &segaic16_tileram_0, },
	{ 0x110000, 0x110fff, segaic16_textram_0_w, &segaic16_textram_0 },
	{ 0x130000, 0x130fff, SYS16_MWA16_SPRITERAM,  &segaic16_spriteram_0 },
	{ 0x120000, 0x121fff, segaic16_paletteram_w, &paletteram16 },
	{ 0x140004, 0x140005, outrun_ctrl1_w_new },
	{ 0x140020, 0x140021, outrun_ctrl2_w },
	{ 0x140030, 0x140031, outrun_analog_select_w },/*segaic16_sprites_draw_0_w */
	{ 0x140070, 0x140071, segaic16_sprites_draw_0_w },/*segaic16_sprites_draw_0_w */

	{ 0x200000, 0x25ffff, MWA16_ROM },
	{ 0x260000, 0x267fff, shared_ram_w, &shared_ram },
	{ 0x280000, 0X280FFF, MWA16_RAM, &segaic16_roadram_0 },
	{ 0X290000, 0X29FFFF, segaic16_road_control_0_w },
	{ 0xffff06, 0xffff07, outrun_sound_write_w },
MEMORY_END


static MEMORY_READ16_START( toutrun_readmem2 )
    { 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x060000, 0x067fff, shared_ram_r },
	{ 0x080000, 0x080fff, segaic16_roadram_r },/*MWA16_RAM, &segaic16_roadram_0 */
	{ 0x090000, 0x09ffff, segaic16_road_control_0_r },
MEMORY_END

static MEMORY_WRITE16_START( toutrun_writemem2 )
    { 0x000000, 0x03ffff, MWA16_ROM },
    { 0x060000, 0x067fff, shared_ram_w },/*MWA16_RAM, &segaic16_roadram_0 */
	{ 0x080000, 0x080fff, MWA16_RAM, &segaic16_roadram_0 },
	{ 0x090000, 0x09ffff, segaic16_road_control_0_w },
MEMORY_END

/***************************************************************************/

static void outrun_update_proc( void ){
	set_fg_page( sys16_textram[0x740] );
	set_bg_page( sys16_textram[0x741] );
	sys16_fg_scrolly = sys16_textram[0x748];
	sys16_bg_scrolly = sys16_textram[0x749];
	sys16_fg_scrollx = sys16_textram[0x74c];
	sys16_bg_scrollx = sys16_textram[0x74d];
}

static MACHINE_INIT( outrun ){
	static int bank[8] = {
		7,0,1,2,
		3,4,5,6
	};
	RealDashCanClientInit();
	RealDashCanClientStartServer();
	sys16_obj_bank = bank;
	sys16_spritesystem = sys16_sprite_outrun;
	sys16_textlayer_lo_min=0;
	sys16_textlayer_lo_max=0;
	sys16_textlayer_hi_min=0;
	sys16_textlayer_hi_max=0xff;
	sys16_sprxoffset = -0xbd;
	ctrl1 = 0x20;

/* *forced sound cmd (eww) */
	if (!strcmp(Machine->gamedrv->name,"outrun")) sys16_patch_code( 0x55ed, 0x00);
	if (!strcmp(Machine->gamedrv->name,"outruna")) sys16_patch_code( 0x5661, 0x00);
	if (!strcmp(Machine->gamedrv->name,"outrunb")) sys16_patch_code( 0x5661, 0x00);

     cpu_set_m68k_reset(0, outrun_reset);


	sys16_update_proc = outrun_update_proc;

	sys16_gr_ver = &sys16_extraram[0];
	sys16_gr_hor = sys16_gr_ver+0x400/2;
	sys16_gr_flip= sys16_gr_ver+0xc00/2;

	sys16_gr_palette= 0xf00 / 2;
	sys16_gr_palette_default = 0x800 /2;
	sys16_gr_colorflip[0][0]=0x08 / 2;
	sys16_gr_colorflip[0][1]=0x04 / 2;
	sys16_gr_colorflip[0][2]=0x00 / 2;
	sys16_gr_colorflip[0][3]=0x00 / 2;
	sys16_gr_colorflip[1][0]=0x0a / 2;
	sys16_gr_colorflip[1][1]=0x06 / 2;
	sys16_gr_colorflip[1][2]=0x02 / 2;
	sys16_gr_colorflip[1][3]=0x00 / 2;

	sys16_gr_second_road = &sys16_extraram[0x8000];

	cpu_set_halt_line(2, ASSERT_LINE);
}

static MACHINE_INIT( toutrun ){
	ctrl1 = 0x20;
    segaic16_tilemap_reset(0);
/* *forced sound cmd (eww) */
	if (!strcmp(Machine->gamedrv->name,"outrun")) sys16_patch_code( 0x55ed, 0x00);
	if (!strcmp(Machine->gamedrv->name,"outruna")) sys16_patch_code( 0x5661, 0x00);
	if (!strcmp(Machine->gamedrv->name,"outrunb")) sys16_patch_code( 0x5661, 0x00);

     cpu_set_m68k_reset(0, outrun_reset);
	cpu_set_halt_line(2, ASSERT_LINE);
}

void outrun_hud_patch(void)
{
	/********************
	 * ND: ROM Patching *
	 *******************/

	/* 
	From Cannonball source on text in ROM encoded with position: in textram
	See also Section 7 'Text layer and text RAM' in Charles MacDoanld's Sys16 notes:
	https://jammarcade.net/images/2024/04/s16b.txt

	Format of the input data is as follows:

	Long 1: Destination address to move data to. [e.g. 0x00110D34 which would go to the text ram]
	Word 1: Number of tiles to draw / counter
	Byte 1: High byte to apply to every copy, containing priority info [86]
	Byte 2: Not used

	Byte 3: Second byte to copy containing tile number (low)
	Byte 4: Third byte to copy containing tile number (low)
	Byte 5: etc.

	Text layer name table format:
	
	MSB          LSB
	p???cccnnnnnnnnn
	
	p : Priority. If 0, sprites with priority level 3 are shown over the text.
	              If 1, the text layer is shown over sprites regardless of priority.
	c : Color palette
	n : Tile index to use
	? : Unknown

	NOTE: All LONG (32-bit) address patching are done by 2 x WORD (16-bit) operations
	due to Big endian AND not all addresses are 32-bit aligned

	const definitions below like 'HUD_STRUCT_...' refer to addr of DATA structures in ROM in the format described above
	const definitions below like 'HUD_ADDR_...' refer to an addr included directly in 68K asm CODE (e.g. 'lea $1100B2,A0')

	*/

	/* SCORE Graphic (2 Lines) */
	const uint16_t HUD_STRUCT_SCORE1 		= 0xBC3E;	/* 0x001100c6,0004, ... */
	const uint16_t HUD_STRUCT_SCORE2 		= 0xBC4C;	/* 0x00110146,0004, ... */
	const uint16_t HUD_ADDR_SCORE_NUM 		= 0x739A; 	/* Addr of textram pos in code */

	/* TIME Graphic (2 Lines) */
	/* const uint16_t HUD_STRUCT_TIME1 		= 0xBC5A; */ 	/* 0x001100B4,0003, ... */
	/* const uint16_t HUD_STRUCT_TIME2 		= 0xBC66; */	/* 0x00110134,0003, ... */

	/* KPH (2 Lines) */
	/* const uint16_t HUD_STRUCT_KPH1 		= 0xBC72; */ 	/* 0x00110CBC,0002, ... */
	/* const uint16_t HUD_STRUCT_KPH2 		= 0xBC7E; */	/* 0x00110D3C,0002, ... */

	/* STAGE (2 Lines) */
	const uint16_t HUD_STRUCT_STAGE1_START	= 0xBC8A;	/* 0x00110CEA,0004, ... */
	const uint16_t HUD_STRUCT_STAGE2_START	= 0xBC98;	/* 0x00110D6A,0004, ... */
	const uint16_t HUD_STRUCT_STAGE1_CP		= 0x9198;	/* 0x00110CEA,0004, ... */
	const uint16_t HUD_STRUCT_STAGE2_CP		= 0x91A6;	/* 0x00110D6A,0004, ... */

	/* Number to appear after stage */
	const uint16_t HUD_STRUCT_STAGEN_START	= 0xBCA6;	/* 0x00110D76,0000, ... */
	const uint16_t HUD_ADDR_STAGEN_CP		= 0x9036;	/* Addr of textram pos in code */

	/* LAP (2 Lines) */
	const uint16_t HUD_STRUCT_LAP1 			= 0xBCDA;	/* 0x001100E2,0003, ... */
	const uint16_t HUD_STRUCT_LAP2 			= 0xBCE6;	/* 0x00110162,0003, ... */
	const uint16_t HUD_ADDR_LAP_TIME 		= 0x8180;	/* Addr of textram pos in code */

	/* Mini-Map */
	const uint16_t HUD_ADDR_MINI_MAP 		= 0x8B98;	/* Addr of textram pos in code */
	
	/* Get ptr to ROM data in RAM */
	data16_t *RAM = (data16_t *)memory_region(REGION_CPU1);

	/* This ROM patch only supports outruna & outrunb ROM sets */
	if (0 == strcmp(Machine->gamedrv->name, "outrun")) {
		return;
	}
	
	/* 
		Knock out 'Time' graphic by replacing call to render routine (bsr $c526) with 'nop' instr's (0x4E71) 
		HUD_STRUCT_TIME1 = 0xBC5A - bsr $c526 is at 0xB47C (2 words)
		HUD_STRUCT_TIME2 = 0xBC66 - bsr $c526 is at 0xB486 (2 words)
		SUBR to render time (s) bsr $8216, called at 0x7D1E
	*/
	RAM[0xB47C >> 1] = 0x4e71;	RAM[(0xB47C >> 1) + 1] = 0x4e71; 	/* HUD_STRUCT_TIME1 */
	RAM[0xB486 >> 1] = 0x4e71;	RAM[(0xB486 >> 1) + 1] = 0x4e71;	/* HUD_STRUCT_TIME2 */
	RAM[0x7D1E >> 1] = 0x4e71;	RAM[(0x7D1E >> 1) + 1] = 0x4e71;	/* draw_time SUBR */
	/*
		Knock out Speed 'km/h' graphic by replacing call to render routine (bsr $c526) with 'nop' instr's (0x4E71)
		Knock out speed digits by replacing call to render routine (bsr $bb72) at 0xBA2A with 2x 'nop' instr's (0x4E71)
	*/
	RAM[0xBA34 >> 1] = 0x4e71;	RAM[(0xBA34 >> 1) + 1] = 0x4e71; 	/* HUD_STRUCT_KPH1 */
	RAM[0xBA3E >> 1] = 0x4e71;	RAM[(0xBA3E >> 1) + 1] = 0x4e71; 	/* HUD_STRUCT_KPH2 */
	RAM[0xBA2A >> 1] = 0x4e71;	RAM[(0xBA2A >> 1) + 1] = 0x4e71;	/* draw_speed SUBR */
	/*
		Knock out the rev counter by replacing 'single' (0x81fe) and 'double' (0x81fd) digit tiles 
		with 'space' tile (0x8120) in the draw_rev_counter render routine at 0x6b08.
		We don't knock out the entire bsr $6b08 with 'nop' since the routine also does engine sound!
	*/
	RAM[0x6B54 >> 1] = 0x8120;	RAM[0x6B64 >> 1] = 0x8120;	/* Empty tiles for rev counter */


	/*
		Move 'Score'to where 'Time' was (but one space further left)
	*/
	RAM[(HUD_STRUCT_SCORE1 >> 1) + 1] 	= 0x00B2;	/* HUD_STRUCT_SCORE1 pos */
	RAM[(HUD_STRUCT_SCORE2 >> 1) + 1] 	= 0x0132;	/* HUD_STRUCT_SCORE2 pos */
	RAM[(HUD_ADDR_SCORE_NUM >> 1) + 1] 	= 0x013E;	/* Score pos (HUD_STRUCT_SCORE2 + 0xC)*/
	/*
		Move 'Stage' to right of score
	*/
	RAM[(HUD_STRUCT_STAGE1_START >> 1) + 1]	= 0x00D0;	/* HUD_STRUCT_STAGE1_START pos (start) */
	RAM[(HUD_STRUCT_STAGE2_START >> 1) + 1] = 0x0150;	/* HUD_STRUCT_STAGE2_START pos (start) */
	RAM[(HUD_STRUCT_STAGE1_CP >> 1) + 1] 	= 0x00D0;	/* HUD_STRUCT_STAGE1_CP pos (checkpoint) */
	RAM[(HUD_STRUCT_STAGE2_CP >> 1) + 1] 	= 0x0150;	/* HUD_STRUCT_STAGE2_CP pos (checkpoint) */
	RAM[(HUD_STRUCT_STAGEN_START >> 1) + 1] = 0x015C;	/* '1' pos (static @ start HUD_STRUCT_STAGE2_START + 0xC) */
	/* - patch pos arg for render arg at stage update (long at $9036 - default is 0x00110d6e)*/
	RAM[(HUD_ADDR_STAGEN_CP >> 1) + 1] 		= 0x0154;	/* Stage NUM (checkpoint HUD_STRUCT_STAGE2_CP + 0x4) */
	/*
		Move 'mini-map' to between 'STAGE' number and 'LAP' graphic. textram pos defaults to 0x110cfa @ 0x8B98
	*/
	RAM[(HUD_ADDR_MINI_MAP >> 1) + 1] 	= 0x00E0;	/* Mini-Map pos */
	/*
		Move 'Lap' to right of mini-map
	*/
	RAM[(HUD_STRUCT_LAP1 >> 1) + 1] 	= 0x00E6;	/* HUD_STRUCT_LAP1 pos */
	RAM[(HUD_STRUCT_LAP2 >> 1) + 1] 	= 0x0166;	/* HUD_STRUCT_LAP2 pos */
	RAM[(HUD_ADDR_LAP_TIME >> 1) + 1] 	= 0x0170;	/* Lap time pos (HUD_STRUCT_LAP2 + 0xA)*/

	/* 'INSERT COINS' -> 'PUSH START BUTTON' */
	RAM[0xb82e >> 1] = 0xbbd0; /* SET */
	RAM[0xb83c >> 1] = 0xbbec; /* CLR */

	/* 
		Hide 'CREDIT[S] render routine is at 0x6cde and is called from multiple places.
		We just patch the first instruction in the routing to 'rts' early:
		tst.b $60c50 (3 WORDs) -> {rts, nop, nop}
	*/
	RAM[0x6cde >> 1] = 0x4e75; RAM[(0x6cde >> 1) + 1] = 0x4e71;	RAM[(0x6cde >> 1) + 2] = 0x4e71;

	/* 1987 ©'BETA' -> 'SEGA' */
	RAM[0xbd00 >> 1] = 0x5345;	RAM[(0xbd00 >> 1) + 1] = 0x4741;
}

static DRIVER_INIT( outrun )
{
	sys16_interleave_sprite_data( 0x100000 );
	generate_gr_screen(512,2048,0,0,3,0x8000);
#ifdef REALDASH
	outrun_hud_patch();
#endif
}

static DRIVER_INIT( toutrun )
{
	sys16_interleave_sprite_data( 0x100000 );
	generate_gr_screen(512,2048,0,0,0,0x8000); /* fixes road 2 */
}

static DRIVER_INIT( outrunb )
{
	data16_t *RAM = (data16_t *)memory_region(REGION_CPU1);
	int i;

/*
  Main Processor
	Comparing the bootleg with the custom bootleg, it seems that:-

  if even bytes &0x28 == 0x20 or 0x08 then they are xored with 0x28
  if odd bytes &0xc0 == 0x40 or 0x80 then they are xored with 0xc0

  ie. data lines are switched.
*/

	for( i=0;i<0x40000;i+=2 ){
		data16_t word = RAM[i/2];
		UINT8 even = word>>8;
		UINT8 odd = word&0xff;

		/* even byte */
		if((even&0x28) == 0x20 || (even&0x28) == 0x08) even^=0x28;

		/* odd byte */
		if((odd&0xc0) == 0x80 || (odd&0xc0) == 0x40) odd^=0xc0;

		RAM[i/2] = (even<<8)+odd;
	}

/*
  Second Processor

  if even bytes &0xc0 == 0x40 or 0x80 then they are xored with 0xc0
  if odd bytes &0x0c == 0x04 or 0x08 then they are xored with 0x0c
*/
	RAM = (data16_t *)memory_region(REGION_CPU3);
	for(i=0;i<0x40000;i+=2)
	{
		data16_t word = RAM[i/2];
		UINT8 even = word>>8;
		UINT8 odd = word&0xff;

		/* even byte */
		if((even&0xc0) == 0x80 || (even&0xc0) == 0x40) even^=0xc0;

		/* odd byte */
		if((odd&0x0c) == 0x08 || (odd&0x0c) == 0x04) odd^=0x0c;

		RAM[i/2] = (even<<8)+odd;
	}
/*
  Road GFX

	rom orun_me.rom
	if bytes &0x60 == 0x40 or 0x20 then they are xored with 0x60

	rom orun_mf.rom
	if bytes &0xc0 == 0x40 or 0x80 then they are xored with 0xc0

  I don't know why there's 2 road roms, but I'm using orun_me.rom
*/
	{
		UINT8 *mem = memory_region(REGION_GFX3);
		for(i=0;i<0x8000;i++){
			if( (mem[i]&0x60) == 0x20 || (mem[i]&0x60) == 0x40 ) mem[i]^=0x60;
		}
	}

	generate_gr_screen(512,2048,0,0,3,0x8000);
	sys16_interleave_sprite_data( 0x100000 );

/*
  Z80 Code
	rom orun_ma.rom
	if bytes &0x60 == 0x40 or 0x20 then they are xored with 0x60

*/
	{
		UINT8 *mem = memory_region(REGION_CPU2);
		for(i=0;i<0x8000;i++){
			if( (mem[i]&0x60) == 0x20 || (mem[i]&0x60) == 0x40 ) mem[i]^=0x60;
		}
	}
#ifdef REALDASH
	outrun_hud_patch();
#endif
}

/***************************************************************************/

INPUT_PORTS_START( outrun )
PORT_START	/* Steering */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_CENTER, 100, 3, 0x48, 0xb8 )
/*	PORT_ANALOG( 0xff, 0x7f, IPT_PADDLE , 70, 3, 0x48, 0xb8 ) */

#ifdef HANGON_DIGITAL_CONTROLS

PORT_START	/* Buttons */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON4 )

#else

PORT_START	/* Accel / Decel */
	PORT_ANALOG( 0xff, 0x30, IPT_AD_STICK_Y | IPF_CENTER | IPF_REVERSE, 100, 16, 0x30, 0x90 )

#endif

PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
/*	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 ) */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	SYS16_COINAGE

PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x02, "Up Cockpit" )
	PORT_DIPSETTING(    0x01, "Mini Up" )
	PORT_DIPSETTING(    0x03, "Moving" )
/*	PORT_DIPSETTING(    0x00, "No Use" ) */
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, "Time" )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0xc0, 0xc0, "Enemies" )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0xc0, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )


#ifndef HANGON_DIGITAL_CONTROLS

PORT_START	/* Brake */
	PORT_ANALOG( 0xff, 0x30, IPT_AD_STICK_Y | IPF_PLAYER2 | IPF_CENTER | IPF_REVERSE, 100, 16, 0x30, 0x90 )

#endif

INPUT_PORTS_END


INPUT_PORTS_START( toutrun )
PORT_START	/* Steering */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_CENTER, 100, 3, 0x48, 0xb8 )
/*	PORT_ANALOG( 0xff, 0x7f, IPT_PADDLE , 70, 3, 0x48, 0xb8 ) */

#ifdef HANGON_DIGITAL_CONTROLS

PORT_START	/* Buttons */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON4 )

#else

PORT_START	/* Accel / Decel */
	PORT_ANALOG( 0xff, 0x30, IPT_AD_STICK_Y | IPF_CENTER | IPF_REVERSE, 100, 16, 0x30, 0x90 )

#endif

PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
/*	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 ) */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON5 ) /* Turbo */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	SYS16_COINAGE

PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x02, "Cockpit Conversion" )
	PORT_DIPSETTING(    0x01, "Mini Up" )
	PORT_DIPSETTING(    0x03, "Moving" )
	PORT_DIPSETTING(    0x00, "Cockpit" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Turbo" )
	PORT_DIPSETTING(    0x00, "Start Button" )
	PORT_DIPSETTING(    0x08, "Use Turbo Shifter" )
	PORT_DIPNAME( 0x30, 0x10, "Credits" )
	PORT_DIPSETTING(    0x20, "3 to Start/2 to Continue" )
	PORT_DIPSETTING(    0x30, "2 to Start/1 to Continue" )
	PORT_DIPSETTING(    0x10, "1 to Start/1 to Continue" )
	PORT_DIPSETTING(    0x00, "2 to Start/2 to Continue" )
	PORT_DIPNAME( 0xc0, 0xc0, "Enemies" )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0xc0, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )


#ifndef HANGON_DIGITAL_CONTROLS

PORT_START	/* Brake */
	PORT_ANALOG( 0xff, 0x30, IPT_AD_STICK_Y | IPF_PLAYER2 | IPF_CENTER | IPF_REVERSE, 100, 16, 0x30, 0x90 )

#endif

INPUT_PORTS_END

/***************************************************************************/
static INTERRUPT_GEN( or_interrupt ){
	int intleft=cpu_getiloops();
	if(intleft!=0) cpu_set_irq_line(0, 2, HOLD_LINE);
	else cpu_set_irq_line(0, 4, HOLD_LINE);
}


static MACHINE_DRIVER_START( outrun )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_MEMORY(outrun_readmem,outrun_writemem)
	MDRV_CPU_VBLANK_INT(or_interrupt,2)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(outrun_sound_readmem,outrun_sound_writemem)
	MDRV_CPU_PORTS(sound_readport,sound_writeport)

	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_MEMORY(outrun_readmem2,outrun_writemem2)
	MDRV_CPU_VBLANK_INT(sys16_interrupt,2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	MDRV_MACHINE_INIT(outrun)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK)
	MDRV_SCREEN_SIZE(40*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(sys16_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096*ShadowColorsMultiplier)

	/* initilize system16 variables prior to driver_init and video_start */
	machine_init_sys16_onetime();

	MDRV_VIDEO_START(outrun_old)
	MDRV_VIDEO_UPDATE(outrun_old)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, sys16_ym2151_interface)
	MDRV_SOUND_ADD(SEGAPCM, sys16_segapcm_interface_15k)

	MDRV_INSTALL_OST_SUPPORT(OST_SUPPORT_OUTRUN)
MACHINE_DRIVER_END

#if 0
static MACHINE_DRIVER_START( outruna )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(outrun)

	MDRV_MACHINE_INIT(outruna)
MACHINE_DRIVER_END
#endif

static MACHINE_DRIVER_START( toutrun )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_MEMORY(outrun_readmem,outrun_writemem)
	MDRV_CPU_VBLANK_INT(or_interrupt,2)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(outrun_sound_readmem,outrun_sound_writemem)
	MDRV_CPU_PORTS(sound_readport,sound_writeport)

	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_MEMORY(outrun_readmem2,outrun_writemem2)
	MDRV_CPU_VBLANK_INT(sys16_interrupt,2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	MDRV_MACHINE_INIT(outrun)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK)
	MDRV_SCREEN_SIZE(40*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(sys16_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096*ShadowColorsMultiplier)

	/* initilize system16 variables prior to driver_init and video_start */
	machine_init_sys16_onetime();

	MDRV_VIDEO_START(outrun_old)
	MDRV_VIDEO_UPDATE(outrun_old)


	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, sys16_ym2151_interface)
	MDRV_SOUND_ADD(SEGAPCM, sys16_segapcm_interface_15k_512)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( toutrun_new )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_MEMORY(toutrun_readmem,toutrun_writemem)
	MDRV_CPU_VBLANK_INT(or_interrupt,2)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(outrun_sound_readmem,outrun_sound_writemem)
	MDRV_CPU_PORTS(sound_readport,sound_writeport)

	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_MEMORY(toutrun_readmem2,toutrun_writemem2)
	MDRV_CPU_VBLANK_INT(sys16_interrupt,2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(1000000 * (262 - 224) / (262 * 60))
	MDRV_INTERLEAVE(100)

	MDRV_MACHINE_INIT(toutrun)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK)
	MDRV_SCREEN_SIZE(40*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(sys16_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096*3)

	/* initilize system16 variables prior to driver_init and video_start */
	/*machine_init_sys16_onetime(); */

	MDRV_VIDEO_START(outrun)
	MDRV_VIDEO_UPDATE(outrun)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, sys16_ym2151_interface)
	MDRV_SOUND_ADD(SEGAPCM, sys16_segapcm_interface_15k_512)
MACHINE_DRIVER_END

static data16_t *shared_ram2;
static READ16_HANDLER( shared_ram2_r ){
	return shared_ram2[offset];
}
static WRITE16_HANDLER( shared_ram2_w ){
	COMBINE_DATA(&shared_ram2[offset]);
}

/*
	ND: Hook the Super Hang-On input port 0 (buttons/coins)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
*/
static data16_t shangon_btns = 0xffff; /* ipt0 bitfield */
static data16_t sho_credits_val = 0xffff; /* any invalid value so first write is always triggered */
static READ16_HANDLER( shangon_input_port_2_word_r ) {
	data16_t buttons_ipt = readinputport(2), buttons_ret = buttons_ipt;
	if ((buttons_ipt ^ shangon_btns) & 0x10) {
		/* Start button state changed */

		/* printf("shangon: Start Button %s\n", (0 == (buttons_ipt & 0x08)) ? "Pressed" : "Released"); */
		if (0 == (buttons_ipt & 0x10) && (0 == sho_credits_val)) {
			/* Start button pressed and zero credits - Simulate coin */
			buttons_ret |= 0x10; /* Disable Start button */
			buttons_ret &= (~0x01); /* Simulate Coin*/
			/* printf("shangon: Simulate Coin In!\n"); */
		}
	}
	/* We need to save the input port state as read from the port to catch changes next time ... */
	shangon_btns = buttons_ipt;
	/* ... but we return our modfifed state to fool the game */
	return buttons_ret;
}

static MEMORY_READ16_START( shangon_readmem )
    { 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x20c640, 0x20c647, sound_shared_ram_r },
	{ 0x20c000, 0x20ffff, SYS16_MRA16_EXTRAM2 },
	{ 0x400000, 0x40ffff, SYS16_MRA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MRA16_TEXTRAM },
	{ 0x600000, 0x600fff, SYS16_MRA16_SPRITERAM },
	{ 0xa00000, 0xa00fff, SYS16_MRA16_PALETTERAM },
	{ 0xc68000, 0xc68fff, shared_ram_r },
	{ 0xc7c000, 0xc7ffff, shared_ram2_r },
	{ 0xe00002, 0xe00003, sys16_coinctrl_r },
	{ 0xe01000, 0xe01001, shangon_input_port_2_word_r },	/* service */
	{ 0xe0100c, 0xe0100d, input_port_4_word_r },			/* dip2 */
	{ 0xe0100a, 0xe0100b, input_port_3_word_r },			/* dip1 */
	{ 0xe030f8, 0xe030f9, ho_io_x_r },
	{ 0xe030fa, 0xe030fb, ho_io_y_r },
MEMORY_END

static WRITE16_HANDLER( sho_credits_w )
{
	/*
		MSB=Credits 0-9, 
		LSB=NOT(IPT2/input_port_2_word_r)
	*/

	if (ACCESSING_MSB16) { /* Credits */
		data16_t credits = (data >> 8) & 0xff;
		if (credits != sho_credits_val) {
			/* Credits changed */
			output_set_value(SHO_CREDITS_NAME, credits);
			sho_credits_val = credits;
		}
	} 
}

static WRITE16_HANDLER( sho_coin_start_toggle_w )
{
	/*
		LSB=frame ctr. 00->3C (61 frames !?!)
		0 === not in use (light off)
		01->1E == text shown (light on) for 30 frames
		1F-3C->00 = text hidden (light off) for 31 frames
		Note: 0x3D is briefly written, but re-written with 0x00 in the same frame so ignored here
		To avoid too many signals to outputs server, we modify this to 4Hz for the purposes of heartbeat blinking etc
	*/

	if (ACCESSING_LSB16) {
		data16_t frame_ctr = data & 0xff;
		switch (frame_ctr)
		{
		case 0x0f:	/* 15 */
		case 0x1e:	/* 30 */
		case 0x2d:	/* 45 */
		case 0x3c:	/* 60 */
			/*printf("shangon: start_btn frames %04x\n", frame_ctr);*/
			output_set_value(SHO_START_BTN_NAME, frame_ctr);
			break;
		default:
			break;
		}
	}
}

static WRITE16_HANDLER( sho_stage_bcd_w )
{
	/*
		LSB = stage in BCD format
	*/

	if (ACCESSING_LSB16) {
		data16_t stage = data & 0xff;
		output_set_value(SHO_STAGE_BCD_NAME, stage);
		/* printf("shangon: stage %02x\n", stage); */
	}
}

static WRITE16_HANDLER( sho_time_w )
{
	/* 
		Writing time - used in gameplay, attract, track select, music select

		MSB is BCD time in seconds (0-99)
		We don't write the output directly when this changes (unless time == 0)
		We write a 16-bit value (time/frames - MSB/LSB) on frame count in LSB change below

		LSB is frame count between seconds (0x3C-0x01 DESC 60Hz). Does NOT descend from 0x3C when MSB secs hits 1 -> 0 
		(i.e immediate game over at 0x003C. Road stops moving, etc - no 'grace last second' to reach CP!)
		frame count is only active during gameplay/attract. Not track/music select (stays at 0x00/0x3C in these cases)
		To avoid too many signals to outputs server, we modify this to 4Hz for the purposes of heartbeat blinking etc
	*/
	if (ACCESSING_MSB && 0 == ((data >> 8) & 0xff)) {
		/* 
			Special case. LSB is zero here and we get no LSB update immediately after writing MSB=0
			so we explicitly force write an output in this case
		*/
		mem_mask &= 0xff00; /* force ACCESSING_LSB16 */
	}
	if (ACCESSING_LSB16) {
		data16_t frame_count = sys16_extraram2[offset] & 0xff;
		switch (frame_count)
		{
		case 0x0f:	/* 15 */
		case 0x1e:	/* 30 */
		case 0x2d:	/* 45 */
		case 0x3c:	/* 60 */
			/*printf("shangon: time/frames %04x\n", sys16_extraram2[offset])*/;
			output_set_value(SHO_TIME_NAME, sys16_extraram2[offset]);
			break;
		default:
			break;
		}
	}
}

static data16_t sho_speed_val = 0xffff; /* any invalid speed so first write is always triggered */
static bool sho_turbo_available = true; /* to force intial write to false */
static bool sho_turbo_active = true; /* to force intial write to false */
static WRITE16_HANDLER( sho_speed_w )
{
	if (sho_speed_val != data) {
		/* Writing speed */
		bool turbo_available = false, turbo_active = false;
		/* printf("shangon: speed changed: %03x -> %03x\n", sho_speed_val, data); */
		sho_speed_val = data;
		output_set_value(SHO_SPEED_NAME, sho_speed_val); /* output speed for SHO speed calculation */

		/* Check for change in turbo available state */
		turbo_available = (sho_speed_val >= 0x280);
		if (turbo_available != sho_turbo_available) {
			sho_turbo_available = turbo_available;
			/* printf("shangon: Turbo %s!\n", turbo_available ? "Available" : "Unavailable"); */
			output_set_value(SHO_TURBO_AVAILABLE_NAME, turbo_available ? 1 : 0);
		}

		/* Check for turbo active if turbo available and IPT_BUTTON3 is pressed */
		turbo_active = (turbo_available && ((readinputport(2) & 0x20) == 0)); 
		if (turbo_active != sho_turbo_active) {
			sho_turbo_active = turbo_active;
			/* printf("shangon: Turbo %s!\n", turbo_active ? "Activated" : "Deactivated"); */
			output_set_value(SHO_TURBO_ACTIVE_NAME, turbo_active ? 1 : 0);
		}
		return;
	}
}

static WRITE16_HANDLER( sho_start_lights_w )
{
	if (ACCESSING_MSB16) {
		/* Writing Starting lights state
			MSB is state (0-4)
			LSB is frame count between states (0x00 - 0x3B ASC)
			0 = pre-start
			1,2,3 = lights in order
			4 = race started.
			Reset is odd. May stay as 4 during next attract loop
		*/
		data16_t lights_state = (data >> 8) & 0xff;
		/* printf("shangon: Start lights changed: %x\n", lights_state); */
		output_set_value(SHO_START_LIGHTS_NAME, lights_state);
	}
}



/* SHO Credits / Service port/buttons */
static const offs_t sho_credits_offset = (0x20c052 - 0x20c000) / 2;
/* SHO 'Insert Coins' & 'Push Start Button' frame ctr */
static const offs_t sho_coin_start_frame_ctr_offset = (0x20c428 - 0x20c000) / 2;
static const offs_t sho_stage_bcd_offset = (0x20c42a - 0x20c000) / 2;		/* SHO Stage (BCD) */
static const offs_t sho_time_offset = (0x20c500 - 0x20c000) / 2;			/* SHO Time */
static const offs_t sho_speed_offset = (0x20c530 - 0x20c000) / 2; 			/* SHO Speed */
static const offs_t sho_start_lights_offset = (0x20f048 - 0x20c000) / 2;	/* SHO start lights */

static WRITE16_HANDLER( sho_sys16_extraram2_w ) {
	/* Replace default memory handler by writing to mem bank */
	COMBINE_DATA( sys16_extraram2 + offset );
	if (sho_start_lights_offset == offset) { 
		sho_start_lights_w(offset, data, mem_mask); 
	} else if (sho_speed_offset == offset) {
		sho_speed_w(offset, data, mem_mask);
	} else if (sho_time_offset == offset) {
		sho_time_w(offset, data, mem_mask);
	} else if (sho_stage_bcd_offset == offset) {
		sho_stage_bcd_w(offset, data, mem_mask);
	} else if (sho_coin_start_frame_ctr_offset == offset) {
		sho_coin_start_toggle_w(offset, data, mem_mask);
	} else if (sho_credits_offset == offset) {
		sho_credits_w(offset, data, mem_mask);
	}
}

static MEMORY_WRITE16_START( shangon_writemem )
    { 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x20c640, 0x20c647, sound_shared_ram_w },
	{ 0x20c000, 0x20ffff, sho_sys16_extraram2_w, &sys16_extraram2 },
	{ 0x400000, 0x40ffff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x410000, 0x410fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x600000, 0x600fff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0xa00000, 0xa00fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0xc68000, 0xc68fff, shared_ram_w, &shared_ram },
	{ 0xc7c000, 0xc7ffff, shared_ram2_w, &shared_ram2 },
	{ 0xe00002, 0xe00003, sys16_3d_coinctrl_w },
MEMORY_END

static MEMORY_READ16_START( shangon_readmem2 )
    { 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x454000, 0x45401f, SYS16_MRA16_EXTRAM3 },
	{ 0x7e8000, 0x7e8fff, shared_ram_r },
	{ 0x7fc000, 0x7ffbff, shared_ram2_r },
	{ 0x7ffc00, 0x7fffff, SYS16_MRA16_EXTRAM },
MEMORY_END

static MEMORY_WRITE16_START( shangon_writemem2 )
    { 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x454000, 0x45401f, SYS16_MWA16_EXTRAM3, &sys16_extraram3 },
	{ 0x7e8000, 0x7e8fff, shared_ram_w },
	{ 0x7fc000, 0x7ffbff, shared_ram2_w },
	{ 0x7ffc00, 0x7fffff, SYS16_MWA16_EXTRAM, &sys16_extraram },
MEMORY_END

static MEMORY_READ_START( shangon_sound_readmem )
    { 0x0000, 0x7fff, MRA_ROM },
	{ 0xf000, 0xf7ff, SegaPCM_r },
	{ 0xf800, 0xf807, sound2_shared_ram_r },
	{ 0xf808, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( shangon_sound_writemem )
    { 0x0000, 0x7fff, MWA_ROM },
	{ 0xf000, 0xf7ff, SegaPCM_w },
	{ 0xf800, 0xf807, sound2_shared_ram_w, &sound_shared_ram },
	{ 0xf808, 0xffff, MWA_RAM },
MEMORY_END

/***************************************************************************/

static void shangon_update_proc( void ){
	set_bg_page1( sys16_textram[0x74e] );
	set_fg_page1( sys16_textram[0x74f] );
	sys16_fg_scrollx = sys16_textram[0x7fc] & 0x01ff;
	sys16_bg_scrollx = sys16_textram[0x7fd] & 0x01ff;
	sys16_fg_scrolly = sys16_textram[0x792] & 0x00ff;
	sys16_bg_scrolly = sys16_textram[0x793] & 0x01ff;
}

static MACHINE_INIT( shangon ){
	sys16_textmode=1;
	sys16_spritesystem = sys16_sprite_hangon;
	sys16_sprxoffset = -0xc0;
	sys16_fgxoffset = 8;
	sys16_textlayer_lo_min=0;
	sys16_textlayer_lo_max=0;
	sys16_textlayer_hi_min=0;
	sys16_textlayer_hi_max=0xff;

	sys16_patch_code( 0x65bd, 0xf9);
	sys16_patch_code( 0x6677, 0xfa);
	sys16_patch_code( 0x66d5, 0xfb);
	sys16_patch_code( 0x9621, 0xfb);

	sys16_update_proc = shangon_update_proc;

	sys16_gr_ver = shared_ram;
	sys16_gr_hor = sys16_gr_ver+0x200/2;
	sys16_gr_pal = sys16_gr_ver+0x400/2;
	sys16_gr_flip= sys16_gr_ver+0x600/2;

	sys16_gr_palette= 0xf80 / 2;
	sys16_gr_palette_default = 0x70 /2;
	sys16_gr_colorflip[0][0]=0x08 / 2;
	sys16_gr_colorflip[0][1]=0x04 / 2;
	sys16_gr_colorflip[0][2]=0x00 / 2;
	sys16_gr_colorflip[0][3]=0x06 / 2;
	sys16_gr_colorflip[1][0]=0x0a / 2;
	sys16_gr_colorflip[1][1]=0x04 / 2;
	sys16_gr_colorflip[1][2]=0x02 / 2;
	sys16_gr_colorflip[1][3]=0x02 / 2;
}

static DRIVER_INIT( shangon ){
	generate_gr_screen(512,1024,0,0,4,0x8000);

	sys16_patch_z80code( 0x1087, 0x20);
	sys16_patch_z80code( 0x1088, 0x01);
}

/* ND: HUD mods in shangon */
#define SHO_HUD_HIDE_ALL

#ifdef SHO_HUD_HIDE_ALL
#define SHO_HUD_HIDE_SPEED
#define SHO_HUD_HIDE_COURSE
#define SHO_HUD_HIDE_STAGE_TXT
#define SHO_HUD_HIDE_STAGE_GFX
#endif /* SHO_HUD_HIDE_ALL */

void shangon_hud_patch(UINT16 *RAM) {

#ifdef SHO_HUD_HIDE_SPEED
	/* Hide 'Speed NNN KM' by patching out call to render routines with 2x 'nop' (0x4e71) instr's */
	RAM[0x168a >> 1] = 0x4e71;	RAM[(0x168a >> 1) + 1] = 0x4e71; /* 'SPEED' & 'KM' Labels: (bsr $1da6) @ 0x168a */
	RAM[0x16d8 >> 1] = 0x4e71;	RAM[(0x16d8 >> 1) + 1] = 0x4e71; /* Speed numerals: (bsr $1e34) @ 0x16d8 */
#endif /* SHO_HUD_HIDE_SPEED */

#ifdef SHO_HUD_HIDE_COURSE
	/* Hide 'Course' by patching out call to render routine with 2x 'nop' (0x4e71) instr's*/
	RAM[0x5b46 >> 1] = 0x4e71;	RAM[(0x5b46 >> 1) + 1] = 0x4e71; /* (bsr $2956) @ 0x54bc */
#endif /* SHO_HUD_HIDE_COURSE */

#ifdef SHO_HUD_HIDE_STAGE_TXT
	/* Hide 'Stage N' text by patching out call to render routines with 2x 'nop' (0x4e71) instr's*/
	RAM[0x5b8c >> 1] = 0x4e71;	RAM[(0x5b8c >> 1) + 1] = 0x4e71; /* 'Stage' label: (bsr $2956) @ 0x5b8c */
	RAM[0x5b9e >> 1] = 0x4e71;	RAM[(0x5b9e >> 1) + 1] = 0x4e71; /* digit render via bsr $2a1c @ 0x5b9e */
#endif /* SHO_HUD_HIDE_STAGE_TXT */

#ifdef SHO_HUD_HIDE_STAGE_GFX
	/* Hide Stage graphic by patching out call to render routines with 2x 'nop' (0x4e71) instr's*/
	RAM[0x64ec >> 1] = 0x4e71;	RAM[(0x64ec >> 1) + 1] = 0x4e71; /* 'Outline' :	bsr $5f22 @ 0x64ec */
	RAM[0x5c08 >> 1] = 0x4e71;	RAM[(0x5c08 >> 1) + 1] = 0x4e71; /* 'Fill' :	bsr $5f66 @ 0x5c08 */
#endif /* SHO_HUD_HIDE_STAGE_GFX */

}

static DRIVER_INIT( shangonb ){
	generate_gr_screen(512,1024,8,0,4,0x8000);
	output_init("shangonb");

	/* **************** *
	 * ND: ROM patching *
	 * **************** */

	/* 
		Note: when patching text we work 'WORD-wise' (2 chars == 16-bits at a time) due to endianness differences: 
		x86, amd64 & ARM (hosts) are all Little Endian (LE), but 68000 (emulated hw) is Big Endian (BE).
		So text in our host RAM is byte swapped vs what you see in MAME debugger which translates for us
		Libretro obviously handles this endianness translation at runtime.
		When we write a 16-bit word through a UINT16 *ptr in C, the compiler will generate byte-swapped code for our host's LE layout
		So the bytes will actually end up in the correct BE order for the emulator.
		i.e. We DO NOT need to manually swap the odd/even chars if we write through a UINT16* ptr
		but we WOULD have to manually swap the odd/even chars if we write through UINT8*
	*/

	{
	/* Get ptr to ROM data in RAM */
	UINT16 *RAM = (UINT16 *)memory_region(REGION_CPU1);

	/* "Insert Coins" -> "Push Start Button" - mod addr's in render routine at 0x1a60 */
	RAM[0x1a64 >> 1] = 0x0746;	/* Patch textram destination address in A0 (2 places to left to centre longer text)*/
	RAM[0x1a6a >> 1] = 0x155e;	/* Patch ROM address in A1 of text to render from IC (0x006e) to PSB (0x155e) */

	/* Knock out 'Credit(s)' text and num */
	RAM[0x28de >> 1] = 0x4e71;	RAM[(0x28de >> 1) + 1] = 0x4e71; /* 'CREDIT[S]' text: bsr $2956 @ 0x28de -> nop for text render*/
	RAM[0x28e2 >> 1] = 0x4e75;	/* replace lea $410cc4.l,A0  with early rts to skip render digit at 0x28e2 */
	RAM[(0x28e2 >> 1) + 1] = 0x4e71;	RAM[(0x28e2 >> 1) + 2] = 0x4e71; /* Fill double word gap left from previous lea with nop */
	
	/* Selective HUD patches */
	shangon_hud_patch(RAM);
	}	
}
/***************************************************************************/

INPUT_PORTS_START( shangon )
PORT_START	/* Steering */
	PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_X | IPF_REVERSE | IPF_CENTER , 100, 3, 0x42, 0xbd )

#ifdef HANGON_DIGITAL_CONTROLS

PORT_START	/* Buttons */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 )

#else

PORT_START	/* Accel / Decel */
	PORT_ANALOG( 0xff, 0x1, IPT_AD_STICK_Y | IPF_CENTER | IPF_REVERSE, 100, 16, 1, 0xa2 )

#endif

PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	SYS16_COINAGE

PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x04, "Easy" )
	PORT_DIPSETTING(    0x06, "Normal" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x18, 0x18, "Time Adj." )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x18, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x20, 0x20, "Play Music" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )


#ifndef HANGON_DIGITAL_CONTROLS

PORT_START	/* Brake */
	PORT_ANALOG( 0xff, 0x1, IPT_AD_STICK_Y | IPF_PLAYER2 | IPF_CENTER | IPF_REVERSE, 100, 16, 1, 0xa2 )

#endif
INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( shangon )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_MEMORY(shangon_readmem,shangon_writemem)
	MDRV_CPU_VBLANK_INT(sys16_interrupt,1)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(shangon_sound_readmem,shangon_sound_writemem)
	MDRV_CPU_PORTS(sound_readport,sound_writeport)

	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_MEMORY(shangon_readmem2,shangon_writemem2)
	MDRV_CPU_VBLANK_INT(sys16_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(100)

	MDRV_MACHINE_INIT(shangon)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK)
	MDRV_SCREEN_SIZE(40*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(sys16_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048*ShadowColorsMultiplier)

	/* initilize system16 variables prior to driver_init and video_start */
	machine_init_sys16_onetime();

	MDRV_VIDEO_START(hangon)
	MDRV_VIDEO_UPDATE(hangon)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, sys16_ym2151_interface)
	MDRV_SOUND_ADD(SEGAPCM, sys16_segapcm_interface_15k_512)
MACHINE_DRIVER_END

GAMEX(1992, shangon,  0,        shangon,  shangon,  shangon,  ROT0,         "Sega",    "Super Hang-On", GAME_NOT_WORKING )
GAME( 1992, shangonb, shangon,  shangon,  shangon,  shangonb, ROT0,         "bootleg", "Super Hang-On (bootleg)" )

GAME( 1986, outrun,   0,        outrun,   outrun,   outrun,   ROT0,         "Sega",    "Out Run (set 1)" )
GAME( 1986, outruna,  outrun,   outrun,   outrun,   outrun,   ROT0,         "Sega",    "Out Run (set 2)" )
GAME( 1986, outrunb,  outrun,   outrun,   outrun,   outrunb,  ROT0,         "bootleg", "Out Run (set 3)" )
GAME( 1989, toutrun,  0,        toutrun,  toutrun,  toutrun,  ROT0,         "Sega",    "Turbo Out Run (Out Run upgrade) (bootleg of FD1094 317-0118 set)" ) /* decrypted */
GAME( 1989, toutrun3, toutrun,  toutrun,  toutrun,  toutrun,  ROT0,         "Sega",    "Turbo Out Run (cockpit) (bootleg of FD1094 317-0107 set)" ) /* decrypted */

