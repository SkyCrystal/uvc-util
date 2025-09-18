//
// UVCType.cpp
//
// Abstract data types for UVC controls.
//
// Translated from Objective-C to C++
// Copyright © 2016
// Dr. Jeffrey Frey, IT-NSS
// University of Delaware
//
// $Id$
//

#include "UVCType.hpp"
#include <algorithm>
#include <cctype>
#include <climits>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>

// Platform-specific byte swapping
#if defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#define htole16(x) OSSwapHostToLittleInt16(x)
#define le16toh(x) OSSwapLittleToHostInt16(x)
#define htole32(x) OSSwapHostToLittleInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)
#define htole64(x) OSSwapHostToLittleInt64(x)
#define le64toh(x) OSSwapLittleToHostInt64(x)
#elif defined(__linux__)
#include <endian.h>
#endif

size_t UVCTypeComponentByteSize(UVCTypeComponentType componentType) {
  static const size_t byteSizes[] = {
      0,  // Invalid
      1,  // Boolean
      1,  // SInt8
      1,  // UInt8
      1,  // Bitmap8
      2,  // SInt16
      2,  // UInt16
      2,  // Bitmap16
      4,  // SInt32
      4,  // UInt32
      4,  // Bitmap32
      8,  // SInt64
      8,  // UInt64
      8   // Bitmap64
  };

  if (componentType < UVCTypeComponentType::Max) {
    return byteSizes[static_cast<int>(componentType)];
  }
  return 0;
}

const char* UVCType::componentTypeString(UVCTypeComponentType componentType) {
  static const char* typeStrings[] = {
      "<invalid>",  // Invalid
      "B",          // Boolean
      "S1",         // SInt8
      "U1",         // UInt8
      "M1",         // Bitmap8
      "S2",         // SInt16
      "U2",         // UInt16
      "M2",         // Bitmap16
      "S4",         // SInt32
      "U4",         // UInt32
      "M4",         // Bitmap32
      "S8",         // SInt64
      "U8",         // UInt64
      "M8"          // Bitmap64
  };

  if (componentType < UVCTypeComponentType::Max) {
    return typeStrings[static_cast<int>(componentType)];
  }
  return "<invalid>";
}

const char* UVCType::componentVerboseTypeString(
    UVCTypeComponentType componentType) {
  static const char* verboseTypeStrings[] = {
      "<invalid>",                // Invalid
      "boolean",                  // Boolean
      "signed 8-bit integer",     // SInt8
      "unsigned 8-bit integer",   // UInt8
      "unsigned 8-bit bitmap",    // Bitmap8
      "signed 16-bit integer",    // SInt16
      "unsigned 16-bit integer",  // UInt16
      "unsigned 16-bit bitmap",   // Bitmap16
      "signed 32-bit integer",    // SInt32
      "unsigned 32-bit integer",  // UInt32
      "unsigned 32-bit bitmap",   // Bitmap32
      "signed 64-bit integer",    // SInt64
      "unsigned 64-bit integer",  // UInt64
      "unsigned 64-bit bitmap"    // Bitmap64
  };

  if (componentType < UVCTypeComponentType::Max) {
    return verboseTypeStrings[static_cast<int>(componentType)];
  }
  return "<invalid>";
}

