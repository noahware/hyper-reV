/** @file
  Global Platform Configuration Data (PCD)

Copyright (c) 2015, Alex Ionescu. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Pi/PiStatusCode.h>

BOOLEAN _PCD_GET_MODE_BOOL_PcdComponentNameDisable = FALSE;
BOOLEAN _PCD_GET_MODE_BOOL_PcdDriverDiagnosticsDisable = FALSE;
BOOLEAN _PCD_GET_MODE_BOOL_PcdComponentName2Disable = FALSE;
BOOLEAN _PCD_GET_MODE_BOOL_PcdDriverDiagnostics2Disable = FALSE;
BOOLEAN _PCD_GET_MODE_BOOL_PcdUefiVariableDefaultLangDeprecate  = FALSE;
BOOLEAN _PCD_GET_MODE_BOOL_PcdUgaConsumeSupport = TRUE;
BOOLEAN _PCD_GET_MODE_BOOL_PcdVerifyNodeInList = FALSE;
BOOLEAN _PCD_GET_MODE_BOOL_PcdValidateOrderedCollection = FALSE;

UINT32 _PCD_GET_MODE_32_PcdStatusCodeValueEfiWatchDogTimerExpired = EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_EC_TIMER_EXPIRED;
UINT32 _PCD_GET_MODE_32_PcdStatusCodeValueSetVirtualAddressMap = EFI_SOFTWARE_EFI_RUNTIME_SERVICE | EFI_SW_RS_PC_SET_VIRTUAL_ADDRESS_MAP;
UINT32 _PCD_GET_MODE_32_PcdStatusCodeValueMemoryTestStarted = EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_PC_TEST;
UINT32 _PCD_GET_MODE_32_PcdStatusCodeValueUncorrectableMemoryError = EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_EC_UNCORRECTABLE;
UINT32 _PCD_GET_MODE_32_PcdStatusCodeValueRemoteConsoleError = EFI_PERIPHERAL_REMOTE_CONSOLE | EFI_P_EC_CONTROLLER_ERROR;
UINT32 _PCD_GET_MODE_32_PcdStatusCodeValueRemoteConsoleReset = EFI_PERIPHERAL_REMOTE_CONSOLE | EFI_P_PC_RESET;
UINT32 _PCD_GET_MODE_32_PcdStatusCodeValueRemoteConsoleInputError = EFI_PERIPHERAL_REMOTE_CONSOLE | EFI_P_EC_INPUT_ERROR;
UINT32 _PCD_GET_MODE_32_PcdStatusCodeValueRemoteConsoleOutputError = EFI_PERIPHERAL_REMOTE_CONSOLE | EFI_P_EC_OUTPUT_ERROR;
UINT32 _PCD_GET_MODE_32_PcdStatusCodeValueMouseInterfaceError = EFI_PERIPHERAL_MOUSE | EFI_P_EC_INTERFACE_ERROR;
UINT32 _PCD_GET_MODE_32_PcdStatusCodeValueMouseEnable = EFI_PERIPHERAL_MOUSE | EFI_P_PC_ENABLE;
UINT32 _PCD_GET_MODE_32_PcdStatusCodeValueMouseDisable = EFI_PERIPHERAL_MOUSE | EFI_P_PC_DISABLE;
UINT32 _PCD_GET_MODE_32_PcdStatusCodeValueKeyboardEnable = EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_ENABLE;
UINT32 _PCD_GET_MODE_32_PcdStatusCodeValueKeyboardDisable = EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_DISABLE;
UINT32 _PCD_GET_MODE_32_PcdStatusCodeValueKeyboardPresenceDetect = EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_PRESENCE_DETECT;
UINT32 _PCD_GET_MODE_32_PcdStatusCodeValueKeyboardReset = EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_RESET;
UINT32 _PCD_GET_MODE_32_PcdStatusCodeValueKeyboardClearBuffer = EFI_PERIPHERAL_KEYBOARD | EFI_P_KEYBOARD_PC_CLEAR_BUFFER;
UINT32 _PCD_GET_MODE_32_PcdStatusCodeValueKeyboardSelfTest = EFI_PERIPHERAL_KEYBOARD | EFI_P_KEYBOARD_PC_SELF_TEST;
UINT32 _PCD_GET_MODE_32_PcdStatusCodeValueKeyboardInterfaceError = EFI_PERIPHERAL_KEYBOARD | EFI_P_EC_INTERFACE_ERROR;
UINT32 _PCD_GET_MODE_32_PcdStatusCodeValueKeyboardInputError = EFI_PERIPHERAL_KEYBOARD | EFI_P_EC_INPUT_ERROR;
UINT32 _PCD_GET_MODE_32_PcdStatusCodeValueMouseInputError = EFI_PERIPHERAL_MOUSE | EFI_P_EC_INPUT_ERROR;
UINT32 _PCD_GET_MODE_32_PcdStatusCodeValueMouseReset = EFI_PERIPHERAL_MOUSE | EFI_P_PC_RESET;
UINT32 _PCD_GET_MODE_32_PcdStatusCodeValuePeiHandoffToDxe = EFI_SOFTWARE_PEI_CORE | EFI_SW_PEI_CORE_PC_HANDOFF_TO_NEXT;
UINT32 _PCD_GET_MODE_32_PcdStatusCodeValuePeimDispatch = EFI_SOFTWARE_PEI_CORE | EFI_SW_PC_INIT_BEGIN;
UINT32 _PCD_GET_MODE_32_PcdStatusCodeValuePeiCoreEntry = EFI_SOFTWARE_PEI_CORE | EFI_SW_PC_INIT;
UINT32 _PCD_GET_MODE_32_PcdStatusCodeValueDxeCoreEntry = EFI_SOFTWARE_DXE_CORE | EFI_SW_DXE_CORE_PC_ENTRY_POINT;
UINT32 _PCD_GET_MODE_32_PcdStatusCodeValueDxeCoreHandoffToBds = EFI_SOFTWARE_DXE_CORE | EFI_SW_DXE_CORE_PC_HANDOFF_TO_NEXT;
UINT32 _PCD_GET_MODE_32_PcdStatusCodeValueBootServiceExit = EFI_SOFTWARE_EFI_BOOT_SERVICE | EFI_SW_BS_PC_EXIT_BOOT_SERVICES;
UINT32 _PCD_GET_MODE_32_PcdStatusCodeValueDxeDriverBegin = EFI_SOFTWARE_DXE_CORE | EFI_SW_PC_INIT_BEGIN;
UINT32 _PCD_GET_MODE_32_PcdStatusCodeValueDxeDriverEnd = EFI_SOFTWARE_DXE_CORE | EFI_SW_PC_INIT_END;

UINT32 _PCD_GET_MODE_32_PcdFixedDebugPrintErrorLevel = 0xFFFFFFFF;
UINT32 _PCD_GET_MODE_32_PcdMaximumUnicodeStringLength = 1000000;
UINT32 _PCD_GET_MODE_32_PcdMaximumAsciiStringLength = 1000000;
UINT32 _PCD_GET_MODE_32_PcdMaximumLinkedListLength = 1000000;
UINT32 _PCD_GET_MODE_32_PcdMaximumDevicePathNodeCount = 0;
UINT32 _PCD_GET_MODE_32_PcdSpinLockTimeout = 10000000;

UINT8 _PCD_GET_MODE_8_PcdDebugPropertyMask = 0;
UINT32 _PCD_GET_MODE_32_PcdDebugPrintErrorLevel = 0x80000000;
UINT8 _PCD_GET_MODE_8_PcdReportStatusCodePropertyMask = 0;
UINT8 _PCD_GET_MODE_8_PcdDebugClearMemoryValue = 0xAF;
UINT8 _PCD_GET_MODE_8_PcdPerformanceLibraryPropertyMask = 0;

UINT8 _PCD_GET_MODE_8_PcdPostCodePropertyMask = 0;

UINT32 _PCD_GET_MODE_32_PcdFSBClock = 200000000;
UINT32 _PCD_GET_MODE_32_PcdUefiLibMaxPrintBufferSize = 320;

UINT16 _PCD_GET_MODE_16_PcdUefiFileHandleLibPrintBufferSize = 1536;

UINT32 _PCD_GET_MODE_32_PcdMaximumGuidedExtractHandler = 0x10;
UINT32 _PCD_GET_MODE_32_PcdUsbTransferTimeoutValue = 3000;

UINT64 _PCD_GET_MODE_64_PcdGuidedExtractHandlerTableAddress = 0x1000000;
UINT64 _PCD_GET_MODE_64_PcdIoBlockBaseAddressForIpf = 0x0ffffc000000;
UINT64 _PCD_GET_MODE_64_PcdPciExpressBaseAddress = 0xE0000000;

CHAR8* PcdUefiVariableDefaultLangCodes = "engfraengfra";
CHAR8* PcdUefiVariableDefaultLang = "eng";
CHAR8* PcdUefiVariableDefaultPlatformLangCodes = "en;fr;en-US;fr-FR";
CHAR8* PcdUefiVariableDefaultPlatformLang = "en-US";

UINT64 _PCD_GET_MODE_64_PcdUartDefaultBaudRate = 115200;
UINT8 _PCD_GET_MODE_8_PcdUartDefaultDataBits = 8;
UINT8 _PCD_GET_MODE_8_PcdUartDefaultParity = 1;
UINT8 _PCD_GET_MODE_8_PcdUartDefaultStopBits = 1;
UINT8 _PCD_GET_MODE_8_PcdDefaultTerminalType = 0;

UINT16 _PCD_GET_MODE_16_PcdHardwareErrorRecordLevel = 0;
UINT16 _PCD_GET_MODE_16_PcdPlatformBootTimeOut = 0xffff;

BOOLEAN _PCD_GET_MODE_BOOL_PcdShellLibAutoInitialize = FALSE;
UINT16 _PCD_GET_MODE_16_PcdShellPrintBufferSize = 16000;