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

#ifdef __cplusplus
extern "C" {
#endif

#include "macinfo.h"

unsigned char HexStringToDecimalInt (unsigned char p_Character)
   {
    unsigned char l_Value = 0;

    if (p_Character >= 'A' && p_Character <= 'F')
       l_Value = (p_Character - 'A') + 10;
    else if (p_Character >= 'a' && p_Character <= 'f')
       l_Value = (p_Character - 'a') + 10;
    else if (p_Character >= '0' && p_Character <= '9')
       l_Value = p_Character - '0';

    return l_Value;
   }

void ConvertMacInfo (MACADDR p_Destination, unsigned char *p_Source, unsigned long p_Length)
   {
    unsigned long l_Index, l_HexIdx, l_Ind = 0, l_Init = 1;

    MYASSERT (p_Destination);
    MYASSERT (p_Source);
    MYASSERT (p_Length);

    for (l_Index = l_HexIdx = l_Ind = 0;
	 l_Index < p_Length && l_HexIdx < sizeof (MACADDR) && p_Source [l_Index];
	 ++l_Index)
       {
        if (IsMacDelimiter (p_Source [l_Index]))
           l_Ind = 0, ++l_HexIdx, l_Init = 1;
        else if (++l_Ind == 3)
           (++l_HexIdx < sizeof (MACADDR)
	    ? (p_Destination [l_HexIdx]
	       = HexStringToDecimalInt (p_Source [l_Index]), l_Ind = 0) : 0);
        else
           p_Destination [l_HexIdx]
	     = (l_Init ? 0 : p_Destination [l_HexIdx] * 16)
	     + HexStringToDecimalInt (p_Source [l_Index]), l_Init = 0;
       }
   }

/*
 * Generate a MAC using the GUID in the adapter name.
 *
 * The mac is constructed as 00:FF:xx:xx:xx:xx where
 * the Xs are taken from the first 32 bits of the GUID in the
 * adapter name.  This is similar to the Linux 2.4 tap MAC
 * generator, except linux uses 32 random bits for the Xs.
 *
 * In general, this solution is reasonable for most
 * applications except for very large bridged TAP networks,
 * where the probability of address collisions becomes more
 * than infintesimal.
 *
 * Using the well-known "birthday paradox", on a 1000 node
 * network the probability of collision would be
 * 0.000116292153.  On a 10,000 node network, the probability
 * of collision would be 0.01157288998621678766.
 */

void GenerateRandomMac (MACADDR mac, unsigned char *adapter_name)
{
  unsigned const char *cp = adapter_name;
  unsigned char c;
  unsigned int i = 2;
  unsigned int byte = 0;
  int brace = 0;
  int state = 0;

  NdisZeroMemory (mac, sizeof (MACADDR));

  mac[0] = 0x00;
  mac[1] = 0xFF;

  while (c = *cp++)
    {
      if (i >= sizeof (MACADDR))
	break;
      if (c == '{')
	brace = 1;
      if (IsHexDigit (c) && brace)
	{
	  const unsigned int digit = HexStringToDecimalInt (c);
	  if (state)
	    {
	      byte <<= 4;
	      byte |= digit;
	      mac[i++] = (unsigned char) byte;
	      state = 0;
	    }
	  else
	    {
	      byte = digit;
	      state = 1;
	    }
	}
    }
}

void GenerateRelatedMAC (MACADDR dest, const MACADDR src)
{
  COPY_MAC (dest, src);
  (*((ULONG *) ((unsigned char *) dest + 2)))++;
}

#ifdef __cplusplus
}
#endif
