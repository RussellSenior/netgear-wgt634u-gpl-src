/*
 * sbp2.c - SBP-2 protocol driver for IEEE-1394
 *
 * Copyright (C) 2000 James Goodwin, Filanet Corporation (www.filanet.com)
 * jamesg@filanet.com (JSG)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */



/*
 * Includes
 */

#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/blk.h>
#include <linux/smp_lock.h>
#include <linux/init.h>
#include <linux/blk.h>
#include <asm/current.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/byteorder.h>
#include <asm/atomic.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/scatterlist.h>

#ifdef CONFIG_KBUILD_2_5
#include <scsi.h>
#include <hosts.h>
#include <sd.h>
#else
#include "../scsi/scsi.h"
#include "../scsi/hosts.h"
#include "../scsi/sd.h"
#endif

#include "ieee1394.h"
#include "ieee1394_types.h"
#include "ieee1394_core.h"
#include "hosts.h"
#include "nodemgr.h"
#include "highlevel.h"
#include "ieee1394_transactions.h"
#include "ieee1394_hotplug.h"
#include "sbp2.h"

static char version[] __devinitdata =
	"$Rev: 584 $ James Goodwin <jamesg@filanet.com>";

/*
 * Module load parameter definitions
 */

/*
 * Change sbp2_max_speed on module load if you have a bad IEEE-1394
 * controller that has trouble running 2KB packets at 400mb.
 *
 * NOTE: On certain OHCI parts I have seen short packets on async transmit
 * (probably due to PCI latency/throughput issues with the part). You can
 * bump down the speed if you are running into problems.
 *
 * Valid values:
 * sbp2_max_speed = 2 (default: max speed 400mb)
 * sbp2_max_speed = 1 (max speed 200mb)
 * sbp2_max_speed = 0 (max speed 100mb)
 */
MODULE_PARM(sbp2_max_speed,"i");
MODULE_PARM_DESC(sbp2_max_speed, "Force max speed (2 = 400mb default, 1 = 200mb, 0 = 100mb)");
static int sbp2_max_speed = SPEED_400;

/*
 * Set sbp2_serialize_io to 1 if you'd like only one scsi command sent
 * down to us at a time (debugging). This might be necessary for very
 * badly behaved sbp2 devices.
 */
MODULE_PARM(sbp2_serialize_io,"i");
MODULE_PARM_DESC(sbp2_serialize_io, "Serialize all I/O coming down from the scsi drivers (default = 0)");
static int sbp2_serialize_io = 0;	/* serialize I/O - available for debugging purposes */

/*
 * Bump up sbp2_max_sectors if you'd like to support very large sized
 * transfers. Please note that some older sbp2 bridge chips are broken for
 * transfers greater or equal to 128KB.  Default is a value of 255
 * sectors, or just under 128KB (at 512 byte sector size). I can note that
 * the Oxsemi sbp2 chipsets have no problems supporting very large
 * transfer sizes.
 */
MODULE_PARM(sbp2_max_sectors,"i");
MODULE_PARM_DESC(sbp2_max_sectors, "Change max sectors per I/O supported (default = 255)");
static int sbp2_max_sectors = SBP2_MAX_SECTORS;

/*
 * Adjust sbp2_max_outstanding_cmds to tune performance if you have many
 * sbp2 devices attached (or if you need to do some debugging).
 */
MODULE_PARM(sbp2_max_outstanding_cmds,"i");
MODULE_PARM_DESC(sbp2_max_outstanding_cmds, "Change max outstanding concurrent commands (default = 8)");
static int sbp2_max_outstanding_cmds = SBP2SCSI_MAX_OUTSTANDING_CMDS;

/*
 * Adjust sbp2_max_cmds_per_lun to tune performance. Enabling more than
 * one concurrent/linked command per sbp2 device may allow some
 * performance gains, but some older sbp2 devices have firmware bugs
 * resulting in problems when linking commands... so, enable this with
 * care.  I can note that the Oxsemi OXFW911 sbp2 chipset works very well
 * with large numbers of concurrent/linked commands.  =)
 */
MODULE_PARM(sbp2_max_cmds_per_lun,"i");
MODULE_PARM_DESC(sbp2_max_cmds_per_lun, "Change max concurrent commands per sbp2 device (default = 1)");
static int sbp2_max_cmds_per_lun = SBP2SCSI_MAX_CMDS_PER_LUN;

/*
 * Exclusive login to sbp2 device? In most cases, the sbp2 driver should
 * do an exclusive login, as it's generally unsafe to have two hosts
 * talking to a single sbp2 device at the same time (filesystem coherency,
 * etc.). If you're running an sbp2 device that supports multiple logins,
 * and you're either running read-only filesystems or some sort of special
 * filesystem supporting multiple hosts, then set sbp2_exclusive_login to
 * zero. Note: The Oxsemi OXFW911 sbp2 chipset supports up to four
 * concurrent logins.
 */
MODULE_PARM(sbp2_exclusive_login,"i");
MODULE_PARM_DESC(sbp2_exclusive_login, "Exclusive login to sbp2 device (default = 1)");
static int sbp2_exclusive_login = 1;

/*
 * SCSI inquiry hack for really badly behaved sbp2 devices. Turn this on
 * if your sbp2 device is not properly handling the SCSI inquiry command.
 * This hack makes the inquiry look more like a typical MS Windows
 * inquiry.
 * 
 * If sbp2_force_inquiry_hack=1 is required for your device to work,
 * please submit the logged sbp2_firmware_revision value of this device to
 * the linux1394-devel mailing list.
 */
MODULE_PARM(sbp2_force_inquiry_hack,"i");
MODULE_PARM_DESC(sbp2_force_inquiry_hack, "Force SCSI inquiry hack (default = 0)");
static int sbp2_force_inquiry_hack = 0;


/*
 * Export information about protocols/devices supported by this driver.
 */
static struct ieee1394_device_id sbp2_id_table[] = {
	{
		.match_flags =IEEE1394_MATCH_SPECIFIER_ID |
		              IEEE1394_MATCH_VERSION,
		.specifier_id = SBP2_UNIT_SPEC_ID_ENTRY & 0xffffff,
		.version =    SBP2_SW_VERSION_ENTRY & 0xffffff
	},
	{ }
};

MODULE_DEVICE_TABLE(ieee1394, sbp2_id_table);

/*
 * Debug levels, configured via kernel config, or enable here.
 */

/* #define CONFIG_IEEE1394_SBP2_DEBUG_ORBS */
/* #define CONFIG_IEEE1394_SBP2_DEBUG_DMA */
/* #define CONFIG_IEEE1394_SBP2_DEBUG 1 */
/* #define CONFIG_IEEE1394_SBP2_DEBUG 2 */
/* #define CONFIG_IEEE1394_SBP2_PACKET_DUMP */

#ifdef CONFIG_IEEE1394_SBP2_DEBUG_ORBS
#define SBP2_ORB_DEBUG(fmt, args...)	HPSB_ERR("sbp2(%s): "fmt, __FUNCTION__, ## args)
static u32 global_outstanding_command_orbs = 0;
#define outstanding_orb_incr global_outstanding_command_orbs++
#define outstanding_orb_decr global_outstanding_command_orbs--
#else
#define SBP2_ORB_DEBUG(fmt, args...)
#define outstanding_orb_incr
#define outstanding_orb_decr
#endif

