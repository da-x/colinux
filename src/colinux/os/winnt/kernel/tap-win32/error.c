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

//-----------------
// DEBUGGING OUTPUT
//-----------------

#if DBG

VOID
MyAssert (const unsigned char *file, int line)
{
      DEBUGP (("MYASSERT failed %s/%d\n", file, line));
      KeBugCheckEx (0x0F00BABA,
		    (ULONG_PTR) line,
		    (ULONG_PTR) 0,
		    (ULONG_PTR) 0,
		    (ULONG_PTR) 0);
}

VOID
PrMac (const MACADDR mac)
{
  DbgPrint ("%x:%x:%x:%x:%x:%x",
	    mac[0], mac[1], mac[2],
	    mac[3], mac[4], mac[5]);
}

VOID
PrIP (IPADDR ip_addr)
{
  const unsigned char *ip = (const unsigned char *) &ip_addr;

  DbgPrint ("%d.%d.%d.%d",
	    ip[0], ip[1], ip[2], ip[3]);
}

VOID
DebugPacket (const char *prefix,
	     const unsigned char *data,
	     int len)
{
  const ETH_HEADER *eth = (const ETH_HEADER *) data;
  const ARP_PACKET *arp = (const ARP_PACKET *) data;

  if (len < sizeof (ETH_HEADER))
    {
      DbgPrint ("%s BAD LEN=%d\n", prefix, len);
      return;
    }

  if (len >= sizeof (ARP_PACKET)
      && arp->m_Proto == htons (ETH_P_ARP))
    {
      DbgPrint ("%s ARP src=", prefix);
      PrMac (arp->m_MAC_Source);
      DbgPrint (" dest=");
      PrMac (arp->m_MAC_Destination);
      DbgPrint (" OP=0x%04x",
		(int)ntohs(arp->m_ARP_Operation));
      DbgPrint (" M=0x%04x(%d)",
		(int)ntohs(arp->m_MAC_AddressType),
		(int)arp->m_MAC_AddressSize);
      DbgPrint (" P=0x%04x(%d)",
		(int)ntohs(arp->m_PROTO_AddressType),
		(int)arp->m_PROTO_AddressSize);

      DbgPrint (" MacSrc=");
      PrMac (arp->m_ARP_MAC_Source);
      DbgPrint (" MacDest=");
      PrMac (arp->m_ARP_MAC_Destination);

      DbgPrint (" IPSrc=");
      PrIP (arp->m_ARP_IP_Source);
      DbgPrint (" IPDest=");
      PrIP (arp->m_ARP_IP_Destination);

      DbgPrint ("\n");
    }
  else
    {
      DbgPrint ("%s EH src=", prefix);
      PrMac (eth->src);
      DbgPrint (" dest=");
      PrMac (eth->dest);
      DbgPrint (" proto=0x%04x len=%d\n",
		(int) ntohs(eth->proto),
		len);
    }
}

#endif
