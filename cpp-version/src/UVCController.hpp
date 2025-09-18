//
// UVCController.hpp
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

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <IOKit/IOCFPlugIn.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOMessage.h>
#include <IOKit/usb/IOUSBLib.h>

#include "UVCValue.hpp"

// Forward declaration
class UVCControl;

using uvc_capabilities_t = uint32_t;

// UVC Control Capability Flags
enum {
  // Bits 0-7 from UVC standard
  kUVCControlSupportsGet = 1 << 0,
  kUVCControlSupportsSet = 1 << 1,
  kUVCControlDisabledDueToAutomaticMode = 1 << 2,
  kUVCControlAutoUpdateControl = 1 << 3,
  kUVCControlAsynchronousControl = 1 << 4,
  // Bits 8+ are custom
  kUVCControlHasRange = 1 << 8,
  kUVCControlHasStepSize = 1 << 9,
  kUVCControlHasDefaultValue = 1 << 10
};

/*!
  @class UVCController
  @abstract USB Video Class (UVC) device control wrapper.

  An instance of this class is used to interact with the software controls on a
  USB video capture device.

  The class performs extensive checking of the USB device when an instance is
  instantiated.  The vendor- and product-id; USB location id; interface index;
  version of the UVC specification implemented; and the control enablement bit
  vectors are all explored and retained when available.
*/
class UVCDeviceController
    : public std::enable_shared_from_this<UVCDeviceController> {
  friend class UVCControl;

 private:
  std::string _deviceName;
  uint32_t _locationId;
  uint16_t _vendorId, _productId;
  std::string _serialNumber;

  // All necessary functionality comes from USB standard 2.2.0:
  IOUSBInterfaceInterface220** _controllerInterface;

  bool _isInterfaceOpen;
  bool _shouldNotCloseInterface;
  uint8_t _videoInterfaceIndex;
  std::map<std::string, std::shared_ptr<UVCControl>> _controls;
  std::map<std::string, int> _unitIds;
  uint16_t _uvcVersion;
  std::vector<uint8_t> _terminalControlsAvailable;
  std::vector<uint8_t> _processingUnitControlsAvailable;

 public:
  /*!
    @method getUVCControllers

    Scan the USB bus and locate all video devices that appear to be
    UVC-compliant. Returns a vector containing all such devices, or empty vector
    if no devices were present.
  */
  static std::vector<std::shared_ptr<UVCDeviceController>> getUVCControllers();

  /*!
    @method createWithService

    Returns a shared_ptr to an instance which wraps the given device from
    the I/O Registry.  The caller retains ownership of the reference ioService
    and is responsible for releasing it.

    If the device referenced by ioService is not UVC-compliant, nullptr is
    returned.
  */
  static std::shared_ptr<UVCDeviceController> createWithService(
      io_service_t ioService);

  /*!
    @method createWithLocationId

    Attempts to locate a USB device with the given locationID property.  If the
    device is found (and appears to be UVC-compliant) a shared_ptr to an
    instance is returned.  Otherwise, nullptr is returned.

    Note that the locationID should uniquely identify a single device.
  */
  static std::shared_ptr<UVCDeviceController> createWithLocationId(
      uint32_t locationId);

  /*!
    @method createWithVendorIdProductId

    Attempts to locate a USB device with the given vendor and product identifier
    properties.  If a device is found (and appears to be UVC-compliant) a
    shared_ptr to an instance is returned.  Otherwise, nullptr is returned.

    Note that this merely chooses the first USB device found in the I/O Registry
    with the given vendor and product identifier.  If there are multiple
    devices, the getUVCControllers method should be used to retrieve a vector of
    all UVC-compliant devices.
  */
  static std::shared_ptr<UVCDeviceController> createWithVendorIdProductId(
      uint16_t vendorId,
      uint16_t productId);

  /*!
    @method getControlStrings

    Returns the vector of all control names to which this class responds.
  */
  static std::vector<std::string> getAllControlStrings();

  // Constructor and destructor
  UVCDeviceController(uint32_t locationId,
                      uint16_t vendorId,
                      uint16_t productId,
                      io_service_t ioServiceObject);
  ~UVCDeviceController();

  // Delete copy constructor and assignment operator
  UVCDeviceController(const UVCDeviceController&) = delete;
  UVCDeviceController& operator=(const UVCDeviceController&) = delete;

  /*!
    @method deviceName

    Returns the name of the USB device.
  */
  std::string deviceName() const;

  /*!
    @method serialNumber

    Returns the serial number of the USB device.
  */
  std::string serialNumber() const;

  /*!
    @method locationId

    Returns the 32-bit USB locationId of the device on this system.
  */
  uint32_t locationId() const;

  /*!
    @method vendorId

    Returns the 16-bit USB vendor identifier for the device.
  */
  uint16_t vendorId() const;

  /*!
    @method productId

    Returns the 16-bit USB product identifier for the device.
  */
  uint16_t productId() const;

  /*!
    @method uvcVersion

    Returns the version of the UVC specification which the device
    implements (as a binary-coded decimal value, e.g. 0x0210 = 2.10).
  */
  uint16_t uvcVersion() const;

  /*!
    @method isInterfaceOpen

    Returns true if the device interface is open.  The interface must be
    open in order to send/receive control requests.
  */
  bool isInterfaceOpen() const;

  /*!
    @method setIsInterfaceOpen

    Force the device interface into an open- or closed-state.
  */
  void setIsInterfaceOpen(bool isInterfaceOpen);

  /*!
    @method controlStrings

    Returns the vector of all control names to which this class responds.
  */
  std::vector<std::string> controlStrings() const;

  /*!
    @method controlWithName

    Attempt to retrieve a UVCControl wrapper for the given controlName.  If
    this instance has previously instantiated the control, the cached copy is
    returned.  If not, the capability data pulled from the device is
    consulted (if it exists) to determine whether or not the control is
    available.  If it is (or the device returned no such capability
    information) a new UVCControl object is instantiated.  If successfully
    instantiated, the new control is cached and returned to the caller.

    Returns nullptr if the control is not available or cannot be instantiated.
  */
  std::shared_ptr<UVCControl> controlWithName(const std::string& controlName);

  /*!
    @method description

    Returns a human-readable description of this controller.
  */
  std::string description() const;

 private:
  // Private helper methods
  bool findControllerInterfaceForServiceObject(io_service_t ioServiceObject);
  void parseUVCDescriptors();
  bool sendControlRequest(IOUSBDevRequest controlRequest);
  bool setData(void* value, int length, int selector, int unitId);
  bool getData(void* value, int type, int length, int selector, int unitId);
  bool capabilities(uvc_capabilities_t* capabilities, size_t controlId);
  void getLowValue(std::shared_ptr<UVCValue>* lowValue,
                   std::shared_ptr<UVCValue>* highValue,
                   std::shared_ptr<UVCValue>* stepSize,
                   std::shared_ptr<UVCValue>* defaultValue,
                   uvc_capabilities_t* capabilities,
                   size_t controlId);
  bool getValue(std::shared_ptr<UVCValue> value, size_t controlId);
  bool setValue(std::shared_ptr<UVCValue> value, size_t controlId);

  // Static helper methods
  static std::map<std::string, int> getControlMapping();
  static std::map<std::string, int> getTerminalControlEnableMapping();
  static std::map<std::string, int> getProcessingUnitControlEnableMapping();
  static size_t controlIndexForString(const std::string& controlString);

  bool controlIsNotAvailable(const std::string& controlString) const;
};

