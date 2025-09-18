//
// UVCController.cpp
//
// USB Video Class (UVC) interface to UVC-compatible video devices.
//
// Translated from Objective-C to C++
// Copyright © 2016
// Dr. Jeffrey Frey, IT-NSS
// University of Delaware
//
// $Id$
//

#include "UVCController.hpp"

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/usb/USB.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <map>
#include <sstream>

// UVC Class and Subclass definitions
#define UVC_INTERFACE_CLASS 14
#define UVC_INTERFACE_SUBCLASS_CONTROL 1
#define UVC_INTERFACE_SUBCLASS_STREAMING 2

// Note: kIOReturnExclusiveAccess is already defined in IOKit headers

// UVC Control Selectors
#define UVC_VC_CONTROL_UNDEFINED 0x00
#define UVC_VC_VIDEO_POWER_MODE_CONTROL 0x01
#define UVC_VC_REQUEST_ERROR_CODE_CONTROL 0x02

// Processing Unit Control Selectors
#define UVC_PU_CONTROL_UNDEFINED 0x00
#define UVC_PU_BACKLIGHT_COMPENSATION_CONTROL 0x01
#define UVC_PU_BRIGHTNESS_CONTROL 0x02
#define UVC_PU_CONTRAST_CONTROL 0x03
#define UVC_PU_GAIN_CONTROL 0x04
#define UVC_PU_POWER_LINE_FREQUENCY_CONTROL 0x05
#define UVC_PU_HUE_CONTROL 0x06
#define UVC_PU_SATURATION_CONTROL 0x07
#define UVC_PU_SHARPNESS_CONTROL 0x08
#define UVC_PU_GAMMA_CONTROL 0x09
#define UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL 0x0A
#define UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL 0x0B
#define UVC_PU_WHITE_BALANCE_COMPONENT_CONTROL 0x0C
#define UVC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL 0x0D
#define UVC_PU_DIGITAL_MULTIPLIER_CONTROL 0x0E
#define UVC_PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL 0x0F
#define UVC_PU_HUE_AUTO_CONTROL 0x10
#define UVC_PU_ANALOG_VIDEO_STANDARD_CONTROL 0x11
#define UVC_PU_ANALOG_LOCK_STATUS_CONTROL 0x12

// Camera Terminal Control Selectors
#define UVC_CT_CONTROL_UNDEFINED 0x00
#define UVC_CT_SCANNING_MODE_CONTROL 0x01
#define UVC_CT_AE_MODE_CONTROL 0x02
#define UVC_CT_AE_PRIORITY_CONTROL 0x03
#define UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL 0x04
#define UVC_CT_EXPOSURE_TIME_RELATIVE_CONTROL 0x05
#define UVC_CT_FOCUS_ABSOLUTE_CONTROL 0x06
#define UVC_CT_FOCUS_RELATIVE_CONTROL 0x07
#define UVC_CT_FOCUS_AUTO_CONTROL 0x08
#define UVC_CT_IRIS_ABSOLUTE_CONTROL 0x09
#define UVC_CT_IRIS_RELATIVE_CONTROL 0x0A
#define UVC_CT_ZOOM_ABSOLUTE_CONTROL 0x0B
#define UVC_CT_ZOOM_RELATIVE_CONTROL 0x0C
#define UVC_CT_PANTILT_ABSOLUTE_CONTROL 0x0D
#define UVC_CT_PANTILT_RELATIVE_CONTROL 0x0E
#define UVC_CT_ROLL_ABSOLUTE_CONTROL 0x0F
#define UVC_CT_ROLL_RELATIVE_CONTROL 0x10
#define UVC_CT_PRIVACY_CONTROL 0x11

// UVC Request Types
#define UVC_SET_CUR 0x01
#define UVC_GET_CUR 0x81
#define UVC_GET_MIN 0x82
#define UVC_GET_MAX 0x83
#define UVC_GET_RES 0x84
#define UVC_GET_LEN 0x85
#define UVC_GET_INFO 0x86
#define UVC_GET_DEF 0x87

// UVC Unit Types
#define UVC_VC_INPUT_TERMINAL 0x02
#define UVC_VC_OUTPUT_TERMINAL 0x03
#define UVC_VC_SELECTOR_UNIT 0x04
#define UVC_VC_PROCESSING_UNIT 0x05
#define UVC_VC_EXTENSION_UNIT 0x06

// UVC descriptor constants (from original Objective-C code)
#define CS_INTERFACE 0x24
#define VC_HEADER 0x01
#define VC_INPUT_TERMINAL 0x02
#define VC_PROCESSING_UNIT 0x05

// UVC descriptor header structure
struct UVC_Descriptor_Header {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bDescriptorSubType;
} __attribute__((packed));

// Control structure for tracking UVC controls
struct UVCControlDef {
  std::string name;
  std::string typeSignature;
  int controlSelector;
  int unitType;  // 0 = processing unit, 1 = camera terminal

  UVCControlDef(const std::string& n, const std::string& ts, int cs, int ut)
      : name(n), typeSignature(ts), controlSelector(cs), unitType(ut) {}
};

