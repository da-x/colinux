/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 * Service support by Jaroslaw Kowalski <jaak@zd.com.pl>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <ddk/ntddk.h>
#include <ddk/winddk.h>
#include <unistd.h>

#include <colinux/common/libc.h>
#include <colinux/common/common.h>
#include <colinux/os/user/cobdpath.h>

co_rc_t co_canonize_cobd_path(co_pathname_t *pathname)
{
	co_pathname_t copied_path;

	memcpy(&copied_path, pathname, sizeof(copied_path));

	if (!(co_strncmp(copied_path, "\\Device\\", 8) == 0  ||
	      co_strncmp(copied_path, "\\DosDevices\\", 12) == 0)) 
	{
		UNICODE_STRING dos_uni;
		UNICODE_STRING nt_uni;
		ANSI_STRING dos_ansi;
		ANSI_STRING nt_ansi;
		NTSTATUS status;
		
		/* If it's not an absolute path */
		if (!(co_strlen(copied_path) >= 3  &&  (copied_path[1] == ':' && copied_path[2] == '\\'))) {
			co_pathname_t cwd_path;

			/* ... then make it so */
			getcwd(cwd_path, sizeof(cwd_path));
			co_snprintf(*pathname, sizeof(*pathname), "%s\\%s", cwd_path, copied_path);
		}

		dos_ansi.Buffer = &(*pathname)[0];
		dos_ansi.Length = strlen(*pathname);
		dos_ansi.MaximumLength = dos_ansi.Length + 1;

		status = RtlAnsiStringToUnicodeString(&dos_uni, &dos_ansi, TRUE);
		if (!NT_SUCCESS(status))
			return CO_RC(ERROR);

		RtlDosPathNameToNtPathName_U(dos_uni.Buffer, &nt_uni, NULL, NULL);
		RtlFreeUnicodeString(&dos_uni);

		status = RtlUnicodeStringToAnsiString(&nt_ansi, &nt_uni, TRUE);
		if (!NT_SUCCESS(status)) {
			RtlFreeUnicodeString(&nt_uni);
			return CO_RC(ERROR);
		}
		
		co_snprintf(*pathname, sizeof(*pathname), "%s", nt_ansi.Buffer);
		
		RtlFreeUnicodeString(&nt_uni);
		RtlFreeAnsiString(&nt_ansi);
	}

	return CO_RC(OK);
}