#ifdef CONFIG_IEEE1394_SBP2_DEBUG_DMA
#define SBP2_DMA_ALLOC(fmt, args...) \
	HPSB_ERR("sbp2(%s)alloc(%d): "fmt, __FUNCTION__, \
		 ++global_outstanding_dmas, ## args)
#define SBP2_DMA_FREE(fmt, args...) \
	HPSB_ERR("sbp2(%s)free(%d): "fmt, __FUNCTION__, \
		 --global_outstanding_dmas, ## args)
static u32 global_outstanding_dmas = 0;
#else
#define SBP2_DMA_ALLOC(fmt, args...)
#define SBP2_DMA_FREE(fmt, args...)
#endif

#if CONFIG_IEEE1394_SBP2_DEBUG >= 2
#define SBP2_DEBUG(fmt, args...)	HPSB_ERR("sbp2: "fmt, ## args)
#define SBP2_INFO(fmt, args...)		HPSB_ERR("sbp2: "fmt, ## args)
#define SBP2_NOTICE(fmt, args...)	HPSB_ERR("sbp2: "fmt, ## args)
#define SBP2_WARN(fmt, args...)		HPSB_ERR("sbp2: "fmt, ## args)
#elif CONFIG_IEEE1394_SBP2_DEBUG == 1
#define SBP2_DEBUG(fmt, args...)	HPSB_DEBUG("sbp2: "fmt, ## args)
#define SBP2_INFO(fmt, args...)		HPSB_INFO("sbp2: "fmt, ## args)
#define SBP2_NOTICE(fmt, args...)	HPSB_NOTICE("sbp2: "fmt, ## args)
#define SBP2_WARN(fmt, args...)		HPSB_WARN("sbp2: "fmt, ## args)
#else 
#define SBP2_DEBUG(fmt, args...)
#define SBP2_INFO(fmt, args...)		HPSB_INFO("sbp2: "fmt, ## args)
#define SBP2_NOTICE(fmt, args...)       HPSB_NOTICE("sbp2: "fmt, ## args)
#define SBP2_WARN(fmt, args...)         HPSB_WARN("sbp2: "fmt, ## args)
#endif

#define SBP2_ERR(fmt, args...)		HPSB_ERR("sbp2: "fmt, ## args)

/*
 * Spinlock debugging stuff.
 */
#define SBP2_USE_REAL_SPINLOCKS

#ifdef SBP2_USE_REAL_SPINLOCKS
#define sbp2_spin_lock(lock, flags)	spin_lock_irqsave(lock, flags)	
#define sbp2_spin_unlock(lock, flags)	spin_unlock_irqrestore(lock, flags);
static spinlock_t sbp2_host_info_lock = SPIN_LOCK_UNLOCKED;
#else
#define sbp2_spin_lock(lock, flags)	do {save_flags(flags); cli();} while (0)	
#define sbp2_spin_unlock(lock, flags)	do {restore_flags(flags);} while (0)
#endif

/*
 * Globals
 */

static Scsi_Host_Template scsi_driver_template;

static u8 sbp2_speedto_maxrec[] = { 0x7, 0x8, 0x9 };

static LIST_HEAD(sbp2_host_info_list);

static struct hpsb_highlevel *sbp2_hl_handle = NULL;

static struct hpsb_highlevel_ops sbp2_hl_ops = {
	.add_host =	sbp2_add_host,
	.remove_host =	sbp2_remove_host,
};

static struct hpsb_address_ops sbp2_ops = {
	.write = sbp2_handle_status_write
};

#ifdef CONFIG_IEEE1394_SBP2_PHYS_DMA
static struct hpsb_address_ops sbp2_physdma_ops = {
        .read = sbp2_handle_physdma_read,
        .write = sbp2_handle_physdma_write,
};
#endif

static struct hpsb_protocol_driver sbp2_driver = {
	.name =		"SBP2 Driver",
	.id_table = 	sbp2_id_table,
	.probe = 		sbp2_probe,
	.disconnect = 	sbp2_disconnect,
	.update = 	sbp2_update
};

/* List of device firmware's that require a forced 36 byte inquiry.  */
static u32 sbp2_broken_inquiry_list[] = {
	0x00002800,	/* Stefan Richter <richtest@bauwesen.tu-cottbus.de> */
			/* DViCO Momobay CX-1 */
	0x00000200	/* Andreas Plesch <plesch@fas.harvard.edu> */
			/* QPS Fire DVDBurner */
};

#define NUM_BROKEN_INQUIRY_DEVS \
	(sizeof(sbp2_broken_inquiry_list)/sizeof(*sbp2_broken_inquiry_list))

/**************************************
 * General utility functions
 **************************************/


#ifndef __BIG_ENDIAN
/*
 * Converts a buffer from be32 to cpu byte ordering. Length is in bytes.
 */
static __inline__ void sbp2util_be32_to_cpu_buffer(void *buffer, int length)
{
	u32 *temp = buffer;

	for (length = (length >> 2); length--; )
		temp[length] = be32_to_cpu(temp[length]);

	return;
}

/*
 * Converts a buffer from cpu to be32 byte ordering. Length is in bytes.
 */
static __inline__ void sbp2util_cpu_to_be32_buffer(void *buffer, int length)
{
	u32 *temp = buffer;

	for (length = (length >> 2); length--; )
		temp[length] = cpu_to_be32(temp[length]);

	return;
}
#else /* BIG_ENDIAN */
/* Why waste the cpu cycles? */
#define sbp2util_be32_to_cpu_buffer(x,y)
#define sbp2util_cpu_to_be32_buffer(x,y)
#endif

#ifdef CONFIG_IEEE1394_SBP2_PACKET_DUMP
/*
 * Debug packet dump routine. Length is in bytes.
 */
static void sbp2util_packet_dump(void *buffer, int length, char *dump_name, u32 dump_phys_addr)
{
	int i;
	unsigned char *dump = buffer;

	if (!dump || !length || !dump_name)
		return;

	if (dump_phys_addr)
		printk("[%s, 0x%x]", dump_name, dump_phys_addr);
	else
		printk("[%s]", dump_name);
	for (i = 0; i < length; i++) {
		if (i > 0x3f) {
			printk("\n   ...");
			break;
		}
		if ((i & 0x3) == 0)
			printk("  ");
		if ((i & 0xf) == 0)
			printk("\n   ");
		printk("%02x ", (int) dump[i]);
	}
	printk("\n");

	return;
}
#else
#define sbp2util_packet_dump(w,x,y,z)
#endif

/*
 * Goofy routine that basically does a down_timeout function.
 */
static int sbp2util_down_timeout(atomic_t *done, int timeout)
{
	int i;

	for (i = timeout; (i > 0 && atomic_read(done) == 0); i-= HZ/10) {
		set_current_state(TASK_INTERRUPTIBLE);
		if (schedule_timeout(HZ/10))	/* 100ms */
			return(1);
	}
	return ((i > 0) ? 0:1);
}

/*
 * This function is called to initially create a packet pool for use in
 * sbp2 I/O requests. This packet pool is used when sending out sbp2
 * command and agent reset requests, and allows us to remove all
 * kmallocs/kfrees from the critical I/O paths.
 */
static int sbp2util_create_request_packet_pool(struct sbp2scsi_host_info *hi)
{
	struct hpsb_packet *packet;
	int i;

	hi->request_packet = kmalloc(sizeof(struct sbp2_request_packet) * SBP2_MAX_REQUEST_PACKETS, 
				     GFP_KERNEL);

	if (!hi->request_packet) {
		SBP2_ERR("sbp2util_create_request_packet_pool - packet allocation failed!");
		return(-ENOMEM);
	}
	memset(hi->request_packet, 0, sizeof(struct sbp2_request_packet) * SBP2_MAX_REQUEST_PACKETS);

	/* 
	 * Create a pool of request packets. Just take the max supported 
	 * concurrent commands and multiply by two to be safe... 
	 */
	for (i=0; i<SBP2_MAX_REQUEST_PACKETS; i++) {

		/*
		 * Max payload of 8 bytes since the sbp2 command request
		 * uses a payload of 8 bytes, and agent reset is a quadlet
		 * write request. Bump this up if we plan on using this
		 * pool for other stuff.
		 */
		packet = alloc_hpsb_packet(8);

		if (!packet) {
			SBP2_ERR("sbp2util_create_request_packet_pool - packet allocation failed!");
			return(-ENOMEM);
		}

		/* 
		 * Put these request packets into a free list
		 */
		INIT_LIST_HEAD(&hi->request_packet[i].list);
		hi->request_packet[i].packet = packet;
		list_add_tail(&hi->request_packet[i].list, &hi->sbp2_req_free);

	}

	return(0);
}

/*
 * This function is called to remove the packet pool. It is called when
 * the sbp2 driver is unloaded.
 */
static void sbp2util_remove_request_packet_pool(struct sbp2scsi_host_info *hi)
{
	struct list_head *lh;
	struct sbp2_request_packet *request_packet;
	unsigned long flags;

	/* 
	 * Go through free list releasing packets
	 */
	sbp2_spin_lock(&hi->sbp2_request_packet_lock, flags);
	while (!list_empty(&hi->sbp2_req_free)) {

		lh = hi->sbp2_req_free.next;
		list_del(lh);

		request_packet = list_entry(lh, struct sbp2_request_packet, list);

		/*
		 * Free the hpsb packets that we allocated for the pool
		 */
		if (request_packet) {
			free_hpsb_packet(request_packet->packet);
		}

	}
	kfree(hi->request_packet);
	sbp2_spin_unlock(&hi->sbp2_request_packet_lock, flags);

	return;
}

/*
 * This function is called to retrieve a block write packet from our
 * packet pool. This function is used in place of calling
 * alloc_hpsb_packet (which costs us three kmallocs). Instead we just pull
 * out a free request packet and re-initialize values in it. I'm sure this
 * can still stand some more optimization.
 */
static struct sbp2_request_packet *
sbp2util_allocate_write_request_packet(struct sbp2scsi_host_info *hi,
				       struct node_entry *ne, u64 addr,
				       size_t data_size,
				       quadlet_t data) {
	struct list_head *lh;
	struct sbp2_request_packet *request_packet = NULL;
	struct hpsb_packet *packet;
	unsigned long flags;

	sbp2_spin_lock(&hi->sbp2_request_packet_lock, flags);
	if (!list_empty(&hi->sbp2_req_free)) {

		/*
		 * Pull out a free request packet
		 */
		lh = hi->sbp2_req_free.next;
		list_del(lh);

		request_packet = list_entry(lh, struct sbp2_request_packet, list);
		packet = request_packet->packet;

		/*
		 * Initialize the packet (this is really initialization
		 * the core 1394 stack should do, but I'm doing it myself
		 * to avoid the overhead).
		 */
		packet->data_size = data_size;
		INIT_LIST_HEAD(&packet->list);
		sema_init(&packet->state_change, 0);
		packet->state = hpsb_unused;
		packet->data_be = 1;
		
		hpsb_node_fill_packet(ne, packet);

		packet->tlabel = get_tlabel(hi->host, packet->node_id, 0);

		if (!data_size) {
			fill_async_writequad(packet, addr, data);
		} else {
			fill_async_writeblock(packet, addr, data_size);         
		}

		/*
		 * Set up a task queue completion routine, which returns
		 * the packet to the free list and releases the tlabel.
		 */
		request_packet->tq.routine = (void (*)(void*))sbp2util_free_request_packet;
		request_packet->tq.data = request_packet;
		request_packet->hi_context = hi;
		hpsb_add_packet_complete_task(packet, &request_packet->tq);

		/*
		 * Now, put the packet on the in-use list.
		 */
		list_add_tail(&request_packet->list, &hi->sbp2_req_inuse);

	} else {
		SBP2_ERR("sbp2util_allocate_request_packet - no packets available!");
	}
	sbp2_spin_unlock(&hi->sbp2_request_packet_lock, flags);

	return(request_packet);
}

/*
 * This function is called to return a packet to our packet pool. It is
 * also called as a completion routine when a request packet is completed.
 */
static void sbp2util_free_request_packet(struct sbp2_request_packet *request_packet)
{
	unsigned long flags;
	struct sbp2scsi_host_info *hi = request_packet->hi_context;

	/*
	 * Free the tlabel, and return the packet to the free pool.
	 */
	sbp2_spin_lock(&hi->sbp2_request_packet_lock, flags);
	free_tlabel(hi->host, LOCAL_BUS | request_packet->packet->node_id,
		    request_packet->packet->tlabel);
	list_del(&request_packet->list);
	list_add_tail(&request_packet->list, &hi->sbp2_req_free);
	sbp2_spin_unlock(&hi->sbp2_request_packet_lock, flags);

	return;
}

/*
 * This function is called to create a pool of command orbs used for
 * command processing. It is called when a new sbp2 device is detected.
 */
static int sbp2util_create_command_orb_pool(struct scsi_id_instance_data *scsi_id,
					    struct sbp2scsi_host_info *hi)
{
	int i;
	unsigned long flags;
	struct sbp2_command_info *command;
        
	sbp2_spin_lock(&scsi_id->sbp2_command_orb_lock, flags);
	for (i = 0; i < scsi_id->sbp2_total_command_orbs; i++) {
		command = (struct sbp2_command_info *)
		    kmalloc(sizeof(struct sbp2_command_info), GFP_KERNEL);
		if (!command) {
			sbp2_spin_unlock(&scsi_id->sbp2_command_orb_lock, flags);
			return(-ENOMEM);
		}
		memset(command, '\0', sizeof(struct sbp2_command_info));
		command->command_orb_dma =
			pci_map_single (hi->host->pdev, &command->command_orb,
					sizeof(struct sbp2_command_orb),
					PCI_DMA_BIDIRECTIONAL);
		SBP2_DMA_ALLOC("single command orb DMA");
		command->sge_dma =
			pci_map_single (hi->host->pdev, &command->scatter_gather_element,
					sizeof(command->scatter_gather_element),
					PCI_DMA_BIDIRECTIONAL);
		SBP2_DMA_ALLOC("scatter_gather_element");
		INIT_LIST_HEAD(&command->list);
		list_add_tail(&command->list, &scsi_id->sbp2_command_orb_completed);
	}
	sbp2_spin_unlock(&scsi_id->sbp2_command_orb_lock, flags);
	return 0;
}

/*
 * This function is called to delete a pool of command orbs.
 */
static void sbp2util_remove_command_orb_pool(struct scsi_id_instance_data *scsi_id,
					     struct sbp2scsi_host_info *hi)
{
	struct list_head *lh, *next;
	struct sbp2_command_info *command;
	unsigned long flags;
        
	sbp2_spin_lock(&scsi_id->sbp2_command_orb_lock, flags);
	if (!list_empty(&scsi_id->sbp2_command_orb_completed)) {
		list_for_each_safe(lh, next, &scsi_id->sbp2_command_orb_completed) {
			command = list_entry(lh, struct sbp2_command_info, list);

			/* Release our generic DMA's */
			pci_unmap_single(hi->host->pdev, command->command_orb_dma,
					 sizeof(struct sbp2_command_orb),
					 PCI_DMA_BIDIRECTIONAL);
			SBP2_DMA_FREE("single command orb DMA");
			pci_unmap_single(hi->host->pdev, command->sge_dma,
					 sizeof(command->scatter_gather_element),
					 PCI_DMA_BIDIRECTIONAL);
			SBP2_DMA_FREE("scatter_gather_element");

			kfree(command);
		}
	}
	sbp2_spin_unlock(&scsi_id->sbp2_command_orb_lock, flags);
	return;
}

/* 
 * This function finds the sbp2_command for a given outstanding command
 * orb.Only looks at the inuse list.
 */
static struct sbp2_command_info *sbp2util_find_command_for_orb(
		struct scsi_id_instance_data *scsi_id, dma_addr_t orb)
{
	struct list_head *lh;
	struct sbp2_command_info *command;
	unsigned long flags;

	sbp2_spin_lock(&scsi_id->sbp2_command_orb_lock, flags);
	if (!list_empty(&scsi_id->sbp2_command_orb_inuse)) {
		list_for_each(lh, &scsi_id->sbp2_command_orb_inuse) {
			command = list_entry(lh, struct sbp2_command_info, list);
			if (command->command_orb_dma == orb) {
				sbp2_spin_unlock(&scsi_id->sbp2_command_orb_lock, flags);
				return (command);
			}
		}
	}
	sbp2_spin_unlock(&scsi_id->sbp2_command_orb_lock, flags);

	SBP2_ORB_DEBUG("could not match command orb %x", (unsigned int)orb);

	return(NULL);
}

/* 
 * This function finds the sbp2_command for a given outstanding SCpnt.
 * Only looks at the inuse list.
 */
static struct sbp2_command_info *sbp2util_find_command_for_SCpnt(struct scsi_id_instance_data *scsi_id, void *SCpnt)
{
	struct list_head *lh;
	struct sbp2_command_info *command;
	unsigned long flags;

	sbp2_spin_lock(&scsi_id->sbp2_command_orb_lock, flags);
	if (!list_empty(&scsi_id->sbp2_command_orb_inuse)) {
		list_for_each(lh, &scsi_id->sbp2_command_orb_inuse) {
			command = list_entry(lh, struct sbp2_command_info, list);
			if (command->Current_SCpnt == SCpnt) {
				sbp2_spin_unlock(&scsi_id->sbp2_command_orb_lock, flags);
				return (command);
			}
		}
	}
	sbp2_spin_unlock(&scsi_id->sbp2_command_orb_lock, flags);
	return(NULL);
}

/*
 * This function allocates a command orb used to send a scsi command.
 */
static struct sbp2_command_info *sbp2util_allocate_command_orb(
		struct scsi_id_instance_data *scsi_id, 
		Scsi_Cmnd *Current_SCpnt, 
		void (*Current_done)(Scsi_Cmnd *),
		struct sbp2scsi_host_info *hi)
{
	struct list_head *lh;
	struct sbp2_command_info *command = NULL;
	unsigned long flags;

	sbp2_spin_lock(&scsi_id->sbp2_command_orb_lock, flags);
	if (!list_empty(&scsi_id->sbp2_command_orb_completed)) {
		lh = scsi_id->sbp2_command_orb_completed.next;
		list_del(lh);
		command = list_entry(lh, struct sbp2_command_info, list);
		command->Current_done = Current_done;
		command->Current_SCpnt = Current_SCpnt;
		list_add_tail(&command->list, &scsi_id->sbp2_command_orb_inuse);
	} else {
		SBP2_ERR("sbp2util_allocate_command_orb - No orbs available!");
	}
	sbp2_spin_unlock(&scsi_id->sbp2_command_orb_lock, flags);
	return (command);
}

/* Free our DMA's */
static void sbp2util_free_command_dma(struct sbp2_command_info *command)
{
	struct sbp2scsi_host_info *hi;
	
	hi = (struct sbp2scsi_host_info *) command->Current_SCpnt->host->hostdata[0];

	if (hi == NULL) {
		printk(KERN_ERR "%s: hi == NULL\n", __FUNCTION__);
		return;
	}

	if (command->cmd_dma) {
		if (command->dma_type == CMD_DMA_SINGLE) {
			pci_unmap_single(hi->host->pdev, command->cmd_dma,
					 command->dma_size, command->dma_dir);
			SBP2_DMA_FREE("single bulk");
		} else if (command->dma_type == CMD_DMA_PAGE) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,13)
			pci_unmap_single(hi->host->pdev, command->cmd_dma,
					 command->dma_size, command->dma_dir);
#else
			pci_unmap_page(hi->host->pdev, command->cmd_dma,
				       command->dma_size, command->dma_dir);
#endif /* Linux version < 2.4.13 */
			SBP2_DMA_FREE("single page");
		} 
		command->dma_type = CMD_DMA_NONE;
		command->cmd_dma = 0;
	}

	if (command->sge_buffer) {
		pci_unmap_sg(hi->host->pdev, command->sge_buffer,
			     command->dma_size, command->dma_dir);
		SBP2_DMA_FREE("scatter list");
		command->sge_buffer = NULL;
	}
}

/*
 * This function moves a command to the completed orb list.
 */
static void sbp2util_mark_command_completed(struct scsi_id_instance_data *scsi_id, struct sbp2_command_info *command)
{
	unsigned long flags;

	sbp2_spin_lock(&scsi_id->sbp2_command_orb_lock, flags);
	list_del(&command->list);
	sbp2util_free_command_dma(command);
	list_add_tail(&command->list, &scsi_id->sbp2_command_orb_completed);
	sbp2_spin_unlock(&scsi_id->sbp2_command_orb_lock, flags);
}



