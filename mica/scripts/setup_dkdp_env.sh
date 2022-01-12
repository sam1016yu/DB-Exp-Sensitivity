#!/bin/bash

##### from DPDK/tools/setup.sh

#
# Sets up envronment variables for ICC.
#
setup_icc()
{
	DEFAULT_PATH=/opt/intel/bin/iccvars.sh
	param=$1
	shpath=`which iccvars.sh 2> /dev/null`
	if [ $? -eq 0 ] ; then
		echo "Loading iccvars.sh from $shpath for $param"
		source $shpath $param
	elif [ -f $DEFAULT_PATH ] ; then
		echo "Loading iccvars.sh from $DEFAULT_PATH for $param"
		source $DEFAULT_PATH $param
	else
		echo "## ERROR: cannot find 'iccvars.sh' script to set up ICC."
		echo "##     To fix, please add the directory that contains"
		echo "##     iccvars.sh  to your 'PATH' environment variable."
		quit
	fi
}

#
# Sets RTE_TARGET and does a "make install".
#
setup_target()
{
	#option=$1
	#export RTE_TARGET=${TARGETS[option]}

	compiler=${RTE_TARGET##*-}
	if [ "$compiler" == "icc" ] ; then
		platform=${RTE_TARGET%%-*}
		if [ "$platform" == "x86_64" ] ; then
			setup_icc intel64
		else
			setup_icc ia32
		fi
	fi
	#if [ "$QUIT" == "0" ] ; then
		if [ ! -d $RTE_SDK/$RTE_TARGET ]; then
			make config T=${RTE_TARGET} O=$RTE_SDK/$RTE_TARGET
			sed -i 's/CONFIG_RTE_MEMPOOL_CACHE_MAX_SIZE=.*/CONFIG_RTE_MEMPOOL_CACHE_MAX_SIZE=8192/g' $RTE_SDK/$RTE_TARGET/.config
			rm $RTE_SDK/$RTE_TARGET/include/rte_config.h
		fi
		make -C $RTE_SDK/$RTE_TARGET
	#fi
	#echo "------------------------------------------------------------------------------"
	#echo " RTE_TARGET exported as $RTE_TARGET"
	#echo "------------------------------------------------------------------------------"
}

#
# Uninstall all targets.
#
uninstall_targets()
{
	make uninstall
}

#
# Creates hugepage filesystem.
#
create_mnt_huge()
{
	echo "Creating /mnt/huge and mounting as hugetlbfs"
	sudo mkdir -p /mnt/huge

	grep -s '/mnt/huge' /proc/mounts > /dev/null
	if [ $? -ne 0 ] ; then
		sudo mount -t hugetlbfs nodev /mnt/huge
	fi
}

#
# Removes hugepage filesystem.
#
remove_mnt_huge()
{
	echo "Unmounting /mnt/huge and removing directory"
	grep -s '/mnt/huge' /proc/mounts > /dev/null
	if [ $? -eq 0 ] ; then
		sudo umount /mnt/huge
	fi

	if [ -d /mnt/huge ] ; then
		sudo rm -R /mnt/huge
	fi
}

#
# Unloads igb_uio.ko.
#
remove_igb_uio_module()
{
	echo "Unloading any existing DPDK UIO module"
	/sbin/lsmod | grep -s igb_uio > /dev/null
	if [ $? -eq 0 ] ; then
		sudo /sbin/rmmod igb_uio
	fi
}

#
# Loads new igb_uio.ko (and uio module if needed).
#
load_igb_uio_module()
{
	if [ ! -f $RTE_SDK/$RTE_TARGET/kmod/igb_uio.ko ];then
		echo "## ERROR: Target does not have the DPDK UIO Kernel Module."
		echo "       To fix, please try to rebuild target."
		return
	fi

	remove_igb_uio_module

	/sbin/lsmod | grep -s uio > /dev/null
	if [ $? -ne 0 ] ; then
		if [ -f /lib/modules/$(uname -r)/kernel/drivers/uio/uio.ko ] ; then
			echo "Loading uio module"
			sudo /sbin/modprobe uio
		fi
	fi

	# UIO may be compiled into kernel, so it may not be an error if it can't
	# be loaded.

	echo "Loading DPDK UIO module"
	sudo /sbin/insmod $RTE_SDK/$RTE_TARGET/kmod/igb_uio.ko
	if [ $? -ne 0 ] ; then
		echo "## ERROR: Could not load kmod/igb_uio.ko."
		quit
	fi
}

#
# Removes all reserved hugepages.
#
clear_huge_pages()
{
	echo > .echo_tmp
	for d in /sys/devices/system/node/node? ; do
		echo "echo 0 > $d/hugepages/hugepages-2048kB/nr_hugepages" >> .echo_tmp
	done
	echo "Removing currently reserved hugepages"
	sudo sh .echo_tmp
	rm -f .echo_tmp

	remove_mnt_huge
}

#
# Creates hugepages.
#
set_non_numa_pages()
{
	clear_huge_pages

	echo ""
	echo "  Input the number of 2MB pages"
	echo "  Example: to have 128MB of hugepages available, enter '64' to"
	echo "  reserve 64 * 2MB pages"
	echo -n "Number of pages: "
	#read Pages
	Pages=$1

	echo "echo $Pages > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages" > .echo_tmp

	echo "Reserving hugepages"
	sudo sh .echo_tmp
	rm -f .echo_tmp

	create_mnt_huge
}

#
# Creates hugepages on specific NUMA nodes.
#
set_numa_pages()
{
	clear_huge_pages

	echo ""
	echo "  Input the number of 2MB pages for each node"
	echo "  Example: to have 128MB of hugepages available per node,"
	echo "  enter '64' to reserve 64 * 2MB pages on each node"

	echo > .echo_tmp
	for d in /sys/devices/system/node/node? ; do
		node=$(basename $d)
		echo -n "Number of pages for $node: "
		#read Pages
		Pages=$1
		shift
		echo "echo $Pages > $d/hugepages/hugepages-2048kB/nr_hugepages" >> .echo_tmp
	done
	echo "Reserving hugepages"
	sudo sh .echo_tmp
	rm -f .echo_tmp

	create_mnt_huge
}

#
# Run unit test application.
#
run_test_app()
{
	echo ""
	echo "  Enter hex bitmask of cores to execute test app on"
	echo "  Example: to execute app on cores 0 to 7, enter 0xff"
	echo -n "bitmask: "
	read Bitmask
	echo "Launching app"
	sudo ${RTE_TARGET}/app/test -c $Bitmask $EAL_PARAMS
}

#
# Run unit testpmd application.
#
run_testpmd_app()
{
	echo ""
	echo "  Enter hex bitmask of cores to execute testpmd app on"
	echo "  Example: to execute app on cores 0 to 7, enter 0xff"
	echo -n "bitmask: "
	read Bitmask
	echo "Launching app"
	sudo ${RTE_TARGET}/app/testpmd -c $Bitmask $EAL_PARAMS -- -i
}

#
# Print hugepage information.
#
grep_meminfo()
{
	grep -i huge /proc/meminfo
}

#
# List all hugepage file references
#
ls_mnt_huge()
{
	ls -lh /mnt/huge
}

##### from DPDK/tools/setup.sh


export RTE_SDK=`readlink -f $(dirname ${BASH_SOURCE[0]})/../../DPDK`
export RTE_TARGET=x86_64-default-linuxapp-gcc

pushd "$RTE_SDK"; setup_target; popd

#if [ "$HOSTNAME" == "server" ]; then
	set_numa_pages 8192 8192	# 32 GiB
#else
#	set_numa_pages 2048 2048	# 8 GiB
#fi
load_igb_uio_module

grep_meminfo


#sudo $RTE_SDK/tools/pci_unbind.py --force --bind=igb_uio xge0 xge1 xge2 xge3
#sudo $RTE_SDK/tools/pci_unbind.py --force --bind=igb_uio xge4 xge5 xge6 xge7
sudo $RTE_SDK/tools/pci_unbind.py --force --bind=igb_uio 0000:06:00.0
sudo $RTE_SDK/tools/pci_unbind.py --force --bind=igb_uio 0000:06:00.1

# disable OOM kills
sudo sysctl -w vm.overcommit_memory=1
sudo sysctl -w kernel.shmmax=12884901888
sudo sysctl -w kernel.shmall=12884901888