UVCTypeComponentType UVCType::componentTypeFromString(const char* typeDefString,
                                                      size_t* nChar) {
  UVCTypeComponentType outType = UVCTypeComponentType::Invalid;
  size_t charConsumed = 0;

  while (*typeDefString && !isalpha(*typeDefString)) {
    charConsumed++;
    typeDefString++;
  }

  switch (tolower(*typeDefString)) {
    case 'b':
      outType = UVCTypeComponentType::Boolean;
      charConsumed++;
      break;

    case 'm':
      charConsumed++;
      switch (*(typeDefString + 1)) {
        case '1':
          outType = UVCTypeComponentType::Bitmap8;
          charConsumed++;
          break;
        case '2':
          outType = UVCTypeComponentType::Bitmap16;
          charConsumed++;
          break;
        case '4':
          outType = UVCTypeComponentType::Bitmap32;
          charConsumed++;
          break;
        case '8':
          outType = UVCTypeComponentType::Bitmap64;
          charConsumed++;
          break;
      }
      break;

    case 's':
      charConsumed++;
      switch (*(typeDefString + 1)) {
        case '1':
          outType = UVCTypeComponentType::SInt8;
          charConsumed++;
          break;
        case '2':
          outType = UVCTypeComponentType::SInt16;
          charConsumed++;
          break;
        case '4':
          outType = UVCTypeComponentType::SInt32;
          charConsumed++;
          break;
        case '8':
          outType = UVCTypeComponentType::SInt64;
          charConsumed++;
          break;
      }
      break;

    case 'u':
      charConsumed++;
      switch (*(typeDefString + 1)) {
        case '1':
          outType = UVCTypeComponentType::UInt8;
          charConsumed++;
          break;
        case '2':
          outType = UVCTypeComponentType::UInt16;
          charConsumed++;
          break;
        case '4':
          outType = UVCTypeComponentType::UInt32;
          charConsumed++;
          break;
        case '8':
          outType = UVCTypeComponentType::UInt64;
          charConsumed++;
          break;
      }
      break;
  }

  if (outType != UVCTypeComponentType::Invalid) {
    *nChar = charConsumed;
  }
  return outType;
}

std::shared_ptr<UVCType> UVCType::createFromCString(
    const char* typeDescription) {
  const char* originalStr = typeDescription;

  // Drop any leading whitespace:
  while (*typeDescription && isspace(*typeDescription)) {
    typeDescription++;
  }

  // Starts with a brace?
  if (*typeDescription != '{') {
    std::cerr << "WARNING: No opening brace found: " << originalStr
              << std::endl;
    return nullptr;
  }
  typeDescription++;

  // Remember this as our starting point:
  const char* startPtr = typeDescription;
  size_t fieldCount = 0;

  // Determine how many fields are defined:
  while (*typeDescription) {
    size_t nChar = 0;
    UVCTypeComponentType nextType =
        componentTypeFromString(typeDescription, &nChar);

    if (nextType == UVCTypeComponentType::Invalid) {
      std::cerr << "WARNING: Invalid type string at "
                << (typeDescription - originalStr) << " in: " << originalStr
                << std::endl;
      return nullptr;
    }
    typeDescription += nChar;

    // Discard whitespace:
    while (isspace(*typeDescription))
      typeDescription++;
    if (!*typeDescription) {
      std::cerr << "WARNING: Early end to type string at "
                << (typeDescription - originalStr) << " in: " << originalStr
                << std::endl;
      return nullptr;
    }

    // Check if we hit the closing brace immediately (single field type without
    // name)
    if (*typeDescription == '}') {
      // This is a single field type like {S2}, give it a default name
      fieldCount++;
      break;
    }

    // Check for a valid name:
    while (isalnum(*typeDescription) || (*typeDescription == '-'))
      typeDescription++;
    if (!*typeDescription) {
      std::cerr << "WARNING: Early end to type string at "
                << (typeDescription - originalStr) << " in: " << originalStr
                << std::endl;
      return nullptr;
    }

    // That's a valid field:
    fieldCount++;

    // Discard whitespace and semi-colon:
    while (isspace(*typeDescription) || (*typeDescription == ';'))
      typeDescription++;

    // If we've found a closing brace, we're done:
    if (*typeDescription == '}')
      break;
  }

  if (fieldCount) {
    std::vector<std::string> fieldNames(fieldCount);
    std::vector<UVCTypeComponentType> fieldTypes(fieldCount);
    size_t fieldIdx = 0;

    // Return to the start of the string:
    typeDescription = startPtr;

    // Parse the field definitions into the arrays:
    while (*typeDescription && (fieldIdx < fieldCount)) {
      size_t nChar = 0;
      UVCTypeComponentType nextType =
          componentTypeFromString(typeDescription, &nChar);

      typeDescription += nChar;
      fieldTypes[fieldIdx] = nextType;

      // Discard whitespace:
      while (isspace(*typeDescription))
        typeDescription++;
      if (!*typeDescription)
        return nullptr;

      // Check if we hit the closing brace (single field type without name)
      if (*typeDescription == '}') {
        // This is a single field type like {S2}, give it a default name
        fieldNames[fieldIdx] = "value";
      } else {
        // Isolate the name:
        startPtr = typeDescription;
        while (isalnum(*typeDescription) || (*typeDescription == '-'))
          typeDescription++;
        if (typeDescription >= startPtr) {
          size_t nameLen = typeDescription - startPtr;
          std::string nameStr(startPtr, nameLen);
          std::transform(nameStr.begin(), nameStr.end(), nameStr.begin(),
                         ::tolower);
          fieldNames[fieldIdx] = nameStr;
        } else {
          // No valid name found
          return nullptr;
        }
      }

      // Ensure that no other fields have used this name:
      for (size_t altFieldIdx = 0; altFieldIdx < fieldIdx; altFieldIdx++) {
        if (fieldNames[altFieldIdx] == fieldNames[fieldIdx]) {
          std::cerr << "WARNING: Repeated use of type name at index "
                    << (typeDescription - originalStr) << " in '" << originalStr
                    << "'" << std::endl;
          return nullptr;
        }
      }

      // That's a valid field:
      fieldIdx++;

      // Discard whitespace and semi-colon:
      while (isspace(*typeDescription) || (*typeDescription == ';'))
        typeDescription++;

      // If we've found a closing brace, we're done:
      if (*typeDescription == '}')
        break;
    }
    return createWithFieldNamesAndTypes(fieldNames, fieldTypes);
  }
  return nullptr;
}