/*********************************************
 * IEEE-1394 core driver stack related section
 *********************************************/

/*
 * This function is called at SCSI init in order to register our driver
 * with the IEEE-1394 stack.
 */
int sbp2_init(void)
{
	SBP2_DEBUG("sbp2_init");

	/*
	 * Register our high level driver with 1394 stack
	 */
	sbp2_hl_handle = hpsb_register_highlevel(SBP2_DEVICE_NAME, &sbp2_hl_ops);

	if (sbp2_hl_handle == NULL) {
		SBP2_ERR("sbp2 failed to register with ieee1394 highlevel");
		return(-ENOMEM);
	}

	/*
	 * Register our sbp2 status address space...
	 */
	hpsb_register_addrspace(sbp2_hl_handle, &sbp2_ops, SBP2_STATUS_FIFO_ADDRESS,
				SBP2_STATUS_FIFO_ADDRESS + 
				SBP2_STATUS_FIFO_ENTRY_TO_OFFSET(SBP2SCSI_MAX_SCSI_IDS+1));

	/*
	 * Handle data movement if physical dma is not enabled/supported on host controller
	 */
#ifdef CONFIG_IEEE1394_SBP2_PHYS_DMA
	hpsb_register_addrspace(sbp2_hl_handle, &sbp2_physdma_ops, 0x0ULL, 0xfffffffcULL);
#endif

	hpsb_register_protocol(&sbp2_driver);

	return 0;
}

/*
 * This function is called from cleanup module, or during shut-down, in
 * order to unregister our driver.
 */
void sbp2_cleanup(void)
{
	SBP2_DEBUG("sbp2_cleanup");

	hpsb_unregister_protocol(&sbp2_driver);

	if (sbp2_hl_handle) {
		hpsb_unregister_highlevel(sbp2_hl_handle);
		sbp2_hl_handle = NULL;
	}
}

static int sbp2_probe(struct unit_directory *ud)
{
	struct sbp2scsi_host_info *hi;

	SBP2_DEBUG("sbp2_probe");
	hi = sbp2_find_host_info(ud->ne->host);

	return sbp2_start_device(hi, ud);
}

static void sbp2_disconnect(struct unit_directory *ud)
{
	struct sbp2scsi_host_info *hi;
	struct scsi_id_instance_data *scsi_id = ud->driver_data;

	SBP2_DEBUG("sbp2_disconnect");
	hi = sbp2_find_host_info(ud->ne->host);

	if (hi != NULL) {
		sbp2_logout_device(hi, scsi_id);
 		sbp2_remove_device(hi, scsi_id);
	}
}

static void sbp2_update(struct unit_directory *ud)
{
	struct sbp2scsi_host_info *hi;
	struct scsi_id_instance_data *scsi_id = ud->driver_data;
	unsigned long flags;

	SBP2_DEBUG("sbp2_update");
	hi = sbp2_find_host_info(ud->ne->host);

	if (sbp2_reconnect_device(hi, scsi_id)) {
		
		/* 
		 * Ok, reconnect has failed. Perhaps we didn't
		 * reconnect fast enough. Try doing a regular login.
		 */
		if (sbp2_login_device(hi, scsi_id)) {

			/* Login failed too, just remove the device. */
			SBP2_ERR("sbp2_reconnect_device failed!");
			sbp2_remove_device(hi, scsi_id);
			hpsb_release_unit_directory(ud);
			return;
		}
	}

	/* Set max retries to something large on the device. */
	sbp2_set_busy_timeout(hi, scsi_id);

	/* Do a SBP-2 fetch agent reset. */
	sbp2_agent_reset(hi, scsi_id, 0);
	
	/* Get the max speed and packet size that we can use. */
	sbp2_max_speed_and_size(hi, scsi_id);

	/* Complete any pending commands with busy (so they get
	 * retried) and remove them from our queue
	 */
	sbp2_spin_lock(&hi->sbp2_command_lock, flags);
	sbp2scsi_complete_all_commands(hi, scsi_id, DID_BUS_BUSY);
	sbp2_spin_unlock(&hi->sbp2_command_lock, flags);
}

/*
 * This function is called after registering our operations in sbp2_init.
 * We go ahead and allocate some memory for our host info structure, and
 * init some structures.
 */
static void sbp2_add_host(struct hpsb_host *host)
{
	struct sbp2scsi_host_info *hi;
	unsigned long flags;

	SBP2_DEBUG("sbp2_add_host");

	/* Allocate some memory for our host info structure */
	hi = (struct sbp2scsi_host_info *)kmalloc(sizeof(struct sbp2scsi_host_info),
						  GFP_KERNEL);

	if (hi == NULL) {
		SBP2_ERR("out of memory in sbp2_add_host");
		return;
	}

	/* Initialize some host stuff */
	memset(hi, 0, sizeof(struct sbp2scsi_host_info));
	INIT_LIST_HEAD(&hi->list);
	INIT_LIST_HEAD(&hi->sbp2_req_inuse);
	INIT_LIST_HEAD(&hi->sbp2_req_free);
	hi->host = host;
	hi->sbp2_command_lock = SPIN_LOCK_UNLOCKED;
	hi->sbp2_request_packet_lock = SPIN_LOCK_UNLOCKED;

	/* Create our request packet pool (pool of packets for use in I/O) */
	if (sbp2util_create_request_packet_pool(hi)) {
		SBP2_ERR("sbp2util_create_request_packet_pool failed!");
		return;
	}

	sbp2_spin_lock(&sbp2_host_info_lock, flags);
	list_add_tail(&hi->list, &sbp2_host_info_list);
	sbp2_spin_unlock(&sbp2_host_info_lock, flags);

	/* Register our host with the SCSI stack. */
	hi->scsi_host = scsi_register (&scsi_driver_template, sizeof(void *));
	if (hi->scsi_host) {
		hi->scsi_host->hostdata[0] = (unsigned long)hi;
		hi->scsi_host->max_id = SBP2SCSI_MAX_SCSI_IDS;
	}
	scsi_driver_template.present++;

	return;
}

/*
 * This fuction returns a host info structure from the host structure, in
 * case we have multiple hosts.
 */
static struct sbp2scsi_host_info *sbp2_find_host_info(struct hpsb_host *host)
{
	struct list_head *lh;
	struct sbp2scsi_host_info *hi;

	list_for_each (lh, &sbp2_host_info_list) {
		hi = list_entry(lh, struct sbp2scsi_host_info, list);
		if (hi->host == host)
			return hi;
	}

	return NULL;
}

/*
 * This function returns a host info structure for a given Scsi_Host
 * struct.
 */
static struct sbp2scsi_host_info *sbp2_find_host_info_scsi(struct Scsi_Host *host)
{
	struct list_head *lh;
	struct sbp2scsi_host_info *hi;

	list_for_each (lh, &sbp2_host_info_list) {
		hi = list_entry(lh, struct sbp2scsi_host_info, list);
		if (hi->scsi_host == host)
			return hi;
	}

	return NULL;
}

/*
 * This function is called when a host is removed.
 */
static void sbp2_remove_host(struct hpsb_host *host)
{
	struct sbp2scsi_host_info *hi;
	unsigned long flags;

	SBP2_DEBUG("sbp2_remove_host");

	sbp2_spin_lock(&sbp2_host_info_lock, flags);

	hi = sbp2_find_host_info(host);
	if (hi != NULL) {
		sbp2util_remove_request_packet_pool(hi);
		list_del(&hi->list);
		kfree(hi);
	}
	else
		SBP2_ERR("attempt to remove unknown host %p", host);

	sbp2_spin_unlock(&sbp2_host_info_lock, flags);
}

/*
 * This function is where we first pull the node unique ids, and then
 * allocate memory and register a SBP-2 device.
 */
static int sbp2_start_device(struct sbp2scsi_host_info *hi, struct unit_directory *ud)
{
	struct scsi_id_instance_data *scsi_id = NULL;
	struct node_entry *ne;
	int i;

	SBP2_DEBUG("sbp2_start_device");
	ne = ud->ne;

	/*
	 * This really is a "new" device plugged in. Let's allocate memory
	 * for our scsi id instance data.
	 */
	scsi_id = (struct scsi_id_instance_data *)kmalloc(sizeof(struct scsi_id_instance_data),
							  GFP_KERNEL);
	if (!scsi_id)
		goto alloc_fail_first;
	memset(scsi_id, 0, sizeof(struct scsi_id_instance_data));

	/* Login FIFO DMA */
	scsi_id->login_response =
		pci_alloc_consistent(hi->host->pdev, sizeof(struct sbp2_login_response),
				     &scsi_id->login_response_dma);
	if (!scsi_id->login_response)
		goto alloc_fail;
	SBP2_DMA_ALLOC("consistent DMA region for login FIFO");

	/* Reconnect ORB DMA */
	scsi_id->reconnect_orb =
		pci_alloc_consistent(hi->host->pdev, sizeof(struct sbp2_reconnect_orb),
				     &scsi_id->reconnect_orb_dma);
	if (!scsi_id->reconnect_orb)
		goto alloc_fail;
	SBP2_DMA_ALLOC("consistent DMA region for reconnect ORB");

	/* Logout ORB DMA */
	scsi_id->logout_orb =
		pci_alloc_consistent(hi->host->pdev, sizeof(struct sbp2_logout_orb),
				     &scsi_id->logout_orb_dma);
	if (!scsi_id->logout_orb)
		goto alloc_fail;
	SBP2_DMA_ALLOC("consistent DMA region for logout ORB");

	/* Login ORB DMA */
	scsi_id->login_orb =
		pci_alloc_consistent(hi->host->pdev, sizeof(struct sbp2_login_orb),
				     &scsi_id->login_orb_dma);
	if (scsi_id->login_orb == NULL) {
alloc_fail:
		if (scsi_id->logout_orb) {
			pci_free_consistent(hi->host->pdev,
					sizeof(struct sbp2_logout_orb),
					scsi_id->logout_orb,
					scsi_id->logout_orb_dma);
			SBP2_DMA_FREE("logout ORB DMA");
		}

		if (scsi_id->reconnect_orb) {
			pci_free_consistent(hi->host->pdev,
					sizeof(struct sbp2_reconnect_orb),
					scsi_id->reconnect_orb,
					scsi_id->reconnect_orb_dma);
			SBP2_DMA_FREE("reconnect ORB DMA");
		}

		if (scsi_id->login_response) {
			pci_free_consistent(hi->host->pdev,
					sizeof(struct sbp2_login_response),
					scsi_id->login_response,
					scsi_id->login_response_dma);
			SBP2_DMA_FREE("login FIFO DMA");
		}

		kfree(scsi_id);
alloc_fail_first:
		SBP2_ERR ("Could not allocate memory for scsi_id");
		return(-ENOMEM);
	}
	SBP2_DMA_ALLOC("consistent DMA region for login ORB");

	/*
	 * Initialize some of the fields in this structure
	 */
	scsi_id->ne = ne;
	scsi_id->ud = ud;
	scsi_id->speed_code = SPEED_100;
	scsi_id->max_payload_size = sbp2_speedto_maxrec[SPEED_100];
	ud->driver_data = scsi_id;

	atomic_set(&scsi_id->sbp2_login_complete, 0);

	/* 
	 * Initialize structures needed for the command orb pool.
	 */
	INIT_LIST_HEAD(&scsi_id->sbp2_command_orb_inuse);
	INIT_LIST_HEAD(&scsi_id->sbp2_command_orb_completed);
	scsi_id->sbp2_command_orb_lock = SPIN_LOCK_UNLOCKED;
	scsi_id->sbp2_total_command_orbs = 0;

	/*
	 * Make sure that we've gotten ahold of the sbp2 management agent
	 * address. Also figure out the command set being used (SCSI or
	 * RBC).
	 */
	sbp2_parse_unit_directory(scsi_id);

	scsi_id->sbp2_total_command_orbs = SBP2_MAX_COMMAND_ORBS;

	/* 
	 * Knock the total command orbs down if we are serializing I/O
	 */
	if (sbp2_serialize_io) {
		scsi_id->sbp2_total_command_orbs = 2;	/* one extra for good measure */
	}

	/*
	 * Find an empty spot to stick our scsi id instance data. 
	 */
	for (i = 0; i < SBP2SCSI_MAX_SCSI_IDS; i++) {
		if (!hi->scsi_id[i]) {
			hi->scsi_id[i] = scsi_id;
			scsi_id->id = i;
			SBP2_DEBUG("New SBP-2 device inserted, SCSI ID = %x", (unsigned int) i);
			break;
		}
	}

	/*
	 * Create our command orb pool
	 */
	if (sbp2util_create_command_orb_pool(scsi_id, hi)) {
		SBP2_ERR("sbp2util_create_command_orb_pool failed!");
		sbp2_remove_device(hi, scsi_id);
		return -ENOMEM;
	}

	/*
	 * Make sure we are not out of space
	 */
	if (i == SBP2SCSI_MAX_SCSI_IDS) {
		SBP2_ERR("No slots left for SBP-2 device");
		sbp2_remove_device(hi, scsi_id);
		return -EBUSY;
	}

	/*
	 * Login to the sbp-2 device
	 */
	if (sbp2_login_device(hi, scsi_id)) {

		/* Login failed, just remove the device. */
		SBP2_ERR("sbp2_login_device failed");
		sbp2_remove_device(hi, scsi_id);
		return -EBUSY;
	}

	/*
	 * Set max retries to something large on the device
	 */
	sbp2_set_busy_timeout(hi, scsi_id);
	
	/*
	 * Do a SBP-2 fetch agent reset
	 */
	sbp2_agent_reset(hi, scsi_id, 0);
	
	/*
	 * Get the max speed and packet size that we can use
	 */
	sbp2_max_speed_and_size(hi, scsi_id);

	return 0;
}

