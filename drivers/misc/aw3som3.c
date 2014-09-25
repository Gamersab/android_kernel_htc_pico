/*
 * Copyright 2014 Vineeth Raj <contact.twn@openmailbox.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/miscdevice.h>


#include <asm/mach/flash.h> //flash_platform_data

#include <linux/mtd/nand.h> //idk
#include <linux/mtd/partitions.h> //msm_nand_data.parts

extern struct flash_platform_data msm_nand_data;

#define MSM_MAX_PARTITIONS 8

//static struct mtd_partition msm_nand_data.parts[MSM_MAX_PARTITIONS];
//static char msm_nand_names[MSM_MAX_PARTITIONS * 16];

int configured = 0;


static unsigned int boost_val = 0;
static unsigned boot_part = 0;
static unsigned system_part = 0;
static unsigned cache_part = 0;
static unsigned userdata_part = 0;
static unsigned devlog_part = 0;
static unsigned misc_part = 0;


//awsome='aaa:aaa:bbb:bbb:ccc:ccc'
//if greater than 500, we'll assume subtraction
//we have a lot of tiem to rework algo, so no probs.
//fak this. we can't read from ramdisk

static int configure_awsome(void)
{
	int err = 0;

	printk(KERN_WARNING "awsome: is awsome\n");

	//struct mtd_partition *ptn = msm_nand_data.parts;
	//char *name = msm_nand_names;
	unsigned count, n;

	count = msm_nand_data.nr_parts;

	for (n = 0; n < count; n++) {
		//msm_nand_data.parts[n].name
		//msm_nand_data.parts[n].offset

		printk(KERN_INFO "Partition (for awsome) %s "
				"-- Offset:%llx Size:%llx\n",
				msm_nand_data.parts[n].name, msm_nand_data.parts[n].offset, msm_nand_data.parts[n].size);
		
		if (!(strcmp(msm_nand_data.parts[n].name, "boot")))
			boot_part = n;
		if (!(strcmp(msm_nand_data.parts[n].name, "system")))
			system_part = n;
		if (!(strcmp(msm_nand_data.parts[n].name, "cache")))
			cache_part = n;
		if (!(strcmp(msm_nand_data.parts[n].name, "userdata")))
			userdata_part = n;
		if (!(strcmp(msm_nand_data.parts[n].name, "devlog")))
			devlog_part = n;
		if (!(strcmp(msm_nand_data.parts[n].name, "misc")))
			misc_part = n;

	}

#define CACHE_SIZE_LEAVE 8
#define USERDATA_SIZE_LEAVE 4
#define DEVLOG_SIZE_LEAVE 1

	/* First check if we have all the partitions!
	 * Assuming it is a pico, it *should* have boot, system,
	 * cache, userdata, and devlog(?).
	 */
	if ( (!(boot_part)) || (!(system_part)) || (!(cache_part)) || (!(userdata_part)) || (!(devlog_part)) || (misc_part!=0) ) {
		printk(KERN_INFO "aw3som3: *NOT* modifying partition table!\n");
		printk(KERN_INFO "aw3som3: One or more partitions *NOT* found!\n");
		//msm_nand_data.parts = msm_nand_data.parts;
		return 1;
	}

	/* Lets assume all Pico's have a standard 4 mB boot partition.
	 * This will help us determine how much 1 mB takes up.
	 */
	unsigned one_mb = 0;
	for (one_mb = 0, n = 0; n < msm_nand_data.parts[boot_part].size; one_mb++, n+=4) ;

	// Ctrl + x, Ctrl + v
	msm_nand_data.parts[system_part].size += msm_nand_data.parts[devlog_part].size - (DEVLOG_SIZE_LEAVE * one_mb);
	msm_nand_data.parts[devlog_part].size = DEVLOG_SIZE_LEAVE * one_mb;
	msm_nand_data.parts[system_part].size += msm_nand_data.parts[cache_part].size- (CACHE_SIZE_LEAVE * one_mb);
	msm_nand_data.parts[cache_part].size = CACHE_SIZE_LEAVE * one_mb;
	msm_nand_data.parts[system_part].size += msm_nand_data.parts[userdata_part].size- (USERDATA_SIZE_LEAVE * one_mb);
	msm_nand_data.parts[userdata_part].size = USERDATA_SIZE_LEAVE * one_mb;

	/* fix offsets.
	 * rewrite the table the way we want it to be.
	 * table: 0->misc::1->recovery::2->boot::3->system::4->cache::5->userdata::6->devlog
	 * actual layout: recovery::boot::system::cache::devlog::userdata::misc
	 * new layout   : recovery::boot::misc::devlog::userdata::cache::system
	 */
	msm_nand_data.parts[misc_part].offset = msm_nand_data.parts[boot_part].offset + msm_nand_data.parts[boot_part].size;
	msm_nand_data.parts[devlog_part].offset = msm_nand_data.parts[misc_part].offset + msm_nand_data.parts[misc_part].size;
	msm_nand_data.parts[userdata_part].offset = msm_nand_data.parts[devlog_part].offset + msm_nand_data.parts[devlog_part].size;
	msm_nand_data.parts[cache_part].offset = msm_nand_data.parts[userdata_part].offset + msm_nand_data.parts[userdata_part].size;
	msm_nand_data.parts[system_part].offset = msm_nand_data.parts[cache_part].offset + msm_nand_data.parts[cache_part].size;

	//printk (KERN_WARNING "awsome: volume boost says %x\n", boost_val);

	return 0;
}

/*
static ssize_t volume_boost_get(struct device *dev,
			struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%u\n", boost_val);
}

static ssize_t volume_boost_set(struct device * dev,
		struct device_attribute * attr, const char * buf, size_t size)
{
	unsigned int val = 0;

	sscanf(buf, "%u\n", &val);

	printk(KERN_WARNING "awsome: sets data");

	if ( ( val < 0 ) || ( val > 8 ) )
		boost_val = 0;
	else
		boost_val = val;

	return size;
}

static DEVICE_ATTR(volume_boost,  S_IRUGO | S_IWUGO,
					volume_boost_get, volume_boost_set);

*/

static int __init init_awsome(void)
{
	/* Already configured? */
	if (configured == 1)
		return 0;

	if (configured == -1)
		return 0;

	//struct kobject *awsome_control_kobj;
	//awsome_control_kobj = kobject_create_and_add("awsome", NULL);
    //
	//int ret = sysfs_create_file(awsome_control_kobj,
	//		&dev_attr_volume_boost.attr);

	return configure_awsome();
}

//dangerous. but needed. postcore_initcall maybe?
//todo: check this later.
core_initcall(init_awsome);
