#!/bin/bash
#
# Script to create a installer for the just built SCTP.kext
#
# (c) Copyright 2006-2012 Andreas Fink <andreas@fink.org>
# provided as open source to the community
#
# 4.8.2012: updated for MountainLion and possible multiple installed Xcodes in
#           /Applications furthermore package maker is expected as
#           /Applications/PackagMaker.app which is now an
#           optional install and doesn't come with Xcode by default
#


if [ ! -x "/Applications/PackageMaker.app/Contents/MacOS/PackageMaker" ]
then
	echo "/Applications/PackageMaker.app is missing."
	echo "Download and install Auxiliary Tools for Xcode from"
	echo "https://developer.apple.com/downloads"
	echo "and move PackageMaker.app to your /Applications folder"
	exit
fi

if [ `id -g` = "0" ]
then
	export LONGVER="6.2.0 built on `date`"
	RELEASE_OR_DEBUG="Debug"
	umask 0022

	xcodebuild -configuration "${RELEASE_OR_DEBUG}" -alltargets

	# maindir must be relative
	MAINDIR="MacOSX_installer_root"
    RSRCDIR="MacOSX_installer_resources"
    SCRIPTCDIR="MacOSX_installer_scripts"
	
	
	rm -rf "${MAINDIR}"
    rm -rf "${RSRCDIR}"
    rm -rf "${SCRIPTCDIR}"
	
    mkdir -p "${MAINDIR}"
    mkdir -p "${RSRCDIR}"
    mkdir -p "${SCRIPTCDIR}"
	

cat > "${SCRIPTCDIR}/Postinstall" << --EOF--
#!/bin/bash 
#
# $1: The full path to the installation package.
# $2: The full path to the installation destination.
# $3: The mountpoint of the destination volume.
# $4: The root directory for the current System folder.
#
# (c) Copyright 2006-2012 Andreas Fink <andreas@fink.org>

chown -R root:wheel "/System/Library/Extensions/SCTP.kext"
chmod 755 "/System/Library/Extensions/SCTP.kext"
chmod 644 "/System/Library/Extensions/SCTP.kext/Contents"
chmod 644 "/System/Library/Extensions/SCTP.kext/Contents/Info.plist"
chmod 755 "/System/Library/Extensions/SCTP.kext/Contents/MacOS"
chmod 644 "/System/Library/Extensions/SCTP.kext/Contents/MacOS/SCTP"
chmod 644 "/System/Library/Extensions/SCTP.kext/Contents/Info-template.plist"
chmod 644 "/System/Library/Extensions/SCTP.kext/Contents/Info.plist"

chown -R root:wheel "/System/Library/Extensions/SCTPSupport.kext"
chmod 755  "/System/Library/Extensions/SCTPSupport.kext"
chmod 755  "/System/Library/Extensions/SCTPSupport.kext/Contents"
chmod 644  "/System/Library/Extensions/SCTPSupport.kext/Contents/Info-template.plist"
chmod 755  "/System/Library/Extensions/SCTPSupport.kext/Contents/MacOS"
chmod 644  "/System/Library/Extensions/SCTPSupport.kext/Contents/MacOS/SCTPSupport"

EXECFILE="/Library/StartupItems/SCTP/SCTP"

if [ -f "\${EXECFILE}" ]
then
	chmod 755 "\${EXECFILE}"
	"\${EXECFILE}" FirstStart 2>&1 > /var/log/sctp/install.log
