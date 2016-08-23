/*
 * NVRAM variable manipulation (Linux kernel half)
 *
 * Copyright 2001-2003, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id$
 */

#include <linux/config.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/smp_lock.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/bootmem.h>
#include <linux/wrapper.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mtd/mtd.h>
#include <asm/addrspace.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include <typedefs.h>
#include <bcmendian.h>
#include <bcmnvram.h>
#include <bcmutils.h>
#include <osl.h>
#include <sbchipc.h>
#include <sbextif.h>
#include <sflash.h>

/* In BSS to minimize text size and page aligned so it can be mmap()-ed */
static char nvram_buf[NVRAM_SPACE] __attribute__((aligned(PAGE_SIZE)));

#define NVRAM_SIZE       (0x1ff0)

/*
 * TLV types.  These codes are used in the "type-length-value"
 * encoding of the items stored in the NVRAM device (flash or EEPROM)
 *
 * The layout of the flash/nvram is as follows:
 *
 * <type> <length> <data ...> <type> <length> <data ...> <type_end>
 *
 * The type code of "ENV_TLV_TYPE_END" marks the end of the list.
 * The "length" field marks the length of the data section, not
 * including the type and length fields.
 *
 * Environment variables are stored as follows:
 *
 * <type_env> <length> <flags> <name> = <value>
 *
 * If bit 0 (low bit) is set, the length is an 8-bit value.
 * If bit 0 (low bit) is clear, the length is a 16-bit value
 * 
 * Bit 7 set indicates "user" TLVs.  In this case, bit 0 still
 * indicates the size of the length field.  
 *
 * Flags are from the constants below:
 *
 */
#define ENV_LENGTH_16BITS	0x00	/* for low bit */
#define ENV_LENGTH_8BITS	0x01

#define ENV_TYPE_USER		0x80

#define ENV_CODE_SYS(n,l) (((n)<<1)|(l))
#define ENV_CODE_USER(n,l) ((((n)<<1)|(l)) | ENV_TYPE_USER)

/*
 * The actual TLV types we support
 */

#define ENV_TLV_TYPE_END	0x00	
#define ENV_TLV_TYPE_ENV	ENV_CODE_SYS(0,ENV_LENGTH_8BITS)

/*
 * Environment variable flags 
 */

#define ENV_FLG_NORMAL		0x00	/* normal read/write */
#define ENV_FLG_BUILTIN		0x01	/* builtin - not stored in flash */
#define ENV_FLG_READONLY	0x02	/* read-only - cannot be changed */

#define ENV_FLG_MASK		0xFF	/* mask of attributes we keep */
#define ENV_FLG_ADMIN		0x100	/* lets us internally override permissions */



/*  *********************************************************************
    *  NVRAM Storage access.
    ********************************************************************* */

/* NVRAM shadow space */
char _nvdata[NVRAM_SIZE] __initdata;
char _valuestr[256] __initdata;

/*  *********************************************************************
    *  nvram_getsize()
    *  
    *  Return the total size of the NVRAM device.  Note that
    *  this is the total size that is used for the NVRAM functions,
    *  not the size of the underlying media.
    *  
    *  Input parameters: 
    *  	   nothing
    *  	   
    *  Return value:
    *  	   size.  <0 if error
    ********************************************************************* */

static int _nvram_getsize(void)
{
    return NVRAM_SIZE;
}


/*  *********************************************************************
    *  _nvram_read(buffer,offset,length)
    *  
    *  Read data from the NVRAM device
    *  
    *  Input parameters: 
    *  	   buffer - destination buffer
    *  	   offset - offset of data to read
    *  	   length - number of bytes to read
    *  	   
    *  Return value:
    *  	   number of bytes read, or <0 if error occured
    ********************************************************************* */
static int
_nvram_read(unsigned char *nv_buf, unsigned char *buffer, int offset, int length)
{
    int i;
    if (offset > NVRAM_SIZE)
	return -1; 

    for ( i = 0; i < length; i++) {
	buffer[i] = ((volatile unsigned char*)nv_buf)[offset + i];
    }
    return length;
}

static char*
strnchr(const char *dest,int c,size_t cnt)
{
    while (*dest && (cnt > 0)) {
	if (*dest == c) return (char *) dest;
	dest++;
	cnt--;
    }
    return NULL;
}


/*
 * Core support API: Externally visible.
 */

/*
 * Get the value of an NVRAM variable
 * @param	name	name of variable to get
 * @return	value of variable or NULL if undefined
 */

