//
// main.cpp
//
// The utility program that makes use of the UVCController class to handle
// introspection and interaction with software controls exposed by UVC-compliant
// USB video devices.
//
// Translated from Objective-C to C++
// Copyright © 2016
// Dr. Jeffrey Frey, IT-NSS
// University of Delaware
//
// $Id$
//

#include <getopt.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "UVCController.hpp"

#if (MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_9)
#define UVC_UTIL_COMPAT_VERSION "pre-10.9"
#elif (MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_10)
#define UVC_UTIL_COMPAT_VERSION "10.9"
#elif (MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_11)
#define UVC_UTIL_COMPAT_VERSION "10.10"
#else
#define UVC_UTIL_COMPAT_VERSION "10.11"
#endif

struct UVCUtilVersion {
  uint8_t majorRev;
  uint8_t minorAndBugRev;
  uint8_t stage;
  uint8_t nonRelRev;
};

static UVCUtilVersion uvcUtilVersion = {.majorRev = 1,
                                        .minorAndBugRev = 0x20,
                                        .stage = 3,  // finalStage
                                        .nonRelRev = 0};

const char* UVCUtilVersionString() {
  static char versionString[64];
  static bool ready = false;

  if (!ready) {
    const char* format;

    switch (uvcUtilVersion.stage) {
      case 1:  // developStage
        format = "%d.%x.%xdev%d (for Mac OS X %s)";
        break;
      case 2:  // alphaStage
        format = "%d.%x.%xa%d (for Mac OS X %s)";
        break;
      case 3:  // betaStage
        format = "%d.%x.%xb%d (for Mac OS X %s)";
        break;
      case 4:  // finalStage
        format = (uvcUtilVersion.minorAndBugRev & 0xF)
                     ? "%d.%x.%x (for Mac OS X %s)"
                     : "%d.%x (for Mac OS X %s)";
        break;
      default:
        format = "%d.%x.%x (for Mac OS X %s)";
        break;
    }

    snprintf(versionString, sizeof(versionString), format,
             uvcUtilVersion.majorRev,
             ((uvcUtilVersion.minorAndBugRev & 0xF0) >> 4),
             (uvcUtilVersion.minorAndBugRev & 0xF), uvcUtilVersion.nonRelRev,
             UVC_UTIL_COMPAT_VERSION);
    ready = true;
  }

  return versionString;
}

static struct option uvcUtilOptions[] = {
    {"list-devices", no_argument, nullptr, 'd'},
    {"list-controls", no_argument, nullptr, 'c'},
    {"show-control", required_argument, nullptr, 'S'},
    {"set", required_argument, nullptr, 's'},
    {"get", required_argument, nullptr, 'g'},
    {"get-value", required_argument, nullptr, 'o'},
    {"reset-all", no_argument, nullptr, 'r'},
    {"select-none", no_argument, nullptr, '0'},
    {"select-by-vendor-and-product-id", required_argument, nullptr, 'V'},
    {"select-by-location-id", required_argument, nullptr, 'L'},
    {"select-by-name", required_argument, nullptr, 'N'},
    {"select-by-index", required_argument, nullptr, 'I'},
    {"keep-running", no_argument, nullptr, 'k'},
    {"help", no_argument, nullptr, 'h'},
    {"version", no_argument, nullptr, 'v'},
    {"debug", no_argument, nullptr, 'D'},
    {nullptr, 0, nullptr, 0}};

