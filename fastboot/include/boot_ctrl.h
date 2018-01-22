#ifndef _BOOT_CTRL_H_
#define _BOOT_CTRL_H_

#define BOOT_NORMAL 0
#define BOOT_UPDATE 1

#define PARTITION_PRIMARY 0
#define PARTITION_BACKUP 1

#define UPDATE_FINISH 0
#define UPDATE_RUNNING 1
#define UPDATE_FAILURE 2

struct partition_update_info {
	unsigned char update_flag;
	unsigned char which_partition;
	unsigned char partition_update_state;
} __attribute__((__packed__));

struct boot_info {
	struct partition_update_info kernel;
	struct partition_update_info rootfs;
	struct partition_update_info appfs;
} __attribute__((__packed__));

struct visking_product_info {
	unsigned char magic[4]; // 4
	unsigned long chip_type;  // 5
	unsigned long hw_version;
	unsigned long product_model;
	unsigned long product_version;
	unsigned long product_disk; // 12
	unsigned long product_channel; // 16
	unsigned long product_display;  // 20
	unsigned long product_alarmout;
	unsigned long product_alarmin;
	unsigned long product_serial;
	unsigned long product_screen;
	unsigned long product_reserv[6];
	unsigned char product_language[4]; // 24
	unsigned char product_feature[12]; // 36
	unsigned char product_oem; // 37
	unsigned char reserve[3];   // 40
	unsigned char macaddr[6];   // 46
	unsigned char macaddr2[6];   // 52
	unsigned char serial_number[10]; // 62
	unsigned char cpyto_at88[16];   // 78
	struct boot_info how_boot; 
	unsigned char reserve2[35];
	unsigned long crc32;
} __attribute__((__packed__));

#endif