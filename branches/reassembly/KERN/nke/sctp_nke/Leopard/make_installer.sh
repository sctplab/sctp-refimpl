#!/bin/bash
#
# Script to create a installer for the just built SCTP.kext
#
# (c) Copyright 2006/2007/2008 Andreas Fink <andreas@fink.org>
# provided as open source to the community
#


if [ `id -g` = "0" ]
then
	export LONGVER="5.2.0 built on `date`"
	RELEASE_OR_DEVELOPMENT="Development"
	umask 0022

	rm -rf MacOS_Installer/Files

	mkdir -p MacOS_Installer/Files/usr/include/netinet
#	mkdir -p MacOS_Installer/Files/Developer/SDKs/MacOSX10.4u.sdk/usr/include/netinet
#	mkdir -p MacOS_Installer/Files/Developer/SDKs/MacOSX10.5.sdk/usr/include/netinet

	mkdir -p MacOS_Installer/Files/usr/lib
#	mkdir -p MacOS_Installer/Files/Developer/SDKs/MacOSX10.4u.sdk/usr/lib
#	mkdir -p MacOS_Installer/Files/Developer/SDKs/MacOSX10.5.sdk/usr/lib

	cp ./netinet/sctp.h ./netinet/sctp_uio.h MacOS_Installer/Files/usr/include/netinet
#	cp ./netinet/sctp.h ./netinet/sctp_uio.h MacOS_Installer/Files/Developer/SDKs/MacOSX10.4u.sdk/usr/include/netinet
#	cp ./netinet/sctp.h ./netinet/sctp_uio.h MacOS_Installer/Files/Developer/SDKs/MacOSX10.5.sdk/usr/include/netinet

	cp "build/${RELEASE_OR_DEVELOPMENT}/libsctp.dylib" MacOS_Installer/Files/usr/lib
#	cp "build/${RELEASE_OR_DEVELOPMENT}/libsctp.dylib" MacOS_Installer/Files/Developer/SDKs/MacOSX10.4u.sdk/usr/lib
#	cp "build/${RELEASE_OR_DEVELOPMENT}/libsctp.dylib" MacOS_Installer/Files/Developer/SDKs/MacOSX10.5.sdk/usr/lib

	chown -R root:wheel MacOS_Installer/Files

	cd "build/${RELEASE_OR_DEVELOPMENT}"
	find SCTP.kext | cpio -pdmuv ../../MacOS_Installer/Files/System/Library/Extensions/ 2> /dev/null
	cd ..
	cd ..
	cp Info.plist.template MacOS_Installer/Files/System/Library/Extensions/SCTP.kext/Contents/Info.plist.template

	chown -R root:wheel  MacOS_Installer/Files/System/Library/Extensions/SCTP.kext
	chmod 755 MacOS_Installer/Files/System/Library/Extensions/SCTP.kext
	chmod 644 MacOS_Installer/Files/System/Library/Extensions/SCTP.kext/Contents
	chmod 644 MacOS_Installer/Files/System/Library/Extensions/SCTP.kext/Contents/Info.plist
	chmod 755 MacOS_Installer/Files/System/Library/Extensions/SCTP.kext/Contents/MacOS
	chmod 644 MacOS_Installer/Files/System/Library/Extensions/SCTP.kext/Contents/MacOS/SCTP
	chmod 755 MacOS_Installer/Files/System/Library/Extensions/SCTP.kext/Contents/Resources
	chmod 755 MacOS_Installer/Files/System/Library/Extensions/SCTP.kext/Contents/Resources/English.lproj

