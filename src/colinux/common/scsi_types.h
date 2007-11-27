
#ifndef __COLINUX_SCSI_TYPES_H__
#define __COLINUX_SCSI_TYPES_H__

/* Device types */
#define SCSI_PTYPE_DISK 		0x00
#define SCSI_PTYPE_TAPE			0x01
#define SCSI_PTYPE_PRINTER		0x02
#define SCSI_PTYPE_PROC			0x03
#define SCSI_PTYPE_WORM			0x04
#define SCSI_PTYPE_CDDVD		0x05
#define SCSI_PTYPE_SCANNER		0x06
#define SCSI_PTYPE_OPTICAL		0x07
#define SCSI_PTYPE_CHANGER		0x08
#define SCSI_PTYPE_COMM			0x09
#define SCSI_PTYPE_RAID			0x0C
#define SCSI_PTYPE_ENC			0x0D
#define SCSI_PTYPE_SDISK		0x0E
#define SCSI_PTYPE_CARD			0x0F
#define SCSI_PTYPE_BRIDGE		0x10
#define SCSI_PTYPE_OSD			0x11
#define SCSI_PTYPE_PASS			0xFF		/* Special */

#endif
