#include <USB_MSD.h>

uint16_t USB_MSD::getDescriptorLength() {
    return 9 + 7 + 7;
}

uint8_t USB_MSD::getInterfaceCount() {
    return 1;
}

bool USB_MSD::getStringDescriptor(uint8_t idx, uint16_t maxlen) {
    return false;
}

uint32_t USB_MSD::populateConfigurationDescriptor(uint8_t *buf) {
    uint8_t i = 0;

    // Interface Descriptor
    buf[i++] = 0x09;            // bLength
    buf[i++] = 0x04;            // bDescriptorType
    buf[i++] = _ifBulk;         // bInterfaceNumber
    buf[i++] = 0x00;            // bAlternateSetting
    buf[i++] = 0x02;            // bNumEndpoints
    buf[i++] = 0x08;            // bInterfaceClass
    buf[i++] = 0x06;            // bInterafceSubClass
    buf[i++] = 0x50;            // bInterfaceProtocol
    buf[i++] = 0x00;            // iInterface

    // Bulk-In Endpoint
    buf[i++] = 0x07;            // bLength
    buf[i++] = 0x05;            // bDescriptorType
    buf[i++] = 0x80 | _epBulk;  // bEndpointAddress
    buf[i++] = 0x02;            // bmAttributes
    if (_manager->isHighSpeed()) {
        buf[i++] = 0x00;
        buf[i++] = 0x02;        // packet size
    } else {
        buf[i++] = 0x40;
        buf[i++] = 0x00;        // packet size
    }
    buf[i++] = 0x00;            // bInterval

    // Bulk-Out Endpoint
    buf[i++] = 0x07;            // bLength
    buf[i++] = 0x05;            // bDescriptorType
    buf[i++] = _epBulk;         // bEndpointAddress
    buf[i++] = 0x02;            // bmAttributes
    if (_manager->isHighSpeed()) {
        buf[i++] = 0x00;
        buf[i++] = 0x02;        // packet size
    } else {
        buf[i++] = 0x40;
        buf[i++] = 0x00;        // packet size
    }
    buf[i++] = 0x00;            // bInterval

    return i;
}

void USB_MSD::initDevice(USBManager *manager) {
    _manager = manager;
    _ifBulk = _manager->allocateInterface();
    _epBulk = _manager->allocateEndpoint();
    _state = IDLE;
}

bool USB_MSD::getDescriptor(uint8_t ep, uint8_t target, uint8_t id, uint8_t maxlen) {
    return false;
}

bool USB_MSD::getReportDescriptor(uint8_t ep, uint8_t target, uint8_t id, uint8_t maxlen) {
    return false;
}

void USB_MSD::configureEndpoints() {
    if (_manager->isHighSpeed()) {
        _manager->addEndpoint(_epBulk, EP_IN, EP_BLK, 512, _bulkRxA, _bulkRxB);
        _manager->addEndpoint(_epBulk, EP_OUT, EP_BLK, 512, _bulkTxA, _bulkTxB);
    } else {
        _manager->addEndpoint(_epBulk, EP_IN, EP_BLK, 64, _bulkRxA, _bulkRxB);
        _manager->addEndpoint(_epBulk, EP_OUT, EP_BLK, 64, _bulkTxA, _bulkTxB);
    }
}

//extern HardwareSerial Serial;
bool USB_MSD::onSetupPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l) {
    if (ep == _epBulk) {
//        Serial.printf("Control EP %d Target %d Length %d\r\n", ep, target, l);
//        for (int i = 0; i < l; i++) {
//            Serial.printf("%02x ", data[i]);
//        }
//        Serial.println();
        return true;
    }
    return false;
}

