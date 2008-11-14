
all:
	(cd ../..; make colinux) && make copy

kern:
	(cd ../..; make kernel) && make kcopy

#	echo "/dev/VolGroup00/LogVol00 / ext3 defaults 0 0" > /test/fstab.test
#		mkinitrd -v --with-mod=dm-mirror --with-mod=dm-zero -f /test/initrd-2.6.22-co-0.8.0.img `uname -r`
kcopy:
	cd ../..; . bin/build-common.sh; \
	(cd /root/colinux-devel/build/linux-$$KERNEL_VERSION-build; \
		cp vmlinux /test/vmlinux-$$COMPLETE_KERNEL_NAME; \
		cp vmlinux-modules.tar.gz /test/modules-$$COMPLETE_KERNEL_NAME.tgz; \
		cp _install/lib/modules/$$COMPLETE_KERNEL_NAME/kernel/drivers/md/dm-mod.ko /usr/src/initrd/lib; \
		cp _install/lib/modules/$$COMPLETE_KERNEL_NAME/kernel/drivers/md/dm-mirror.ko /usr/src/initrd/lib; \
		cd /usr/src/initrd; find . | cpio -H newc --quiet -o | gzip -9 > /test/initrd-$$COMPLETE_KERNEL_NAME; \
	)

iscsi:
	cd ../..; . bin/build-common.sh; \
	cd /usr/src/initrd.iscsi; find . | cpio -H newc --quiet -o | gzip -9 > /test/initrd-$$COMPLETE_KERNEL_NAME-iscsi

package:
	(cd ../..; make package)

#		rm -f $$1; 
copy:
	@mover() { \
		strip --strip-all --remove-section=.comment $$1; \
		cmd="cp $$1 /test"; \
		echo $$cmd; $$cmd; \
	}; \
	mover os/winnt/build/linux.sys; \
	mover os/winnt/build/colinux-daemon.exe; \
	mover os/winnt/build/colinux-debug-daemon.exe; \
	mover os/winnt/build/colinux-console-wx.exe

installer:
#	rm -rf os/winnt/user/install/premaid
#	(cd ../..; make clean; make)
	make kcopy
	mkdir -p os/winnt/user/install/premaid/netdriver
	cp /test/netdriver/* os/winnt/user/install/premaid/netdriver
	(cd ../..; . bin/build-common.sh; \
	cp /test/initrd-$$COMPLETE_KERNEL_NAME src/colinux/os/winnt/user/install/premaid/initrd.gz)
	(cd ../..; make installer) && cp os/winnt/user/install/coLinux.exe /test

debug:
	rm -f debug.bin; python ./user/debug/server.py debug.bin

dump:
	colinux-debug-daemon -p < debug.bin > debug.xml
	python ./user/debug/dump.py debug.xml > /tmp/debug.tmp
	IFS="|"; cat /tmp/debug.tmp | while read a b c d e f g; do echo $$b $$d $$e; done; rm -f /tmp/debug.tmp

clean:
	(cd ../..; make clean)

touch:
	touch kernel/scsi.c kernel/monitor.c
	touch os/winnt/kernel/scsi.c