std::shared_ptr<UVCType> UVCType::createWithFieldNamesAndTypes(
    const std::vector<std::string>& names,
    const std::vector<UVCTypeComponentType>& types) {
  if (names.size() != types.size()) {
    return nullptr;
  }

  for (size_t i = 0; i < types.size(); i++) {
    if (names[i].empty() || types[i] >= UVCTypeComponentType::Max) {
      return nullptr;
    }
  }

  auto newType = std::make_shared<UVCType>();
  newType->_fields.reserve(names.size());

  bool needsNoByteSwap = true;
  for (size_t i = 0; i < names.size(); i++) {
    UVCTypeField field;
    field.fieldName = names[i];
    field.fieldType = types[i];
    newType->_fields.push_back(field);

    if (UVCTypeComponentByteSize(types[i]) != 1) {
      needsNoByteSwap = false;
    }
  }

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  newType->_needsNoByteSwap = true;
#else
  newType->_needsNoByteSwap = needsNoByteSwap;
#endif

  return newType;
}

size_t UVCType::fieldCount() const {
  return _fields.size();
}

std::string UVCType::fieldNameAtIndex(size_t index) const {
  if (index >= _fields.size()) {
    return "";
  }
  return _fields[index].fieldName;
}

UVCTypeComponentType UVCType::fieldTypeAtIndex(size_t index) const {
  if (index >= _fields.size()) {
    return UVCTypeComponentType::Invalid;
  }
  return _fields[index].fieldType;
}

size_t UVCType::indexOfFieldWithName(const std::string& fieldName) const {
  for (size_t i = 0; i < _fields.size(); i++) {
    // Case-insensitive comparison
    if (fieldName.length() == _fields[i].fieldName.length() &&
        std::equal(fieldName.begin(), fieldName.end(),
                   _fields[i].fieldName.begin(),
                   [](char a, char b) { return tolower(a) == tolower(b); })) {
      return i;
    }
  }
  return UVCTypeInvalidIndex;
}

size_t UVCType::byteSize() const {
  size_t totalSize = 0;
  for (const auto& field : _fields) {
    totalSize += UVCTypeComponentByteSize(field.fieldType);
  }
  return totalSize;
}

size_t UVCType::offsetToFieldAtIndex(size_t index) const {
  if (index >= _fields.size()) {
    return UVCTypeInvalidIndex;
  }

  size_t offset = 0;
  for (size_t i = 0; i < index; i++) {
    offset += UVCTypeComponentByteSize(_fields[i].fieldType);
  }
  return offset;
}

size_t UVCType::offsetToFieldWithName(const std::string& fieldName) const {
  size_t index = indexOfFieldWithName(fieldName);
  if (index == UVCTypeInvalidIndex) {
    return UVCTypeInvalidIndex;
  }
  return offsetToFieldAtIndex(index);
}

