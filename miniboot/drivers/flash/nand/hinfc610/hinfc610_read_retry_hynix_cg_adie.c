/******************************************************************************
 *    NAND Flash Controller V610 Device Driver
 *    Copyright (c) 2009-2010 by Hisilicon.
 *    All rights reserved.
 * ***
 *    Create By Czyong.
 *    
******************************************************************************/
#include <stdio.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <asm/io.h>

#include "hinfc610.h"

/*****************************************************************************/

static char *hinfc610_hynix_cg_adie_otp_check(const char otp[8 * 128])
{
	int index = 0;
	int ix, jx;
	char *ptr = NULL;
	int min, cur;
	char *otp_origin, *otp_inverse;

	min = 64;
	for (ix = 0; ix < 8; ix++) {

		otp_origin  = (char *)otp + ix * 128;
		otp_inverse = otp_origin + 64;
		cur = 0;

		for (jx = 0; jx < 64; jx++, otp_origin++, otp_inverse++) {
			if (((*otp_origin) ^ (*otp_inverse)) == 0xFF)
				continue;
			cur ++;
		}

		if (cur < min) {
			min = cur;
			index = ix;
			ptr = (char *)otp + ix * 128;
			if (!cur)
				break;
		}
	}

	printf("RR select parameter %d from %d, error %d\n",
		index, ix, min);
	return ptr;
}
/*****************************************************************************/

static int hinfc610_hynix_cg_adie_get_rr_param(struct hinfc_host *host)
{
	char *otp;

	host->set_ecc_randomizer(host, DISABLE, DISABLE);
	/* step1: reset the chip */
	host->send_cmd_reset(host, host->chipselect);

	/* step2: cmd: 0x36, address: 0xFF, data: 0x40 */
	hinfc_write(host, 1, HINFC610_DATA_NUM);/* data length 1 */
	writel(0x40, host->iobase); /* data: 0x00 */
	hinfc_write(host, 0xFF, HINFC610_ADDRL);/* address: 0xAE */
	hinfc_write(host, 0x36, HINFC610_CMD);  /* cmd: 0x36 */
	/*
	 * no need to config HINFC610_OP_WAIT_READY_EN, here not config this bit.
	 */	
	hinfc_write(host, HINFC610_WRITE_1CMD_1ADD_DATA, HINFC610_OP);
	WAIT_CONTROLLER_FINISH();

	/* step3: address: 0xCC, data: 0x4D */
	hinfc_write(host, 1, HINFC610_DATA_NUM);/* data length 1 */
	writel(0x4D, host->iobase); /* data: 0x4d */
	hinfc_write(host, 0xCC, HINFC610_ADDRL);/* address: 0xB0 */
	/*
	 * no need to config HINFC610_OP_WAIT_READY_EN, here not config this bit.
	 * only address and data, without cmd
	 */	
	hinfc_write(host, HINFC610_WRITE_0CMD_1ADD_DATA, HINFC610_OP);
	WAIT_CONTROLLER_FINISH();

	/* step4: cmd: 0x16, 0x17, 0x04, 0x19 */
	hinfc_write(host, 0x17 << 8 | 0x16, HINFC610_CMD);
	/*
	 * no need to config HINFC610_OP_WAIT_READY_EN, here not config this bit.
	 */
	hinfc_write(host, HINFC610_WRITE_2CMD_0ADD_NODATA, HINFC610_OP);
	WAIT_CONTROLLER_FINISH();

	hinfc_write(host, 0x19 << 8 | 0x04, HINFC610_CMD);
	/*
	 * no need to config HINFC610_OP_WAIT_READY_EN, here not config this bit.
	 */
	hinfc_write(host, HINFC610_WRITE_2CMD_0ADD_NODATA, HINFC610_OP);
	WAIT_CONTROLLER_FINISH();

	/* step5: cmd: 0x00 0x30, address: 0x02 00 00 00 */
	hinfc_write(host, 0x2000000, HINFC610_ADDRL);
	hinfc_write(host, 0x00, HINFC610_ADDRH);
	hinfc_write(host, 0x30 << 8 | 0x00, HINFC610_CMD);
	hinfc_write(host, 0x800, HINFC610_DATA_NUM);
	/*
	 * need to config HINFC610_OP_WAIT_READY_EN, here config this bit.
	 */
	hinfc_write(host, HINFC610_READ_2CMD_5ADD, HINFC610_OP);
	WAIT_CONTROLLER_FINISH();

	/*step6 save otp read retry table to mem*/
	otp = hinfc610_hynix_cg_adie_otp_check(host->iobase + 2);
	if (!otp) {
		printf("Read Retry select parameter failed, "
		       "this Nand Chip maybe unstable!\n");
		return -1;
	}
	memcpy(host->rr_data, otp, 64);
	host->need_rr_data = 1;

	/* step7: reset the chip */
	host->send_cmd_reset(host, host->chipselect);

	/* step8: cmd: 0x38 */
	hinfc_write(host, 0x38, HINFC610_CMD);
	/*
	 * need to config HINFC610_OP_WAIT_READY_EN, here config this bit.
	 */
	hinfc_write(host, HINFC610_WRITE_1CMD_0ADD_NODATA_WAIT_READY, HINFC610_OP);
	WAIT_CONTROLLER_FINISH();

	host->set_ecc_randomizer(host, ENABLE, ENABLE);
	/* get hynix otp table finish */
	return 0;
}
/*****************************************************************************/
static char hinfc610_hynix_cg_adie__rr_reg[8] = {
	0xCC, 0xBF, 0xAA, 0xAB, 0xCD, 0xAD, 0xAE, 0xAF};