fi
--EOF--

    cp "${SCRIPTCDIR}/Postinstall"  "${SCRIPTCDIR}/Postupdate"
    echo 'echo "Postinstall run on `date`">> /var/log/sctp/install.log' >> "${SCRIPTCDIR}/Postinstall" 
    echo 'echo "Postupdate  run on `date`">> /var/log/sctp/install.log' >> "${SCRIPTCDIR}/Postupdate"

	chmod 755 "${SCRIPTCDIR}/Postinstall"
	chown -R root:wheel "${SCRIPTCDIR}/Postinstall"
    chmod 755 "${SCRIPTCDIR}/Postupdate"
    chown -R root:wheel "${SCRIPTCDIR}/Postupdate"

	mkdir -p "${MAINDIR}/usr/include/netinet"
	mkdir -p "${MAINDIR}/usr/lib"

	mkdir -p "${MAINDIR}/System/Library/Extensions/SCTP.kext/Contents"
	mkdir -p "${MAINDIR}/Library/StartupItems/SCTP"

	cd "build/${RELEASE_OR_DEBUG}"
	find SCTP.kext | cpio -pdmuv "../../${MAINDIR}/System/Library/Extensions" 2> /dev/null
	cd ..
	cd ..
	cp Info-template.plist  "${MAINDIR}/System/Library/Extensions/SCTP.kext/Contents/Info-template.plist"
	cp ./netinet/sctp.h ./netinet/sctp_uio.h "${MAINDIR}/usr/include/netinet"
	cp ./sys/socket.h  "${MAINDIR}/usr/include"
	cp "build/${RELEASE_OR_DEBUG}/libsctp.dylib" "${MAINDIR}/usr/lib"


	chown -R root:wheel "${MAINDIR}"

	chown -R root:wheel "${MAINDIR}/usr"
	chown -R root:wheel "${MAINDIR}/System/Library/Extensions"
	chown -R root:wheel "${MAINDIR}/Library/StartupItems/SCTP"

	cp Info-template.plist  "${MAINDIR}/System/Library/Extensions/SCTP.kext/Contents/Info-template.plist"
	chown root:wheel "${MAINDIR}/Library/StartupItems/SCTP/"
    tar -xvzf "SCTPSupport.kext.tar.gz"
    mv SCTPSupport.kext "${MAINDIR}/System/Library/Extensions"

	chown -R root:wheel  "${MAINDIR}/System/Library/Extensions/SCTP"*".kext"
	chmod 755 "${MAINDIR}/System/Library/Extensions/SCTP.kext"
	chmod 644 "${MAINDIR}/System/Library/Extensions/SCTP.kext/Contents"
	chmod 644 "${MAINDIR}/System/Library/Extensions/SCTP.kext/Contents/Info.plist"
	chmod 755 "${MAINDIR}/System/Library/Extensions/SCTP.kext/Contents/MacOS"
	chmod 644 "${MAINDIR}/System/Library/Extensions/SCTP.kext/Contents/MacOS/SCTP"
	chmod 644  "${MAINDIR}/System/Library/Extensions/SCTP.kext/Contents/Info-template.plist"
	mkdir -p 	"${MAINDIR}/Library/StartupItems/SCTP/"

	cat > "${MAINDIR}/Library/StartupItems/SCTP/StartupParameters.plist" << --EOF--
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
	fi
	chmod 644			 /System/Library/Extensions/SCTPSupport.kext/Contents/Info.plist

	if [ -f /System/Library/Extensions/SCTP.kext/Contents/Info-template.plist ]
	then
	   sed	"s/UNAME_MINUS_R_HERE/`uname -r`/g" < /System/Library/Extensions/SCTP.kext/Contents/Info-template.plist >/System/Library/Extensions/SCTP.kext/Contents/Info.plist
	fi
	chmod 644			 /System/Library/Extensions/SCTP.kext/Contents/Info.plist
}