void usage(const char* exe) {
  printf(
      "usage:\n"
      "\n"
      "    %s {options/actions/target selection}\n"
      "\n"
      "  Options:\n"
      "\n"
      "    -h/--help                              Show this information\n"
      "    -v/--version                           Show the version of the "
      "program\n"
      "    -k/--keep-running                      Continue processing "
      "additional actions despite\n"
      "                                           encountering errors\n"
      "\n"
      "  Actions:\n"
      "\n"
      "    -d/--list-devices                      Display a list of all "
      "UVC-capable devices\n"
      "    -c/--list-controls                     Display a list of UVC "
      "controls implemented\n"
      "\n"
      "    Available after a target device is selected:\n"
      "\n"
      "    -c/--list-controls                     Display a list of UVC "
      "controls available for\n"
      "                                           the target device\n"
      "\n"
      "    -S (<control-name>|*)                  Display available "
      "information for the given\n"
      "    --show-control=(<control-name>|*)      UVC control (or all controls "
      "for \"*\").\n"
      "\n"
      "    -g <control-name>                      Get the value of a control.\n"
      "    --get=<control-name>\n"
      "\n"
      "    -o <control-name>                      Same as -g/--get, but ONLY "
      "the value of the control\n"
      "    --get-value=<control-name>             is displayed (no label)\n"
      "\n"
      "    -s <control-name>=<value>              Set the value of a control\n"
      "    --set=<control-name>=<value>\n"
      "\n"
      "    -r/--reset-all                         Reset all controls with a "
      "default value to that value\n"
      "\n"
      "  Methods for selecting the target device:\n"
      "\n"
      "    -0/--select-none                       Drop the selected target "
      "device\n"
      "\n"
      "    -I <device-index>                      Index of the device in the "
      "list of all devices (zero-based)\n"
      "    --select-by-index=<device-index>\n"
      "\n"
      "    -V <vendor-id>:<product-id>            Provide the hexadecimal- or "
      "integer-valued vendor and product identifier\n"
      "    --select-by-vendor-and-product-id=<vendor-id>:<product-id>\n"
      "\n"
      "    -L <location-id>                       Provide the hexadecimal- or "
      "integer-valued USB locationID attribute\n"
      "    --select-by-location-id=<location-id>\n"
      "\n"
      "    -N <device-name>                       Provide the USB product "
      "name\n"
      "    --select-by-name=<device-name>\n"
      "\n",
      exe);
}

std::shared_ptr<UVCDeviceController> UVCUtilGetControllerWithName(
    const std::vector<std::shared_ptr<UVCDeviceController>>& uvcDevices,
    const std::string& name) {
  for (const auto& controller : uvcDevices) {
    // Case-insensitive comparison
    std::string deviceName = controller->deviceName();
    if (std::equal(name.begin(), name.end(), deviceName.begin(),
                   deviceName.end(),
                   [](char a, char b) { return tolower(a) == tolower(b); })) {
      return controller;
    }
  }
  return nullptr;
}

std::shared_ptr<UVCDeviceController> UVCUtilGetControllerWithVendorAndProductId(
    const std::vector<std::shared_ptr<UVCDeviceController>>& uvcDevices,
    uint16_t vendorId,
    uint16_t productId) {
  for (const auto& controller : uvcDevices) {
    if (controller->vendorId() == vendorId &&
        controller->productId() == productId) {
      return controller;
    }
  }
  return nullptr;
}

std::shared_ptr<UVCDeviceController> UVCUtilGetControllerWithLocationId(
    const std::vector<std::shared_ptr<UVCDeviceController>>& uvcDevices,
    uint32_t locationId) {
  for (const auto& controller : uvcDevices) {
    if (controller->locationId() == locationId) {
      return controller;
    }
  }
  return nullptr;
}

