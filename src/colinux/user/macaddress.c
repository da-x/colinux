#include <string.h>
#include <stdio.h>

#include "macaddress.h"

co_rc_t co_parse_mac_address(const char *text, unsigned char *binary)
{
	int ret, i;
	unsigned int mac[6];

	if (strlen(text) != 17) { 
		return CO_RC(ERROR);
	}
	
	ret = sscanf(text, "%02x:%02x:%02x:%02x:%02x:%02x", 
		     &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);

	for (i=0; i < 6; i++)
		binary[i] = mac[i];

	if (ret != 6)
		return CO_RC(ERROR);

	return CO_RC(OK);
}

void co_build_mac_address(char *text, int ntext, const unsigned char *mac)
{
	snprintf(text, ntext, "%02x:%02x:%02x:%02x:%02x:%02x", 
		 (unsigned int)mac[0], 
		 (unsigned int)mac[1], 
		 (unsigned int)mac[2], 
		 (unsigned int)mac[3], 
		 (unsigned int)mac[4], 
		 (unsigned int)mac[5]);
}
