/*
 *  TAP-Win32 -- A kernel driver to provide virtual tap device functionality
 *               on Windows.  Originally derived from the CIPE-Win32
 *               project by Damion K. Wilson, with extensive modifications by
 *               James Yonan.
 *
 *  All source code which derives from the CIPE-Win32 project is
 *  Copyright (C) Damion K. Wilson, 2003, and is released under the
 *  GPL version 2 (see below).
 *
 *  All other source code is Copyright (C) James Yonan, 2003,
 *  and is released under the GPL version 2 (see below).
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program (see the file COPYING included with this
 *  distribution); if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef MacInfoDefined
#define MacInfoDefined

#ifdef __cplusplus
extern "C" {
#endif

//=====================================================================================
//                                  NDIS + Win32 Settings
//=====================================================================================
#ifdef NDIS_MINIPORT_DRIVER
#   include <ndis.h>
#endif

//===================================================================================
//                                      Macros
//===================================================================================
#define IsMacDelimiter(a) (a == ':' || a == '-' || a == '.')
#define IsHexDigit(c) ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))

#ifdef ASSERT
#   undef ASSERT
#endif

#define ASSERT(a) if (! (a)) return

//===================================================================================
//                          MAC Address Manipulation Routines
//===================================================================================
unsigned char HexStringToDecimalInt (unsigned char p_Character);
void ConvertMacInfo (MACADDR p_Destination, unsigned char *p_Source, unsigned long p_Length);
void GenerateRandomMac (MACADDR mac, unsigned char *adapter_name);

#define COPY_MAC(dest, src) memcpy(dest, src, sizeof (MACADDR));

#ifdef __cplusplus
}
#endif

#endif