char* 
_nvram_get(unsigned char *nv_buf, char* name)
{
    int size;
    unsigned char *buffer;
    unsigned char *ptr;
    unsigned char *envval;
    unsigned int reclen;
    unsigned int rectype;
    int offset;
    int flg;
    
    size = _nvram_getsize();
    buffer = &_nvdata[0];

    ptr = buffer;
    offset = 0;

    /* Read the record type and length */
    if (_nvram_read(nv_buf, ptr,offset,1) != 1) {
	goto error;
    }
    
    while ((*ptr != ENV_TLV_TYPE_END)  && (size > 1)) {

	/* Adjust pointer for TLV type */
	rectype = *(ptr);
	offset++;
	size--;

	/* 
	 * Read the length.  It can be either 1 or 2 bytes
	 * depending on the code 
	 */
	if (rectype & ENV_LENGTH_8BITS) {
	    /* Read the record type and length - 8 bits */
	    if (_nvram_read(nv_buf, ptr,offset,1) != 1) {
		goto error;
	    }
	    reclen = *(ptr);
	    size--;
	    offset++;
	}
	else {
	    /* Read the record type and length - 16 bits, MSB first */
	    if (_nvram_read(nv_buf, ptr,offset,2) != 2) {
		goto error;
	    }
	    reclen = (((unsigned int) *(ptr)) << 8) + (unsigned int) *(ptr+1);
	    size -= 2;
	    offset += 2;
	}

	if (reclen > size)
	    break;	/* should not happen, bad NVRAM */

	switch (rectype) {
	    case ENV_TLV_TYPE_ENV:
		/* Read the TLV data */
		if (_nvram_read(nv_buf, ptr,offset,reclen) != reclen)
		    goto error;
		flg = *ptr++;
		envval = (unsigned char *) strnchr(ptr,'=',(reclen-1));
		if (envval) {
		    *envval++ = '\0';
		    memcpy(_valuestr,envval,(reclen-1)-(envval-ptr));
		    _valuestr[(reclen-1)-(envval-ptr)] = '\0';
#ifdef NVRAM_DEBUG
		    printk(KERN_INFO "NVRAM:%s=%s\n", ptr, _valuestr);
#endif
		    if(!strcmp(ptr, name)){
			return _valuestr;
		    }
		    if((strlen(ptr) > 1) && !strcmp(&ptr[1], name))
			return _valuestr;
		}
		break;
		
	    default: 
		/* Unknown TLV type, skip it. */
		break;
	    }

	/*
	 * Advance to next TLV 
	 */
		
	size -= (int)reclen;
	offset += reclen;

	/* Read the next record type */
	ptr = buffer;
	if (_nvram_read(nv_buf, ptr,offset,1) != 1)
	    goto error;
	}

error:
    return NULL;

}

#ifdef MODULE

#define early_nvram_get(name) nvram_get(name)

#else /* !MODULE */

#define KB * 1024
#define MB * 1024 * 1024

/* Probe for NVRAM header */
static void * __init
early_nvram_init(void)
{
	int i;
	long offset = 8 MB - NVRAM_SPACE;
	unsigned long *src = (unsigned long *) KSEG1ADDR(0x1c000000 + offset);
	unsigned long *dst = (unsigned long *) nvram_buf;

	/* PR2620 WAR: reload flash into memory */
	for (i = 0; i < NVRAM_SPACE; i += 4) {
		if (*src == 0xFFFFFFFF) {
			break;
		}
		*dst++ = *src++;
	}
	return (void *)nvram_buf;
}

/* Early (before mm or mtd) read-only access to NVRAM */
static char * __init
early_nvram_get(const char *name)
{
	static char *nvrambuf = NULL;

	if (!name)
		return NULL;

	if (!nvrambuf)
		if (!(nvrambuf = (char *)early_nvram_init()))
			return NULL;

	return _nvram_get(nvrambuf, (char *)name);
}

#endif /* !MODULE */

/* Globals */
static spinlock_t nvram_lock = SPIN_LOCK_UNLOCKED;
static struct nvram_tuple * nvram_hash[257] = { NULL };
static struct nvram_tuple * nvram_dead = NULL;
static unsigned long nvram_offset = 0;
static int nvram_major = -1;
static devfs_handle_t nvram_handle = NULL;
static struct mtd_info *nvram_mtd = NULL;

static inline unsigned int
hash(const char *s)
{
	unsigned int hash = 0;

	while (*s)
		hash = 31 * hash + *s++;

	return hash;
}