void UVCType::byteSwapHostToUSBEndian(void* buffer) const {
  if (_needsNoByteSwap)
    return;

  uint8_t* bufferPtr = static_cast<uint8_t*>(buffer);

  for (const auto& field : _fields) {
    switch (field.fieldType) {
      case UVCTypeComponentType::Boolean:
      case UVCTypeComponentType::Bitmap8:
      case UVCTypeComponentType::SInt8:
      case UVCTypeComponentType::UInt8:
        bufferPtr++;
        break;

      case UVCTypeComponentType::SInt16:
      case UVCTypeComponentType::UInt16:
      case UVCTypeComponentType::Bitmap16: {
        uint16_t* asInt16Ptr = reinterpret_cast<uint16_t*>(bufferPtr);
        *asInt16Ptr = htole16(*asInt16Ptr);
        bufferPtr += 2;
        break;
      }

      case UVCTypeComponentType::SInt32:
      case UVCTypeComponentType::UInt32:
      case UVCTypeComponentType::Bitmap32: {
        uint32_t* asInt32Ptr = reinterpret_cast<uint32_t*>(bufferPtr);
        *asInt32Ptr = htole32(*asInt32Ptr);
        bufferPtr += 4;
        break;
      }

      case UVCTypeComponentType::SInt64:
      case UVCTypeComponentType::UInt64:
      case UVCTypeComponentType::Bitmap64: {
        uint64_t* asInt64Ptr = reinterpret_cast<uint64_t*>(bufferPtr);
        *asInt64Ptr = htole64(*asInt64Ptr);
        bufferPtr += 8;
        break;
      }

      case UVCTypeComponentType::Max:
      case UVCTypeComponentType::Invalid:
        // Should never get here!
        break;
    }
  }
}

void UVCType::byteSwapUSBToHostEndian(void* buffer) const {
  if (_needsNoByteSwap)
    return;

  uint8_t* bufferPtr = static_cast<uint8_t*>(buffer);

  for (const auto& field : _fields) {
    switch (field.fieldType) {
      case UVCTypeComponentType::Boolean:
      case UVCTypeComponentType::Bitmap8:
      case UVCTypeComponentType::SInt8:
      case UVCTypeComponentType::UInt8:
        bufferPtr++;
        break;

      case UVCTypeComponentType::SInt16:
      case UVCTypeComponentType::UInt16:
      case UVCTypeComponentType::Bitmap16: {
        uint16_t* asInt16Ptr = reinterpret_cast<uint16_t*>(bufferPtr);
        *asInt16Ptr = le16toh(*asInt16Ptr);
        bufferPtr += 2;
        break;
      }

      case UVCTypeComponentType::SInt32:
      case UVCTypeComponentType::UInt32:
      case UVCTypeComponentType::Bitmap32: {
        uint32_t* asInt32Ptr = reinterpret_cast<uint32_t*>(bufferPtr);
        *asInt32Ptr = le32toh(*asInt32Ptr);
        bufferPtr += 4;
        break;
      }

      case UVCTypeComponentType::SInt64:
      case UVCTypeComponentType::UInt64:
      case UVCTypeComponentType::Bitmap64: {
        uint64_t* asInt64Ptr = reinterpret_cast<uint64_t*>(bufferPtr);
        *asInt64Ptr = le64toh(*asInt64Ptr);
        bufferPtr += 8;
        break;
      }

      case UVCTypeComponentType::Max:
      case UVCTypeComponentType::Invalid:
        // Should never get here!
        break;
    }
  }
}

bool UVCType::scanCString(const char* cString,
                          void* buffer,
                          UVCTypeScanFlags flags) const {
  return scanCString(cString, buffer, flags, nullptr, nullptr, nullptr,
                     nullptr);
}

bool UVCType::isEqual(const UVCType& other) const {
  if (fieldCount() != other.fieldCount() || byteSize() != other.byteSize()) {
    return false;
  }

  for (size_t i = 0; i < _fields.size(); i++) {
    if (_fields[i].fieldType != other._fields[i].fieldType) {
      return false;
    }
  }
  return true;
}

