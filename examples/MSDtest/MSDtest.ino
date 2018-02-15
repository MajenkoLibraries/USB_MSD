#include <DFATFS.h>
#include <RAMVOL.h>
#include <USB_MSD.h>


const uint32_t diskSectors = RAMVOL::CMINSECTORS;
const uint32_t sectorSize = RAMVOL::CBSECTOR;

uint8_t ramVolBuff[diskSectors * sectorSize];
RAMVOL      ramVol(ramVolBuff, diskSectors * sectorSize);

USBFS usbDevice;
USBManager USB(usbDevice, 0xf055, 0x1234);
//CDCACM debug;
USB_MSD ramdisk(ramVol);

void setup() {
//    Serial.begin(115200);
//    delay(1000);
//    Serial.println("Press any key to start ... ");
//    while (Serial.available()) (void)Serial.read();
//    while (!Serial.available());
//    while (Serial.available()) (void)Serial.read();


    
    ramVol.disk_initialize();

    DFATFS::fsmkfs(ramVol);
    
//    USB.addDevice(debug);
    USB.addDevice(ramdisk);

    ramdisk.setCapacity(diskSectors, sectorSize);
    
    USB.begin();
//    delay(1000);
//    Serial.println("Running");
}

void loop() {
    
}