// Global control definitions
static std::vector<UVCControlDef> uvcControlDefinitions = {
    // Processing Unit Controls
    UVCControlDef("brightness", "{S2}", UVC_PU_BRIGHTNESS_CONTROL, 0),
    UVCControlDef("contrast", "{U2}", UVC_PU_CONTRAST_CONTROL, 0),
    UVCControlDef("hue", "{S2}", UVC_PU_HUE_CONTROL, 0),
    UVCControlDef("saturation", "{U2}", UVC_PU_SATURATION_CONTROL, 0),
    UVCControlDef("sharpness", "{U2}", UVC_PU_SHARPNESS_CONTROL, 0),
    UVCControlDef("gamma", "{U2}", UVC_PU_GAMMA_CONTROL, 0),
    UVCControlDef("backlight-compensation",
                  "{U2}",
                  UVC_PU_BACKLIGHT_COMPENSATION_CONTROL,
                  0),
    UVCControlDef("gain", "{U2}", UVC_PU_GAIN_CONTROL, 0),
    UVCControlDef("power-line-frequency",
                  "{U1}",
                  UVC_PU_POWER_LINE_FREQUENCY_CONTROL,
                  0),
    UVCControlDef("white-balance-temp",
                  "{U2}",
                  UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL,
                  0),
    UVCControlDef("auto-white-balance-temp",
                  "{B}",
                  UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL,
                  0),

    // Camera Terminal Controls
    UVCControlDef("auto-exposure-mode", "{U1}", UVC_CT_AE_MODE_CONTROL, 1),
    UVCControlDef("auto-exposure-priority",
                  "{B}",
                  UVC_CT_AE_PRIORITY_CONTROL,
                  1),
    UVCControlDef("exposure-time-abs",
                  "{U4}",
                  UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL,
                  1),
    UVCControlDef("focus-abs", "{U2}", UVC_CT_FOCUS_ABSOLUTE_CONTROL, 1),
    UVCControlDef("focus-rel", "{S1}", UVC_CT_FOCUS_RELATIVE_CONTROL, 1),
    UVCControlDef("auto-focus", "{B}", UVC_CT_FOCUS_AUTO_CONTROL, 1),
    UVCControlDef("iris-abs", "{U2}", UVC_CT_IRIS_ABSOLUTE_CONTROL, 1),
    UVCControlDef("zoom-abs", "{U2}", UVC_CT_ZOOM_ABSOLUTE_CONTROL, 1),
    UVCControlDef("zoom-rel",
                  "{S1 zoom;U1 digital-zoom;U1 speed}",
                  UVC_CT_ZOOM_RELATIVE_CONTROL,
                  1),
    UVCControlDef("pan-tilt-abs",
                  "{S4 pan; S4 tilt}",
                  UVC_CT_PANTILT_ABSOLUTE_CONTROL,
                  1),
    UVCControlDef("pan-tilt-rel",
                  "{S1 pan;U1 pan-speed; S1 tilt;U1 tilt-speed}",
                  UVC_CT_PANTILT_RELATIVE_CONTROL,
                  1),
    UVCControlDef("privacy", "{B}", UVC_CT_PRIVACY_CONTROL, 1),
};

// Static helper functions
CFStringRef CreateCFStringFromIORegistryKey(io_service_t ioService,
                                            const char* key) {
  CFStringRef keyString = CFStringCreateWithCString(kCFAllocatorDefault, key,
                                                    kCFStringEncodingUTF8);
  if (!keyString)
    return nullptr;

  CFStringRef result = static_cast<CFStringRef>(
      IORegistryEntryCreateCFProperty(ioService, keyString, kCFAllocatorDefault,
                                      kIORegistryIterateRecursively));
  CFRelease(keyString);
  return result;
}

std::string GetStringFromIORegistry(io_service_t ioService, const char* key) {
  CFStringRef cfString = CreateCFStringFromIORegistryKey(ioService, key);
  if (!cfString)
    return "";

  char buffer[256];
  Boolean success = CFStringGetCString(cfString, buffer, sizeof(buffer),
                                       kCFStringEncodingUTF8);
  CFRelease(cfString);

  return success ? std::string(buffer) : std::string("");
}

uint32_t GetUInt32FromIORegistry(io_service_t ioService, const char* key) {
  CFStringRef keyString = CFStringCreateWithCString(kCFAllocatorDefault, key,
                                                    kCFStringEncodingUTF8);
  if (!keyString)
    return 0;

  CFNumberRef number = static_cast<CFNumberRef>(
      IORegistryEntryCreateCFProperty(ioService, keyString, kCFAllocatorDefault,
                                      kIORegistryIterateRecursively));
  CFRelease(keyString);

  if (!number)
    return 0;

  uint32_t value = 0;
  CFNumberGetValue(number, kCFNumberSInt32Type, &value);
  CFRelease(number);
  return value;
}

