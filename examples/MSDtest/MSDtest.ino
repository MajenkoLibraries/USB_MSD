#include <DFATFS.h>
#include <RAMVOL.h>
#include <USB_MSD.h>


const uint32_t diskSectors = RAMVOL::CMINSECTORS;
const uint32_t sectorSize = RAMVOL::CBSECTOR;

uint8_t ramVolBuff[diskSectors * sectorSize];
RAMVOL      ramVol(ramVolBuff, diskSectors);

USBFS usbDevice;
USBManager USB(usbDevice, 0xf055, 0x1234);
USB_MSD ramdisk(ramVol);

void setup() {
    ramVol.disk_initialize();
    DFATFS::fsmkfs(ramVol);
    USB.addDevice(ramdisk);
    USB.begin();
}

void loop() {
    
}
