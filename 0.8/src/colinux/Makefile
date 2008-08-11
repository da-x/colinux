
all:
	(cd ../..; make colinux) && make copy

kern:
	(cd ../..; make kernel) && make kcopy

#		mkinitrd -v --with-mod=dm-mirror --with-mod=dm-zero -f /test/initrd-2.6.22-co-0.8.0.img `uname -r`
kcopy:
	echo "/dev/VolGroup00/LogVol00 / ext3 defaults 0 0" > /test/fstab.test
	cd ../..; . bin/build-common.sh; \
	(cd /root/colinux-devel/build/linux-$$KERNEL_VERSION-build; \
		cp vmlinux /test; \
		cp vmlinux-modules.tar.gz /test; \
		cp _install/lib/modules/$$COMPLETE_KERNEL_NAME/kernel/drivers/md/dm-mod.ko /usr/src/initrd-test/lib; \
		cp _install/lib/modules/$$COMPLETE_KERNEL_NAME/kernel/drivers/md/dm-mirror.ko /usr/src/initrd-test/lib; \
		cd /usr/src/initrd-test; find . | cpio -H newc --quiet -o | gzip -9 > /test/initrd-test.img; \
	)

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

debug:
	rm -f debug.bin; python ./user/debug/server.py debug.bin

dump:
	colinux-debug-daemon -p < debug.bin > debug.xml
	python ./user/debug/dump.py debug.xml > /tmp/debug.tmp
	IFS="|"; cat /tmp/debug.tmp | while read a b c d e f g; do echo $$b $$d $$e; done; rm -f /tmp/debug.tmp

clean:
	(cd ../..; make clean)

touch:
	touch kernel/scsi.c
	touch os/winnt/kernel/scsi.c