/*!
  @class UVCControl
  @abstract Wrapper for individual UVC controls.

  Each instance of UVCController manages a collection of UVC controls that the
  device has available.  Each control is represented by an instance of the
  UVCControl class, which abstracts the control meta-data and interaction with
  the control.
*/
class UVCControl {
 private:
  std::weak_ptr<UVCDeviceController> _parentController;
  size_t _controlIndex;
  std::string _controlName;
  uvc_capabilities_t _capabilities;
  std::shared_ptr<UVCValue> _currentValue;
  std::shared_ptr<UVCValue> _minimum, _maximum, _stepSize;
  std::shared_ptr<UVCValue> _defaultValue;

 public:
  // Constructor
  UVCControl(const std::string& controlName,
             std::weak_ptr<UVCDeviceController> parentController,
             size_t controlIndex);

  // Destructor
  ~UVCControl() = default;

  // Delete copy constructor and assignment operator
  UVCControl(const UVCControl&) = delete;
  UVCControl& operator=(const UVCControl&) = delete;

  /*!
    @method supportsGetValue

    Returns true if the value of this control can be read.
  */
  bool supportsGetValue() const;

  /*!
    @method supportsSetValue

    Returns true if the value of this control can be modified.
  */
  bool supportsSetValue() const;

  /*!
    @method hasRange

    Returns true if this control has minimum and maximum values.
  */
  bool hasRange() const;

  /*!
    @method hasStepSize

    Returns true if this control has a step size value.
  */
  bool hasStepSize() const;

  /*!
    @method hasDefaultValue

    Returns true if this control has a default value.
  */
  bool hasDefaultValue() const;

  /*!
    @method controlName

    Returns the textual name of the control.  This is the same string used
    to reference the control in the controlWithName method of UVCController.
  */
  std::string controlName() const;

  /*!
    @method currentValue

    Attempts to read the current value of the control from the device.  If
    successful, the returned reference to the UVCValue object
    contains the current value.

    Returns nullptr if the control could not be read.
  */
  std::shared_ptr<UVCValue> currentValue();

