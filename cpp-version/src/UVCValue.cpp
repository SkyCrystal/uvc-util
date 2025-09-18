//
// UVCValue.cpp
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

#include "UVCValue.hpp"
#include <cstring>
#include <iomanip>
#include <sstream>

std::shared_ptr<UVCValue> UVCValue::create(std::shared_ptr<UVCType> valueType) {
  if (!valueType) {
    return nullptr;
  }

  return std::make_shared<UVCValue>(valueType);
}

UVCValue::UVCValue(std::shared_ptr<UVCType> valueType)
    : _isSwappedToUSBEndian(false), _valueType(valueType) {
  if (_valueType) {
    size_t bufferSize = _valueType->byteSize();
    _valueData.resize(bufferSize, 0);
  }
}

std::shared_ptr<UVCType> UVCValue::valueType() const {
  return _valueType;
}

void* UVCValue::valuePtr() {
  return _valueData.data();
}

const void* UVCValue::valuePtr() const {
  return _valueData.data();
}

size_t UVCValue::byteSize() const {
  if (_valueType) {
    return _valueType->byteSize();
  }
  return 0;
}

void* UVCValue::pointerToFieldAtIndex(size_t index) {
  if (!_valueType) {
    return nullptr;
  }

  size_t offset = _valueType->offsetToFieldAtIndex(index);
  if (offset == UVCTypeInvalidIndex) {
    return nullptr;
  }

  return _valueData.data() + offset;
}

const void* UVCValue::pointerToFieldAtIndex(size_t index) const {
  if (!_valueType) {
    return nullptr;
  }

  size_t offset = _valueType->offsetToFieldAtIndex(index);
  if (offset == UVCTypeInvalidIndex) {
    return nullptr;
  }

  return _valueData.data() + offset;
}

void* UVCValue::pointerToFieldWithName(const std::string& fieldName) {
  if (!_valueType) {
    return nullptr;
  }

  size_t offset = _valueType->offsetToFieldWithName(fieldName);
  if (offset == UVCTypeInvalidIndex) {
    return nullptr;
  }

  return _valueData.data() + offset;
}

const void* UVCValue::pointerToFieldWithName(
    const std::string& fieldName) const {
  if (!_valueType) {
    return nullptr;
  }

  size_t offset = _valueType->offsetToFieldWithName(fieldName);
  if (offset == UVCTypeInvalidIndex) {
    return nullptr;
  }

  return _valueData.data() + offset;
}

bool UVCValue::isSwappedToUSBEndian() const {
  return _isSwappedToUSBEndian;
}

void UVCValue::byteSwapHostToUSBEndian() {
  if (!_isSwappedToUSBEndian && _valueType) {
    _valueType->byteSwapHostToUSBEndian(_valueData.data());
    _isSwappedToUSBEndian = true;
  }
}

void UVCValue::byteSwapUSBToHostEndian() {
  if (_isSwappedToUSBEndian && _valueType) {
    _valueType->byteSwapUSBToHostEndian(_valueData.data());
    _isSwappedToUSBEndian = false;
  }
}

bool UVCValue::scanCString(const char* cString, UVCTypeScanFlags flags) {
  if (!_valueType) {
    return false;
  }

  return _valueType->scanCString(cString, _valueData.data(), flags);
}

bool UVCValue::scanCString(const char* cString,
                           UVCTypeScanFlags flags,
                           std::shared_ptr<UVCValue> minimum,
                           std::shared_ptr<UVCValue> maximum) {
  if (!_valueType) {
    return false;
  }

  void* minPtr = minimum ? const_cast<void*>(minimum->valuePtr()) : nullptr;
  void* maxPtr = maximum ? const_cast<void*>(maximum->valuePtr()) : nullptr;

  return _valueType->scanCString(cString, _valueData.data(), flags, minPtr,
                                 maxPtr, nullptr, nullptr);
}

bool UVCValue::scanCString(const char* cString,
                           UVCTypeScanFlags flags,
                           std::shared_ptr<UVCValue> minimum,
                           std::shared_ptr<UVCValue> maximum,
                           std::shared_ptr<UVCValue> stepSize) {
  if (!_valueType) {
    return false;
  }

  void* minPtr = minimum ? const_cast<void*>(minimum->valuePtr()) : nullptr;
  void* maxPtr = maximum ? const_cast<void*>(maximum->valuePtr()) : nullptr;
  void* stepPtr = stepSize ? const_cast<void*>(stepSize->valuePtr()) : nullptr;

  return _valueType->scanCString(cString, _valueData.data(), flags, minPtr,
                                 maxPtr, stepPtr, nullptr);
}

bool UVCValue::scanCString(const char* cString,
                           UVCTypeScanFlags flags,
                           std::shared_ptr<UVCValue> minimum,
                           std::shared_ptr<UVCValue> maximum,
                           std::shared_ptr<UVCValue> stepSize,
                           std::shared_ptr<UVCValue> defaultValue) {
  if (!_valueType) {
    return false;
  }

  void* minPtr = minimum ? const_cast<void*>(minimum->valuePtr()) : nullptr;
  void* maxPtr = maximum ? const_cast<void*>(maximum->valuePtr()) : nullptr;
  void* stepPtr = stepSize ? const_cast<void*>(stepSize->valuePtr()) : nullptr;
  void* defaultPtr =
      defaultValue ? const_cast<void*>(defaultValue->valuePtr()) : nullptr;

  return _valueType->scanCString(cString, _valueData.data(), flags, minPtr,
                                 maxPtr, stepPtr, defaultPtr);
}

std::string UVCValue::stringValue() const {
  if (!_valueType) {
    return "";
  }

  return _valueType->stringFromBuffer(
      const_cast<void*>(static_cast<const void*>(_valueData.data())));
}

bool UVCValue::copyValue(std::shared_ptr<UVCValue> otherValue) {
  if (!otherValue || !_valueType || !otherValue->_valueType) {
    return false;
  }

  if (_valueType->isEqual(*otherValue->_valueType)) {
    size_t copySize = _valueType->byteSize();
    std::memcpy(_valueData.data(), otherValue->_valueData.data(), copySize);
    _isSwappedToUSBEndian = otherValue->_isSwappedToUSBEndian;
    return true;
  }
  return false;
}

bool UVCValue::isEqual(const UVCValue& other) const {
  if (!_valueType || !other._valueType) {
    return false;
  }

  if (!_valueType->isEqual(*other._valueType)) {
    return false;
  }

  return std::memcmp(_valueData.data(), other._valueData.data(),
                     _valueType->byteSize()) == 0;
}