// UVCController implementation
std::vector<std::shared_ptr<UVCDeviceController>>
UVCDeviceController::getUVCControllers() {
  std::vector<std::shared_ptr<UVCDeviceController>> controllers;

  // Get matching dictionary for USB devices
  CFMutableDictionaryRef matchingDict =
      IOServiceMatching(kIOUSBDeviceClassName);
  if (!matchingDict) {
    std::cerr << "ERROR: Could not create USB matching dictionary" << std::endl;
    return controllers;
  }

  // Get iterator for matching services
  io_iterator_t serviceIterator;
  kern_return_t kr = IOServiceGetMatchingServices(
      kIOMasterPortDefault, matchingDict, &serviceIterator);
  if (kr != KERN_SUCCESS) {
    std::cerr << "ERROR: IOServiceGetMatchingServices failed: " << kr
              << std::endl;
    return controllers;
  }

  // Iterate through matching devices
  io_service_t usbService;
  while ((usbService = IOIteratorNext(serviceIterator))) {
    uint32_t locationId = GetUInt32FromIORegistry(usbService, "locationID");
    uint32_t vendorId = GetUInt32FromIORegistry(usbService, "idVendor");
    uint32_t productId = GetUInt32FromIORegistry(usbService, "idProduct");
    std::cout << "locationId: " << locationId << std::endl;
    std::cout << "vendorId: " << vendorId << std::endl;
    std::cout << "productId: " << productId << std::endl;
    // Check if this is a UVC device
    auto controller = createWithService(usbService);
    if (controller) {
      controllers.push_back(controller);
    }
    IOObjectRelease(usbService);
  }

  IOObjectRelease(serviceIterator);
  return controllers;
}

std::shared_ptr<UVCDeviceController> UVCDeviceController::createWithService(
    io_service_t ioService) {
  // Get device properties
  uint32_t locationId = GetUInt32FromIORegistry(ioService, "locationID");
  uint32_t vendorId = GetUInt32FromIORegistry(ioService, "idVendor");
  uint32_t productId = GetUInt32FromIORegistry(ioService, "idProduct");

  if (!locationId || !vendorId || !productId) {
    return nullptr;
  }

  // Check for UVC interface using the same method as original Objective-C code
  bool hasUVCInterface = false;

  IOCFPlugInInterface** plugInInterface = nullptr;
  IOUSBDeviceInterface** deviceInterface = nullptr;
  SInt32 score;

  // Get device plugin interface
  IOReturn result = IOCreatePlugInInterfaceForService(
      ioService, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID,
      &plugInInterface, &score);

  if (result == kIOReturnSuccess && plugInInterface) {
    // Get device interface
    HRESULT res =
        (*plugInInterface)
            ->QueryInterface(plugInInterface,
                             CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID),
                             (LPVOID*)&deviceInterface);

    (*plugInInterface)->Release(plugInInterface);

    if (!res && deviceInterface) {
      // Try to find UVC control interface using CreateInterfaceIterator
      io_iterator_t interfaceIterator;
      IOUSBFindInterfaceRequest interfaceRequest;
      interfaceRequest.bInterfaceClass = UVC_INTERFACE_CLASS;  // 14
      interfaceRequest.bInterfaceSubClass =
          UVC_INTERFACE_SUBCLASS_CONTROL;  // 1
      interfaceRequest.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
      interfaceRequest.bAlternateSetting = kIOUSBFindInterfaceDontCare;

      result = (*deviceInterface)
                   ->CreateInterfaceIterator(deviceInterface, &interfaceRequest,
                                             &interfaceIterator);
      (*deviceInterface)->Release(deviceInterface);

      if (result == kIOReturnSuccess && interfaceIterator) {
        io_service_t usbInterface = IOIteratorNext(interfaceIterator);
        IOObjectRelease(interfaceIterator);

        if (usbInterface) {
          hasUVCInterface = true;
          IOObjectRelease(usbInterface);
        }
      }
    } else if (deviceInterface) {
      (*deviceInterface)->Release(deviceInterface);
    }
  }

  if (!hasUVCInterface) {
    return nullptr;
  }

  // Create the controller instance
  return std::make_shared<UVCDeviceController>(
      locationId, static_cast<uint16_t>(vendorId),
      static_cast<uint16_t>(productId), ioService);
}

std::shared_ptr<UVCDeviceController> UVCDeviceController::createWithLocationId(
    uint32_t locationId) {
  auto controllers = getUVCControllers();
  for (const auto& controller : controllers) {
    if (controller->locationId() == locationId) {
      return controller;
    }
  }
  return nullptr;
}

std::shared_ptr<UVCDeviceController>
UVCDeviceController::createWithVendorIdProductId(uint16_t vendorId,
                                                 uint16_t productId) {
  auto controllers = getUVCControllers();
  for (const auto& controller : controllers) {
    if (controller->vendorId() == vendorId &&
        controller->productId() == productId) {
      return controller;
    }
  }
  return nullptr;
}

std::vector<std::string> UVCDeviceController::getAllControlStrings() {
  std::vector<std::string> names;
  for (const auto& def : uvcControlDefinitions) {
    names.push_back(def.name);
  }
  return names;
}

