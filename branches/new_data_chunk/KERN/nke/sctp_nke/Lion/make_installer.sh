#!/bin/bash
#
# Script to create a installer for the just built SCTP.kext
#
# (c) Copyright 2006/2007/2008 Andreas Fink <andreas@fink.org>
# provided as open source to the community
#


if [ `id -g` = "0" ]
then
	export LONGVER="6.1.0 built on `date`"
	RELEASE_OR_DEBUG="Debug"
	umask 0022


	# maindir must be relative
	MAINDIR="MacOSX_installer_root"
	SDK106="${MAINDIR}/Developer/SDKs/MacOSX10.6.sdk/"
	SDK107="${MAINDIR}/Developer/SDKs/MacOSX10.7.sdk/"

	rm -rf "${MAINDIR}"

	mkdir -p "${MAINDIR}/usr/include/netinet"
	mkdir -p "${SDK106}/usr/include/netinet"
	mkdir -p "${SDK107}/usr/include/netinet"

	mkdir -p "${MAINDIR}/usr/lib"
	mkdir -p "${SDK106}/usr/lib"
	mkdir -p "${SDK107}/usr/lib"

	mkdir -p "${MAINDIR}/System/Library/Extensions/SCTP.kext/Contents"
	mkdir -p "${MAINDIR}/Library/StartupItems/SCTP"
	for DIR in "${MAINDIR}" "${SDK106}" "${SDK107}"
	do
		cp ./netinet/sctp.h ./netinet/sctp_uio.h "${DIR}/usr/include/netinet"
		cp ./sys/socket.h  "${DIR}/usr/include"
		cp "build/${RELEASE_OR_DEBUG}/libsctp.dylib" "${DIR}/usr/lib"
	done

	chown -R root:wheel "${MAINDIR}"

	xcodebuild -configuration "${RELEASE_OR_DEBUG}" -alltargets

	cd "build/${RELEASE_OR_DEBUG}"
	find SCTP.kext | cpio -pdmuv "../../${MAINDIR}/System/Library/Extensions" 2> /dev/null
	cd ..
	cd ..
	cp Info-template.plist  "${MAINDIR}/System/Library/Extensions/SCTP.kext/Contents/Info-template.plist"

    tar -xvzf "SCTPSupport.kext.tar.gz"
    mv SCTPSupport.kext "${MAINDIR}/System/Library/Extensions"

	chown -R root:wheel  "${MAINDIR}/System/Library/Extensions/SCTP*.kext"
	chmod 755 "${MAINDIR}/System/Library/Extensions/SCTP.kext"
	chmod 644 "${MAINDIR}/System/Library/Extensions/SCTP.kext/Contents"
	chmod 644 "${MAINDIR}/System/Library/Extensions/SCTP.kext/Contents/Info.plist"
	chmod 755 "${MAINDIR}/System/Library/Extensions/SCTP.kext/Contents/MacOS"
	chmod 644 "${MAINDIR}/System/Library/Extensions/SCTP.kext/Contents/MacOS/SCTP"
	chmod 644  "${MAINDIR}/System/Library/Extensions/SCTP.kext/Contents/Info-template.plist"
	mkdir -p 	"${MAINDIR}/Library/StartupItems/SCTP/
	chown root:wheel "${MAINDIR}/Library/StartupItems/SCTP/

	cat > "${MAINDIR}/Library/StartupItems/SCTP/StartupParameters.plist "<< --EOF--
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist SYSTEM "file://localhost/System/Library/DTDs/PropertyList.dtd">
<plist version="0.9">
<dict>
	<key>CFBundleVersion</key>
	<string>1.0</string>
	<key>Description</key>
	<string>SCTP</string>
	<key>Messages</key>
	<dict>
		<key>start</key>
		<string>Starting SCTP Driver</string>
		<key>stop</key>
		<string>Stopping SCTP Driver</string>
	</dict>
	<key>OrderPreference</key>
	<string>Late</string>
	<key>Provides</key>
	<array>
		<string>SCTP</string>
	</array>
    <key>Requires</key>
    <array>
        <string>Resolver</string>
    </array>
</dict>
</plist>
--EOF--
	chmod 644		"${MAINDIR}/Library/StartupItems/SCTP/StartupParameters.plist"
	chown root:wheel	"${MAINDIR}/Library/StartupItems/SCTP/StartupParameters.plist"

	cat > "${MAINDIR}/Library/StartupItems/SCTP/SCTP" << --EOF--
#!/bin/sh
#
##
# SCTP Startup Script
# (c) Copyright 2006-2012 Andreas Fink <andreas@fink.org>
#
##

. /etc/rc.common

SYMDIR="/var/log/sctp"
SYMFILE="org.sctp.nke.sctp.sym"

