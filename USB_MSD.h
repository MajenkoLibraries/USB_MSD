#ifndef _USB_MSD_H
#define _USB_MSD_H

#include <Arduino.h>
#include <DFATFS.h>

// Command Block Wrapper
struct msdCBW {
    uint32_t dCBWSignature;
    uint32_t dCBWTag;
    uint32_t dCBWDataTransferLength;
    uint8_t bmCBWFlags;
    uint8_t bCBWLUN;
    uint8_t bCBWCBLength;
    uint8_t CBWCB[16];
} __attribute__((packed));

// Command Status Wrapper
struct msdCSW {
    uint32_t dCSWSignature;
    uint32_t dCSWTag;
    uint32_t dCSWDataResidue;
    uint8_t bCSWStatus;
} __attribute__((packed));

struct msdCDBInquiryCommand {
    uint8_t opCode;
    uint8_t evpd;
    uint8_t pageCode;
    uint8_t length8;
    uint8_t length0;
    uint8_t control;
} __attribute__((packed));

struct msdCDBInquiryData {
    uint8_t peripheral;
    uint8_t rmb;
    uint8_t version;
    uint8_t format;
    uint8_t additionalLength;
    uint8_t flags1;
    uint8_t flags2;
    uint8_t flags3;
    char t10Vendor[8];
    char productId[16];
    char productRevision[4];
} __attribute__((packed));

struct msdCDBReadCapacityCommand {
    uint8_t opCode;
    uint8_t reserved1;
    uint8_t lba24;
    uint8_t lba16;
    uint8_t lba8;
    uint8_t lba0;
    uint8_t reserved2;
    uint8_t reserved3;
    uint8_t pmi;
    uint8_t control;
} __attribute__((packed));

struct msdCDBReadCapacityData {
    uint8_t lba24;
    uint8_t lba16;
    uint8_t lba8;
    uint8_t lba0;
    uint8_t size24;
    uint8_t size16;
    uint8_t size8;
    uint8_t size0;
} __attribute__((packed));

struct msdCDBModeSenseCommand {
    uint8_t opCode;
    uint8_t dbd;
    uint8_t pageCode;
    uint8_t subpageCode;
    uint8_t allocationLength;   
    uint8_t control;
} __attribute__((packed));

struct msdCDBModeSense10Command {
    uint8_t opCode;
    uint8_t dbd;
    uint8_t pageCode;
    uint8_t subpageCode;
    uint8_t reserved1;
    uint8_t reserved2;
    uint8_t reserved3;
    uint8_t allocationLength8;   
    uint8_t allocationLength0;   
    uint8_t control;
} __attribute__((packed));

struct msdCDBRequestSenseCommand {
    uint8_t opCode;
    uint8_t desc;
    uint8_t reserved2;
    uint8_t reserved3;
    uint8_t allocationLength;
    uint8_t control;
} __attribute__((packed));

struct msdCDBRead10Command {
    uint8_t opCode;
    uint8_t flags;
    uint8_t lba24;
    uint8_t lba16;
    uint8_t lba8;
    uint8_t lba0;
    uint8_t group;
    uint8_t length8;
    uint8_t length0;
    uint8_t control;
} __attribute__((packed));

struct msdCDBWrite10Command {
    uint8_t opCode;
    uint8_t flags;
    uint8_t lba24;
    uint8_t lba16;
    uint8_t lba8;
    uint8_t lba0;
    uint8_t group;
    uint8_t length8;
    uint8_t length0;
    uint8_t control;
} __attribute__((packed));

class USB_MSD : public USBDevice {
    private:
        DFSVOL *_volume;
        uint8_t _ifBulk;
        uint8_t _epBulk;
        USBManager *_manager;
        uint32_t _diskSectors;
        uint32_t _sectorSize;

        bool processCommandBlock(msdCBW *);

#if defined (__PIC32MX__)
#define USBMSD_BULKEP_SIZE 64
        uint8_t _bulkRxA[USBMSD_BULKEP_SIZE];
        uint8_t _bulkRxB[USBMSD_BULKEP_SIZE];
        uint8_t _bulkTxA[USBMSD_BULKEP_SIZE];
        uint8_t _bulkTxB[USBMSD_BULKEP_SIZE];
#define USBMSD_BUFFER_HIGH 4
#elif defined(__PIC32MZ__)
#define USBMSD_BUFFER_SIZE 256
#define USBMSD_BULKEP_SIZE 512
        uint8_t _bulkRxA[USBMSD_BULKEP_SIZE];
        uint8_t _bulkRxB[USBMSD_BULKEP_SIZE];
        uint8_t _bulkTxA[USBMSD_BULKEP_SIZE];
        uint8_t _bulkTxB[USBMSD_BULKEP_SIZE];
#define USBMSD_BUFFER_HIGH 8
#endif

        struct msdCSW _response;

        uint32_t _state;

        static const uint32_t IDLE = 0;
        static const uint32_t RESPOND = 1;
        static const uint32_t READ = 2;
        static const uint32_t WRITE = 3;

        uint32_t _nextSector;
        uint32_t _toTransfer;
        uint32_t _fragmentOffset;

    public:
        USB_MSD(DFSVOL &dv) : _volume(&dv) {}
        USB_MSD(DFSVOL *dv) : _volume(dv) {}

        uint16_t getDescriptorLength();
        uint8_t getInterfaceCount();
        bool getStringDescriptor(uint8_t idx, uint16_t maxlen);
        uint32_t populateConfigurationDescriptor(uint8_t *buf);
        void initDevice(USBManager *manager);
        bool getDescriptor(uint8_t ep, uint8_t target, uint8_t id, uint8_t maxlen);
        bool getReportDescriptor(uint8_t ep, uint8_t target, uint8_t id, uint8_t maxlen);
        void configureEndpoints();
        bool onSetupPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l);
        bool onOutPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l);
        bool onInPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l);
        void onEnumerated();

        void setCapacity(uint32_t nsec, uint32_t secsiz) {
            _diskSectors = nsec;
            _sectorSize = secsiz;
        }
         
};

#endif