bool USB_MSD::onOutPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l) {
    if (ep == _epBulk) {

        struct msdCBW * cbw = (struct msdCBW *)data;

        if (_state == IDLE) {
            if (cbw->dCBWSignature == 0x43425355) {
    //            Serial.printf("CBW Signature: %08x\r\n", cbw->dCBWSignature);
    //            Serial.printf("CBW Tag:       %08x\r\n", cbw->dCBWTag);
//                Serial.printf("CBW DT Length: %08x\r\n", cbw->dCBWDataTransferLength);
//                Serial.printf("CBW Flags:     %02x\r\n", cbw->bmCBWFlags);
    //            Serial.printf("CBW Lun:       %02x\r\n", cbw->bCBWLUN);
    //            Serial.printf("CBW CB Length: %02x\r\n", cbw->bCBWCBLength);

//                for (int i = 0; i < cbw->bCBWCBLength; i++) {
//                    Serial.printf("%02x ", cbw->CBWCB[i]);
//                }
//                Serial.println();

                return processCommandBlock(cbw);
            }
        } else if (_state == WRITE) {
            // Received data should be stored...
Serial.printf("Write sector %d (%d remaining) offset %d\r\n", _nextSector, _toTransfer, _fragmentOffset);

            uint8_t *buf = (uint8_t *)alloca(_sectorSize);
            _volume->disk_read(buf, _nextSector, 1);
            memcpy(buf + _fragmentOffset, data, l);
            _volume->disk_write(buf, _nextSector, 1);
            _fragmentOffset += l;
            if (_fragmentOffset >= _sectorSize) {
                _nextSector ++;
                _toTransfer --;
                _fragmentOffset = 0;

                if (_toTransfer == 0) {
                    _manager->enqueuePacket(_epBulk, (uint8_t *)&_response, sizeof(_response));
                    _response.dCSWSignature = 0;
                    _state = IDLE;
                }
            }
        }
    }
    return false;
}

bool USB_MSD::processCommandBlock(struct msdCBW *cbw) {
    _response.dCSWTag = cbw->dCBWTag;
    _response.bCSWStatus = 0x01; // Default to failed.
    _response.dCSWSignature = 0x53425355;


    switch (cbw->CBWCB[0]) {
        case 0x00: { // TEST UNIT READY
            _response.bCSWStatus = 0x00;
            _manager->sendBuffer(_epBulk, (uint8_t *)&_response, sizeof(_response));
            _state = IDLE;
        } break;

        case 0x03: { // REQUEST SENSE
            struct msdCDBRequestSenseCommand *requestSense = (struct msdCDBRequestSenseCommand *)&cbw->CBWCB[0];

            uint8_t *buf = (uint8_t *)alloca(requestSense->allocationLength);

//Serial.printf("Sending sense data of size %d\r\n", requestSense->allocationLength);

            memset(buf, 0, requestSense->allocationLength);
            _manager->sendBuffer(_epBulk, buf, requestSense->allocationLength);
            _response.bCSWStatus = 0x00;
            _state = RESPOND;

        } break;

        case 0x12: { // INQUIRY

            struct msdCDBInquiryCommand *inquiryCommand = (struct msdCDBInquiryCommand *)&cbw->CBWCB[0];
            int size = (inquiryCommand->length8 << 8) | inquiryCommand->length0;
            if (size < 36) { // Ummm... not enough room for standard INQ data?
            } else {
                uint8_t *buf = (uint8_t *)alloca(size);
                struct msdCDBInquiryData *inquiryData = (struct msdCDBInquiryData *)buf;

                inquiryData->peripheral = 0x0E;
                inquiryData->rmb = 0;
                inquiryData->version = 0;
                inquiryData->format = 2;
                inquiryData->additionalLength = size - 4;
                inquiryData->flags1 = 0;
                inquiryData->flags2 = 0;
                inquiryData->flags3 = 0;
                memcpy(inquiryData->t10Vendor, "chipKIT ", 8);
                memcpy(inquiryData->productId, "USB Mass Storage", 16);
                memcpy(inquiryData->productRevision, "1.00", 4);

                _manager->sendBuffer(_epBulk, buf, size);
                _response.bCSWStatus = 0x00;
                _state = RESPOND;

            }
        } break;

        case 0x25: { // READ CAPACITY
            struct msdCDBReadCapacityCommand *capacityCommand = (struct msdCDBReadCapacityCommand *)&cbw->CBWCB[0];
            struct msdCDBReadCapacityData capacityData;

            capacityData.lba24 = (_diskSectors >> 24) & 0xFF;
            capacityData.lba16 = (_diskSectors >> 16) & 0xFF;
            capacityData.lba8 = (_diskSectors >> 8) & 0xFF;
            capacityData.lba0 = _diskSectors & 0xFF;

            capacityData.size24 = (_sectorSize >> 24) & 0xFF;
            capacityData.size16 = (_sectorSize >> 16) & 0xFF;
            capacityData.size8 = (_sectorSize >> 8) & 0xFF;
            capacityData.size0 = _sectorSize & 0xFF;
            
            _manager->sendBuffer(_epBulk, (uint8_t *)&capacityData, sizeof(struct msdCDBReadCapacityData));
            _response.bCSWStatus = 0x00;
            _state = RESPOND;
            
        } break;

        case 0x28: { // READ (10)
            struct msdCDBRead10Command *readCommand = (struct msdCDBRead10Command *)&cbw->CBWCB[0];
            uint32_t lba = (readCommand->lba24 << 24) | (readCommand->lba16 << 16) | (readCommand->lba8 << 8) | readCommand->lba0;
            uint32_t length = (readCommand->length8 << 8) | readCommand->length0;

            uint8_t *buf = (uint8_t *)alloca(_sectorSize);
            _volume->disk_read(buf, lba, 1);
            _manager->sendBuffer(_epBulk, buf, _sectorSize);
//            Serial.printf("Sending sector %d\r\n", lba);
            _nextSector = lba + 1;
            _toTransfer = length - 1;

            _state = READ;

            _response.bCSWStatus = 0x00;
        
        } break;

        case 0x1A: { // MODE SENSE
            struct msdCDBModeSenseCommand *modeSenseCommand = (struct msdCDBModeSenseCommand *)&cbw->CBWCB[0];
                if (modeSenseCommand->subpageCode == 0x00) { //Return all mode pages
                    uint8_t *buf = (uint8_t *)alloca(modeSenseCommand->allocationLength);
                    memset(buf, 0, modeSenseCommand->allocationLength);
                    _manager->sendBuffer(_epBulk, buf, modeSenseCommand->allocationLength);
                }
            _response.bCSWStatus = 0x00;
            _state = RESPOND;

            
        } break;

        case 0x2A: { // WRITE 10
            struct msdCDBWrite10Command *writeCommand = (struct msdCDBWrite10Command *)&cbw->CBWCB[0];
            uint32_t lba = (writeCommand->lba24 << 24) | (writeCommand->lba16 << 16) | (writeCommand->lba8 << 8) | writeCommand->lba0;
            uint32_t length = (writeCommand->length8 << 8) | writeCommand->length0;

            _toTransfer = length;
            _nextSector = lba;
            _fragmentOffset = 0;
            _state = WRITE;
            _response.bCSWStatus = 0x00;
            
        } break;

        case 0x5A: { // MODE SENSE 10
            struct msdCDBModeSense10Command *modeSense10Command = (struct msdCDBModeSense10Command *)&cbw->CBWCB[0];

            uint32_t size = (modeSense10Command->allocationLength8 << 8) | modeSense10Command->allocationLength0;
            uint8_t *buf = (uint8_t *)alloca(size);
            memset(buf, 0, size);
            _manager->sendBuffer(_epBulk, buf, size);
            _response.bCSWStatus = 0x00;
            _state = RESPOND;

        } break;


        default:
//            if (cbw->bmCBWFlags & 0x80) {
//                _manager->haltEndpoint(_epBulk);
//            }
            break;

    }

    // Do stuff here


    return true;
}