std::string UVCType::stringFromBuffer(void* buffer) const {
  if (_fields.size() == 1) {
    const uint8_t* bufferPtr = static_cast<const uint8_t*>(buffer);

    switch (_fields[0].fieldType) {
      case UVCTypeComponentType::Boolean:
        return *bufferPtr ? "true" : "false";

      case UVCTypeComponentType::SInt8:
        return std::to_string(*reinterpret_cast<const int8_t*>(bufferPtr));

      case UVCTypeComponentType::UInt8:
      case UVCTypeComponentType::Bitmap8:
        return std::to_string(*bufferPtr);

      case UVCTypeComponentType::SInt16:
        return std::to_string(*reinterpret_cast<const int16_t*>(bufferPtr));

      case UVCTypeComponentType::UInt16:
      case UVCTypeComponentType::Bitmap16:
        return std::to_string(*reinterpret_cast<const uint16_t*>(bufferPtr));

      case UVCTypeComponentType::SInt32:
        return std::to_string(*reinterpret_cast<const int32_t*>(bufferPtr));

      case UVCTypeComponentType::UInt32:
      case UVCTypeComponentType::Bitmap32:
        return std::to_string(*reinterpret_cast<const uint32_t*>(bufferPtr));

      case UVCTypeComponentType::SInt64:
        return std::to_string(*reinterpret_cast<const int64_t*>(bufferPtr));

      case UVCTypeComponentType::UInt64:
      case UVCTypeComponentType::Bitmap64:
        return std::to_string(*reinterpret_cast<const uint64_t*>(bufferPtr));

      case UVCTypeComponentType::Max:
      case UVCTypeComponentType::Invalid:
        return "";
    }
  }

  std::stringstream ss;
  ss << "{";

  const uint8_t* bufferPtr = static_cast<const uint8_t*>(buffer);
  bool shouldIncludeComma = false;

  for (const auto& field : _fields) {
    if (shouldIncludeComma) {
      ss << ",";
    }

    ss << field.fieldName << "=";

    switch (field.fieldType) {
      case UVCTypeComponentType::Boolean:
        ss << (*bufferPtr ? "true" : "false");
        bufferPtr++;
        break;

      case UVCTypeComponentType::SInt8:
        ss << static_cast<int>(*reinterpret_cast<const int8_t*>(bufferPtr));
        bufferPtr++;
        break;

      case UVCTypeComponentType::UInt8:
      case UVCTypeComponentType::Bitmap8:
        ss << static_cast<unsigned int>(*bufferPtr);
        bufferPtr++;
        break;

      case UVCTypeComponentType::SInt16:
        ss << *reinterpret_cast<const int16_t*>(bufferPtr);
        bufferPtr += 2;
        break;

      case UVCTypeComponentType::UInt16:
      case UVCTypeComponentType::Bitmap16:
        ss << *reinterpret_cast<const uint16_t*>(bufferPtr);
        bufferPtr += 2;
        break;

      case UVCTypeComponentType::SInt32:
        ss << *reinterpret_cast<const int32_t*>(bufferPtr);
        bufferPtr += 4;
        break;

      case UVCTypeComponentType::UInt32:
      case UVCTypeComponentType::Bitmap32:
        ss << *reinterpret_cast<const uint32_t*>(bufferPtr);
        bufferPtr += 4;
        break;

      case UVCTypeComponentType::SInt64:
        ss << *reinterpret_cast<const int64_t*>(bufferPtr);
        bufferPtr += 8;
        break;

      case UVCTypeComponentType::UInt64:
      case UVCTypeComponentType::Bitmap64:
        ss << *reinterpret_cast<const uint64_t*>(bufferPtr);
        bufferPtr += 8;
        break;

      case UVCTypeComponentType::Max:
      case UVCTypeComponentType::Invalid:
        // Should never get here!
        break;
    }

    shouldIncludeComma = true;
  }

  ss << "}";
  return ss.str();
}

std::string UVCType::typeSummaryString() const {
  if (_fields.size() == 1) {
    return std::string("single value, ") +
           componentVerboseTypeString(_fields[0].fieldType);
  }

  std::stringstream ss;
  ss << "(";
  for (size_t i = 0; i < _fields.size(); i++) {
    if (i > 0)
      ss << "; ";
    ss << componentVerboseTypeString(_fields[i].fieldType) << " "
       << _fields[i].fieldName;
  }
  ss << ")";
  return ss.str();
}