static int
nvram_rehash(void)
{
	unsigned int i;
	struct nvram_tuple *t, *next;
	size_t len;
	unsigned long flags;
	char *buf;

        int size;
        unsigned char *buffer;
        unsigned char *ptr;
        unsigned char *envval;
        unsigned int reclen;
        unsigned int rectype;
        int offset;
        int flg, ret = 0;
    
	memset((void *)nvram_buf, 0, NVRAM_SPACE);

	if (!(buf = kmalloc(NVRAM_SPACE, GFP_ATOMIC))) {
		printk("%s: out of memory\n", __FUNCTION__);
		return -ENOMEM;
	}

	if (nvram_mtd &&
	    MTD_READ(nvram_mtd, nvram_mtd->size - NVRAM_SPACE, NVRAM_SPACE, &len, buf) == 0 &&
	    len == NVRAM_SPACE) {
		spin_lock_irqsave(&nvram_lock, flags);

		/* (Re)initialize hash table */
		for (i = 0; i < ARRAYSIZE(nvram_hash); i++) {
			for (t = nvram_hash[i]; t; t = next) {
				next = t->next;
				kfree(t);
			}
			nvram_hash[i] = NULL;
		}
		for (t = nvram_dead; t; t = next) {
			next = t->next;
			kfree(t);
		}
		nvram_dead = NULL;
		nvram_offset = 0;

		spin_unlock_irqrestore(&nvram_lock, flags);
		
#if 0
		/* Parse and set "name=value\0 ... \0\0" */
		for (value = (char *) &header[1];
		     value <= (buf + NVRAM_SPACE - 2) && *value && *(value + 1);
		     value += strlen(value) + 1) {
			name = strsep(&value, "=");
			if (name && value)
				nvram_set(name, value);
		}
#endif
        	size = _nvram_getsize();
        	buffer = &_nvdata[0];

        	ptr = buffer;
        	offset = 0;

	        /* Read the record type and length */
	        if (_nvram_read(buf, ptr,offset,1) != 1) {
		    ret = -1;
		    goto rehash_error;
    		}

    		while ((*ptr != ENV_TLV_TYPE_END)  && (size > 1)) {

			/* Adjust pointer for TLV type */
			rectype = *(ptr);
			offset++;
			size--;

			/* 
	 		 * Read the length.  It can be either 1 or 2 bytes
	 		 * depending on the code 
	 		 */
			if (rectype & ENV_LENGTH_8BITS) {
	    			/* Read the record type and length - 8 bits */
	    			if (_nvram_read(buf, ptr,offset,1) != 1) {
					ret = -1;
					goto rehash_error;
	    			}
	    			reclen = *(ptr);
	    			size--;
	    			offset++;
			}
			else {
	    			/* Read the record type and length - 16 bits, MSB first */
	    			if (_nvram_read(buf, ptr,offset,2) != 2) {
					ret = -1;
					goto rehash_error;
	    			}
	    			reclen = (((unsigned int) *(ptr)) << 8) + (unsigned int) *(ptr+1);
	    			size -= 2;
	    			offset += 2;
			}

			if (reclen > size)
	    			break;	/* should not happen, bad NVRAM */

			switch (rectype) {
	    		case ENV_TLV_TYPE_ENV:
				/* Read the TLV data */
				if (_nvram_read(buf, ptr,offset,reclen) != reclen) {
					ret = -1;
		    			goto rehash_error;
				}
				flg = *ptr++;
				envval = (unsigned char *) strnchr(ptr,'=',(reclen-1));
				if (envval) {
		    			*envval++ = '\0';
		    			memcpy(_valuestr,envval,(reclen-1)-(envval-ptr));
		    			_valuestr[(reclen-1)-(envval-ptr)] = '\0';
#ifdef NVRAM_DEBUG
		    			printk(KERN_INFO "NVRAM:%s=%s\n", ptr, _valuestr);
#endif
					nvram_set(ptr, _valuestr);
				}
				break;
		
	    		default: 
				/* Unknown TLV type, skip it. */
				break;
	    		}

			/*
	 		 * Advance to next TLV 
	 		 */
		
			size -= (int)reclen;
			offset += reclen;

			/* Read the next record type */
			ptr = buffer;
			if (_nvram_read(buf, ptr,offset,1) != 1) {
				ret = -1;
	    			goto rehash_error;
			}
		}
	}

rehash_error:

	kfree(buf);
	return ret;
}