bool USB_MSD::onInPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l) {
    if (ep == _epBulk) {
        if (_manager->isIdle(_epBulk)) {
            switch (_state) {
                case RESPOND: {
//                    Serial.printf("Sending response tag %d code %d\r\n", _response.dCSWTag, _response.bCSWStatus);
                    _manager->enqueuePacket(_epBulk, (uint8_t *)&_response, sizeof(_response));
                    _response.dCSWSignature = 0;
                    _state = IDLE;
                } break;
                case READ: {
                    if (_toTransfer > 0) {
                        uint8_t *buf = (uint8_t *)alloca(_sectorSize);
                        _volume->disk_read(buf, _nextSector, 1);
//                        Serial.printf("Sending sector %d\r\n", _nextSector);
                        _nextSector--;
                        _toTransfer--;
                        _manager->sendBuffer(_epBulk, buf, _sectorSize);
                    } else {
//        Serial.printf("Sending response tag %d code %d\r\n", _response.dCSWTag, _response.bCSWStatus);
                        _manager->enqueuePacket(_epBulk, (uint8_t *)&_response, sizeof(_response));
                        _response.dCSWSignature = 0;
                        _state = IDLE;
                    }
                } break;
            }
        }
        return true;
    }


//        Serial.printf("Bulk in length %d\r\n", l);
//        return true;
    return false;
}

void USB_MSD::onEnumerated() {
}