/*
 * This function removes an sbp2 device from the sbp2scsi_host_info struct.
 */
static void sbp2_remove_device(struct sbp2scsi_host_info *hi, 
			       struct scsi_id_instance_data *scsi_id)
{
	SBP2_DEBUG("sbp2_remove_device");

	/* Complete any pending commands with selection timeout */
	sbp2scsi_complete_all_commands(hi, scsi_id, DID_NO_CONNECT);
       			
	/* Clean up any other structures */
	if (scsi_id->sbp2_total_command_orbs) {
		sbp2util_remove_command_orb_pool(scsi_id, hi);
	}

	if (scsi_id->login_response) {
		pci_free_consistent(hi->host->pdev,
				    sizeof(struct sbp2_login_response),
				    scsi_id->login_response,
				    scsi_id->login_response_dma);
		SBP2_DMA_FREE("single login FIFO");
	}

	if (scsi_id->login_orb) {
		pci_free_consistent(hi->host->pdev,
				    sizeof(struct sbp2_login_orb),
				    scsi_id->login_orb,
				    scsi_id->login_orb_dma);
		SBP2_DMA_FREE("single login ORB");
	}

	if (scsi_id->reconnect_orb) {
		pci_free_consistent(hi->host->pdev,
				    sizeof(struct sbp2_reconnect_orb),
				    scsi_id->reconnect_orb,
				    scsi_id->reconnect_orb_dma);
		SBP2_DMA_FREE("single reconnect orb");
	}

	if (scsi_id->logout_orb) {
		pci_free_consistent(hi->host->pdev,
				    sizeof(struct sbp2_logout_orb),
				    scsi_id->logout_orb,
				    scsi_id->reconnect_orb_dma);
		SBP2_DMA_FREE("single logout orb");
	}

	SBP2_DEBUG("SBP-2 device removed, SCSI ID = %d", scsi_id->id);
	hi->scsi_id[scsi_id->id] = NULL;
	kfree(scsi_id);
}

#ifdef CONFIG_IEEE1394_SBP2_PHYS_DMA
/*
 * This function deals with physical dma write requests (for adapters that do not support
 * physical dma in hardware). Mostly just here for debugging...
 */
static int sbp2_handle_physdma_write(struct hpsb_host *host, int nodeid, int destid, quadlet_t *data,
                                     u64 addr, unsigned int length)
{

        /*
         * Manually put the data in the right place.
         */
        memcpy(bus_to_virt((u32)addr), data, length);
	sbp2util_packet_dump(data, length, "sbp2 phys dma write by device", (u32)addr);
        return(RCODE_COMPLETE);
}

/*
 * This function deals with physical dma read requests (for adapters that do not support
 * physical dma in hardware). Mostly just here for debugging...
 */
static int sbp2_handle_physdma_read(struct hpsb_host *host, int nodeid, quadlet_t *data,
                                    u64 addr, unsigned int length)
{

        /*
         * Grab data from memory and send a read response.
         */
        memcpy(data, bus_to_virt((u32)addr), length);
	sbp2util_packet_dump(data, length, "sbp2 phys dma read by device", (u32)addr);
        return(RCODE_COMPLETE);
}
#endif


/**************************************
 * SBP-2 protocol related section
 **************************************/

/*
 * This function determines if we should convert scsi commands for a particular sbp2 device type
 */
static __inline__ int sbp2_command_conversion_device_type(u8 device_type)
{
	return (((device_type == TYPE_DISK) ||
		 (device_type == TYPE_SDAD) ||
		 (device_type == TYPE_ROM)) ? 1:0);
}

/*
 * This function is called in order to login to a particular SBP-2 device,
 * after a bus reset.
 */
static int sbp2_login_device(struct sbp2scsi_host_info *hi, struct scsi_id_instance_data *scsi_id) 
{
	quadlet_t data[2];

	SBP2_DEBUG("sbp2_login_device");

	if (!scsi_id->login_orb) {
		SBP2_DEBUG("sbp2_login_device: login_orb not alloc'd!");
		return(-EIO);
	}

	/* Set-up login ORB, assume no password */
	scsi_id->login_orb->password_hi = 0; 
	scsi_id->login_orb->password_lo = 0;
	SBP2_DEBUG("sbp2_login_device: password_hi/lo initialized");

	scsi_id->login_orb->login_response_lo = scsi_id->login_response_dma;
	scsi_id->login_orb->login_response_hi = ORB_SET_NODE_ID(hi->host->node_id);
	SBP2_DEBUG("sbp2_login_device: login_response_hi/lo initialized");

	scsi_id->login_orb->lun_misc = ORB_SET_FUNCTION(LOGIN_REQUEST);
	scsi_id->login_orb->lun_misc |= ORB_SET_RECONNECT(0);	/* One second reconnect time */
	scsi_id->login_orb->lun_misc |= ORB_SET_EXCLUSIVE(sbp2_exclusive_login);	/* Exclusive access to device */
	scsi_id->login_orb->lun_misc |= ORB_SET_NOTIFY(1);	/* Notify us of login complete */
	/* Set the lun if we were able to pull it from the device's unit directory */
	if (scsi_id->sbp2_device_type_and_lun != SBP2_DEVICE_TYPE_LUN_UNINITIALIZED) {
		scsi_id->login_orb->lun_misc |= ORB_SET_LUN(scsi_id->sbp2_device_type_and_lun);
	}
	SBP2_DEBUG("sbp2_login_device: lun_misc initialized");

	scsi_id->login_orb->passwd_resp_lengths =
		ORB_SET_LOGIN_RESP_LENGTH(sizeof(struct sbp2_login_response));
	SBP2_DEBUG("sbp2_login_device: passwd_resp_lengths initialized");

	scsi_id->login_orb->status_FIFO_lo = SBP2_STATUS_FIFO_ADDRESS_LO + 
					     SBP2_STATUS_FIFO_ENTRY_TO_OFFSET(scsi_id->id);
	scsi_id->login_orb->status_FIFO_hi = (ORB_SET_NODE_ID(hi->host->node_id) |
					      SBP2_STATUS_FIFO_ADDRESS_HI);
	SBP2_DEBUG("sbp2_login_device: status FIFO initialized");

	/*
	 * Byte swap ORB if necessary
	 */
	sbp2util_cpu_to_be32_buffer(scsi_id->login_orb, sizeof(struct sbp2_login_orb));

	SBP2_DEBUG("sbp2_login_device: orb byte-swapped");

	sbp2util_packet_dump(scsi_id->login_orb, sizeof(struct sbp2_login_orb), 
			     "sbp2 login orb", scsi_id->login_orb_dma);

	/*
	 * Initialize login response and status fifo
	 */
	memset(scsi_id->login_response, 0, sizeof(struct sbp2_login_response));
	memset(&scsi_id->status_block, 0, sizeof(struct sbp2_status_block));

	SBP2_DEBUG("sbp2_login_device: login_response/status FIFO memset");

	/*
	 * Ok, let's write to the target's management agent register
	 */
	data[0] = ORB_SET_NODE_ID(hi->host->node_id);
	data[1] = scsi_id->login_orb_dma;
	sbp2util_cpu_to_be32_buffer(data, 8);

	atomic_set(&scsi_id->sbp2_login_complete, 0);

	SBP2_DEBUG("sbp2_login_device: prepared to write");
	hpsb_node_write(scsi_id->ne, scsi_id->sbp2_management_agent_addr, data, 8);
	SBP2_DEBUG("sbp2_login_device: written");

	/*
	 * Wait for login status (up to 20 seconds)... 
	 */
	if (sbp2util_down_timeout(&scsi_id->sbp2_login_complete, 20*HZ)) {
		SBP2_ERR("Error logging into SBP-2 device - login timed-out");
		return(-EIO);
	}

	/*
	 * Sanity. Make sure status returned matches login orb.
	 */
	if (scsi_id->status_block.ORB_offset_lo != scsi_id->login_orb_dma) {
		SBP2_ERR("Error logging into SBP-2 device - login timed-out");
		return(-EIO);
	}

	/*
	 * Check status
	 */
	if (STATUS_GET_RESP(scsi_id->status_block.ORB_offset_hi_misc) ||
	    STATUS_GET_DEAD_BIT(scsi_id->status_block.ORB_offset_hi_misc) ||
	    STATUS_GET_SBP_STATUS(scsi_id->status_block.ORB_offset_hi_misc)) {

		SBP2_ERR("Error logging into SBP-2 device - login failed");
		return(-EIO);
	}

	/*
	 * Byte swap the login response, for use when reconnecting or
	 * logging out.
	 */
	sbp2util_cpu_to_be32_buffer(scsi_id->login_response, sizeof(struct sbp2_login_response));

	/*
	 * Grab our command block agent address from the login response.
	 */
	SBP2_DEBUG("command_block_agent_hi = %x",
		   (unsigned int)scsi_id->login_response->command_block_agent_hi);
	SBP2_DEBUG("command_block_agent_lo = %x",
		   (unsigned int)scsi_id->login_response->command_block_agent_lo);

	scsi_id->sbp2_command_block_agent_addr =
		((u64)scsi_id->login_response->command_block_agent_hi) << 32;
	scsi_id->sbp2_command_block_agent_addr |= ((u64)scsi_id->login_response->command_block_agent_lo);
	scsi_id->sbp2_command_block_agent_addr &= 0x0000ffffffffffffULL;

	SBP2_INFO("Logged into SBP-2 device");

	return(0);

}

/*
 * This function is called in order to logout from a particular SBP-2
 * device, usually called during driver unload.
 */
static int sbp2_logout_device(struct sbp2scsi_host_info *hi, struct scsi_id_instance_data *scsi_id) 
{
	quadlet_t data[2];

	SBP2_DEBUG("sbp2_logout_device");

	/*
	 * Set-up logout ORB
	 */
	scsi_id->logout_orb->reserved1 = 0x0;
	scsi_id->logout_orb->reserved2 = 0x0;
	scsi_id->logout_orb->reserved3 = 0x0;
	scsi_id->logout_orb->reserved4 = 0x0;

	scsi_id->logout_orb->login_ID_misc = ORB_SET_FUNCTION(LOGOUT_REQUEST);
	scsi_id->logout_orb->login_ID_misc |= ORB_SET_LOGIN_ID(scsi_id->login_response->length_login_ID);

	/* Notify us when complete */
	scsi_id->logout_orb->login_ID_misc |= ORB_SET_NOTIFY(1);

	scsi_id->logout_orb->reserved5 = 0x0;
	scsi_id->logout_orb->status_FIFO_lo = SBP2_STATUS_FIFO_ADDRESS_LO + 
					      SBP2_STATUS_FIFO_ENTRY_TO_OFFSET(scsi_id->id);
	scsi_id->logout_orb->status_FIFO_hi = (ORB_SET_NODE_ID(hi->host->node_id) |
					       SBP2_STATUS_FIFO_ADDRESS_HI);

	/*
	 * Byte swap ORB if necessary
	 */
	sbp2util_cpu_to_be32_buffer(scsi_id->logout_orb, sizeof(struct sbp2_logout_orb));

	sbp2util_packet_dump(scsi_id->logout_orb, sizeof(struct sbp2_logout_orb), 
			     "sbp2 logout orb", scsi_id->logout_orb_dma);

	/*
	 * Ok, let's write to the target's management agent register
	 */
	data[0] = ORB_SET_NODE_ID(hi->host->node_id);
	data[1] = scsi_id->logout_orb_dma;
	sbp2util_cpu_to_be32_buffer(data, 8);

	atomic_set(&scsi_id->sbp2_login_complete, 0);

	hpsb_node_write(scsi_id->ne, scsi_id->sbp2_management_agent_addr, data, 8);

	/* Wait for device to logout...1 second. */
	sbp2util_down_timeout(&scsi_id->sbp2_login_complete, HZ);

	SBP2_INFO("Logged out of SBP-2 device");

	return(0);

}

/*
 * This function is called in order to reconnect to a particular SBP-2
 * device, after a bus reset.
 */
static int sbp2_reconnect_device(struct sbp2scsi_host_info *hi, struct scsi_id_instance_data *scsi_id) 
{
	quadlet_t data[2];

	SBP2_DEBUG("sbp2_reconnect_device");

	/*
	 * Set-up reconnect ORB
	 */
	scsi_id->reconnect_orb->reserved1 = 0x0;
	scsi_id->reconnect_orb->reserved2 = 0x0;
	scsi_id->reconnect_orb->reserved3 = 0x0;
	scsi_id->reconnect_orb->reserved4 = 0x0;

	scsi_id->reconnect_orb->login_ID_misc = ORB_SET_FUNCTION(RECONNECT_REQUEST);
	scsi_id->reconnect_orb->login_ID_misc |=
		ORB_SET_LOGIN_ID(scsi_id->login_response->length_login_ID);

	/* Notify us when complete */
	scsi_id->reconnect_orb->login_ID_misc |= ORB_SET_NOTIFY(1);

	scsi_id->reconnect_orb->reserved5 = 0x0;
	scsi_id->reconnect_orb->status_FIFO_lo = SBP2_STATUS_FIFO_ADDRESS_LO + 
						 SBP2_STATUS_FIFO_ENTRY_TO_OFFSET(scsi_id->id);
	scsi_id->reconnect_orb->status_FIFO_hi =
		(ORB_SET_NODE_ID(hi->host->node_id) | SBP2_STATUS_FIFO_ADDRESS_HI);

	/*
	 * Byte swap ORB if necessary
	 */
	sbp2util_cpu_to_be32_buffer(scsi_id->reconnect_orb, sizeof(struct sbp2_reconnect_orb));

	sbp2util_packet_dump(scsi_id->reconnect_orb, sizeof(struct sbp2_reconnect_orb), 
			     "sbp2 reconnect orb", scsi_id->reconnect_orb_dma);

	/*
	 * Initialize status fifo
	 */
	memset(&scsi_id->status_block, 0, sizeof(struct sbp2_status_block));

	/*
	 * Ok, let's write to the target's management agent register
	 */
	data[0] = ORB_SET_NODE_ID(hi->host->node_id);
	data[1] = scsi_id->reconnect_orb_dma;
	sbp2util_cpu_to_be32_buffer(data, 8);

	atomic_set(&scsi_id->sbp2_login_complete, 0);

	hpsb_node_write(scsi_id->ne, scsi_id->sbp2_management_agent_addr, data, 8);

	/*
	 * Wait for reconnect status (up to 1 second)...
	 */
	if (sbp2util_down_timeout(&scsi_id->sbp2_login_complete, HZ)) {
		SBP2_ERR("Error reconnecting to SBP-2 device - reconnect timed-out");
		return(-EIO);
	}

	/*
	 * Sanity. Make sure status returned matches reconnect orb.
	 */
	if (scsi_id->status_block.ORB_offset_lo != scsi_id->reconnect_orb_dma) {
		SBP2_ERR("Error reconnecting to SBP-2 device - reconnect timed-out");
		return(-EIO);
	}

	/*
	 * Check status
	 */
	if (STATUS_GET_RESP(scsi_id->status_block.ORB_offset_hi_misc) ||
	    STATUS_GET_DEAD_BIT(scsi_id->status_block.ORB_offset_hi_misc) ||
	    STATUS_GET_SBP_STATUS(scsi_id->status_block.ORB_offset_hi_misc)) {

		SBP2_ERR("Error reconnecting to SBP-2 device - reconnect failed");
		return(-EIO);
	}

	SBP2_INFO("Reconnected to SBP-2 device");

	return(0);

}

