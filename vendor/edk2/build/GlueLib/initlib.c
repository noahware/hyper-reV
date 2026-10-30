/** @file
  Glue Library Initialization Code

Copyright (c) 2015, Alex Ionescu. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>

#include <Library/DevicePathLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

EFI_STATUS
EFIAPI
UefiUnload (
  IN EFI_HANDLE        ImageHandle
  );

EFI_STATUS
EFIAPI
UefiBootServicesTableLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
);

EFI_STATUS
EFIAPI
UefiRuntimeServicesTableLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
);

EFI_STATUS
EFIAPI
UefiLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
);

EFI_STATUS
EFIAPI
DevicePathLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
);

EFI_STATUS
EFIAPI
RuntimeDriverLibConstruct (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
);

EFI_STATUS
EFIAPI
RuntimeDriverLibDeconstruct (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
);

EFI_STATUS
EFIAPI
UefiHiiServicesLibConstructor (
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE  *SystemTable
);

/**
  Calls library constructors.

  This function calls all the constructors that the UEFI libraries contain. As
  of now this includes the Boot and Runtime Services Table Library constructor,
  the UEFI Library constructor, and the Device Path Library constructor.

  @param  ImageHandle           The image handle of the UEFI Application.
  @param  SystemTable           A pointer to the EFI System Table.

  @retval EFI_SUCCESS           Operation was completed sucessfully.
**/
EFI_STATUS 
EFIAPI
ProcessLibraryConstructorList (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UefiBootServicesTableLibConstructor(ImageHandle, SystemTable);

  UefiRuntimeServicesTableLibConstructor(ImageHandle, SystemTable);

  UefiLibConstructor(ImageHandle, SystemTable);

  DevicePathLibConstructor(ImageHandle, SystemTable);

//  RuntimeDriverLibConstruct(ImageHandle, SystemTable);

  UefiHiiServicesLibConstructor(ImageHandle, SystemTable);

  return EFI_SUCCESS;
}

/**
  Call library destructors.

  This function calls all the destructors that the UEFI libraries contain. As
  of now, none of the libraries have any destructors.

  @param  ImageHandle           The image handle of the UEFI Application.
  @param  SystemTable           A pointer to the EFI System Table.
**/
VOID
EFIAPI
ProcessLibraryDestructorList (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
 // RuntimeDriverLibDeconstruct(ImageHandle, SystemTable);
}

/**
  Calls module entrypoints.

  This function calls all the entrypoints that are part of this UEFI module.
  For modules being built with this library, we assume that only one entry
  point exists, and that it is called UefiMain.

  @param  ImageHandle           The image handle of the UEFI Application.
  @param  SystemTable           A pointer to the EFI System Table.

  @retval EFI_SUCCESS           Operation was completed sucessfully.
**/
EFI_STATUS
EFIAPI
ProcessModuleEntryPointList (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return UefiMain (ImageHandle, SystemTable);
}

/**
  Calls module unload functions.

  This function calls all the unload functions part of this UEFI module.
  For modules being built with this library, we assume that only one unload
  function exists, and that it is called UefiUnload.

  @param  ImageHandle           The image handle of the UEFI Application.

  @retval EFI_SUCCESS           Operation was completed sucessfully.
**/
EFI_STATUS
EFIAPI
ProcessModuleUnloadList (
  IN EFI_HANDLE        ImageHandle
  )
{
  return UefiUnload (ImageHandle);
}