static int hinfc610_hynix_cg_adie_set_rr_reg(struct hinfc_host *host, char *val)
{
	int i;
	
	host->set_ecc_randomizer(host, DISABLE, DISABLE);
	hinfc_write(host, 1, HINFC610_DATA_NUM);/* data length 1 */

	for (i = 0; i <= 8; i++) {
		switch (i) {
		case 0:
			writel(val[i], host->iobase);
			hinfc_write(host, 
				hinfc610_hynix_cg_adie__rr_reg[i], HINFC610_ADDRL);
			hinfc_write(host,
				0x36, HINFC610_CMD);
			/*
			 * no need to config HINFC610_OP_WAIT_READY_EN, here not config this bit.
			 */
			hinfc_write(host,
				HINFC610_WRITE_1CMD_1ADD_DATA,
				HINFC610_OP);
			break;
		case 8:
			hinfc_write(host,
				0x16, HINFC610_CMD);
			/*
			 * only have 1 cmd: 0x16
			 * no need to config HINFC610_OP_WAIT_READY_EN, here not config this bit.
			 */
			hinfc_write(host,
				HINFC610_WRITE_1CMD_0ADD_NODATA,
				HINFC610_OP);
			break;
		default:
			writel(val[i], host->iobase);
			hinfc_write(host, 
				hinfc610_hynix_cg_adie__rr_reg[i], HINFC610_ADDRL);
			/*
			 * no need to config HINFC610_OP_WAIT_READY_EN, here not config this bit.
			 */
			hinfc_write(host,
				HINFC610_WRITE_0CMD_1ADD_DATA,
				HINFC610_OP);
			break;
		}
		WAIT_CONTROLLER_FINISH();
	}
	host->set_ecc_randomizer(host, ENABLE, ENABLE);
	return 0;
}
/*****************************************************************************/
# if 0
static void hinfc610_hynix_cg_adie_get_rr(struct hinfc_host *host)
{
	int index;

	host->set_ecc_randomizer(host, DISABLE, DISABLE);
	hinfc_write(host, 1, HINFC610_DATA_NUM);

	for (index = 0; index < 8; index++) {
		memset(host->iobase, 0xff, 1024);
		hinfc_write(host, 
			0x37, HINFC610_CMD);
		hinfc_write(host, 
			hinfc610_hynix_cg_adie__rr_reg[index], 
			HINFC610_ADDRL);
		hinfc_write(host, 
			HINFC610_READ_1CMD_1ADD_DATA, 
			HINFC610_OP);

		WAIT_CONTROLLER_FINISH();
	
		printf("%02X ", readl(host->iobase) & 0xff);
	}

	host->set_ecc_randomizer(host, ENABLE, ENABLE);
}
#endif
/*****************************************************************************/

static int hinfc610_hynix_cg_adie_set_rr_param(struct hinfc_host *host, int param)
{
	unsigned char *rr;

	if (!(host->rr_data[0] | host->rr_data[1]
	    | host->rr_data[2] | host->rr_data[3]) || !param)
		return -1;

	rr = (unsigned char *)&host->rr_data[((param & 0x07) << 3)];

	/* set the read retry regs to adjust reading level */
	return hinfc610_hynix_cg_adie_set_rr_reg(host, (char *)rr);
}
/*****************************************************************************/