/*
 * This function is called in order to set the busy timeout (number of
 * retries to attempt) on the sbp2 device. 
 */
static int sbp2_set_busy_timeout(struct sbp2scsi_host_info *hi, struct scsi_id_instance_data *scsi_id)
{      
	quadlet_t data;

	SBP2_DEBUG("sbp2_set_busy_timeout");

	/*
	 * Ok, let's write to the target's busy timeout register
	 */
	data = cpu_to_be32(SBP2_BUSY_TIMEOUT_VALUE);

	if (hpsb_node_write(scsi_id->ne, SBP2_BUSY_TIMEOUT_ADDRESS, &data, 4)) {
		SBP2_ERR("sbp2_set_busy_timeout error");
	}

	return(0);
}

/*
 * This function is called to parse sbp2 device's config rom unit
 * directory. Used to determine things like sbp2 management agent offset,
 * and command set used (SCSI or RBC). 
 */
static void sbp2_parse_unit_directory(struct scsi_id_instance_data *scsi_id)
{
	struct unit_directory *ud;
	int i;

	SBP2_DEBUG("sbp2_parse_unit_directory");

	/* Initialize some fields, in case an entry does not exist */
	scsi_id->sbp2_device_type_and_lun = SBP2_DEVICE_TYPE_LUN_UNINITIALIZED;
	scsi_id->sbp2_management_agent_addr = 0x0;
	scsi_id->sbp2_command_set_spec_id = 0x0;
	scsi_id->sbp2_command_set = 0x0;
	scsi_id->sbp2_unit_characteristics = 0x0;
	scsi_id->sbp2_firmware_revision = 0x0;

	ud = scsi_id->ud;

	/* Handle different fields in the unit directory, based on keys */
	for (i = 0; i < ud->count; i++) {
		switch (CONFIG_ROM_KEY(ud->quadlets[i])) {
		case SBP2_CSR_OFFSET_KEY:
			/* Save off the management agent address */
			scsi_id->sbp2_management_agent_addr =
				CSR_REGISTER_BASE + 
				(CONFIG_ROM_VALUE(ud->quadlets[i]) << 2);

			SBP2_DEBUG("sbp2_management_agent_addr = %x",
				   (unsigned int) scsi_id->sbp2_management_agent_addr);
			break;

		case SBP2_COMMAND_SET_SPEC_ID_KEY:
			/* Command spec organization */
			scsi_id->sbp2_command_set_spec_id
				= CONFIG_ROM_VALUE(ud->quadlets[i]);
			SBP2_DEBUG("sbp2_command_set_spec_id = %x",
				   (unsigned int) scsi_id->sbp2_command_set_spec_id);
			break;

		case SBP2_COMMAND_SET_KEY:
			/* Command set used by sbp2 device */
			scsi_id->sbp2_command_set
				= CONFIG_ROM_VALUE(ud->quadlets[i]);
			SBP2_DEBUG("sbp2_command_set = %x",
				   (unsigned int) scsi_id->sbp2_command_set);
			break;

		case SBP2_UNIT_CHARACTERISTICS_KEY:
			/*
			 * Unit characterisitcs (orb related stuff
			 * that I'm not yet paying attention to)
			 */
			scsi_id->sbp2_unit_characteristics
				= CONFIG_ROM_VALUE(ud->quadlets[i]);
			SBP2_DEBUG("sbp2_unit_characteristics = %x",
				   (unsigned int) scsi_id->sbp2_unit_characteristics);
			break;

		case SBP2_DEVICE_TYPE_AND_LUN_KEY:
			/*
			 * Device type and lun (used for
			 * detemining type of sbp2 device)
			 */
			scsi_id->sbp2_device_type_and_lun
				= CONFIG_ROM_VALUE(ud->quadlets[i]);
			SBP2_DEBUG("sbp2_device_type_and_lun = %x",
				   (unsigned int) scsi_id->sbp2_device_type_and_lun);
			break;

		case SBP2_FIRMWARE_REVISION_KEY:
			/* Firmware revision */
			scsi_id->sbp2_firmware_revision
				= CONFIG_ROM_VALUE(ud->quadlets[i]);
			if (sbp2_force_inquiry_hack)
				SBP2_INFO("sbp2_firmware_revision = %x",
				   (unsigned int) scsi_id->sbp2_firmware_revision);
			else	SBP2_DEBUG("sbp2_firmware_revision = %x",
				   (unsigned int) scsi_id->sbp2_firmware_revision);
			break;

		default:
			break;
		}
	}

	/* This is the start of our broken device checking. We try to hack
	 * around oddities and known defects.  */
	scsi_id->workarounds = 0x0;

	/* If the vendor id is 0xa0b8 (Symbios vendor id), then we have a
	 * bridge with 128KB max transfer size limitation. For sanity, we
	 * only voice this when the current sbp2_max_sectors setting
	 * exceeds the 128k limit. By default, that is not the case.
	 *
	 * It would be really nice if we could detect this before the scsi
	 * host gets initialized. That way we can down-force the
	 * sbp2_max_sectors to account for it. That is not currently
	 * possible.  */
	if ((scsi_id->sbp2_firmware_revision & 0xffff00) ==
			SBP2_128KB_BROKEN_FIRMWARE &&
			(sbp2_max_sectors * 512) > (128 * 1024)) {
		SBP2_WARN("Node " NODE_BUS_FMT ": Bridge only supports 128KB max transfer size.",
				NODE_BUS_ARGS(scsi_id->ne->nodeid));
		SBP2_WARN("WARNING: Current sbp2_max_sectors setting is larger than 128KB (%d sectors)!",
				sbp2_max_sectors);
		scsi_id->workarounds |= SBP2_BREAKAGE_128K_MAX_TRANSFER;
	}

	/* Check for a blacklisted set of devices that require us to force
	 * a 36 byte host inquiry. This can be overriden as a module param
	 * (to force all hosts).  */
	for (i = 0; i < NUM_BROKEN_INQUIRY_DEVS; i++) {
		if ((scsi_id->sbp2_firmware_revision & 0xffff00) ==
				sbp2_broken_inquiry_list[i]) {
			SBP2_WARN("Node " NODE_BUS_FMT ": Using 36byte inquiry workaround",
					NODE_BUS_ARGS(scsi_id->ne->nodeid));
			scsi_id->workarounds |= SBP2_BREAKAGE_INQUIRY_HACK;
			break; /* No need to continue. */
		}
	}
}

/*
 * This function is called in order to determine the max speed and packet
 * size we can use in our ORBs. Note, that we (the driver and host) only
 * initiate the transaction. The SBP-2 device actually transfers the data
 * (by reading from the DMA area we tell it). This means that the SBP-2
 * device decides the actual maximum data it can transfer. We just tell it
 * the speed that it needs to use, and the max_rec the host supports, and
 * it takes care of the rest.
 */
static int sbp2_max_speed_and_size(struct sbp2scsi_host_info *hi, struct scsi_id_instance_data *scsi_id)
{
	SBP2_DEBUG("sbp2_max_speed_and_size");

	/* Initial setting comes from the hosts speed map */
	scsi_id->speed_code = hi->host->speed_map[(hi->host->node_id & NODE_MASK) * 64
						  + (scsi_id->ne->nodeid & NODE_MASK)];

	/* Bump down our speed if the user requested it */
	if (scsi_id->speed_code > sbp2_max_speed) {
		scsi_id->speed_code = sbp2_max_speed;
		SBP2_ERR("Forcing SBP-2 max speed down to %s",
			 hpsb_speedto_str[scsi_id->speed_code]);
	}

	/* Payload size is the lesser of what our speed supports and what
	 * our host supports.  */
	scsi_id->max_payload_size = min(sbp2_speedto_maxrec[scsi_id->speed_code],
					(u8)(((be32_to_cpu(hi->host->csr.rom[2]) >> 12) & 0xf) - 1));

	SBP2_ERR("Node[" NODE_BUS_FMT "]: Max speed [%s] - Max payload [%u]",
		 NODE_BUS_ARGS(scsi_id->ne->nodeid), hpsb_speedto_str[scsi_id->speed_code],
		 1 << ((u32)scsi_id->max_payload_size + 2));

	return(0);
}

/*
 * This function is called in order to perform a SBP-2 agent reset. 
 */
static int sbp2_agent_reset(struct sbp2scsi_host_info *hi, struct scsi_id_instance_data *scsi_id, u32 flags) 
{
	struct sbp2_request_packet *agent_reset_request_packet;

	SBP2_DEBUG("sbp2_agent_reset");

	/*
	 * Ok, let's write to the target's management agent register
	 */
	agent_reset_request_packet =
		sbp2util_allocate_write_request_packet(hi, scsi_id->ne,
						       scsi_id->sbp2_command_block_agent_addr +
						       SBP2_AGENT_RESET_OFFSET,
						       0, ntohl(SBP2_AGENT_RESET_DATA));

	if (!agent_reset_request_packet) {
		SBP2_ERR("sbp2util_allocate_write_request_packet failed");
		return(-EIO);
	}

	if (!hpsb_send_packet(agent_reset_request_packet->packet)) {
		SBP2_ERR("hpsb_send_packet failed");
		sbp2util_free_request_packet(agent_reset_request_packet); 
		return(-EIO);
	}

	if (!(flags & SBP2_SEND_NO_WAIT)) {
		down(&agent_reset_request_packet->packet->state_change);
		down(&agent_reset_request_packet->packet->state_change);
	}

	/*
	 * Need to make sure orb pointer is written on next command
	 */
	scsi_id->last_orb = NULL;

	return(0);

}

/*
 * This function is called to create the actual command orb and s/g list
 * out of the scsi command itself.
 */