UVCDeviceController::UVCDeviceController(uint32_t locationId,
                                         uint16_t vendorId,
                                         uint16_t productId,
                                         io_service_t ioServiceObject)
    : _locationId(locationId),
      _vendorId(vendorId),
      _productId(productId),
      _controllerInterface(nullptr),
      _isInterfaceOpen(false),
      _shouldNotCloseInterface(false),
      _videoInterfaceIndex(0),
      _uvcVersion(0x0100) {  // Default to 1.00, will be updated from descriptor
  // Get device name
  _deviceName = GetStringFromIORegistry(ioServiceObject, "USB Product Name");
  if (_deviceName.empty()) {
    _deviceName = "Unknown UVC Device";
  }
  _serialNumber = GetStringFromIORegistry(ioServiceObject, "USB Serial Number");
  if (_serialNumber.empty()) {
    _serialNumber = "Unknown UVC Device";
  }

  // Initialize interface
  findControllerInterfaceForServiceObject(ioServiceObject);
}

UVCDeviceController::~UVCDeviceController() {
  if (_controllerInterface && _isInterfaceOpen && !_shouldNotCloseInterface) {
    (*_controllerInterface)->USBInterfaceClose(_controllerInterface);
  }

  if (_controllerInterface) {
    (*_controllerInterface)->Release(_controllerInterface);
  }
}

std::string UVCDeviceController::deviceName() const {
  return _deviceName;
}

std::string UVCDeviceController::serialNumber() const {
  return _serialNumber;
}

uint32_t UVCDeviceController::locationId() const {
  return _locationId;
}

uint16_t UVCDeviceController::vendorId() const {
  return _vendorId;
}

uint16_t UVCDeviceController::productId() const {
  return _productId;
}

uint16_t UVCDeviceController::uvcVersion() const {
  return _uvcVersion;
}

bool UVCDeviceController::isInterfaceOpen() const {
  return _isInterfaceOpen;
}

void UVCDeviceController::setIsInterfaceOpen(bool isInterfaceOpen) {
  if (isInterfaceOpen && !_isInterfaceOpen) {
    if (_controllerInterface) {
      IOReturn result =
          (*_controllerInterface)->USBInterfaceOpen(_controllerInterface);
      _isInterfaceOpen = (result == kIOReturnSuccess);
    }
  } else if (!isInterfaceOpen && _isInterfaceOpen) {
    if (_controllerInterface && !_shouldNotCloseInterface) {
      (*_controllerInterface)->USBInterfaceClose(_controllerInterface);
      _isInterfaceOpen = false;
    }
  }
}

std::shared_ptr<UVCControl> UVCDeviceController::controlWithName(
    const std::string& controlName) {
  // Check if we already have this control cached
  auto it = _controls.find(controlName);
  if (it != _controls.end()) {
    return it->second;
  }

  // Check if control is marked as not available
  if (controlIsNotAvailable(controlName)) {
    _controls[controlName] = nullptr;  // Cache the failure
    return nullptr;
  }

  // Find the control definition
  size_t controlIndex = 0;
  bool found = false;
  for (size_t i = 0; i < uvcControlDefinitions.size(); i++) {
    if (uvcControlDefinitions[i].name == controlName) {
      controlIndex = i;
      found = true;
      break;
    }
  }

  if (!found) {
    _controls[controlName] = nullptr;  // Cache the failure
    return nullptr;
  }

  // Check capabilities first (like original Objective-C logic)
  uvc_capabilities_t caps = 0;
  if (!capabilities(&caps, controlIndex)) {
    // If capabilities check fails, this control is not available
    _controls[controlName] = nullptr;  // Cache the failure
    return nullptr;
  }

  // Create the control (only if capabilities check passed)
  auto control = std::make_shared<UVCControl>(controlName, shared_from_this(),
                                              controlIndex);

  // Cache it
  _controls[controlName] = control;
  return control;
}

std::vector<std::string> UVCDeviceController::controlStrings() const {
  // Like the original, this should return ALL defined control names
  // The filtering happens in main.cpp when calling controlWithName()
  return getAllControlStrings();
}

std::string UVCDeviceController::description() const {
  std::stringstream ss;
  ss << "UVCController: " << _deviceName << " (0x" << std::hex << _vendorId
     << ":0x" << _productId << ")"
     << " Serial Number: " << _serialNumber << " LocationID: 0x" << _locationId
     << " UVC Version: " << (_uvcVersion >> 8) << "." << std::hex
     << (_uvcVersion & 0xFF);
  return ss.str();
}

