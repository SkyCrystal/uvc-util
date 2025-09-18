//
// UVCType.hpp
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

#pragma once

#include <climits>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

/*!
  @typedef UVCTypeComponentType

  Enumerates the atomic data types that the UVCType class implements.
  These are underlying types used by the UVC standard in control
  interfaces.
*/
enum class UVCTypeComponentType {
  Invalid = 0,
  Boolean,
  SInt8,
  UInt8,
  Bitmap8,
  SInt16,
  UInt16,
  Bitmap16,
  SInt32,
  UInt32,
  Bitmap32,
  SInt64,
  UInt64,
  Bitmap64,
  Max
};

/*!
  @function UVCTypeComponentByteSize

  Returns the number of bytes occupied by the given componentType or
  zero (0) if componentType was invalid.
*/
size_t UVCTypeComponentByteSize(UVCTypeComponentType componentType);

/*!
  @defined UVCTypeInvalidIndex

  Constant returned by UVCType to indicate that a field index was out
  of range.
*/
const size_t UVCTypeInvalidIndex = SIZE_MAX;

/*!
  @typedef UVCTypeScanFlags

  Enumerates bitmask components that alter the behavior of the scanCString:*
  methods of UVCType.

  The ShowWarnings flag allows warning messages to be
  written to stderr as the routines parse a cString.  Additionally, the
  ShowInfo flag produces more extensive output to stderr
  as the cString is processed (more like debugging information).
*/
enum class UVCTypeScanFlags : uint32_t {
  ShowWarnings = 1 << 0,
  ShowInfo = 1 << 1
};

// Enable bitwise operations for UVCTypeScanFlags
inline UVCTypeScanFlags operator|(UVCTypeScanFlags a, UVCTypeScanFlags b) {
  return static_cast<UVCTypeScanFlags>(static_cast<uint32_t>(a) |
                                       static_cast<uint32_t>(b));
}

inline UVCTypeScanFlags operator&(UVCTypeScanFlags a, UVCTypeScanFlags b) {
  return static_cast<UVCTypeScanFlags>(static_cast<uint32_t>(a) &
                                       static_cast<uint32_t>(b));
}

inline bool operator!(UVCTypeScanFlags flags) {
  return static_cast<uint32_t>(flags) == 0;
}

/*!
  @class UVCType
  @abstract Abstract data type comprised of UVCTypeComponentType atomic types

  Instances of the UVCType class represent the structured data brokered by
  UVC controls.  A UVCType comprises one or more named data structure fields in
  a specific order, with each having an atomic type from the
  UVCTypeComponentType enumeration.

  The set of fields correlate to a packed C struct without word boundary
  padding; this also correlates directly to the format of UVC control data.

  In case this code were to be compiled on a big-endian host, byte-swapping
  routines are included which can reorder an external buffer (containing the
  UVC control data structured by the UVCType) to and from USB (little) endian.

  Methods are included to calculate relative byte offsets of the component
  fields, either by index of the name of the field.  Also, the number of bytes
  occupied by the UVCType is available via the byteSize method.

  Methods are also provided to initialize an external buffer structured by a
  UVCType using textual input (from a C string).
*/
class UVCType {
 private:
  struct UVCTypeField {
    std::string fieldName;
    UVCTypeComponentType fieldType;
  };

  std::vector<UVCTypeField> _fields;
  bool _needsNoByteSwap;

 public:
  /*!
    @method createFromCString

    Returns a shared_ptr to UVCType initialized with the component field(s)
    described by a C string.  The C string must begin and end with curly braces
    and include one or more named types.  Each named type follows the syntax:

      [type] [name];

    where [name] is the component name (alphanumeric characters and '-') and the
    [type] is one of:

      [type]    description
      B         UInt8, accepting only 0 and 1
      S1        SInt8 / char
      U1        UInt8 / unsigned char
      M1        UInt8 / unsigned char as a bitmap
      S2        SInt16 / short
      U2        UInt16 / unsigned short
      M2        UInt16 / unsigned short as a bitmap
      S4        SInt32 / int
      U4        UInt32 / unsigned int
      M4        UInt32 / unsigned int as a bitmap
      S8        SInt64 / long long int
      U8        UInt64 / unsigned long long int
      M8        UInt64 / unsigned long long int as a bitmap

    For types with a single field, the [name] can be omitted:

      {S2}

    Other examples:

      { S2 pan; S2 tilt; }

  */
  static std::shared_ptr<UVCType> createFromCString(
      const char* typeDescription);

  /*!
    @method createWithFieldNamesAndTypes

    Returns a shared_ptr to UVCType initialized with the given field names and
    types.
  */
  static std::shared_ptr<UVCType> createWithFieldNamesAndTypes(
      const std::vector<std::string>& names,
      const std::vector<UVCTypeComponentType>& types);

  // Constructor and destructor
  UVCType() = default;
  ~UVCType() = default;

  // Copy constructor and assignment operator
  UVCType(const UVCType& other) = default;
  UVCType& operator=(const UVCType& other) = default;