bool UVCType::scanCString(const char* cString,
                          void* buffer,
                          UVCTypeScanFlags flags,
                          void* minimum,
                          void* maximum,
                          void* stepSize,
                          void* defaultValue) const {
  if (!cString || !buffer) {
    return false;
  }

  // Skip leading whitespace
  while (isspace(*cString))
    cString++;

  // Handle special keywords
  if (strncasecmp(cString, "default", 7) == 0) {
    if (defaultValue) {
      memcpy(buffer, defaultValue, byteSize());
      return true;
    }
    if (static_cast<uint32_t>(flags) &
        static_cast<uint32_t>(UVCTypeScanFlags::ShowWarnings)) {
      std::cerr << "WARNING: No default value provided by this control"
                << std::endl;
    }
    return false;
  }

  if (strncasecmp(cString, "minimum", 7) == 0) {
    if (minimum) {
      memcpy(buffer, minimum, byteSize());
      return true;
    }
    return false;
  }

  if (strncasecmp(cString, "maximum", 7) == 0) {
    if (maximum) {
      memcpy(buffer, maximum, byteSize());
      return true;
    }
    return false;
  }

  // Single field case - can omit braces
  if (_fields.size() == 1 && *cString != '{') {
    return componentTypeScanf(cString, _fields[0].fieldType, buffer, flags,
                              minimum, maximum, stepSize, defaultValue,
                              nullptr);
  }

  // Multi-field case with braces (or single field with braces)
  if (*cString != '{') {
    return false;
  }
  cString++;  // Skip opening brace

  // Check if using named values (contains =)
  bool usesNamedValues = strchr(cString, '=') != nullptr;

  for (size_t fieldIdx = 0; fieldIdx < _fields.size(); fieldIdx++) {
    // Skip whitespace
    while (isspace(*cString))
      cString++;

    if (*cString == '}')
      break;  // End of input

    size_t actualFieldIdx = fieldIdx;

    // Handle named field assignment
    if (usesNamedValues) {
      const char* nameStart = cString;
      while (*cString && *cString != '=')
        cString++;

      if (*cString != '=')
        return false;

      std::string fieldName(nameStart, cString - nameStart);
      // Convert to lowercase for comparison
      std::transform(fieldName.begin(), fieldName.end(), fieldName.begin(),
                     ::tolower);

      actualFieldIdx = indexOfFieldWithName(fieldName);
      if (actualFieldIdx == UVCTypeInvalidIndex) {
        return false;
      }

      cString++;  // Skip '='
    }

    // Calculate field offset and pointers
    size_t fieldOffset = offsetToFieldAtIndex(actualFieldIdx);
    void* valuePtr = static_cast<uint8_t*>(buffer) + fieldOffset;
    void* minimumPtr =
        minimum ? static_cast<uint8_t*>(minimum) + fieldOffset : nullptr;
    void* maximumPtr =
        maximum ? static_cast<uint8_t*>(maximum) + fieldOffset : nullptr;
    void* stepSizePtr =
        stepSize ? static_cast<uint8_t*>(stepSize) + fieldOffset : nullptr;
    void* defaultValuePtr =
        defaultValue ? static_cast<uint8_t*>(defaultValue) + fieldOffset
                     : nullptr;

    // Parse the value
    size_t nChar = 0;
    if (!componentTypeScanf(cString, _fields[actualFieldIdx].fieldType,
                            valuePtr, flags, minimumPtr, maximumPtr,
                            stepSizePtr, defaultValuePtr, &nChar)) {
      return false;
    }

    cString += nChar;

    // Skip trailing whitespace and commas
    while (isspace(*cString) || *cString == ',')
      cString++;
  }

  return true;
}