// Private helper methods
bool UVCDeviceController::findControllerInterfaceForServiceObject(
    io_service_t ioServiceObject) {
  IOCFPlugInInterface** plugInInterface = nullptr;
  IOUSBDeviceInterface** deviceInterface = nullptr;
  IOUSBInterfaceInterface** interfaceInterface = nullptr;
  SInt32 score;

  // Get device plugin interface
  IOReturn result = IOCreatePlugInInterfaceForService(
      ioServiceObject, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID,
      &plugInInterface, &score);

  if (result != kIOReturnSuccess || !plugInInterface) {
    return false;
  }

  // Get device interface
  HRESULT res =
      (*plugInInterface)
          ->QueryInterface(plugInInterface,
                           CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID),
                           (LPVOID*)&deviceInterface);

  (*plugInInterface)->Release(plugInInterface);

  if (res || !deviceInterface) {
    return false;
  }

  // Find UVC control interface
  io_iterator_t interfaceIterator;
  IOUSBFindInterfaceRequest interfaceRequest;
  interfaceRequest.bInterfaceClass = 14;    // UVC interface class
  interfaceRequest.bInterfaceSubClass = 1;  // UVC control subclass
  interfaceRequest.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
  interfaceRequest.bAlternateSetting = kIOUSBFindInterfaceDontCare;
  result = (*deviceInterface)
               ->CreateInterfaceIterator(deviceInterface, &interfaceRequest,
                                         &interfaceIterator);

  if (result != kIOReturnSuccess) {
    (*deviceInterface)->Release(deviceInterface);
    return false;
  }

  io_service_t interfaceService;
  while ((interfaceService = IOIteratorNext(interfaceIterator))) {
    // Create plugin interface for this interface
    IOCFPlugInInterface** interfacePlugIn = nullptr;
    result = IOCreatePlugInInterfaceForService(
        interfaceService, kIOUSBInterfaceUserClientTypeID,
        kIOCFPlugInInterfaceID, &interfacePlugIn, &score);

    if (result == kIOReturnSuccess && interfacePlugIn) {
      // Get interface interface
      res = (*interfacePlugIn)
                ->QueryInterface(interfacePlugIn,
                                 CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID),
                                 (LPVOID*)&interfaceInterface);

      (*interfacePlugIn)->Release(interfacePlugIn);

      if (!res && interfaceInterface) {
        // Cast to the version we need (220)
        _controllerInterface = (IOUSBInterfaceInterface220**)interfaceInterface;
        interfaceInterface = nullptr;  // We've taken ownership

        // Get interface number
        result = (*_controllerInterface)
                     ->GetInterfaceNumber(_controllerInterface,
                                          &_videoInterfaceIndex);

        // Try to open the interface
        result =
            (*_controllerInterface)->USBInterfaceOpen(_controllerInterface);

        if (result == kIOReturnSuccess) {
          _isInterfaceOpen = true;
          _shouldNotCloseInterface = true;  // We opened it

          // Parse UVC descriptors to get real unit IDs
          parseUVCDescriptors();

          IOObjectRelease(interfaceService);
          break;
        } else if (result == kIOReturnExclusiveAccess) {
          // Interface is already in use (by system driver), but we can still
          // use it
          _isInterfaceOpen = true;
          _shouldNotCloseInterface = false;  // Don't close what we didn't open

          // Parse UVC descriptors to get real unit IDs
          parseUVCDescriptors();

          IOObjectRelease(interfaceService);
          break;
        } else {
          (*_controllerInterface)->Release(_controllerInterface);
          _controllerInterface = nullptr;
        }
      }
    }

    IOObjectRelease(interfaceService);
  }

  IOObjectRelease(interfaceIterator);
  (*deviceInterface)->Release(deviceInterface);

  return _controllerInterface != nullptr && _isInterfaceOpen;
}

void UVCDeviceController::parseUVCDescriptors() {
  if (!_controllerInterface) {
    return;
  }

  // Get UVC interface descriptor
  IOUSBDescriptorHeader* ioDescriptor =
      (*_controllerInterface)
          ->FindNextAssociatedDescriptor(_controllerInterface, nullptr,
                                         CS_INTERFACE);

  if (!ioDescriptor)
    return;

  UVC_Descriptor_Header* descriptor =
      reinterpret_cast<UVC_Descriptor_Header*>(ioDescriptor);

  if (descriptor->bDescriptorSubType == VC_HEADER) {
    // Parse UVC header to get version
    struct UVC_VC_Header {
      uint8_t bLength;
      uint8_t bDescriptorType;
      uint8_t bDescriptorSubType;
      uint16_t bcdUVC;
      uint16_t wTotalLength;
      uint32_t dwClockFrequency;
      uint8_t bInCollection;
      uint8_t baInterfaceNr1;
      // ... more fields
    } __attribute__((packed));

    UVC_VC_Header* header = reinterpret_cast<UVC_VC_Header*>(descriptor);
    _uvcVersion =
        OSSwapLittleToHostInt16(header->bcdUVC);  // Convert from little endian

    // Walk through embedded Unit/Terminal descriptors
    uint8_t* basePtr = reinterpret_cast<uint8_t*>(header);
    uint8_t* endPtr = basePtr + header->wTotalLength;
    basePtr += header->bLength;

    while (basePtr < endPtr) {
      UVC_Descriptor_Header* subDesc =
          reinterpret_cast<UVC_Descriptor_Header*>(basePtr);

      if (subDesc->bDescriptorType == CS_INTERFACE) {
        if (subDesc->bDescriptorSubType == VC_PROCESSING_UNIT) {
          // Processing Unit descriptor - extract unit ID
          struct UVC_PU_Header {
            uint8_t bLength;
            uint8_t bDescriptorType;
            uint8_t bDescriptorSubType;
            uint8_t bUnitId;  // This is what we need!
            uint8_t bSourceId;
            uint16_t wMaxMultiplier;
            uint8_t bControlSize;
            // uint8_t bmControls[]; // variable length
          } __attribute__((packed));

          UVC_PU_Header* puHeader = reinterpret_cast<UVC_PU_Header*>(basePtr);
          _unitIds["UVC_PROCESSING_UNIT_ID"] = puHeader->bUnitId;

          // Store control capabilities if needed
          if (puHeader->bControlSize > 0) {
            uint8_t* controls = basePtr + sizeof(UVC_PU_Header);
            _processingUnitControlsAvailable.clear();
            _processingUnitControlsAvailable.assign(
                controls, controls + puHeader->bControlSize);
          }
        } else if (subDesc->bDescriptorSubType == VC_INPUT_TERMINAL) {
          // Input Terminal descriptor
          struct UVC_IT_Header {
            uint8_t bLength;
            uint8_t bDescriptorType;
            uint8_t bDescriptorSubType;
            uint8_t bTerminalId;  // This is what we need!
            uint16_t wTerminalType;
            uint8_t bAssocTerminal;
            uint8_t iTerminal;
            // Camera terminal has more fields...
          } __attribute__((packed));

          UVC_IT_Header* itHeader = reinterpret_cast<UVC_IT_Header*>(basePtr);
          _unitIds["UVC_INPUT_TERMINAL_ID"] = itHeader->bTerminalId;
        }
      }

      basePtr += subDesc->bLength;
    }
  }
}