static int sbp2_create_command_orb(struct sbp2scsi_host_info *hi, 
				   struct scsi_id_instance_data *scsi_id,
				   struct sbp2_command_info *command,
				   unchar *scsi_cmd,
				   unsigned int scsi_use_sg,
				   unsigned int scsi_request_bufflen,
				   void *scsi_request_buffer, 
				   unsigned char scsi_dir)
{
	struct scatterlist *sgpnt = (struct scatterlist *) scsi_request_buffer;
	struct sbp2_command_orb *command_orb = &command->command_orb;
	struct sbp2_unrestricted_page_table *scatter_gather_element =
		&command->scatter_gather_element[0];
	int dma_dir = scsi_to_pci_dma_dir (scsi_dir);
	u32 sg_count, sg_len, orb_direction;
	dma_addr_t sg_addr;
	int i;

	/*
	 * Set-up our command ORB..
	 *
	 * NOTE: We're doing unrestricted page tables (s/g), as this is
	 * best performance (at least with the devices I have). This means
	 * that data_size becomes the number of s/g elements, and
	 * page_size should be zero (for unrestricted).
	 */
	command_orb->next_ORB_hi = ORB_SET_NULL_PTR(1);
	command_orb->next_ORB_lo = 0x0;
	command_orb->misc = ORB_SET_MAX_PAYLOAD(scsi_id->max_payload_size);
	command_orb->misc |= ORB_SET_SPEED(scsi_id->speed_code);
	command_orb->misc |= ORB_SET_NOTIFY(1);		/* Notify us when complete */

	/*
	 * Get the direction of the transfer. If the direction is unknown, then use our
	 * goofy table as a back-up.
	 */
	switch (scsi_dir) {
		case SCSI_DATA_NONE:
			orb_direction = ORB_DIRECTION_NO_DATA_TRANSFER;
			break;
		case SCSI_DATA_WRITE:
			orb_direction = ORB_DIRECTION_WRITE_TO_MEDIA;
			break;
		case SCSI_DATA_READ:
			orb_direction = ORB_DIRECTION_READ_FROM_MEDIA;
			break;
		case SCSI_DATA_UNKNOWN:
		default:
			SBP2_ERR("SCSI data transfer direction not specified. "
				 "Update the SBP2 direction table in sbp2.h if " 
				 "necessary for your application");
			print_command (scsi_cmd);
			orb_direction = sbp2scsi_direction_table[*scsi_cmd];
			break;
	}

	/*
	 * Set-up our pagetable stuff... unfortunately, this has become
	 * messier than I'd like. Need to clean this up a bit.   ;-)
	 */
	if (orb_direction == ORB_DIRECTION_NO_DATA_TRANSFER) {

		SBP2_DEBUG("No data transfer");

		/*
		 * Handle no data transfer
		 */
		command_orb->data_descriptor_hi = 0x0;
		command_orb->data_descriptor_lo = 0x0;
		command_orb->misc |= ORB_SET_DIRECTION(1);

	} else if (scsi_use_sg) {

		SBP2_DEBUG("Use scatter/gather");

		/*
		 * Special case if only one element (and less than 64KB in size)
		 */
		if ((scsi_use_sg == 1) && (sgpnt[0].length <= SBP2_MAX_SG_ELEMENT_LENGTH)) {

			SBP2_DEBUG("Only one s/g element");
			command->dma_dir = dma_dir;
			command->dma_size = sgpnt[0].length;
			command->dma_type = CMD_DMA_PAGE;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,13)
			command->cmd_dma = pci_map_single (hi->host->pdev, sgpnt[0].address,
							   command->dma_size,
							   command->dma_dir);
#else
			command->cmd_dma = pci_map_page(hi->host->pdev,
							sgpnt[0].page,
							sgpnt[0].offset,
							command->dma_size,
							command->dma_dir);
#endif /* Linux version < 2.4.13 */
			SBP2_DMA_ALLOC("single page scatter element");

			command_orb->data_descriptor_hi = ORB_SET_NODE_ID(hi->host->node_id);
			command_orb->data_descriptor_lo = command->cmd_dma;
			command_orb->misc |= ORB_SET_DATA_SIZE(command->dma_size);
			command_orb->misc |= ORB_SET_DIRECTION(orb_direction);

		} else {
			int count = pci_map_sg(hi->host->pdev, sgpnt, scsi_use_sg, dma_dir);
			SBP2_DMA_ALLOC("scatter list");

			command->dma_size = scsi_use_sg;
			command->dma_dir = dma_dir;
			command->sge_buffer = sgpnt;

			/* use page tables (s/g) */
			command_orb->misc |= ORB_SET_PAGE_TABLE_PRESENT(0x1);
			command_orb->misc |= ORB_SET_DIRECTION(orb_direction);
			command_orb->data_descriptor_hi = ORB_SET_NODE_ID(hi->host->node_id);
			command_orb->data_descriptor_lo = command->sge_dma;

			/*
			 * Loop through and fill out our sbp-2 page tables
			 * (and split up anything too large)
			 */
			for (i = 0, sg_count = 0 ; i < count; i++, sgpnt++) {
				sg_len = sg_dma_len(sgpnt);
				sg_addr = sg_dma_address(sgpnt);
				while (sg_len) {
					scatter_gather_element[sg_count].segment_base_lo = sg_addr;
					if (sg_len > SBP2_MAX_SG_ELEMENT_LENGTH) {
						scatter_gather_element[sg_count].length_segment_base_hi =  
							PAGE_TABLE_SET_SEGMENT_LENGTH(SBP2_MAX_SG_ELEMENT_LENGTH);
						sg_addr += SBP2_MAX_SG_ELEMENT_LENGTH;
						sg_len -= SBP2_MAX_SG_ELEMENT_LENGTH;
					} else {
						scatter_gather_element[sg_count].length_segment_base_hi = 
							PAGE_TABLE_SET_SEGMENT_LENGTH(sg_len);
						sg_len = 0;
					}
					sg_count++;
				}
			}

			/* Number of page table (s/g) elements */
			command_orb->misc |= ORB_SET_DATA_SIZE(sg_count);

			sbp2util_packet_dump(scatter_gather_element, 
					     (sizeof(struct sbp2_unrestricted_page_table)) * sg_count, 
					     "sbp2 s/g list", command->sge_dma);

			/*
			 * Byte swap page tables if necessary
			 */
			sbp2util_cpu_to_be32_buffer(scatter_gather_element, 
						    (sizeof(struct sbp2_unrestricted_page_table)) *
						    sg_count);

		}

	} else {

		SBP2_DEBUG("No scatter/gather");

		command->dma_dir = dma_dir;
		command->dma_size = scsi_request_bufflen;
		command->dma_type = CMD_DMA_SINGLE;
		command->cmd_dma = pci_map_single (hi->host->pdev, scsi_request_buffer,
						   command->dma_size,
						   command->dma_dir);
		SBP2_DMA_ALLOC("single bulk");

		/*
		 * Handle case where we get a command w/o s/g enabled (but
		 * check for transfers larger than 64K)
		 */
		if (scsi_request_bufflen <= SBP2_MAX_SG_ELEMENT_LENGTH) {

			command_orb->data_descriptor_hi = ORB_SET_NODE_ID(hi->host->node_id);
			command_orb->data_descriptor_lo = command->cmd_dma;
			command_orb->misc |= ORB_SET_DATA_SIZE(scsi_request_bufflen);
			command_orb->misc |= ORB_SET_DIRECTION(orb_direction);

			/*
			 * Sanity, in case our direction table is not
			 * up-to-date
			 */
			if (!scsi_request_bufflen) {
				command_orb->data_descriptor_hi = 0x0;
				command_orb->data_descriptor_lo = 0x0;
				command_orb->misc |= ORB_SET_DIRECTION(1);
			}

		} else {
			/*
			 * Need to turn this into page tables, since the
			 * buffer is too large.
			 */                     
			command_orb->data_descriptor_hi = ORB_SET_NODE_ID(hi->host->node_id);
			command_orb->data_descriptor_lo = command->sge_dma;

			/* Use page tables (s/g) */
			command_orb->misc |= ORB_SET_PAGE_TABLE_PRESENT(0x1);
			command_orb->misc |= ORB_SET_DIRECTION(orb_direction);

			/*
			 * fill out our sbp-2 page tables (and split up
			 * the large buffer)
			 */
			sg_count = 0;
			sg_len = scsi_request_bufflen;
			sg_addr = command->cmd_dma;
			while (sg_len) {
				scatter_gather_element[sg_count].segment_base_lo = sg_addr;
				if (sg_len > SBP2_MAX_SG_ELEMENT_LENGTH) {
					scatter_gather_element[sg_count].length_segment_base_hi = 
						PAGE_TABLE_SET_SEGMENT_LENGTH(SBP2_MAX_SG_ELEMENT_LENGTH);
					sg_addr += SBP2_MAX_SG_ELEMENT_LENGTH;
					sg_len -= SBP2_MAX_SG_ELEMENT_LENGTH;
				} else {
					scatter_gather_element[sg_count].length_segment_base_hi = 
						PAGE_TABLE_SET_SEGMENT_LENGTH(sg_len);
					sg_len = 0;
				}
				sg_count++;
			}

			/* Number of page table (s/g) elements */
			command_orb->misc |= ORB_SET_DATA_SIZE(sg_count);

			sbp2util_packet_dump(scatter_gather_element, 
					     (sizeof(struct sbp2_unrestricted_page_table)) * sg_count, 
					     "sbp2 s/g list", command->sge_dma);

			/*
			 * Byte swap page tables if necessary
			 */
			sbp2util_cpu_to_be32_buffer(scatter_gather_element, 
						    (sizeof(struct sbp2_unrestricted_page_table)) *
						     sg_count);

		}

	}

	/*
	 * Byte swap command ORB if necessary
	 */
	sbp2util_cpu_to_be32_buffer(command_orb, sizeof(struct sbp2_command_orb));

	/*
	 * Put our scsi command in the command ORB
	 */
	memset(command_orb->cdb, 0, 12);
	memcpy(command_orb->cdb, scsi_cmd, COMMAND_SIZE(*scsi_cmd));

	return(0);
}
 
/*
 * This function is called in order to begin a regular SBP-2 command. 
 */
static int sbp2_link_orb_command(struct sbp2scsi_host_info *hi, struct scsi_id_instance_data *scsi_id,
				 struct sbp2_command_info *command)
{
        struct sbp2_request_packet *command_request_packet;
	struct sbp2_command_orb *command_orb = &command->command_orb;

	outstanding_orb_incr;
	SBP2_ORB_DEBUG("sending command orb %p, total orbs = %x",
			command_orb, global_outstanding_command_orbs);

	pci_dma_sync_single(hi->host->pdev, command->command_orb_dma,
			    sizeof(struct sbp2_command_orb),
			    PCI_DMA_BIDIRECTIONAL);
	pci_dma_sync_single(hi->host->pdev, command->sge_dma,
			    sizeof(command->scatter_gather_element),
			    PCI_DMA_BIDIRECTIONAL);
	/*
	 * Check to see if there are any previous orbs to use
	 */
	if (scsi_id->last_orb == NULL) {
	
		/*
		 * Ok, let's write to the target's management agent register
		 */
		if (hpsb_node_entry_valid(scsi_id->ne)) {

			command_request_packet =
				sbp2util_allocate_write_request_packet(hi, scsi_id->ne,
								scsi_id->sbp2_command_block_agent_addr +
								SBP2_ORB_POINTER_OFFSET, 8, 0);
		
			if (!command_request_packet) {
				SBP2_ERR("sbp2util_allocate_write_request_packet failed");
				return(-EIO);
			}
		
			command_request_packet->packet->data[0] = ORB_SET_NODE_ID(hi->host->node_id);
			command_request_packet->packet->data[1] = command->command_orb_dma;
			sbp2util_cpu_to_be32_buffer(command_request_packet->packet->data, 8);
		
			SBP2_ORB_DEBUG("write command agent, command orb %p", command_orb);

			if (!hpsb_send_packet(command_request_packet->packet)) {
				SBP2_ERR("hpsb_send_packet failed");
				sbp2util_free_request_packet(command_request_packet); 
				return(-EIO);
			}

			SBP2_ORB_DEBUG("write command agent complete");
		}

		scsi_id->last_orb = command_orb;
		scsi_id->last_orb_dma = command->command_orb_dma;

	} else {

		/*
		 * We have an orb already sent (maybe or maybe not
		 * processed) that we can append this orb to. So do so,
		 * and ring the doorbell. Have to be very careful
		 * modifying these next orb pointers, as they are accessed
		 * both by the sbp2 device and us.
		 */
		scsi_id->last_orb->next_ORB_lo =
			cpu_to_be32(command->command_orb_dma);
		/* Tells hardware that this pointer is valid */
		scsi_id->last_orb->next_ORB_hi = 0x0;
		pci_dma_sync_single(hi->host->pdev, scsi_id->last_orb_dma,
				    sizeof(struct sbp2_command_orb),
				    PCI_DMA_BIDIRECTIONAL);

		/*
		 * Ring the doorbell
		 */
		if (hpsb_node_entry_valid(scsi_id->ne)) {

			command_request_packet = sbp2util_allocate_write_request_packet(hi,
				scsi_id->ne,
				scsi_id->sbp2_command_block_agent_addr + SBP2_DOORBELL_OFFSET,
				0, cpu_to_be32(command->command_orb_dma));
	
			if (!command_request_packet) {
				SBP2_ERR("sbp2util_allocate_write_request_packet failed");
				return(-EIO);
			}

			SBP2_ORB_DEBUG("ring doorbell, command orb %p", command_orb);

			if (!hpsb_send_packet(command_request_packet->packet)) {
				SBP2_ERR("hpsb_send_packet failed");
				sbp2util_free_request_packet(command_request_packet);
				return(-EIO);
			}
		}

		scsi_id->last_orb = command_orb;
		scsi_id->last_orb_dma = command->command_orb_dma;

	}
       	return(0);
}

/*
 * This function is called in order to begin a regular SBP-2 command. 
 */
static int sbp2_send_command(struct sbp2scsi_host_info *hi, struct scsi_id_instance_data *scsi_id,
			     Scsi_Cmnd *SCpnt, void (*done)(Scsi_Cmnd *))
{
	unchar *cmd = (unchar *) SCpnt->cmnd;
	unsigned int request_bufflen = SCpnt->request_bufflen;
	struct sbp2_command_info *command;

	SBP2_DEBUG("sbp2_send_command");
#if (CONFIG_IEEE1394_SBP2_DEBUG >= 2) || defined(CONFIG_IEEE1394_SBP2_PACKET_DUMP)
	printk("[scsi command]\n   ");
	print_command (cmd);
#endif
	SBP2_DEBUG("SCSI transfer size = %x", request_bufflen);
	SBP2_DEBUG("SCSI s/g elements = %x", (unsigned int)SCpnt->use_sg);

	/*
	 * Allocate a command orb and s/g structure
	 */
	command = sbp2util_allocate_command_orb(scsi_id, SCpnt, done, hi);
	if (!command) {
		return(-EIO);
	}

	/*
	 * The scsi stack sends down a request_bufflen which does not match the
	 * length field in the scsi cdb. This causes some sbp2 devices to 
	 * reject this inquiry command. Fix the request_bufflen. 
	 */
	if (*cmd == INQUIRY) {
		if (sbp2_force_inquiry_hack || scsi_id->workarounds & SBP2_BREAKAGE_INQUIRY_HACK)
			request_bufflen = cmd[4] = 0x24;
		else
			request_bufflen = cmd[4];
	}

	/*
	 * Now actually fill in the comamnd orb and sbp2 s/g list
	 */
	sbp2_create_command_orb(hi, scsi_id, command, cmd, SCpnt->use_sg,
				request_bufflen, SCpnt->request_buffer,
				SCpnt->sc_data_direction); 
	/*
	 * Update our cdb if necessary (to handle sbp2 RBC command set
	 * differences). This is where the command set hacks go!   =)
	 */
	sbp2_check_sbp2_command(scsi_id, command->command_orb.cdb);

	sbp2util_packet_dump(&command->command_orb, sizeof(struct sbp2_command_orb), 
			     "sbp2 command orb", command->command_orb_dma);

	/*
	 * Initialize status fifo
	 */
	memset(&scsi_id->status_block, 0, sizeof(struct sbp2_status_block));

	/*
	 * Link up the orb, and ring the doorbell if needed
	 */
	sbp2_link_orb_command(hi, scsi_id, command);
	
	return(0);
}


/*
 * This function deals with command set differences between Linux scsi
 * command set and sbp2 RBC command set.
 */