#
# 8.9.1 is whats in the curent plist file, so you might have to change it here if the info.plist is being changed
#
#	sed	"s/8.9.1/UNAME_MINUS_R_HERE/g" < "build/${RELEASE_OR_DEVELOPMENT}/SCTP.kext/Contents/Info.plist"  > MacOS_Installer/Files/System/Library/Extensions/SCTP.kext/Contents/Info.plist.template
#	sed	"s/8.9.1/`uname -r`/g" < "build/${RELEASE_OR_DEVELOPMENT}/SCTP.kext/Contents/Info.plist" 		> MacOS_Installer/Files/System/Library/Extensions/SCTP.kext/Contents/Info.plist
	chmod 644  MacOS_Installer/Files/System/Library/Extensions/SCTP.kext/Contents/Info.plist.template
	mkdir -p 	MacOS_Installer/Files/Library/StartupItems/SCTP/
	chown root:wheel MacOS_Installer/Files/Library/StartupItems/SCTP/

	cat > MacOS_Installer/Files/Library/StartupItems/SCTP/StartupParameters.plist << --EOF--
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
	chmod 644		MacOS_Installer/Files/Library/StartupItems/SCTP/StartupParameters.plist
	chown root:wheel	MacOS_Installer/Files/Library/StartupItems/SCTP/StartupParameters.plist

	cat > MacOS_Installer/Files/Library/StartupItems/SCTP/SCTP << --EOF--
#!/bin/sh
#
##
# SCTP Startup Script
# (c) Copyright 2006/2007/2008/2009 Andreas Fink <andreas@fink.org>
#
##

. /etc/rc.common

StatusService()
{
    kextstat | grep sctp
}

UpdatePlist()
{
    sed	"s/UNAME_MINUS_R_HERE/\`uname -r\`/g" < /System/Library/Extensions/SCTP.kext/Contents/Info.plist.template >/System/Library/Extensions/SCTP.kext/Contents/Info.plist
    chmod 644			 /System/Library/Extensions/SCTP.kext/Contents/Info.plist
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
    if [ "`arch`" == "ppc" -o  "`arch`" == "ppc64" ]
    then
       /usr/sbin/kextcache -e -a ppc -a ppc64
    else
       /usr/sbin/kextcache -e -a i386 -a x86_64
    fi
}

RotateSyms ()
{
    DIR="/System/Library/Extensions/SCTP.kext"
    BNDL="org.sctp.nke.sctp"
    PREV=6
    for i in 5 4 3 2 1
    do
        if [ -f  "\${DIR}/\${BNDL}.sym.\${i}" ]
        then
            mv -f "\${DIR}/\${BNDL}.sym.\${i}" "\${DIR}/\${BNDL}.sym.\${PREV}"
        fi
    done
    if [ -f  "\${DIR}/\${BNDL}.sym.\${PREV}" ]
    then
        mv -f "\${DIR}/\${BNDL}.sym" "\${DIR}/\${BNDL}.sym.\${PREV}"
    fi
}

StartService ()
{
    ConsoleMessage "Loading SCTP driver"
    UpdatePlist
    RotateSyms
    kextload -t -v -s /System/Library/Extensions/SCTP.kext /System/Library/Extensions/SCTP.kext
}

StopService ()
{
    ConsoleMessage "Unloading SCTP driver"
    kextunload /System/Library/Extensions/SCTP.kext
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

if( [ "\$1" = "rotate" ] )
then
    RotateSyms
fi


--EOF--

	chmod 755	 MacOS_Installer/Files/Library/StartupItems/SCTP/SCTP
	chown root:wheel MacOS_Installer/Files/Library/StartupItems/SCTP/SCTP


/Developer/usr/bin/packagemaker --target 10.5 --root ./MacOS_Installer/Files/  --out SCTP_Leopard_`date +%Y%m%d%H%M`.pkg --id org.sctp.nke --version "$LONGVER" --title SCTP --install-to /  --scripts MacOS_Installer/Scripts/ --verbose --root-volume-only --discard-forks 

#	/Developer/usr/bin/packagemaker -build -p SCTP.pkg -proj SCTP.pmdoc
#sed "s/LONG_VERSION_STRING/$LONGVER/g" < SCTP.pkg/Contents/Info.plist  > SCTP.pkg/Contents/Info.plist.new
#mv SCTP.pkg/Contents/Info.plist.new   SCTP.pkg/Contents/Info.plist
#	mv "SCTP.pkg" "SCTP_`date +%Y%m%d%H%M`.pkg"

else
	echo "You must be root to do this."
	echo "Let me call sudo for you"
	sudo /bin/bash $0 $*
fi