bool UVCDeviceController::sendControlRequest(IOUSBDevRequest controlRequest) {
  if (!_controllerInterface) {
    return false;
  }

  // Auto-open interface if not already open (like original Objective-C code)
  if (!_isInterfaceOpen) {
    setIsInterfaceOpen(true);
    if (!_isInterfaceOpen) {
      return false;
    }
  }

  IOReturn result =
      (*_controllerInterface)
          ->ControlRequest(_controllerInterface, 0, &controlRequest);

  return (result == kIOReturnSuccess);
}

bool UVCDeviceController::setData(void* value,
                                  int length,
                                  int selector,
                                  int unitId) {
  IOUSBDevRequest request;
  memset(&request, 0, sizeof(request));

  request.bmRequestType =
      USBmakebmRequestType(kUSBOut, kUSBClass, kUSBInterface);
  request.bRequest = UVC_SET_CUR;
  request.wValue = (selector << 8);
  request.wIndex =
      (unitId << 8) |
      _videoInterfaceIndex;  // UVC protocol: (unitId << 8) | interfaceIndex
  request.wLength = length;
  request.pData = value;
  request.wLenDone = 0;

  return sendControlRequest(request);
}

bool UVCDeviceController::getData(void* value,
                                  int type,
                                  int length,
                                  int selector,
                                  int unitId) {
  IOUSBDevRequest request;
  memset(&request, 0, sizeof(request));

  request.bmRequestType =
      USBmakebmRequestType(kUSBIn, kUSBClass, kUSBInterface);
  request.bRequest = type;  // GET_CUR, GET_MIN, GET_MAX, etc.
  request.wValue = (selector << 8);
  request.wIndex =
      (unitId << 8) |
      _videoInterfaceIndex;  // UVC protocol: (unitId << 8) | interfaceIndex
  request.wLength = length;
  request.pData = value;
  request.wLenDone = 0;

  return sendControlRequest(request);
}

bool UVCDeviceController::capabilities(uvc_capabilities_t* capabilities,
                                       size_t controlId) {
  if (controlId >= uvcControlDefinitions.size()) {
    return false;
  }

  const auto& controlDef = uvcControlDefinitions[controlId];

  // Get real unit ID from parsed descriptors
  std::string unitKey = (controlDef.unitType == 0) ? "UVC_PROCESSING_UNIT_ID"
                                                   : "UVC_INPUT_TERMINAL_ID";
  auto it = _unitIds.find(unitKey);
  int unitId;

  if (it == _unitIds.end()) {
    // Fallback to default IDs if parsing failed
    unitId = (controlDef.unitType == 0) ? 0x02 : 0x01;
  } else {
    unitId = it->second;
  }

  uint8_t scratch;
  if (getData(&scratch, UVC_GET_INFO, 1, controlDef.controlSelector, unitId)) {
    *capabilities = scratch;
    return true;
  }

  // If USB request fails, this control is not available
  return false;
}