static void sbp2_check_sbp2_command(struct scsi_id_instance_data *scsi_id, unchar *cmd)
{
	unchar new_cmd[16];
	u8 device_type = SBP2_DEVICE_TYPE (scsi_id->sbp2_device_type_and_lun);

	SBP2_DEBUG("sbp2_check_sbp2_command");

	switch (*cmd) {
		
		case READ_6:

			if (sbp2_command_conversion_device_type(device_type)) {

				SBP2_DEBUG("Convert READ_6 to READ_10");
					    
				/*
				 * Need to turn read_6 into read_10
				 */
				new_cmd[0] = 0x28;
				new_cmd[1] = (cmd[1] & 0xe0);
				new_cmd[2] = 0x0;
				new_cmd[3] = (cmd[1] & 0x1f);
				new_cmd[4] = cmd[2];
				new_cmd[5] = cmd[3];
				new_cmd[6] = 0x0;
				new_cmd[7] = 0x0;
				new_cmd[8] = cmd[4];
				new_cmd[9] = cmd[5];
	
				memcpy(cmd, new_cmd, 10);

			}

			break;

		case WRITE_6:

			if (sbp2_command_conversion_device_type(device_type)) {

				SBP2_DEBUG("Convert WRITE_6 to WRITE_10");
	
				/*
				 * Need to turn write_6 into write_10
				 */
				new_cmd[0] = 0x2a;
				new_cmd[1] = (cmd[1] & 0xe0);
				new_cmd[2] = 0x0;
				new_cmd[3] = (cmd[1] & 0x1f);
				new_cmd[4] = cmd[2];
				new_cmd[5] = cmd[3];
				new_cmd[6] = 0x0;
				new_cmd[7] = 0x0;
				new_cmd[8] = cmd[4];
				new_cmd[9] = cmd[5];
	
				memcpy(cmd, new_cmd, 10);

			}

			break;

		case MODE_SENSE:

			if (sbp2_command_conversion_device_type(device_type)) {

				SBP2_DEBUG("Convert MODE_SENSE_6 to MODE_SENSE_10");

				/*
				 * Need to turn mode_sense_6 into mode_sense_10
				 */
				new_cmd[0] = 0x5a;
				new_cmd[1] = cmd[1];
				new_cmd[2] = cmd[2];
				new_cmd[3] = 0x0;
				new_cmd[4] = 0x0;
				new_cmd[5] = 0x0;
				new_cmd[6] = 0x0;
				new_cmd[7] = 0x0;
				new_cmd[8] = cmd[4];
				new_cmd[9] = cmd[5];
	
				memcpy(cmd, new_cmd, 10);

			}

			break;

		case MODE_SELECT:

			/*
			 * TODO. Probably need to change mode select to 10 byte version
			 */

		default:
			break;
	}

	return;
}

/*
 * Translates SBP-2 status into SCSI sense data for check conditions
 */
static unsigned int sbp2_status_to_sense_data(unchar *sbp2_status, unchar *sense_data)
{
	SBP2_DEBUG("sbp2_status_to_sense_data");

	/*
	 * Ok, it's pretty ugly...   ;-)
	 */
	sense_data[0] = 0x70;
	sense_data[1] = 0x0;
	sense_data[2] = sbp2_status[9];
	sense_data[3] = sbp2_status[12];
	sense_data[4] = sbp2_status[13];
	sense_data[5] = sbp2_status[14];
	sense_data[6] = sbp2_status[15];
	sense_data[7] = 10;
	sense_data[8] = sbp2_status[16];
	sense_data[9] = sbp2_status[17];
	sense_data[10] = sbp2_status[18];
	sense_data[11] = sbp2_status[19];
	sense_data[12] = sbp2_status[10];
	sense_data[13] = sbp2_status[11];
	sense_data[14] = sbp2_status[20];
	sense_data[15] = sbp2_status[21];

	return(sbp2_status[8] & 0x3f);	/* return scsi status */
}

/*
 * This function is called after a command is completed, in order to do any necessary SBP-2
 * response data translations for the SCSI stack
 */
static void sbp2_check_sbp2_response(struct scsi_id_instance_data *scsi_id, 
				     Scsi_Cmnd *SCpnt)
{
	u8 *scsi_buf = SCpnt->request_buffer;
	u8 device_type = SBP2_DEVICE_TYPE (scsi_id->sbp2_device_type_and_lun);

	SBP2_DEBUG("sbp2_check_sbp2_response");

	switch (SCpnt->cmnd[0]) {
		
		case INQUIRY:

			/*
			 * If scsi_id->sbp2_device_type_and_lun is uninitialized, then fill 
			 * this information in from the inquiry response data. Lun is set to zero.
			 */
			if (scsi_id->sbp2_device_type_and_lun == SBP2_DEVICE_TYPE_LUN_UNINITIALIZED) {
				SBP2_DEBUG("Creating sbp2_device_type_and_lun from scsi inquiry data");
				scsi_id->sbp2_device_type_and_lun = (scsi_buf[0] & 0x1f) << 16;
			}

			/*
			 * Make sure data length is ok. Minimum length is 36 bytes
			 */
			if (scsi_buf[4] == 0) {
				scsi_buf[4] = 36 - 5;
			}

			/*
			 * Check for Simple Direct Access Device and change it to TYPE_DISK
			 */
			if ((scsi_buf[0] & 0x1f) == TYPE_SDAD) {
				SBP2_DEBUG("Changing TYPE_SDAD to TYPE_DISK");
				scsi_buf[0] &= 0xe0;
			}

			/*
			 * Fix ansi revision and response data format
			 */
			scsi_buf[2] |= 2;
			scsi_buf[3] = (scsi_buf[3] & 0xf0) | 2;

			break;

		case MODE_SENSE:

			if (sbp2_command_conversion_device_type(device_type)) {
			
				SBP2_DEBUG("Modify mode sense response (10 byte version)");

				scsi_buf[0] = scsi_buf[1];	/* Mode data length */
				scsi_buf[1] = scsi_buf[2];	/* Medium type */
				scsi_buf[2] = scsi_buf[3];	/* Device specific parameter */
				scsi_buf[3] = scsi_buf[7];	/* Block descriptor length */
				memcpy(scsi_buf + 4, scsi_buf + 8, scsi_buf[0]);
	
			}

			break;

		case MODE_SELECT:

			/*
			 * TODO. Probably need to change mode select to 10 byte version
			 */

		default:
			break;
	}
	return;
}

/*
 * This function deals with status writes from the SBP-2 device
 */
static int sbp2_handle_status_write(struct hpsb_host *host, int nodeid, int destid,
				    quadlet_t *data, u64 addr, unsigned int length)
{
	struct sbp2scsi_host_info *hi = NULL;
	struct scsi_id_instance_data *scsi_id = NULL;
	u32 id;
	unsigned long flags;
	Scsi_Cmnd *SCpnt = NULL;
	u32 scsi_status = SBP2_SCSI_STATUS_GOOD;
	struct sbp2_command_info *command;

	SBP2_DEBUG("sbp2_handle_status_write");

	sbp2util_packet_dump(data, length, "sbp2 status write by device", (u32)addr);

	if (!host) {
		SBP2_ERR("host is NULL - this is bad!");
		return(RCODE_ADDRESS_ERROR);
	}

	sbp2_spin_lock(&sbp2_host_info_lock, flags);
	hi = sbp2_find_host_info(host);
	sbp2_spin_unlock(&sbp2_host_info_lock, flags);

	if (!hi) {
		SBP2_ERR("host info is NULL - this is bad!");
		return(RCODE_ADDRESS_ERROR);
	}

	sbp2_spin_lock(&hi->sbp2_command_lock, flags);

	/*
	 * Find our scsi_id structure by looking at the status fifo address written to by
	 * the sbp2 device.
	 */
	id = SBP2_STATUS_FIFO_OFFSET_TO_ENTRY((u32)(addr - SBP2_STATUS_FIFO_ADDRESS)); 
	scsi_id = hi->scsi_id[id];

	if (!scsi_id) {
		SBP2_ERR("scsi_id is NULL - device is gone?");
		sbp2_spin_unlock(&hi->sbp2_command_lock, flags);
		return(RCODE_ADDRESS_ERROR);
	}

	/*
	 * Put response into scsi_id status fifo... 
	 */
	memcpy(&scsi_id->status_block, data, length);

	/*
	 * Byte swap first two quadlets (8 bytes) of status for processing
	 */
	sbp2util_be32_to_cpu_buffer(&scsi_id->status_block, 8);

	/*
	 * Handle command ORB status here if necessary. First, need to match status with command.
	 */
	command = sbp2util_find_command_for_orb(scsi_id, scsi_id->status_block.ORB_offset_lo);
	if (command) {

		SBP2_DEBUG("Found status for command ORB");
		pci_dma_sync_single(hi->host->pdev, command->command_orb_dma,
				    sizeof(struct sbp2_command_orb),
				    PCI_DMA_BIDIRECTIONAL);
		pci_dma_sync_single(hi->host->pdev, command->sge_dma,
				    sizeof(command->scatter_gather_element),
				    PCI_DMA_BIDIRECTIONAL);

		SBP2_ORB_DEBUG("matched command orb %p", &command->command_orb);
		outstanding_orb_decr;

		/*
		 * Matched status with command, now grab scsi command pointers and check status
		 */
		SCpnt = command->Current_SCpnt;
		sbp2util_mark_command_completed(scsi_id, command);

		if (SCpnt) {

			/*
			 * See if the target stored any scsi status information
			 */
			if (STATUS_GET_LENGTH(scsi_id->status_block.ORB_offset_hi_misc) > 1) {
				/*
				 * Translate SBP-2 status to SCSI sense data
				 */
				SBP2_DEBUG("CHECK CONDITION");
				scsi_status = sbp2_status_to_sense_data((unchar *)&scsi_id->status_block, SCpnt->sense_buffer);
			}

			/*
			 * Check to see if the dead bit is set. If so, we'll have to initiate
			 * a fetch agent reset.
			 */
			if (STATUS_GET_DEAD_BIT(scsi_id->status_block.ORB_offset_hi_misc)) {

				/*
				 * Initiate a fetch agent reset. 
				 */
				SBP2_DEBUG("Dead bit set - initiating fetch agent reset");
                                sbp2_agent_reset(hi, scsi_id, SBP2_SEND_NO_WAIT);
			}

			SBP2_ORB_DEBUG("completing command orb %p", &command->command_orb);

			/*
			 * Complete the SCSI command
			 */
			SBP2_DEBUG("Completing SCSI command");
			sbp2scsi_complete_command(hi, scsi_id, scsi_status, SCpnt, command->Current_done);
			SBP2_ORB_DEBUG("command orb completed");
		}

		/*
		 * Check here to see if there are no commands in-use. If there are none, we can
		 * null out last orb so that next time around we write directly to the orb pointer... 
		 * Quick start saves one 1394 bus transaction.
		 */
		if (list_empty(&scsi_id->sbp2_command_orb_inuse)) {
			scsi_id->last_orb = NULL;
		}

	} else {
		
		/* 
		 * It's probably a login/logout/reconnect status.
		 */
		if ((scsi_id->login_orb_dma == scsi_id->status_block.ORB_offset_lo) ||
		    (scsi_id->reconnect_orb_dma == scsi_id->status_block.ORB_offset_lo) ||
		    (scsi_id->logout_orb_dma == scsi_id->status_block.ORB_offset_lo)) {
			atomic_set(&scsi_id->sbp2_login_complete, 1);
		}
	}

	sbp2_spin_unlock(&hi->sbp2_command_lock, flags);
	return(RCODE_COMPLETE);
}


/**************************************
 * SCSI interface related section
 **************************************/

/*
 * This routine is the main request entry routine for doing I/O. It is 
 * called from the scsi stack directly.
 */
static int sbp2scsi_queuecommand (Scsi_Cmnd *SCpnt, void (*done)(Scsi_Cmnd *)) 
{
	struct sbp2scsi_host_info *hi = NULL;
	struct scsi_id_instance_data *scsi_id = NULL;
	unsigned long flags;

	SBP2_DEBUG("sbp2scsi_queuecommand");

	/*
	 * Pull our host info and scsi id instance data from the scsi command
	 */
	hi = (struct sbp2scsi_host_info *) SCpnt->host->hostdata[0];

	if (!hi) {
		SBP2_ERR("sbp2scsi_host_info is NULL - this is bad!");
		SCpnt->result = DID_NO_CONNECT << 16;
		done (SCpnt);
		return(0);
	}

	scsi_id = hi->scsi_id[SCpnt->target];

	/*
	 * If scsi_id is null, it means there is no device in this slot,
	 * so we should return selection timeout.
	 */
	if (!scsi_id) {
		SCpnt->result = DID_NO_CONNECT << 16;
		done (SCpnt);
		return(0);
	}

	/*
	 * Until we handle multiple luns, just return selection time-out
	 * to any IO directed at non-zero LUNs
	 */
	if (SCpnt->lun) {
		SCpnt->result = DID_NO_CONNECT << 16;
		done (SCpnt);
		return(0);
	}

	/*
	 * Check for request sense command, and handle it here
	 * (autorequest sense)
	 */
	if (SCpnt->cmnd[0] == REQUEST_SENSE) {
		SBP2_DEBUG("REQUEST_SENSE");
		memcpy(SCpnt->request_buffer, SCpnt->sense_buffer, SCpnt->request_bufflen);
		memset(SCpnt->sense_buffer, 0, sizeof(SCpnt->sense_buffer));
		sbp2scsi_complete_command(hi, scsi_id, SBP2_SCSI_STATUS_GOOD, SCpnt, done);
		return(0);
	}

	/*
	 * Check to see if we are in the middle of a bus reset.
	 */
	if (!hpsb_node_entry_valid(scsi_id->ne)) {
		SBP2_ERR("Bus reset in progress - rejecting command");
		SCpnt->result = DID_BUS_BUSY << 16;
		done (SCpnt);
		return(0);
	}

	/*
	 * Try and send our SCSI command
	 */
	sbp2_spin_lock(&hi->sbp2_command_lock, flags);
	if (sbp2_send_command(hi, scsi_id, SCpnt, done)) {
		SBP2_ERR("Error sending SCSI command");
		sbp2scsi_complete_command(hi, scsi_id, SBP2_SCSI_STATUS_SELECTION_TIMEOUT, SCpnt, done);
	}
	sbp2_spin_unlock(&hi->sbp2_command_lock, flags);

	return(0);
}