int main(int argc, char* argv[]) {
  const char* exe = argv[0];
  int rc = 0;
  std::vector<std::shared_ptr<UVCDeviceController>> uvcDevices;
  std::shared_ptr<UVCDeviceController> targetDevice = nullptr;
  int optCh;
  bool exitOnErrors = true;
  UVCTypeScanFlags uvcScanFlags = UVCTypeScanFlags::ShowWarnings;

  // No CLI arguments, we've got nothing to do:
  if (argc == 1) {
    usage(argv[0]);
    return 0;
  }

  while ((optCh = getopt_long(argc, argv, "dcS:s:g:o:r0V:L:N:I:khfFvD",
                              uvcUtilOptions, nullptr)) != -1) {
    switch (optCh) {
      case 'h':
        usage(exe);
        break;

      case 'v':
        printf("%s\n", UVCUtilVersionString());
        printf("Build timestamp %s %s\n", __TIME__, __DATE__);
        break;

      case 'k':
        exitOnErrors = false;
        break;

      case 'D':
        uvcScanFlags = uvcScanFlags | UVCTypeScanFlags::ShowInfo;
        break;

      case 'd':
        if (uvcDevices.empty()) {
          uvcDevices = UVCDeviceController::getUVCControllers();
        }
        if (!uvcDevices.empty()) {
          printf(
              "------------ -------------- ------------ ------------ "
              "-------------------- "
              "------------------------------------------------\n");
          printf("%-12s %-14s %-12s %-12s %-20s %s\n", "Index", "Vend:Prod",
                 "LocationID", "UVC Version", "Serial Number", "Device name");
          printf(
              "------------ -------------- ------------ ------------ "
              "-------------------- "
              "------------------------------------------------\n");

          for (size_t deviceIndex = 0; deviceIndex < uvcDevices.size();
               deviceIndex++) {
            const auto& device = uvcDevices[deviceIndex];
            uint16_t uvcVersion = device->uvcVersion();
            char versionStr[8];

            snprintf(versionStr, sizeof(versionStr), "%d.%02x",
                     (short)(uvcVersion >> 8), (uvcVersion & 0xFF));
            printf("%-12zu 0x%04x:0x%04x  0x%08x   %-12s %-20s %s\n",
                   deviceIndex, device->vendorId(), device->productId(),
                   device->locationId(), versionStr,
                   device->serialNumber().c_str(),
                   device->deviceName().c_str());
          }
          printf(
              "------------ -------------- ------------ ------------ "
              "-------------------- "
              "------------------------------------------------\n");
        } else {
          fprintf(stderr, "ERROR:  no UVC-capable devices available\n");
          rc = ENODEV;
          if (exitOnErrors)
            goto cleanupAndExit;
        }
        break;

      case 'c':
        if (targetDevice) {
          auto controlNames =
              UVCDeviceController::getAllControlStrings();  // Get ALL defined
                                                            // controls like
                                                            // original

          if (!controlNames.empty()) {
            bool hasAnyControls = false;
            printf("UVC controls implemented by this device:\n");
            for (const auto& name : controlNames) {
              auto control = targetDevice->controlWithName(name);
              if (control) {  // Only show if controlWithName succeeds (like
                              // original line 364-366)
                printf("  %s\n", name.c_str());
                hasAnyControls = true;
              }
            }
            if (!hasAnyControls) {
              fprintf(stderr,
                      "WARNING:  no controls implemented by this device\n");
            }
          } else {
            fprintf(stderr,
                    "WARNING:  no controls implemented by this device\n");
          }
        } else {
          auto controlNames = UVCDeviceController::getAllControlStrings();

          if (!controlNames.empty()) {
            printf("UVC controls implemented by this program:\n");
            for (const auto& name : controlNames) {
              printf("  %s\n", name.c_str());
            }
          }
        }
        break;

      case '0':
        targetDevice = nullptr;
        break;

      case 'S':
        if (!targetDevice) {
          if (uvcDevices.empty()) {
            uvcDevices = UVCDeviceController::getUVCControllers();
          }
          if (!uvcDevices.empty()) {
            targetDevice = uvcDevices[0];  // Use first device
            targetDevice->setIsInterfaceOpen(true);
          }
        }

        if (targetDevice) {
          if (strcmp(optarg, "*") == 0) {
            // Show all controls
            auto controlNames = targetDevice->controlStrings();
            for (const auto& name : controlNames) {
              auto control = targetDevice->controlWithName(name);
              if (control) {
                printf("%s\n", control->summaryString().c_str());
              }
            }
          } else {
            // Show specific control
            auto control = targetDevice->controlWithName(optarg);
            if (control) {
              printf("%s\n", control->summaryString().c_str());
            } else {
              fprintf(stderr, "ERROR: Control '%s' not found\n", optarg);
              rc = ENOENT;
              if (exitOnErrors)
                goto cleanupAndExit;
            }
          }
        } else {
          fprintf(stderr, "ERROR: No UVC device selected\n");
          rc = ENODEV;
          if (exitOnErrors)
            goto cleanupAndExit;
        }
        break;

      case 'g':
      case 'o':
        if (!targetDevice) {
          if (uvcDevices.empty()) {
            uvcDevices = UVCDeviceController::getUVCControllers();
          }
          if (!uvcDevices.empty()) {
            targetDevice = uvcDevices[0];
            targetDevice->setIsInterfaceOpen(true);
          }
        }

        if (targetDevice) {
          auto control = targetDevice->controlWithName(optarg);
          if (control) {
            if (control->readIntoCurrentValue()) {
              auto currentValue = control->currentValue();
              if (currentValue) {
                if (optCh == 'o') {
                  printf("%s\n", currentValue->stringValue().c_str());
                } else {
                  printf("%s = %s\n", optarg,
                         currentValue->stringValue().c_str());
                }
              } else {
                fprintf(stderr, "ERROR: Could not read control '%s'\n", optarg);
                rc = EIO;
                if (exitOnErrors)
                  goto cleanupAndExit;
              }
            } else {
              fprintf(stderr, "ERROR: Failed to read control '%s'\n", optarg);
              rc = EIO;
              if (exitOnErrors)
                goto cleanupAndExit;
            }
          } else {
            fprintf(stderr, "ERROR: Control '%s' not found\n", optarg);
            rc = ENOENT;
            if (exitOnErrors)
              goto cleanupAndExit;
          }
        } else {
          fprintf(stderr, "ERROR: No UVC device selected\n");
          rc = ENODEV;
          if (exitOnErrors)
            goto cleanupAndExit;
        }
        break;

      case 's': {
        if (!targetDevice) {
          if (uvcDevices.empty()) {
            uvcDevices = UVCDeviceController::getUVCControllers();
          }
          if (!uvcDevices.empty()) {
            targetDevice = uvcDevices[0];
            targetDevice->setIsInterfaceOpen(true);
          }
        }

        if (targetDevice) {
          // Parse control=value format
          char* equalSign = strchr(optarg, '=');
          if (!equalSign) {
            fprintf(
                stderr,
                "ERROR: Invalid format for --set, expected control=value\n");
            rc = EINVAL;
            if (exitOnErrors)
              goto cleanupAndExit;
            break;
          }

          *equalSign = '\0';  // Split the string
          const char* controlName = optarg;
          const char* valueString = equalSign + 1;

          auto control = targetDevice->controlWithName(controlName);
          if (control) {
            if (control->setCurrentValueFromCString(valueString,
                                                    uvcScanFlags)) {
              if (control->writeFromCurrentValue()) {
                printf("Successfully set %s = %s\n", controlName, valueString);
              } else {
                fprintf(stderr, "ERROR: Failed to write control '%s'\n",
                        controlName);
                rc = EIO;
                if (exitOnErrors)
                  goto cleanupAndExit;
              }
            } else {
              fprintf(stderr, "ERROR: Invalid value '%s' for control '%s'\n",
                      valueString, controlName);
              rc = EINVAL;
              if (exitOnErrors)
                goto cleanupAndExit;
            }
          } else {
            fprintf(stderr, "ERROR: Control '%s' not found\n", controlName);
            rc = ENOENT;
            if (exitOnErrors)
              goto cleanupAndExit;
          }
        } else {
          fprintf(stderr, "ERROR: No UVC device selected\n");
          rc = ENODEV;
          if (exitOnErrors)
            goto cleanupAndExit;
        }
        break;
      }

      case 'r':
        if (!targetDevice) {
          if (uvcDevices.empty()) {
            uvcDevices = UVCDeviceController::getUVCControllers();
          }
          if (!uvcDevices.empty()) {
            targetDevice = uvcDevices[0];
            targetDevice->setIsInterfaceOpen(true);
          }
        }

        if (targetDevice) {
          auto controlNames = targetDevice->controlStrings();
          int resetCount = 0;
          for (const auto& name : controlNames) {
            auto control = targetDevice->controlWithName(name);
            if (control && control->hasDefaultValue()) {
              if (control->resetToDefaultValue()) {
                printf("Reset %s to default\n", name.c_str());
                resetCount++;
              }
            }
          }
          printf("Reset %d controls to default values\n", resetCount);
        } else {
          fprintf(stderr, "ERROR: No UVC device selected\n");
          rc = ENODEV;
          if (exitOnErrors)
            goto cleanupAndExit;
        }
        break;

      case 'V': {
        if (uvcDevices.empty()) {
          uvcDevices = UVCDeviceController::getUVCControllers();
        }

        // Parse vendor:product format
        char* colonPos = strchr(optarg, ':');
        if (!colonPos) {
          fprintf(
              stderr,
              "ERROR: Invalid format for --select-by-vendor-and-product-id, "
              "expected vendor:product\n");
          rc = EINVAL;
          if (exitOnErrors)
            goto cleanupAndExit;
          break;
        }

        *colonPos = '\0';
        uint32_t vendorId = strtoul(optarg, nullptr, 0);
        uint32_t productId = strtoul(colonPos + 1, nullptr, 0);

        targetDevice = UVCUtilGetControllerWithVendorAndProductId(
            uvcDevices, static_cast<uint16_t>(vendorId),
            static_cast<uint16_t>(productId));

        if (targetDevice) {
          targetDevice->setIsInterfaceOpen(true);
          printf("Selected device: %s\n", targetDevice->description().c_str());
        } else {
          fprintf(stderr,
                  "ERROR: No device found with vendor:product 0x%04x:0x%04x\n",
                  vendorId, productId);
          rc = ENODEV;
          if (exitOnErrors)
            goto cleanupAndExit;
        }
        break;
      }

      case 'L': {
        if (uvcDevices.empty()) {
          uvcDevices = UVCDeviceController::getUVCControllers();
        }

        uint32_t locationId = strtoul(optarg, nullptr, 0);
        targetDevice =
            UVCUtilGetControllerWithLocationId(uvcDevices, locationId);

        if (targetDevice) {
          targetDevice->setIsInterfaceOpen(true);
          printf("Selected device: %s\n", targetDevice->description().c_str());
        } else {
          fprintf(stderr, "ERROR: No device found with location ID 0x%08x\n",
                  locationId);
          rc = ENODEV;
          if (exitOnErrors)
            goto cleanupAndExit;
        }
        break;
      }

      case 'N': {
        if (uvcDevices.empty()) {
          uvcDevices = UVCDeviceController::getUVCControllers();
        }

        targetDevice = UVCUtilGetControllerWithName(uvcDevices, optarg);

        if (targetDevice) {
          targetDevice->setIsInterfaceOpen(true);
          printf("Selected device: %s\n", targetDevice->description().c_str());
        } else {
          fprintf(stderr, "ERROR: No device found with name '%s'\n", optarg);
          rc = ENODEV;
          if (exitOnErrors)
            goto cleanupAndExit;
        }
        break;
      }

      case 'I': {
        if (uvcDevices.empty()) {
          uvcDevices = UVCDeviceController::getUVCControllers();
        }

        size_t deviceIndex = strtoul(optarg, nullptr, 0);
        if (deviceIndex < uvcDevices.size()) {
          targetDevice = uvcDevices[deviceIndex];
          targetDevice->setIsInterfaceOpen(true);
          printf("Selected device: %s\n", targetDevice->description().c_str());
        } else {
          fprintf(stderr, "ERROR: Device index %zu out of range (0-%zu)\n",
                  deviceIndex, uvcDevices.size() - 1);
          rc = ERANGE;
          if (exitOnErrors)
            goto cleanupAndExit;
        }
        break;
      }

      default:
        fprintf(stderr, "ERROR: Unrecognized option\n");
        usage(exe);
        rc = EINVAL;
        if (exitOnErrors)
          goto cleanupAndExit;
        break;
    }
  }

cleanupAndExit:
  return rc;
}