StatusService()
{
    kextstat | grep sctp
}

UpdatePlist()
{
	if [ -f /System/Library/Extensions/SCTPSupport.kext/Contents/Info-template.plist ]
	then
	   sed	"s/UNAME_MINUS_R_HERE/`uname -r`/g" < /System/Library/Extensions/SCTPSupport.kext/Contents/Info-template.plist >/System/Library/Extensions/SCTPSupport.kext/Contents/Info.plist
	   chmod 644			 /System/Library/Extensions/SCTPSupport.kext/Contents/Info.plist
	fi

	if [ -f /System/Library/Extensions/SCTP.kext/Contents/Info-template.plist ]
	then
	   sed	"s/UNAME_MINUS_R_HERE/`uname -r`/g" < /System/Library/Extensions/SCTP.kext/Contents/Info-template.plist >/System/Library/Extensions/SCTP.kext/Contents/Info.plist
	   chmod 644			 /System/Library/Extensions/SCTP.kext/Contents/Info.plist
	fi
}

PrepareService ()
{
    ConsoleMessage "Preparing SCTP driver"
#
# this has to be done to adapt the Info.plist file to the kernel extension version
# It is being called from the postinstall and postupgrade scripts of the installer
# to invalidate the cache and adapt to the local system it is being installed on
#
    UpdatePlist
    ARCH=`arch`
    if [ "${ARCH}" == "ppc" -o  "${ARCH}" == "ppc64" ]
    then
       /usr/sbin/kextcache -e -a ppc -a ppc64
    else
       /usr/sbin/kextcache -e -a i386 -a x86_64
    fi
}

RotateSyms()
{
mkdir -p "${SYMDIR}"
FILE6="${SYMDIR}/${SYMFILE}.6"
FILE5="${SYMDIR}/${SYMFILE}.5"
FILE4="${SYMDIR}/${SYMFILE}.4"
FILE3="${SYMDIR}/${SYMFILE}.3"
FILE2="${SYMDIR}/${SYMFILE}.2"
FILE1="${SYMDIR}/${SYMFILE}.1"
FILE0="${SYMDIR}/${SYMFILE}"


if [ -f "${FILE6}" ]
then
    rm "${FILE6}";
fi

if [ -f "${FILE5}" ]
then
    mv "${FILE5}" "${FILE6}"
fi

if [ -f "${FILE4}" ]
then
    mv "${FILE4}" "${FILE5}"
fi

if [ -f "${FILE3}" ]
then
    mv "${FILE3}" "${FILE4}"
fi

if [ -f "${FILE2}" ]
then
    mv "${FILE2}" "${FILE3}"
fi

if [ -f "${FILE1}" ]
then
    mv "${FILE1}" "${FILE2}"
fi

if [ -f "${FILE0}" ]
then
    mv "${FILE0}" "${FILE1}"
fi
}

#
FirstStart ()
{
	cd /System/Library/Extensions
	chown -R root:wheel SCTP.kext
	chown -R root:wheel SCTPSupport.kext
	PrepareService
}

StartService ()
{
    ConsoleMessage "Loading SCTP driver"
    UpdatePlist
    RotateSyms
#   kextutil -s "${SYMDIR}" -t -v /System/Library/Extensions/SCTPSupport.kext
    kextutil -s "${SYMDIR}" -t -v /System/Library/Extensions/SCTP.kext
    sysctl -w net.inet.sctp.addr_watchdog_limit=120
}

StopService ()
{
    ConsoleMessage "Unloading SCTP driver"
    kextunload /System/Library/Extensions/SCTP.kext
#   kextunload /System/Library/Extensions/SCTPSupport.kext
}

RestartService ()
{
    StopService
    StartService
}


if( [ "$1" = "start" ] )
then
    StartService
fi
if( [ "$1" = "stop" ] )
then
    StopService
fi

if( [ "$1" = "restart" ] )
then
    RestartService
fi

if( [ "$1" = "status" ] )
then
    StatusService
fi

if( [ "$1" = "prepare" ] )
then
    PrepareService
fi

if( [ "$1" = "FirstStart" ] )
then
    FirstStart
fi
--EOF--

	chmod 755	 "${MAINDIR}/Library/StartupItems/SCTP/SCTP"
	chown root:wheel "${MAINDIR}/Library/StartupItems/SCTP/SCTP"


/Developer/usr/bin/packagemaker --target 10.7 --root ./"${MAINDIR}"/  --out SCTP_Lion_`date +%Y%m%d%H%M`.pkg --id org.sctp.nke --version "$LONGVER" --title SCTP --install-to /  --verbose --root-volume-only --discard-forks 

else
	echo "You must be root to do this."
	echo "Let me call sudo for you"
	sudo /bin/bash $0 $*
fi
