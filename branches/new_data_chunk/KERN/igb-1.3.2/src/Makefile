#$FreeBSD$
.PATH:  ${.CURDIR}
KMOD    = if_igb
SRCS    = device_if.h bus_if.h pci_if.h opt_bdg.h
SRCS    += if_igb.c $(SHARED_SRCS)
SRCS    += tcp_lro.c
SHARED_SRCS = e1000_api.c e1000_phy.c e1000_nvm.c e1000_mac.c e1000_manage.c
SHARED_SRCS += e1000_osdep.c e1000_82575.c

# Enable NEW HW VLAN SUPPORT - requires kernel vlan patch
#CFLAGS  += -DIGB_HW_VLAN_SUPPORT

# Enable IEEE 1588 PTP support
#CFLAGS  += -DIGB_TIMESYNC

# DEVICE_POLLING gives you non-interrupt handling
#CFLAGS  += -DDEVICE_POLLING

clean:
	rm -f opt_bdg.h device_if.h bus_if.h pci_if.h setdef*
	rm -f *.o *.kld *.ko
	rm -f @ machine
	rm -f ${CLEANFILES}

man:
	mv /usr/share/man/man4/igb.4.gz /usr/share/man/man4/igbSAVE.4.gz
	cp igb.4 /usr/share/man/man4/

.include <bsd.kmod.mk>