int
nvram_set(const char *name, const char *value)
{
	unsigned int i;
	unsigned long flags;
	struct nvram_tuple *t;
	int ret = 0;

	/* Hash the name */
	i = hash(name) % ARRAYSIZE(nvram_hash);

	spin_lock_irqsave(&nvram_lock, flags);

	if ((nvram_offset + strlen(name) + 1 + strlen(value) + 1) > NVRAM_SIZE) {
		printk("%s: out of space\n", __FUNCTION__);
		ret = -ENOMEM;
		goto done;
	}

	/* Find the associated tuple in the hash table */
	for (t = nvram_hash[i]; t && strcmp(t->name, name); t = t->next);

	if (!t) {
		/* Allocate a new tuple */
		if (!(t = kmalloc(sizeof(struct nvram_tuple), GFP_ATOMIC))) {
			printk("%s: out of memory\n", __FUNCTION__);
			ret = -ENOMEM;
			goto done;
		}

		/* Add it to the hash table */
		t->next = nvram_hash[i];
		nvram_hash[i] = t;

		/* Copy name */
		t->name = &nvram_buf[nvram_offset];
		strcpy(t->name, name);
		nvram_offset += strlen(name) + 1;

		t->value = NULL;
	}

	/* Copy value */
	if (!t->value || strcmp(t->value, value)) {
		t->value = &nvram_buf[nvram_offset];
		strcpy(t->value, value);
		nvram_offset += strlen(value) + 1;
	}

 done:
	spin_unlock_irqrestore(&nvram_lock, flags);

	return ret;
}

static char *
real_nvram_get(const char *name)
{
	unsigned int i;
	unsigned long flags;
	struct nvram_tuple *t;
	char *value;

	if (!name)
		return NULL;

	/* Hash the name */
	i = hash(name) % ARRAYSIZE(nvram_hash);

	spin_lock_irqsave(&nvram_lock, flags);

	/* Find the associated tuple in the hash table */
	for (t = nvram_hash[i]; t && strcmp(t->name, name); t = t->next);

	value = t ? t->value : NULL;

	spin_unlock_irqrestore(&nvram_lock, flags);

	return value;
}

char *
nvram_get(const char *name)
{
	if (nvram_major >= 0)
		return real_nvram_get(name);
	else {
		return early_nvram_get(name);
	}
}

int
nvram_unset(const char *name)
{
	unsigned int i;
	unsigned long flags;
	struct nvram_tuple *t, **prev;

	if (!name)
		return 0;

	/* Hash the name */
	i = hash(name) % ARRAYSIZE(nvram_hash);

	spin_lock_irqsave(&nvram_lock, flags);

	/* Find the associated tuple in the hash table */
	for (prev = &nvram_hash[i], t = *prev; t && strcmp(t->name, name); prev = &t->next, t = *prev);

	/* Move it to the dead table */
	if (t) {
		*prev = t->next;
		t->next = nvram_dead;
		nvram_dead = t;
	}

	spin_unlock_irqrestore(&nvram_lock, flags);

	return 0;
}

static void
erase_callback(struct erase_info *done)
{
	wait_queue_head_t *wait_q = (wait_queue_head_t *) done->priv;
	wake_up(wait_q);
}