PrepareService ()
{
    ConsoleMessage "Preparing SCTP driver"
#
# this has to be done to adapt the Info.plist file to the kernel extension version
# It is being called from the Postinstall and postupgrade scripts of the installer
# to invalidate the cache and adapt to the local system it is being installed on
#
    UpdatePlist
    ARCH=\`arch\`
    if [ "\${ARCH}" == "ppc" -o  "\${ARCH}" == "ppc64" ]
    then
       /usr/sbin/kextcache -e -a ppc -a ppc64
    else
        if [ "\`uname -r | cut -f1 -d.\`" == "12" ]
        then
           /usr/sbin/kextcache -e -a x86_64
        else
           /usr/sbin/kextcache -e -a i386 -a x86_64
        fi
    fi
}

PrepareSDK()
{
#
# now lets update developer SDK's to contain the same libraries and headers
#
cd /Applications
for XCODE in  Xcode*.app
do

    cd "/Applications/\${XCODE}/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs"
    for SDK in MacOSX*.sdk
    do
        cpio -pdmuv "/Applications/\${XCODE}/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/\${SDK}" << --XOF--
/usr/include/netinet/sctp.h
/usr/include/netinet/sctp_uio.h
/usr/include/socket.h
/usr/lib/libsctp.dylib
--XOF--
    done
done
}

RotateSyms()
{
mkdir -p "\${SYMDIR}"
FILE6="\${SYMDIR}/\${SYMFILE}.6"
FILE5="\${SYMDIR}/\${SYMFILE}.5"
FILE4="\${SYMDIR}/\${SYMFILE}.4"
FILE3="\${SYMDIR}/\${SYMFILE}.3"
FILE2="\${SYMDIR}/\${SYMFILE}.2"
FILE1="\${SYMDIR}/\${SYMFILE}.1"
FILE0="\${SYMDIR}/\${SYMFILE}"


if [ -f "\${FILE6}" ]
then
    rm "\${FILE6}";
fi

if [ -f "\${FILE5}" ]
then
    mv "\${FILE5}" "\${FILE6}"
fi

if [ -f "\${FILE4}" ]
then
    mv "\${FILE4}" "\${FILE5}"
fi

if [ -f "\${FILE3}" ]
then
    mv "\${FILE3}" "\${FILE4}"
fi

if [ -f "\${FILE2}" ]
then
    mv "\${FILE2}" "\${FILE3}"
fi

if [ -f "\${FILE1}" ]
then
    mv "\${FILE1}" "\${FILE2}"
fi

if [ -f "\${FILE0}" ]
then
    mv "\${FILE0}" "\${FILE1}"
fi
}

#
FirstStart ()
{
	cd /System/Library/Extensions
	chown -R root:wheel SCTP.kext
	chown -R root:wheel SCTPSupport.kext
	PrepareService
	PrepareSDK
	StartService
}

StartService ()
{
    ConsoleMessage "Loading SCTP driver"
    UpdatePlist
    RotateSyms
#   kextutil -s "\${SYMDIR}" -t -v /System/Library/Extensions/SCTPSupport.kext
    kextutil -s "\${SYMDIR}" -t -v /System/Library/Extensions/SCTP.kext
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


if( [ "\$1" = "start" ] )
then
    StartService
fi
if( [ "\$1" = "stop" ] )
then
    StopService
fi

if( [ "\$1" = "restart" ] )
then
    RestartService
fi

if( [ "\$1" = "status" ] )
then
    StatusService
fi

if( [ "\$1" = "prepare" ] )
then
    PrepareService
fi

if( [ "\$1" = "FirstStart" ] )
then
    FirstStart
fi

if( [ "\$1" = "PrepareSDK" ] )
then
    PrepareSDK
fi

--EOF--

chmod 644 "${MAINDIR}/Library/StartupItems/SCTP/StartupParameters.plist"
chown root:wheel "${MAINDIR}/Library/StartupItems/SCTP/StartupParameters.plist"
chown root:wheel "${MAINDIR}/Library/StartupItems/SCTP/"
chmod 755	     "${MAINDIR}/Library/StartupItems/SCTP/SCTP"
chown root:wheel "${MAINDIR}/Library/StartupItems/SCTP/SCTP"

/Applications/PackageMaker.app/Contents/MacOS/PackageMaker --scripts "${SCRIPTCDIR}" --resources "${RSRCDIR}" --target 10.7 --root "${MAINDIR}"  --out SCTP_MountainLion_`date +%Y%m%d%H%M`.pkg --id org.sctp.nke --version "$LONGVER" --title SCTP --install-to /  --verbose --root-volume-only --discard-forks

else
	echo "You must be root to do this."
	echo "Let me call sudo for you"
	sudo /bin/bash $0 $*
fi