/*
 * This function is called in order to complete all outstanding SBP-2
 * commands (in case of resets, etc.).
 */
static void sbp2scsi_complete_all_commands(struct sbp2scsi_host_info *hi,
					   struct scsi_id_instance_data *scsi_id, 
					   u32 status)
{
	struct list_head *lh;
	struct sbp2_command_info *command;

	SBP2_DEBUG("sbp2_complete_all_commands");

	while (!list_empty(&scsi_id->sbp2_command_orb_inuse)) {
		SBP2_DEBUG("Found pending command to complete");
		lh = scsi_id->sbp2_command_orb_inuse.next;
		command = list_entry(lh, struct sbp2_command_info, list);
		pci_dma_sync_single(hi->host->pdev, command->command_orb_dma,
				    sizeof(struct sbp2_command_orb),
				    PCI_DMA_BIDIRECTIONAL);
		pci_dma_sync_single(hi->host->pdev, command->sge_dma,
				    sizeof(command->scatter_gather_element),
				    PCI_DMA_BIDIRECTIONAL);
		sbp2util_mark_command_completed(scsi_id, command);
		if (command->Current_SCpnt) {
			void (*done)(Scsi_Cmnd *) = command->Current_done;
			command->Current_SCpnt->result = status << 16;
			done (command->Current_SCpnt);
		}
	}

	return;
}

/*
 * This function is called in order to complete a regular SBP-2 command.
 */
static void sbp2scsi_complete_command(struct sbp2scsi_host_info *hi, struct scsi_id_instance_data *scsi_id, u32 scsi_status,
				      Scsi_Cmnd *SCpnt, void (*done)(Scsi_Cmnd *))
{
	SBP2_DEBUG("sbp2scsi_complete_command");

	/*
	 * Sanity
	 */
	if (!SCpnt) {
		SBP2_ERR("SCpnt is NULL");
		return;
	}

	/*
	 * If a bus reset is in progress and there was an error, don't
	 * complete the command, just let it get retried at the end of the
	 * bus reset.
	 */
	if (!hpsb_node_entry_valid(scsi_id->ne) && (scsi_status != SBP2_SCSI_STATUS_GOOD)) {
		SBP2_ERR("Bus reset in progress - retry command later");
		return;
	}
        
	/*
	 * Switch on scsi status
	 */
	switch (scsi_status) {
		case SBP2_SCSI_STATUS_GOOD:
			SCpnt->result = DID_OK;
			break;

		case SBP2_SCSI_STATUS_BUSY:
			SBP2_ERR("SBP2_SCSI_STATUS_BUSY");
			SCpnt->result = DID_BUS_BUSY << 16;
			break;

		case SBP2_SCSI_STATUS_CHECK_CONDITION:
			SBP2_DEBUG("SBP2_SCSI_STATUS_CHECK_CONDITION");
			SCpnt->result = CHECK_CONDITION << 1;

			/*
			 * Debug stuff
			 */
#if CONFIG_IEEE1394_SBP2_DEBUG >= 1
			print_command (SCpnt->cmnd);
			print_sense("bh", SCpnt);
#endif

			break;

		case SBP2_SCSI_STATUS_SELECTION_TIMEOUT:
			SBP2_ERR("SBP2_SCSI_STATUS_SELECTION_TIMEOUT");
			SCpnt->result = DID_NO_CONNECT << 16;
			print_command (SCpnt->cmnd);
			break;

		case SBP2_SCSI_STATUS_CONDITION_MET:
		case SBP2_SCSI_STATUS_RESERVATION_CONFLICT:
		case SBP2_SCSI_STATUS_COMMAND_TERMINATED:
			SBP2_ERR("Bad SCSI status = %x", scsi_status);
			SCpnt->result = DID_ERROR << 16;
			print_command (SCpnt->cmnd);
			break;

		default:
			SBP2_ERR("Unsupported SCSI status = %x", scsi_status);
			SCpnt->result = DID_ERROR << 16;
	}

	/*
	 * Take care of any sbp2 response data mucking here (RBC stuff, etc.)
	 */
	if (SCpnt->result == DID_OK) {
		sbp2_check_sbp2_response(scsi_id, SCpnt);
	}

	/*
	 * If a bus reset is in progress and there was an error, complete
	 * the command as busy so that it will get retried.
	 */
	if (!hpsb_node_entry_valid(scsi_id->ne) && (scsi_status != SBP2_SCSI_STATUS_GOOD)) {
		SBP2_ERR("Completing command with busy (bus reset)");
		SCpnt->result = DID_BUS_BUSY << 16;
	}

	/*
	 * If a unit attention occurs, return busy status so it gets
	 * retried... it could have happened because of a 1394 bus reset
	 * or hot-plug...
	 */

	/*
	 * Tell scsi stack that we're done with this command
	 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
	spin_lock_irq(&io_request_lock);
	done (SCpnt);
	spin_unlock_irq(&io_request_lock);
#else
	spin_lock_irq(hi->scsi_host->host_lock);
	done (SCpnt);
	spin_unlock_irq(hi->scsi_host->host_lock);
#endif

	return;
}

/*
 * Called by scsi stack when something has really gone wrong.  Usually
 * called when a command has timed-out for some reason.
 */
static int sbp2scsi_abort (Scsi_Cmnd *SCpnt) 
{
	struct sbp2scsi_host_info *hi = (struct sbp2scsi_host_info *) SCpnt->host->hostdata[0];
	struct scsi_id_instance_data *scsi_id = hi->scsi_id[SCpnt->target];
	struct sbp2_command_info *command;
	unsigned long flags;

	SBP2_ERR("aborting sbp2 command");
	print_command (SCpnt->cmnd);
        
	if (scsi_id) {

		/*
		 * Right now, just return any matching command structures
		 * to the free pool.
		 */
		sbp2_spin_lock(&hi->sbp2_command_lock, flags);
		command = sbp2util_find_command_for_SCpnt(scsi_id, SCpnt);
		if (command) {
			SBP2_DEBUG("Found command to abort");
			pci_dma_sync_single(hi->host->pdev,
					    command->command_orb_dma,
					    sizeof(struct sbp2_command_orb),
					    PCI_DMA_BIDIRECTIONAL);
			pci_dma_sync_single(hi->host->pdev,
					    command->sge_dma,
					    sizeof(command->scatter_gather_element),
					    PCI_DMA_BIDIRECTIONAL);
			sbp2util_mark_command_completed(scsi_id, command);
			if (command->Current_SCpnt) {
				void (*done)(Scsi_Cmnd *) = command->Current_done;
				command->Current_SCpnt->result = DID_ABORT << 16;
				done (command->Current_SCpnt);
			}
		}

		/*
		 * Initiate a fetch agent reset. 
		 */
		sbp2_agent_reset(hi, scsi_id, SBP2_SEND_NO_WAIT);
		sbp2scsi_complete_all_commands(hi, scsi_id, DID_BUS_BUSY);		
		sbp2_spin_unlock(&hi->sbp2_command_lock, flags);
	}

	return(SUCCESS);
}

/*
 * Called by scsi stack when something has really gone wrong.
 */
static int sbp2scsi_reset (Scsi_Cmnd *SCpnt) 
{
	struct sbp2scsi_host_info *hi = (struct sbp2scsi_host_info *) SCpnt->host->hostdata[0];
	struct scsi_id_instance_data *scsi_id = hi->scsi_id[SCpnt->target];

	SBP2_ERR("reset requested");

	if (scsi_id) {
		SBP2_ERR("Generating sbp2 fetch agent reset");
		sbp2_agent_reset(hi, scsi_id, SBP2_SEND_NO_WAIT);
	}

	return(SUCCESS);
}

/*
 * Called by scsi stack to get bios parameters (used by fdisk, and at boot).
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,28)
static int sbp2scsi_biosparam (Scsi_Disk *disk, kdev_t dev, int geom[]) 
#else
static int sbp2scsi_biosparam (Scsi_Disk *disk, struct block_device *dev, int geom[]) 
#endif
{
	int heads, sectors, cylinders;

	SBP2_DEBUG("Request for bios parameters");

	heads = 64;
	sectors = 32;
	cylinders = disk->capacity / (heads * sectors);

	if (cylinders > 1024) {
		heads = 255;
		sectors = 63;
		cylinders = disk->capacity / (heads * sectors);
	}

	geom[0] = heads;
	geom[1] = sectors;
	geom[2] = cylinders;

	return(0);
}

/*
 * Called by scsi stack after scsi driver is registered
 */
static int sbp2scsi_detect (Scsi_Host_Template *tpnt) 
{
	SBP2_DEBUG("sbp2scsi_detect");

	/*
	 * Call sbp2_init to register with the ieee1394 stack. This
	 * results in a callback to sbp2_add_host for each ieee1394
	 * host controller currently registered, and for each of those
	 * we register a scsi host with the scsi stack.
	 */
	
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
	spin_unlock_irq(&io_request_lock);
	sbp2_init();
	spin_lock_irq(&io_request_lock);
#else
	sbp2_init();
#endif

	/* We return the number of hosts registered. */
	return scsi_driver_template.present;
}


/*
 * Called for contents of procfs
 */
static const char *sbp2scsi_info (struct Scsi_Host *host)
{
	struct sbp2scsi_host_info *hi = sbp2_find_host_info_scsi(host);
	static char info[1024];

	if (!hi) /* shouldn't happen, but... */
        	return "IEEE-1394 SBP-2 protocol driver";

	sprintf(info, "IEEE-1394 SBP-2 protocol driver (host: %s)\n%s\n"
		"SBP-2 module load options:\n"
		"- Max speed supported: %s\n"
		"- Max sectors per I/O supported: %d\n"
		"- Max outstanding commands supported: %d\n"
		"- Max outstanding commands per lun supported: %d\n"
                "- Serialized I/O (debug): %s\n"
		"- Exclusive login: %s",
		hi->host->driver->name,
		version,
		hpsb_speedto_str[sbp2_max_speed],
		sbp2_max_sectors,
		sbp2_max_outstanding_cmds,
		sbp2_max_cmds_per_lun,
		sbp2_serialize_io ? "yes" : "no",
		sbp2_exclusive_login ? "yes" : "no");

	return info;
}


MODULE_AUTHOR("James Goodwin <jamesg@filanet.com>");
MODULE_DESCRIPTION("IEEE-1394 SBP-2 protocol driver");
MODULE_SUPPORTED_DEVICE(SBP2_DEVICE_NAME);
MODULE_LICENSE("GPL");

/* SCSI host template */
static Scsi_Host_Template scsi_driver_template = {
	.name =			"IEEE-1394 SBP-2 protocol driver",
	.info =			sbp2scsi_info,
	.detect =		sbp2scsi_detect,
	.queuecommand =		sbp2scsi_queuecommand,
	.eh_abort_handler =	sbp2scsi_abort,
	.eh_device_reset_handler =sbp2scsi_reset,
	.eh_bus_reset_handler =	sbp2scsi_reset,
	.eh_host_reset_handler =sbp2scsi_reset,
	.bios_param =		sbp2scsi_biosparam,
	.this_id =		-1,
	.sg_tablesize =		SBP2_MAX_SG_ELEMENTS,
	.use_clustering =	SBP2_CLUSTERING,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
	.use_new_eh_code =	TRUE,
#endif
	.emulated =		1,
	.proc_name =	SBP2_DEVICE_NAME,
};

static int sbp2_module_init(void)
{
	SBP2_DEBUG("sbp2_module_init");

	/*
	 * Module load debug option to force one command at a time (serializing I/O)
	 */
	if (sbp2_serialize_io) {
		SBP2_ERR("Driver forced to serialize I/O (serialize_io = 1)");
		scsi_driver_template.can_queue = 1;
		scsi_driver_template.cmd_per_lun = 1;
	} else {
		scsi_driver_template.can_queue = sbp2_max_outstanding_cmds;
		scsi_driver_template.cmd_per_lun = sbp2_max_cmds_per_lun;
	}

	/* 
	 * Set max sectors (module load option). Default is 255 sectors. 
	 */
	scsi_driver_template.max_sectors = sbp2_max_sectors;

	/*
	 * Ideally we would register our scsi_driver_template with the
	 * scsi stack and after that register with the ieee1394 stack
	 * and process the add_host callbacks. However, the detect
	 * function in the scsi host template requires that we find at
	 * least one host, so we "nest" the registrations by calling
	 * sbp2_init from the detect function.
	 */
	scsi_driver_template.module = THIS_MODULE;
	if (SCSI_REGISTER_HOST(&scsi_driver_template) ||
	    !scsi_driver_template.present) {
		SBP2_ERR("Please load the lower level IEEE-1394 driver "
			 "(e.g. ohci1394) before sbp2...");
		sbp2_cleanup();
		return -ENODEV;
	}

	return 0;
}

static void __exit sbp2_module_exit(void)
{
	SBP2_DEBUG("sbp2_module_exit");

	/*
	 * On module unload we unregister with the ieee1394 stack
	 * which results in remove_host callbacks for all ieee1394
	 * host controllers.  In the callbacks we unregister the
	 * corresponding scsi hosts.
	 */
	sbp2_cleanup();

	if (SCSI_UNREGISTER_HOST(&scsi_driver_template))
		SBP2_ERR("sbp2_module_exit: couldn't unregister scsi driver");

}

module_init(sbp2_module_init);
module_exit(sbp2_module_exit);