int
nvram_commit(void)
{
	int ret;
	size_t len;
    	int namelen;
    	int valuelen;
	unsigned int i;
	unsigned long flags;
	struct nvram_tuple *t;
	struct nvram_header *header;
	char *buf, *ptr, *end;
	DECLARE_WAITQUEUE(wait, current);
	wait_queue_head_t wait_q;
	struct erase_info erase;

	if (!nvram_mtd) {
		printk("%s: NVRAM not found\n", __FUNCTION__);
		return -ENODEV;
	}

	if (in_interrupt()) {
		printk("%s: not committing in interrupt\n", __FUNCTION__);
		return -EINVAL;
	}

	/* Read entire partition */
	if (!(buf = kmalloc(NVRAM_SPACE, GFP_ATOMIC))) {
		printk("%s: out of memory\n", __FUNCTION__);
		return -ENOMEM;
	}
	ret = MTD_READ(nvram_mtd, nvram_mtd->size - NVRAM_SPACE, NVRAM_SPACE, &len, buf);
	if (ret || len != NVRAM_SPACE) {
		printk("%s: read error\n", __FUNCTION__);
		ret = -EIO;
		goto done;
	}

	/* Parse NVRAM header */
	ptr = buf;

	memset(ptr, 0, NVRAM_SPACE);

	/* Leave space for a double NUL at the end */
	end = ptr + NVRAM_SPACE - 2;

	spin_lock_irqsave(&nvram_lock, flags);

	/* Write out all tuples */
	for (i = 0; i < ARRAYSIZE(nvram_hash); i++) {
		for (t = nvram_hash[i]; t; t = t->next) {
			namelen = strlen(t->name);
			valuelen = strlen(t->value);

			if ((ptr + 2 + namelen + valuelen + 1 + 1 + 1) > end)
				break;

			*ptr++ = ENV_TLV_TYPE_ENV;		/* TLV record type */
			*ptr++ = (namelen + valuelen + 1 + 1);	/* TLV record length */

			*ptr++ = 0;
			/* *ptr++ = (unsigned char)env->flags; */
			memcpy(ptr,t->name,namelen);		/* TLV record data */
			ptr += namelen;
			*ptr++ = '=';
			memcpy(ptr,t->value,valuelen);
			ptr += valuelen;
		}
	}

	spin_unlock_irqrestore(&nvram_lock, flags);

	init_waitqueue_head(&wait_q);
	erase.mtd = nvram_mtd;
	erase.callback = erase_callback;
	erase.len = nvram_mtd->erasesize;
	erase.priv = (u_long) &wait_q;

	/* Erase entire partition */
	/* for (erase.addr = nvram_mtd->size - NVRAM_SPACE; erase.addr < nvram_mtd->size; */
	for (erase.addr = nvram_mtd->size - nvram_mtd->erasesize; erase.addr < nvram_mtd->size;
	     erase.addr += nvram_mtd->erasesize) {
		set_current_state(TASK_INTERRUPTIBLE);
		add_wait_queue(&wait_q, &wait);

		if ((ret = MTD_ERASE(nvram_mtd, &erase))) {
			set_current_state(TASK_RUNNING);
			remove_wait_queue(&wait_q, &wait);
			printk("%s: erase error\n", __FUNCTION__);
			goto done;
		}

		/* Wait for erase to finish */
		schedule();
		remove_wait_queue(&wait_q, &wait);
	}

	/* Write entire partition */
	ret = MTD_WRITE(nvram_mtd, nvram_mtd->size - NVRAM_SPACE, NVRAM_SPACE, &len, buf);
	/* if (ret || len != nvram_mtd->size) {*/
	if (ret || len != NVRAM_SPACE) {
		printk("%s: write error\n", __FUNCTION__);
		ret = -EIO;
		goto done;
	}

	/* Reinitialize hash table */
	ret = nvram_rehash();

 done:
	kfree(buf);
	return ret;
}

int
nvram_getall(char *buf, int count)
{
	unsigned long flags;
	unsigned int i;
	struct nvram_tuple *t;
	int len = 0;

	memset(buf, 0, count);

	spin_lock_irqsave(&nvram_lock, flags);

	/* Write name=value\0 ... \0\0 */
	for (i = 0; i < ARRAYSIZE(nvram_hash); i++) {
		for (t = nvram_hash[i]; t; t = t->next) {
			if ((count - len) > (strlen(t->name) + 1 + strlen(t->value) + 1))
				len += sprintf(buf + len, "%s=%s", t->name, t->value) + 1;
			else
				break;
		}
	}

	spin_unlock_irqrestore(&nvram_lock, flags);

	return 0;
}

static ssize_t
nvram_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	char tmp[100], *name = tmp, *value;
	ssize_t ret;
	unsigned long off;

	if (count > sizeof(tmp)) {
		if (!(name = kmalloc(count, GFP_KERNEL)))
			return -ENOMEM;
	}

	if (copy_from_user(name, buf, count)) {
		ret = -EFAULT;
		goto done;
	}

	if (*name == '\0') {
		/* Get all variables */
		ret = nvram_getall(name, count);
		if (ret == 0) {
			if (copy_to_user(buf, name, count)) {
				ret = -EFAULT;
				goto done;
			}
			ret = count;
		}
	} else {
		if (!(value = nvram_get(name))) {
			ret = 0;
			goto done;
		}

		/* Provide the offset into mmap() space */
		off = (unsigned long) value - (unsigned long) nvram_buf;

		if (put_user(off, (unsigned long *) buf)) {
			ret = -EFAULT;
			goto done;
		}

		ret = sizeof(unsigned long);
	}

 done:
	if (name != tmp)
		kfree(name);

	return ret;
}