void UVCDeviceController::getLowValue(std::shared_ptr<UVCValue>* lowValue,
                                      std::shared_ptr<UVCValue>* highValue,
                                      std::shared_ptr<UVCValue>* stepSize,
                                      std::shared_ptr<UVCValue>* defaultValue,
                                      uvc_capabilities_t* capabilities,
                                      size_t controlId) {
  if (controlId >= uvcControlDefinitions.size()) {
    *lowValue = nullptr;
    *highValue = nullptr;
    *stepSize = nullptr;
    *defaultValue = nullptr;
    return;
  }

  const auto& controlDef = uvcControlDefinitions[controlId];

  // Get real unit ID from parsed descriptors
  std::string unitKey = (controlDef.unitType == 0) ? "UVC_PROCESSING_UNIT_ID"
                                                   : "UVC_INPUT_TERMINAL_ID";
  auto it = _unitIds.find(unitKey);
  int unitId;

  if (it == _unitIds.end()) {
    // Fallback to default IDs if parsing failed
    unitId = (controlDef.unitType == 0) ? 0x02 : 0x01;
  } else {
    unitId = it->second;
  }

  // Try to get minimum and maximum values
  if (*lowValue && *highValue) {
    if (getData((*lowValue)->valuePtr(), UVC_GET_MIN,
                static_cast<int>((*lowValue)->byteSize()),
                controlDef.controlSelector, unitId) &&
        getData((*highValue)->valuePtr(), UVC_GET_MAX,
                static_cast<int>((*highValue)->byteSize()),
                controlDef.controlSelector, unitId)) {
      *capabilities |= kUVCControlHasRange;
      (*lowValue)->byteSwapUSBToHostEndian();
      (*highValue)->byteSwapUSBToHostEndian();
    } else {
      *lowValue = nullptr;
      *highValue = nullptr;
    }
  }

  // Try to get step size
  if (*stepSize) {
    if (getData((*stepSize)->valuePtr(), UVC_GET_RES,
                static_cast<int>((*stepSize)->byteSize()),
                controlDef.controlSelector, unitId)) {
      *capabilities |= kUVCControlHasStepSize;
      (*stepSize)->byteSwapUSBToHostEndian();
    } else {
      *stepSize = nullptr;
    }
  }

  // Try to get default value
  if (*defaultValue) {
    if (getData((*defaultValue)->valuePtr(), UVC_GET_DEF,
                static_cast<int>((*defaultValue)->byteSize()),
                controlDef.controlSelector, unitId)) {
      *capabilities |= kUVCControlHasDefaultValue;
      (*defaultValue)->byteSwapUSBToHostEndian();
    } else {
      *defaultValue = nullptr;
    }
  }
}

bool UVCDeviceController::getValue(std::shared_ptr<UVCValue> value,
                                   size_t controlId) {
  if (controlId >= uvcControlDefinitions.size()) {
    return false;
  }

  const auto& controlDef = uvcControlDefinitions[controlId];

  // Get real unit ID from parsed descriptors
  std::string unitKey = (controlDef.unitType == 0) ? "UVC_PROCESSING_UNIT_ID"
                                                   : "UVC_INPUT_TERMINAL_ID";
  auto it = _unitIds.find(unitKey);
  int unitId;

  if (it == _unitIds.end()) {
    // Fallback to default IDs if parsing failed
    unitId = (controlDef.unitType == 0) ? 0x02 : 0x01;
  } else {
    unitId = it->second;
  }

  return getData(value->valuePtr(), UVC_GET_CUR,
                 static_cast<int>(value->byteSize()),
                 controlDef.controlSelector, unitId);
}

bool UVCDeviceController::setValue(std::shared_ptr<UVCValue> value,
                                   size_t controlId) {
  if (controlId >= uvcControlDefinitions.size()) {
    return false;
  }

  const auto& controlDef = uvcControlDefinitions[controlId];

  // Get real unit ID from parsed descriptors
  std::string unitKey = (controlDef.unitType == 0) ? "UVC_PROCESSING_UNIT_ID"
                                                   : "UVC_INPUT_TERMINAL_ID";
  auto it = _unitIds.find(unitKey);
  if (it == _unitIds.end()) {
    // Fallback to default IDs if parsing failed
    int unitId = (controlDef.unitType == 0) ? 0x02 : 0x01;
    return setData(value->valuePtr(), static_cast<int>(value->byteSize()),
                   controlDef.controlSelector, unitId);
  }

  int unitId = it->second;

  return setData(value->valuePtr(), static_cast<int>(value->byteSize()),
                 controlDef.controlSelector, unitId);
}

size_t UVCDeviceController::controlIndexForString(
    const std::string& controlString) {
  for (size_t i = 0; i < uvcControlDefinitions.size(); i++) {
    if (uvcControlDefinitions[i].name == controlString) {
      return i;
    }
  }
  return SIZE_MAX;
}

bool UVCDeviceController::controlIsNotAvailable(
    const std::string& controlString) const {
  // For now, assume all controls are available
  return false;
}

std::map<std::string, int> UVCDeviceController::getControlMapping() {
  std::map<std::string, int> mapping;
  for (size_t i = 0; i < uvcControlDefinitions.size(); i++) {
    mapping[uvcControlDefinitions[i].name] = static_cast<int>(i);
  }
  return mapping;
}

std::map<std::string, int>
UVCDeviceController::getTerminalControlEnableMapping() {
  // Placeholder implementation
  return std::map<std::string, int>();
}

std::map<std::string, int>
UVCDeviceController::getProcessingUnitControlEnableMapping() {
  // Placeholder implementation
  return std::map<std::string, int>();
}

