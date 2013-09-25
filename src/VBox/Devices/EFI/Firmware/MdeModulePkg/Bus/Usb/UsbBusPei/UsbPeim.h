/** @file
Usb Peim definition.

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved. <BR>
  
This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PEI_USB_PEIM_H_
#define _PEI_USB_PEIM_H_


#include <PiPei.h>

#include <Ppi/UsbHostController.h>
#include <Ppi/Usb2HostController.h>
#include <Ppi/UsbIo.h>

#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/TimerLib.h>
#include <Library/PcdLib.h>

#include <IndustryStandard/Usb.h>

#define MAX_ROOT_PORT             2
#define MAX_ENDPOINT              16

#define USB_SLOW_SPEED_DEVICE     0x01
#define USB_FULL_SPEED_DEVICE     0x02

#define PEI_USB_DEVICE_SIGNATURE  SIGNATURE_32 ('U', 's', 'b', 'D')
typedef struct {
  UINTN                         Signature;
  PEI_USB_IO_PPI                UsbIoPpi;
  EFI_PEI_PPI_DESCRIPTOR        UsbIoPpiList;
  UINT8                         DeviceAddress;
  UINT8                         MaxPacketSize0;
  UINT8                         DeviceSpeed;
  UINT8                         DataToggle;
  UINT8                         IsHub;
  UINT8                         DownStreamPortNo;
  UINT8                         Reserved[2];  // Padding for IPF
  UINTN                         AllocateAddress;
  PEI_USB_HOST_CONTROLLER_PPI   *UsbHcPpi;
  PEI_USB2_HOST_CONTROLLER_PPI  *Usb2HcPpi;
  UINT8                         ConfigurationData[1024];
  EFI_USB_CONFIG_DESCRIPTOR     *ConfigDesc;
  EFI_USB_INTERFACE_DESCRIPTOR  *InterfaceDesc;
  EFI_USB_ENDPOINT_DESCRIPTOR   *EndpointDesc[MAX_ENDPOINT];
  EFI_USB2_HC_TRANSACTION_TRANSLATOR Translator;  
} PEI_USB_DEVICE;

#define PEI_USB_DEVICE_FROM_THIS(a) CR (a, PEI_USB_DEVICE, UsbIoPpi, PEI_USB_DEVICE_SIGNATURE)


/**
  Submits control transfer to a target USB device.
  
  @param  PeiServices            The pointer of EFI_PEI_SERVICES.
  @param  This                   The pointer of PEI_USB_IO_PPI.
  @param  Request                USB device request to send.
  @param  Direction              Specifies the data direction for the data stage.
  @param  Timeout                Indicates the maximum timeout, in millisecond.
  @param  Data                   Data buffer to be transmitted or received from USB device.
  @param  DataLength             The size (in bytes) of the data buffer.

  @retval EFI_SUCCESS            Transfer was completed successfully.
  @retval EFI_OUT_OF_RESOURCES   The transfer failed due to lack of resources.
  @retval EFI_INVALID_PARAMETER  Some parameters are invalid.
  @retval EFI_TIMEOUT            Transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR       Transfer failed due to host controller or device error.

**/
EFI_STATUS
EFIAPI
PeiUsbControlTransfer (
  IN     EFI_PEI_SERVICES          **PeiServices,
  IN     PEI_USB_IO_PPI            *This,
  IN     EFI_USB_DEVICE_REQUEST    *Request,
  IN     EFI_USB_DATA_DIRECTION    Direction,
  IN     UINT32                    Timeout,
  IN OUT VOID                      *Data,      OPTIONAL
  IN     UINTN                     DataLength  OPTIONAL
  );

/**
  Submits bulk transfer to a bulk endpoint of a USB device.
  
  @param  PeiServices           The pointer of EFI_PEI_SERVICES.
  @param  This                  The pointer of PEI_USB_IO_PPI.
  @param  DeviceEndpoint        Endpoint number and its direction in bit 7.
  @param  Data                  A pointer to the buffer of data to transmit 
                                from or receive into.
  @param  DataLength            The lenght of the data buffer.
  @param  Timeout               Indicates the maximum time, in millisecond, which the
                                transfer is allowed to complete.

  @retval EFI_SUCCESS           The transfer was completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The transfer failed due to lack of resource.
  @retval EFI_INVALID_PARAMETER Parameters are invalid.
  @retval EFI_TIMEOUT           The transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR      The transfer failed due to host controller error.

**/
EFI_STATUS
EFIAPI
PeiUsbBulkTransfer (
  IN     EFI_PEI_SERVICES    **PeiServices,
  IN     PEI_USB_IO_PPI      *This,
  IN     UINT8               DeviceEndpoint,
  IN OUT VOID                *Data,
  IN OUT UINTN               *DataLength,
  IN     UINTN               Timeout
  );

/**
  Get the usb interface descriptor.

  @param  PeiServices          General-purpose services that are available to every PEIM.
  @param  This                 Indicates the PEI_USB_IO_PPI instance.
  @param  InterfaceDescriptor  Request interface descriptor.


  @retval EFI_SUCCESS          Usb interface descriptor is obtained successfully.

**/
EFI_STATUS
EFIAPI
PeiUsbGetInterfaceDescriptor (
  IN  EFI_PEI_SERVICES                **PeiServices,
  IN  PEI_USB_IO_PPI                  *This,
  OUT EFI_USB_INTERFACE_DESCRIPTOR    **InterfaceDescriptor
  );

/**
  Get the usb endpoint descriptor.

  @param  PeiServices          General-purpose services that are available to every PEIM.
  @param  This                 Indicates the PEI_USB_IO_PPI instance.
  @param  EndpointIndex        The valid index of the specified endpoint.
  @param  EndpointDescriptor   Request endpoint descriptor.

  @retval EFI_SUCCESS       Usb endpoint descriptor is obtained successfully.
  @retval EFI_NOT_FOUND     Usb endpoint descriptor is NOT found.

**/
EFI_STATUS
EFIAPI
PeiUsbGetEndpointDescriptor (
  IN  EFI_PEI_SERVICES               **PeiServices,
  IN  PEI_USB_IO_PPI                 *This,
  IN  UINT8                          EndpointIndex,
  OUT EFI_USB_ENDPOINT_DESCRIPTOR    **EndpointDescriptor
  );

/**
  Reset the port and re-configure the usb device.

  @param  PeiServices    General-purpose services that are available to every PEIM.
  @param  This           Indicates the PEI_USB_IO_PPI instance.

  @retval EFI_SUCCESS    Usb device is reset and configured successfully.
  @retval Others         Other failure occurs.

**/
EFI_STATUS
EFIAPI
PeiUsbPortReset (
  IN EFI_PEI_SERVICES    **PeiServices,
  IN PEI_USB_IO_PPI      *This
  );

/**
  Send reset signal over the given root hub port.
  
  @param  PeiServices       Describes the list of possible PEI Services.
  @param  UsbHcPpi          The pointer of PEI_USB_HOST_CONTROLLER_PPI instance.
  @param  Usb2HcPpi         The pointer of PEI_USB2_HOST_CONTROLLER_PPI instance.
  @param  PortNum           The port to be reset.
  @param  RetryIndex        The retry times.

**/
VOID
ResetRootPort (
  IN EFI_PEI_SERVICES               **PeiServices,
  IN PEI_USB_HOST_CONTROLLER_PPI    *UsbHcPpi,
  IN PEI_USB2_HOST_CONTROLLER_PPI   *Usb2HcPpi,
  IN UINT8                          PortNum,
  IN UINT8                          RetryIndex
  );

#endif
