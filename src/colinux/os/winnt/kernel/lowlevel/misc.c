/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include "../ddk.h"

#ifdef WIN64
// FIXME: W64: Struct and function was missing

// See also http://www.woodmann.com/forum/archive/index.php/t-11178.html
// From http://www.nynaeve.net/index.php?cat=8&paged=2
// Remember the ULONG_PTR vs. ULONG
typedef struct _SYSTEM_BASIC_INFORMATION {
	ULONG Reserved;
	ULONG TimerResolution;
	ULONG PageSize;
	ULONG NumberOfPhysicalPages;
	ULONG LowestPhysicalPageNumber;
	ULONG HighestPhysicalPageNumber;
	ULONG AllocationGranularity;
	ULONG_PTR MinimumUserModeAddress;
	ULONG_PTR MaximumUserModeAddress;
	KAFFINITY ActiveProcessorsAffinityMask;
	CCHAR NumberOfProcessors;
} SYSTEM_BASIC_INFORMATION, *PSYSTEM_BASIC_INFORMATION;

// winternl.h
  typedef enum _SYSTEM_INFORMATION_CLASS {
    SystemBasicInformation = 0,SystemPerformanceInformation = 2,SystemTimeOfDayInformation = 3,SystemProcessInformation = 5,
    SystemProcessorPerformanceInformation = 8,SystemInterruptInformation = 23,SystemExceptionInformation = 33,SystemRegistryQuotaInformation = 37,
    SystemLookasideInformation = 45
  } SYSTEM_INFORMATION_CLASS;

NTSTATUS
NTAPI
NtQuerySystemInformation(
  /*IN*/ SYSTEM_INFORMATION_CLASS  SystemInformationClass,
  /*IN OUT*/ PVOID  SystemInformation,
  /*IN*/ ULONG  SystemInformationLength,
  /*OUT*/ PULONG  ReturnLength  /*OPTIONAL*/);
#endif

#include <colinux/os/kernel/misc.h>

unsigned long co_os_virt_to_phys(void *addr)
{
	PHYSICAL_ADDRESS pa;

	pa = MmGetPhysicalAddress((PVOID)addr);

	return pa.QuadPart;
}

co_rc_t co_os_physical_memory_pages(unsigned long *pages)
{
	SYSTEM_BASIC_INFORMATION sbi;
	NTSTATUS status;
	co_rc_t rc = CO_RC(OK);

	status = NtQuerySystemInformation(SystemBasicInformation, &sbi, sizeof(sbi), NULL);
	if (status != STATUS_SUCCESS)
		return CO_RC(ERROR);

	*pages = sbi.NumberOfPhysicalPages;

	/*
	 * Round to 16 MB boundars, since Windows doesn't return the
	 * exact amount but a bit lower.
	 */
	*pages = ~0xfff & ((*pages) + 0xfff);

	return rc;
}
