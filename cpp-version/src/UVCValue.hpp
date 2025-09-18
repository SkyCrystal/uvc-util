//
// UVCValue.hpp
//
// Structured byte-packed data containers for UVC controls.
//
// Translated from Objective-C to C++
// Copyright © 2016
// Dr. Jeffrey Frey, IT-NSS
// University of Delaware
//
// $Id$
//

#pragma once

#include <memory>
#include <vector>
#include "UVCType.hpp"

/*!
  @class UVCValue
  @abstract Structured byte-packed data container

  An instance of UVCValue combines the structural meta-data from a UVCType
  instance with a memory buffer of sufficient size to hold data of that
  type.

  Many of the methods provided by UVCType are duplicated in UVCValue, but
  lack the specification of an external buffer (since UVCValue itself contains
  the buffer in question).
*/
class UVCValue {
 private:
  bool _isSwappedToUSBEndian;
  std::shared_ptr<UVCType> _valueType;
  std::vector<uint8_t> _valueData;

 public:
  /*!
    @method create

    Returns a shared_ptr to UVCValue which wraps a buffer sized
    according to valueType->byteSize() and uses valueType as its structural
    meta-data.
  */
  static std::shared_ptr<UVCValue> create(std::shared_ptr<UVCType> valueType);

  // Constructor and destructor
  explicit UVCValue(std::shared_ptr<UVCType> valueType);
  ~UVCValue() = default;

  // Copy constructor and assignment operator
  UVCValue(const UVCValue& other) = default;
  UVCValue& operator=(const UVCValue& other) = default;

  /*!
    @method valueType

    Returns the UVCType that acts as the structural meta-data for this instance.
  */
  std::shared_ptr<UVCType> valueType() const;

  /*!
    @method valuePtr

    Returns the base address of the memory buffer (where data
    structured according to the valueType should be stored).
  */
  void* valuePtr();
  const void* valuePtr() const;

  /*!
    @method byteSize

    Returns the number of bytes occupied by the valueType.
  */
  size_t byteSize() const;

  /*!
    @method pointerToFieldAtIndex

    Calculates the base pointer of the given field within the
    memory buffer.

    Returns nullptr if index is out of range.
  */
  void* pointerToFieldAtIndex(size_t index);
  const void* pointerToFieldAtIndex(size_t index) const;

  /*!
    @method pointerToFieldWithName

    Calculates the base pointer of the given field (under a case-insensitive
    string comparison against fieldName) within the memory buffer.

    Returns nullptr if fieldName is not found.
  */
  void* pointerToFieldWithName(const std::string& fieldName);
  const void* pointerToFieldWithName(const std::string& fieldName) const;

  /*!
    @method isSwappedToUSBEndian

    Returns true if the memory buffer has been byte-swapped to USB
    (little) endian.
  */
  bool isSwappedToUSBEndian() const;

  /*!
    @method byteSwapHostToUSBEndian

    If currently in host endian order, byte swap all necessary
    component fields of the memory buffer (anything larger than 1
    byte) from the host endian to USB (little) endian.
  */
  void byteSwapHostToUSBEndian();

  /*!
    @method byteSwapUSBToHostEndian

    If currently byte-swapped to USB (little) endian, byte
    swap all necessary component fields of the memory buffer
    (anything larger than 1 byte) from USB (little) endian to host endian.
  */
  void byteSwapUSBToHostEndian();

  /*!
    @method scanCString

    Convenience method that calls the full scanCString method with nullptr
    for minimum, maximum, stepSize, and defaultValue.
  */
  bool scanCString(const char* cString, UVCTypeScanFlags flags);

  /*!
    @method scanCString (with range parameters)

    Convenience method that calls the full scanCString method with nullptr
    for stepSize and defaultValue.
  */
  bool scanCString(const char* cString,
                   UVCTypeScanFlags flags,
                   std::shared_ptr<UVCValue> minimum,
                   std::shared_ptr<UVCValue> maximum);

  /*!
    @method scanCString (with range and step parameters)

    Convenience method that calls the full scanCString method with nullptr
    for defaultValue.
  */
  bool scanCString(const char* cString,
                   UVCTypeScanFlags flags,
                   std::shared_ptr<UVCValue> minimum,
                   std::shared_ptr<UVCValue> maximum,
                   std::shared_ptr<UVCValue> stepSize);

  /*!
    @method scanCString (full version)

    Send the scanCString message to the UVCType, using this instance's valuePtr
    as the buffer.

    See UVCType's documentation for a description of the acceptable C string
    format.

    Returns true if all component fields of the memory buffer were
    successfully set.
  */
  bool scanCString(const char* cString,
                   UVCTypeScanFlags flags,
                   std::shared_ptr<UVCValue> minimum,
                   std::shared_ptr<UVCValue> maximum,
                   std::shared_ptr<UVCValue> stepSize,
                   std::shared_ptr<UVCValue> defaultValue);

  /*!
    @method stringValue

    Returns a human-readable description of the data, as structured by its
    UVCType.

    Example:

      "{pan=3600,tilt=-360000}"

  */
  std::string stringValue() const;

  /*!
    @method copyValue

    If otherValue->valueType() matches this instance's UVCType (same layout of
    atomic types) then the requisite number of bytes from otherValue->valuePtr()
    are copied to this instance's memory buffer.

    Returns true if the copy was successful.
  */
  bool copyValue(std::shared_ptr<UVCValue> otherValue);

  /*!
    @method isEqual

    Returns true if this UVCValue is equal to another UVCValue (same type and
    data).
  */
  bool isEqual(const UVCValue& other) const;
};