  /*!
    @method minimum

    Returns the minimum value(s) provided by the device for this control
    or nullptr if the device provided no minimum.
  */
  std::shared_ptr<UVCValue> minimum() const;

  /*!
    @method maximum

    Returns the maximum value(s) provided by the device for this control
    or nullptr if the device provided no maximum.
  */
  std::shared_ptr<UVCValue> maximum() const;

  /*!
    @method stepSize

    Returns the step size (resolution) value(s) provided by the device for this
    control or nullptr if the device provided no step size.
  */
  std::shared_ptr<UVCValue> stepSize() const;

  /*!
    @method defaultValue

    Returns the default value(s) provided by the device for this control
    or nullptr if the device provided no defaults.
  */
  std::shared_ptr<UVCValue> defaultValue() const;

  /*!
    @method resetToDefaultValue

    If this control has a default value (provided by the device) attempt
    to set the control to the defaults.

    Returns true if a default value was present and was successfully written to
    the device.
  */
  bool resetToDefaultValue();

  /*!
    @method setCurrentValueFromCString

    Attempts to parse cString using the native UVCType, filling-in
    the currentValue UVCValue object with the parsed values.  See the UVCType
    documentation for a description of the acceptable formats, etc.

    Returns true if currentValue was successfully set.
  */
  bool setCurrentValueFromCString(const char* cString, UVCTypeScanFlags flags);

  /*!
    @method readIntoCurrentValue

    Attempts to read the control's value from the device, storing the
    value in the UVCValue object.  The UVCValue object can be accessed
    using the currentValue method.

    Returns true if successful.
  */
  bool readIntoCurrentValue();

  /*!
    @method writeFromCurrentValue

    Attempts to write the value stored in the UVCValue object to the
    control on the device.  The UVCValue object can be accessed using the
    currentValue method; its data buffer can be modified by external software
    agents prior to calling this method.

    Returns true if successful.
  */
  bool writeFromCurrentValue();

  /*!
    @method summaryString

    Returns a string that summarizes the structure and attributes of
    this control; should be adequately human-readable.
  */
  std::string summaryString() const;

  /*!
    @method description

    Returns a human-readable description of this control.
  */
  std::string description() const;
};

//
// Control names, Terminal
//
extern const std::string UVCTerminalControlScanningMode;
extern const std::string UVCTerminalControlAutoExposureMode;
extern const std::string UVCTerminalControlAutoExposurePriority;
extern const std::string UVCTerminalControlExposureTimeAbsolute;
extern const std::string UVCTerminalControlExposureTimeRelative;
extern const std::string UVCTerminalControlFocusAbsolute;
extern const std::string UVCTerminalControlFocusRelative;
extern const std::string UVCTerminalControlAutoFocus;
extern const std::string UVCTerminalControlIrisAbsolute;
extern const std::string UVCTerminalControlIrisRelative;
extern const std::string UVCTerminalControlZoomAbsolute;
extern const std::string UVCTerminalControlZoomRelative;
extern const std::string UVCTerminalControlPanTiltAbsolute;
extern const std::string UVCTerminalControlPanTiltRelative;
extern const std::string UVCTerminalControlRollAbsolute;
extern const std::string UVCTerminalControlRollRelative;
extern const std::string UVCTerminalControlPrivacy;
extern const std::string UVCTerminalControlFocusSimple;
extern const std::string UVCTerminalControlWindow;
extern const std::string UVCTerminalControlRegionOfInterest;

//
// Control names, Processing Unit
//
extern const std::string UVCProcessingUnitControlBacklightCompensation;
extern const std::string UVCProcessingUnitControlBrightness;
extern const std::string UVCProcessingUnitControlContrast;
extern const std::string UVCProcessingUnitControlGain;
extern const std::string UVCProcessingUnitControlPowerLineFrequency;
extern const std::string UVCProcessingUnitControlHue;
extern const std::string UVCProcessingUnitControlSaturation;
extern const std::string UVCProcessingUnitControlSharpness;
extern const std::string UVCProcessingUnitControlGamma;
extern const std::string UVCProcessingUnitControlWhiteBalanceTemperature;
extern const std::string UVCProcessingUnitControlAutoWhiteBalanceTemperature;
extern const std::string UVCProcessingUnitControlWhiteBalanceComponent;
extern const std::string UVCProcessingUnitControlAutoWhiteBalanceComponent;
extern const std::string UVCProcessingUnitControlDigitalMultiplier;
extern const std::string UVCProcessingUnitControlDigitalMultiplierLimit;
extern const std::string UVCProcessingUnitControlAutoHue;
extern const std::string UVCProcessingUnitControlAnalogVideoStandard;
extern const std::string UVCProcessingUnitControlAnalogLockStatus;
extern const std::string UVCProcessingUnitControlAutoContrast;