  /*!
    @method fieldCount

    Returns the number of component fields in the structure represented by this
    instance.
  */
  size_t fieldCount() const;

  /*!
    @method fieldNameAtIndex

    Returns the name associated with the component field at the given index.

    Returns empty string if index is out of range.
  */
  std::string fieldNameAtIndex(size_t index) const;

  /*!
    @method fieldTypeAtIndex

    Returns the type associated with the component field at the given index.

    Returns UVCTypeComponentType::Invalid if index is out of range.
  */
  UVCTypeComponentType fieldTypeAtIndex(size_t index) const;

  /*!
    @method indexOfFieldWithName

    If one of the component fields is named the same as fieldName (under a
    case-insensitive string comparison) returns the index of that field.
    Otherwise, UVCTypeInvalidIndex is returned.
  */
  size_t indexOfFieldWithName(const std::string& fieldName) const;

  /*!
    @method byteSize

    Returns the number of bytes that data structured according to the component
    field types would occupy.
  */
  size_t byteSize() const;

  /*!
    @method offsetToFieldAtIndex

    Returns the relative offset (in bytes) at which the given component field
    would be found in a buffer structured according to the component field
    types.

    Returns UVCTypeInvalidIndex if index is out of range.
  */
  size_t offsetToFieldAtIndex(size_t index) const;

  /*!
    @method offsetToFieldWithName

    Returns the relative offset (in bytes) at which the given component field
    (identified by case-insensitive string comparison against fieldName) would
    be found in a buffer structured according to the component field types.

    Returns UVCTypeInvalidIndex if fieldName is not found.
  */
  size_t offsetToFieldWithName(const std::string& fieldName) const;

  /*!
    @method byteSwapHostToUSBEndian

    Given an external buffer structured according to the component field types,
    byte swap all necessary component fields (anything larger than 1 byte) from
    the host endian to USB (little) endian.
  */
  void byteSwapHostToUSBEndian(void* buffer) const;

  /*!
    @method byteSwapUSBToHostEndian

    Given an external buffer structured according to the component field types,
    byte swap all necessary component fields (anything larger than 1 byte) from
    USB (little) endian to host endian.
  */
  void byteSwapUSBToHostEndian(void* buffer) const;

  /*!
    @method scanCString

    Convenience method that calls the full scanCString method with NULL pointers
    for minimum, maximum, stepSize, and defaultValue.
  */
  bool scanCString(const char* cString,
                   void* buffer,
                   UVCTypeScanFlags flags) const;

  /*!
    @method scanCString (full version)

    The arguments buffer, minimum, maximum, stepSize, and defaultValue are all
    assumed to be at least as large as this instance's byteSize() method would
    return.  If any of minimum, maximum, stepSize, or defaultValue are non-NULL,
    they are assumed to contain the corresponding limit data read from the UVC
    device.

    Parses cString and attempts to fill-in the provided buffer according to the
    component fields. The component values must be contained within curly
    braces, and values should be delimited using a comma.  Whitespace is
    permissible around words and commas.

    Use of a floating-point (fractional) value for a component requires that
    minimum and maximum are non-NULL. The fractional value maps to the
    corresponding value in that range, with 0.0f being the minimum.

    The words "default," "minimum," and "maximum" are permissible on components
    so long as minimum, maximum, or defaultValue are non-NULL.

    Types with a single component may omit the curly braces.

    If the words "default," "minimum," or "maximum" are the entirety of cString,
    then the corresponding value is set for all component fields, provided the
    corresponding pointer argument is non-NULL.

    The flags bitmask controls this function's behavior.  At this time, the only
    flags present enable informative output to stderr as the parsing progresses.

    Returns true if all component fields were successfully set in the provided
    buffer.
  */
  bool scanCString(const char* cString,
                   void* buffer,
                   UVCTypeScanFlags flags,
                   void* minimum,
                   void* maximum,
                   void* stepSize,
                   void* defaultValue) const;

  /*!
    @method stringFromBuffer

    Create a formatted textual description of the data in the external buffer
    structured according to the component field types.  E.g.

      "{pan=3600,tilt=-360000}"

  */
  std::string stringFromBuffer(void* buffer) const;

  /*!
    @method typeSummaryString

    Returns a human-readable description of the component field types.
  */
  std::string typeSummaryString() const;

  /*!
    @method isEqual

    Returns true if this UVCType has the same structure as another UVCType.
  */
  bool isEqual(const UVCType& other) const;

 private:
  // Helper functions for parsing type descriptions
  static UVCTypeComponentType componentTypeFromString(const char* typeDefString,
                                                      size_t* nChar);
  static const char* componentTypeString(UVCTypeComponentType componentType);
  static const char* componentVerboseTypeString(
      UVCTypeComponentType componentType);

  // Helper function for scanning individual component values
  static bool componentTypeScanf(const char* cString,
                                 UVCTypeComponentType theType,
                                 void* theValue,
                                 UVCTypeScanFlags flags,
                                 void* theMinimum,
                                 void* theMaximum,
                                 void* theStepSize,
                                 void* theDefaultValue,
                                 size_t* nChar);
};
