/*
 * Broadcom Home Gateway Reference Design
 * Ports Web Page Configuration Support Routines
 *
 * Copyright 2001-2003, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 * $Id$
 */

#ifndef _nvports_h_
#define _nvports_h_

#define uint32 unsigned long
#define uint16 unsigned short
#define uint unsigned int
#define uint8 unsigned char
#define uint64 unsigned long long

enum FORCE_PORT {
	FORCE_OFF,
	FORCE_10H,
	FORCE_10F,
	FORCE_100H,
	FORCE_100F,
	FORCE_DOWN,
	POWER_OFF
};

typedef struct _PORT_ATTRIBS
{
	uint 	autoneg;
	uint	force;
	uint	native;	
} PORT_ATTRIBS;

extern uint
nvExistsPortAttrib(char *attrib, uint portno);

extern int
nvExistsAnyForcePortAttrib(uint portno);

extern void
nvSetPortAttrib(char *attrib, uint portno);

extern void
nvUnsetPortAttrib(char *attrib, uint portno);

extern void
nvUnsetAllForcePortAttrib(uint portno);

extern PORT_ATTRIBS
nvGetSwitchPortAttribs(uint portno);

#endif /* _nvports_h_ */