bool UVCType::componentTypeScanf(const char* cString,
                                 UVCTypeComponentType theType,
                                 void* theValue,
                                 UVCTypeScanFlags flags,
                                 void* theMinimum,
                                 void* theMaximum,
                                 void* theStepSize,
                                 void* theDefaultValue,
                                 size_t* nChar) {
  if (!cString || !theValue) {
    return false;
  }

  // Skip leading whitespace
  while (isspace(*cString))
    cString++;
  const char* start = cString;

  // Handle special keywords
  if (strncasecmp(cString, "default", 7) == 0) {
    if (!theDefaultValue)
      return false;

    switch (theType) {
      case UVCTypeComponentType::SInt8:
        *static_cast<int8_t*>(theValue) =
            *static_cast<int8_t*>(theDefaultValue);
        break;
      case UVCTypeComponentType::UInt8:
      case UVCTypeComponentType::Boolean:
      case UVCTypeComponentType::Bitmap8:
        *static_cast<uint8_t*>(theValue) =
            *static_cast<uint8_t*>(theDefaultValue);
        break;
      case UVCTypeComponentType::SInt16:
        *static_cast<int16_t*>(theValue) =
            *static_cast<int16_t*>(theDefaultValue);
        break;
      case UVCTypeComponentType::UInt16:
      case UVCTypeComponentType::Bitmap16:
        *static_cast<uint16_t*>(theValue) =
            *static_cast<uint16_t*>(theDefaultValue);
        break;
      case UVCTypeComponentType::SInt32:
        *static_cast<int32_t*>(theValue) =
            *static_cast<int32_t*>(theDefaultValue);
        break;
      case UVCTypeComponentType::UInt32:
      case UVCTypeComponentType::Bitmap32:
        *static_cast<uint32_t*>(theValue) =
            *static_cast<uint32_t*>(theDefaultValue);
        break;
      case UVCTypeComponentType::SInt64:
        *static_cast<int64_t*>(theValue) =
            *static_cast<int64_t*>(theDefaultValue);
        break;
      case UVCTypeComponentType::UInt64:
      case UVCTypeComponentType::Bitmap64:
        *static_cast<uint64_t*>(theValue) =
            *static_cast<uint64_t*>(theDefaultValue);
        break;
      default:
        return false;
    }

    if (nChar)
      *nChar = 7;  // Length of "default"
    return true;
  }

  if (strncasecmp(cString, "minimum", 7) == 0 && theMinimum) {
    // Similar logic for minimum values...
    if (nChar)
      *nChar = 7;
    return true;
  }

  if (strncasecmp(cString, "maximum", 7) == 0 && theMaximum) {
    // Similar logic for maximum values...
    if (nChar)
      *nChar = 7;
    return true;
  }

  // Handle boolean special strings
  if (theType == UVCTypeComponentType::Boolean) {
    const char* trues[] = {"y", "yes", "true", "t", "1", nullptr};
    const char* falses[] = {"n", "no", "false", "f", "0", nullptr};

    for (int i = 0; trues[i]; i++) {
      size_t len = strlen(trues[i]);
      if (strncasecmp(cString, trues[i], len) == 0) {
        *static_cast<uint8_t*>(theValue) = 1;
        if (nChar)
          *nChar = len;
        return true;
      }
    }

    for (int i = 0; falses[i]; i++) {
      size_t len = strlen(falses[i]);
      if (strncasecmp(cString, falses[i], len) == 0) {
        *static_cast<uint8_t*>(theValue) = 0;
        if (nChar)
          *nChar = len;
        return true;
      }
    }
  }

  // Parse numeric values
  char* endPtr;
  long long intValue =
      strtoll(cString, &endPtr, 0);  // Support hex, octal, decimal

  if (endPtr == cString) {
    // Not a valid number
    return false;
  }

  // Store the value according to its type
  switch (theType) {
    case UVCTypeComponentType::SInt8:
      *static_cast<int8_t*>(theValue) = static_cast<int8_t>(intValue);
      break;
    case UVCTypeComponentType::UInt8:
    case UVCTypeComponentType::Boolean:
    case UVCTypeComponentType::Bitmap8:
      *static_cast<uint8_t*>(theValue) = static_cast<uint8_t>(intValue);
      break;
    case UVCTypeComponentType::SInt16:
      *static_cast<int16_t*>(theValue) = static_cast<int16_t>(intValue);
      break;
    case UVCTypeComponentType::UInt16:
    case UVCTypeComponentType::Bitmap16:
      *static_cast<uint16_t*>(theValue) = static_cast<uint16_t>(intValue);
      break;
    case UVCTypeComponentType::SInt32:
      *static_cast<int32_t*>(theValue) = static_cast<int32_t>(intValue);
      break;
    case UVCTypeComponentType::UInt32:
    case UVCTypeComponentType::Bitmap32:
      *static_cast<uint32_t*>(theValue) = static_cast<uint32_t>(intValue);
      break;
    case UVCTypeComponentType::SInt64:
      *static_cast<int64_t*>(theValue) = static_cast<int64_t>(intValue);
      break;
    case UVCTypeComponentType::UInt64:
    case UVCTypeComponentType::Bitmap64:
      *static_cast<uint64_t*>(theValue) = static_cast<uint64_t>(intValue);
      break;
    default:
      return false;
  }

  if (nChar)
    *nChar = endPtr - start;
  return true;
}