// UVCControl implementation
UVCControl::UVCControl(const std::string& controlName,
                       std::weak_ptr<UVCDeviceController> parentController,
                       size_t controlIndex)
    : _parentController(parentController),
      _controlIndex(controlIndex),
      _controlName(controlName),
      _capabilities(0) {
  if (_controlIndex < uvcControlDefinitions.size()) {
    const auto& controlDef = uvcControlDefinitions[_controlIndex];

    // Create the UVCType for this control
    auto uvcType = UVCType::createFromCString(controlDef.typeSignature.c_str());

    if (uvcType) {
      _currentValue = UVCValue::create(uvcType);
      _minimum = UVCValue::create(uvcType);
      _maximum = UVCValue::create(uvcType);
      _stepSize = UVCValue::create(uvcType);
      _defaultValue = UVCValue::create(uvcType);
    }

    // Get capabilities and range values from parent controller
    if (auto controller = _parentController.lock()) {
      controller->capabilities(&_capabilities, _controlIndex);

      // Get range values (min, max, step, default)
      controller->getLowValue(&_minimum, &_maximum, &_stepSize, &_defaultValue,
                              &_capabilities, _controlIndex);
    }
  }
}

bool UVCControl::supportsGetValue() const {
  return (_capabilities & kUVCControlSupportsGet) != 0;
}

bool UVCControl::supportsSetValue() const {
  return (_capabilities & kUVCControlSupportsSet) != 0;
}

bool UVCControl::hasRange() const {
  return (_capabilities & kUVCControlHasRange) != 0;
}

bool UVCControl::hasStepSize() const {
  return (_capabilities & kUVCControlHasStepSize) != 0;
}

bool UVCControl::hasDefaultValue() const {
  return (_capabilities & kUVCControlHasDefaultValue) != 0;
}

std::string UVCControl::controlName() const {
  return _controlName;
}

std::shared_ptr<UVCValue> UVCControl::currentValue() {
  if (!_currentValue)
    return nullptr;

  if (auto controller = _parentController.lock()) {
    if (controller->getValue(_currentValue, _controlIndex)) {
      return _currentValue;
    }
  }
  return nullptr;
}

std::shared_ptr<UVCValue> UVCControl::minimum() const {
  return _minimum;
}

std::shared_ptr<UVCValue> UVCControl::maximum() const {
  return _maximum;
}

std::shared_ptr<UVCValue> UVCControl::stepSize() const {
  return _stepSize;
}

std::shared_ptr<UVCValue> UVCControl::defaultValue() const {
  return _defaultValue;
}

bool UVCControl::resetToDefaultValue() {
  if (!_defaultValue || !_currentValue) {
    return false;
  }

  _currentValue->copyValue(_defaultValue);
  return writeFromCurrentValue();
}

bool UVCControl::setCurrentValueFromCString(const char* cString,
                                            UVCTypeScanFlags flags) {
  if (!_currentValue) {
    return false;
  }

  return _currentValue->scanCString(cString, flags, _minimum, _maximum,
                                    _stepSize, _defaultValue);
}

bool UVCControl::readIntoCurrentValue() {
  if (!_currentValue) {
    return false;
  }

  if (auto controller = _parentController.lock()) {
    return controller->getValue(_currentValue, _controlIndex);
  }
  return false;
}

bool UVCControl::writeFromCurrentValue() {
  if (!_currentValue) {
    return false;
  }

  if (auto controller = _parentController.lock()) {
    return controller->setValue(_currentValue, _controlIndex);
  }
  return false;
}

std::string UVCControl::summaryString() const {
  std::stringstream ss;

  // Start with control name and opening brace
  ss << _controlName << " {\n";

  // Add type description
  if (_currentValue && _currentValue->valueType()) {
    ss << "  type-description: {\n";
    ss << _currentValue->valueType()->typeSummaryString();
    ss << "  },";
  }

  // Add range values if available
  if (hasRange() && _minimum && _maximum) {
    ss << "\n  minimum: " << _minimum->stringValue();
    ss << "\n  maximum: " << _maximum->stringValue();
  }

  // Add step size if available
  if (hasStepSize() && _stepSize) {
    ss << "\n  step-size: " << _stepSize->stringValue();
  }

  // Add default value if available
  if (hasDefaultValue() && _defaultValue) {
    ss << "\n  default-value: " << _defaultValue->stringValue();
  }

  // Add current value (make sure we have the latest value)
  if (_currentValue) {
    const_cast<UVCControl*>(this)->readIntoCurrentValue();
    ss << "\n  current-value: " << _currentValue->stringValue();
  }

  ss << "\n}";

  return ss.str();
}

std::string UVCControl::description() const {
  std::stringstream ss;
  ss << "UVCControl: " << _controlName << "\n";
  ss << "  Capabilities: ";
  if (supportsGetValue())
    ss << "GET ";
  if (supportsSetValue())
    ss << "SET ";
  ss << "\n";

  if (_currentValue) {
    ss << "  Current Value: " << _currentValue->stringValue() << "\n";
  }
  if (_minimum) {
    ss << "  Minimum: " << _minimum->stringValue() << "\n";
  }
  if (_maximum) {
    ss << "  Maximum: " << _maximum->stringValue() << "\n";
  }
  if (_stepSize) {
    ss << "  Step Size: " << _stepSize->stringValue() << "\n";
  }
  if (_defaultValue) {
    ss << "  Default: " << _defaultValue->stringValue() << "\n";
  }

  return ss.str();
}

// Control name constants are already defined at the end of the previous file