static int hinfc610_hynix_cg_adie_reset_rr_param(struct hinfc_host *host)
{
	return hinfc610_hynix_cg_adie_set_rr_param(host, 0);
}
/*****************************************************************************/

static char HYNIX_CG_ADIE_ESLC_PARAM[4];

static int hinfc610_hynix_cg_adie_eslc_enable(struct hinfc_host *host, int enable)
{
	int ix;
	char HYNIX_CG_ADIE_ESLC_REG[4] = {0xB0, 0xB1, 0xA0, 0xA1};

	host->set_ecc_randomizer(host, DISABLE, DISABLE);
	hinfc_write(host, 1, HINFC610_DATA_NUM);

	for (ix = 0; enable && ix < 4; ix++) {
		memset(host->iobase, 0xff, 32);
		hinfc_write(host, 
			0x37, HINFC610_CMD);
		hinfc_write(host, 
			HYNIX_CG_ADIE_ESLC_REG[ix], 
			HINFC610_ADDRL);
		/*
		 * no need to config HINFC610_OP_WAIT_READY_EN, here not config this bit.
		 */
		hinfc_write(host, 
			HINFC610_READ_1CMD_1ADD_DATA, 
			HINFC610_OP);
		WAIT_CONTROLLER_FINISH();

		HYNIX_CG_ADIE_ESLC_PARAM[ix] = 
			(char)(readl(host->iobase) & 0xff);
	}

	for (ix = 0; ix <= 4; ix++) {
		int offset = enable ? 0x0A : 0x00;
		switch (ix) {
		case 0:
			writel(HYNIX_CG_ADIE_ESLC_PARAM[ix] + offset, 
				host->iobase);
			hinfc_write(host, 
				HYNIX_CG_ADIE_ESLC_REG[ix], 
				HINFC610_ADDRL);
			hinfc_write(host,
				0x36, HINFC610_CMD);
			/*
			 * no need to config HINFC610_OP_WAIT_READY_EN, here not config this bit.
			 */
			hinfc_write(host,
				HINFC610_WRITE_1CMD_1ADD_DATA,
				HINFC610_OP);
			break;
		case 4:
			hinfc_write(host,
				0x16, HINFC610_CMD);
			/*
			 * only have 1 cmd: 0x16
			 * no need to config HINFC610_OP_WAIT_READY_EN, here not config this bit.
			 */
			hinfc_write(host,
				HINFC610_WRITE_1CMD_0ADD_NODATA,
				HINFC610_OP);
			break;
		default:
			writel(HYNIX_CG_ADIE_ESLC_PARAM[ix] + offset, 
				host->iobase);
			hinfc_write(host, 
				HYNIX_CG_ADIE_ESLC_REG[ix], 
				HINFC610_ADDRL);
			/*
			 * no need to config HINFC610_OP_WAIT_READY_EN, here not config this bit.
			 */
			hinfc_write(host,
				HINFC610_WRITE_0CMD_1ADD_DATA,
				HINFC610_OP);
			break;
		}
		WAIT_CONTROLLER_FINISH();
	}

	host->set_ecc_randomizer(host, ENABLE, ENABLE);

	if (!enable) {
		hinfc_write(host, 
			host->addr_value[0] & 0xffff0000, 
			HINFC610_ADDRL);
		hinfc_write(host, 
			host->addr_value[1], 
			HINFC610_ADDRH);
		hinfc_write(host, 
			NAND_CMD_READSTART << 8 | NAND_CMD_READ0,
			HINFC610_CMD);
		/*
		 * need to config HINFC610_OP_WAIT_READY_EN, here config this bit.
		 */		
		hinfc_write(host,
			HINFC610_READ_2CMD_5ADD,
			HINFC610_OP);

		WAIT_CONTROLLER_FINISH();
	}

	return 0;
}
/*****************************************************************************/

struct read_retry_t hinfc610_hynix_cg_adie_read_retry = {
	.type = NAND_RR_HYNIX_CG_ADIE,
	.count = 8,
	.set_rr_param = hinfc610_hynix_cg_adie_set_rr_param,
	.get_rr_param = hinfc610_hynix_cg_adie_get_rr_param,
	.reset_rr_param = hinfc610_hynix_cg_adie_reset_rr_param,
	.enable_enhanced_slc = hinfc610_hynix_cg_adie_eslc_enable,
};