static ssize_t
nvram_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
	char tmp[100], *name = tmp, *value;
	ssize_t ret;

	if (count > sizeof(tmp)) {
		if (!(name = kmalloc(count, GFP_KERNEL)))
			return -ENOMEM;
	}

	if (copy_from_user(name, buf, count)) {
		ret = -EFAULT;
		goto done;
	}

	value = name;
	name = strsep(&value, "=");
	if (value)
		ret = nvram_set(name, value) ? : count;
	else
		ret = nvram_unset(name) ? : count;

 done:
	if (name != tmp)
		kfree(name);

	/* nvram_commit(); */
	return ret;
}	

static int
nvram_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	return nvram_commit();
}

static int
nvram_mmap(struct file *file, struct vm_area_struct *vma)
{
	unsigned long offset = virt_to_phys(nvram_buf);

	if (remap_page_range(vma->vm_start, offset, vma->vm_end-vma->vm_start,
			     vma->vm_page_prot))
		return -EAGAIN;

	return 0;
}

static int
nvram_open(struct inode *inode, struct file * file)
{
	MOD_INC_USE_COUNT;
	return 0;
}

static int
nvram_release(struct inode *inode, struct file * file)
{
	MOD_DEC_USE_COUNT;
	return 0;
}

static struct file_operations nvram_fops = {
	owner:		THIS_MODULE,
	open:		nvram_open,
	release:	nvram_release,
	read:		nvram_read,
	write:		nvram_write,
	ioctl:		nvram_ioctl,
	mmap:		nvram_mmap,
};

static void __exit
nvram_exit(void)
{
	unsigned int i;
	int order = 0;
	struct nvram_tuple *t, *next;
	struct page *page, *end;

	if (nvram_handle)
		devfs_unregister(nvram_handle);

	if (nvram_major >= 0)
		devfs_unregister_chrdev(nvram_major, "nvram");

	if (nvram_mtd)
		put_mtd_device(nvram_mtd);

	for (i = 0; i < ARRAYSIZE(nvram_hash); i++) {
		for (t = nvram_hash[i]; t; t = next) {
			next = t->next;
			kfree(t);
		}
	}
	for (t = nvram_dead; t; t = next) {
		next = t->next;
		kfree(t);
	}

	while ((PAGE_SIZE << order) < NVRAM_SPACE)
		order++;
	end = virt_to_page(nvram_buf + (PAGE_SIZE << order) - 1);
	for (page = virt_to_page(nvram_buf); page <= end; page++)
		mem_map_unreserve(page);
}

static int __init
nvram_init(void *sbh)
{
	int order = 0, ret = 0;
	struct page *page, *end;
	unsigned int i;

	/* Allocate and reserve memory to mmap() */
	while ((PAGE_SIZE << order) < NVRAM_SPACE)
		order++;
	end = virt_to_page(nvram_buf + (PAGE_SIZE << order) - 1);
	for (page = virt_to_page(nvram_buf); page <= end; page++)
		mem_map_reserve(page);

#ifdef CONFIG_MTD
	/* Find associated MTD device */
	for (i = 0; i < MAX_MTD_DEVICES; i++) {
		nvram_mtd = get_mtd_device(NULL, i);
		if (nvram_mtd) {
			if (!strcmp(nvram_mtd->name, "nvram") &&
			    nvram_mtd->size >= NVRAM_SPACE)
				break;
			put_mtd_device(nvram_mtd);
		}
	}
	if (i >= MAX_MTD_DEVICES)
		nvram_mtd = NULL;
#endif

	/* Initialize hash table lock */
	spin_lock_init(&nvram_lock);

	/* Initialize hash table */
	nvram_rehash();

	/* Register char device */
	if ((nvram_major = devfs_register_chrdev(252, "nvram", &nvram_fops)) < 0) {
		ret = nvram_major;
		goto err;
	}

	/* Create /dev/nvram handle */
	nvram_handle = devfs_register(NULL, "nvram", DEVFS_FL_NONE, nvram_major, 0,
				      S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP, &nvram_fops, NULL);

	return 0;

 err:
	nvram_exit();
	return ret;
}

module_init(nvram_init);
module_exit(nvram_exit);

EXPORT_SYMBOL(nvram_get);
EXPORT_SYMBOL(nvram_getall);
EXPORT_SYMBOL(nvram_set);
EXPORT_SYMBOL(nvram_unset);
EXPORT_SYMBOL(nvram_commit);
