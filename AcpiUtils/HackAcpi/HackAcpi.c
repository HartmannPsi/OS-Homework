#include <Guid/Acpi.h>
#include <IndustryStandard/Acpi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/AcpiSystemDescriptionTable.h>
#include <Protocol/AcpiTable.h>
#include <Uefi.h>

// 自定义 GUID 用于标识存储的 ACPI 表变量
#define MODIFIED_ACPI_TABLE_GUID \
  {0x3d3a7c12, 0x3f4a, 0x4bdc, {0x92, 0x1a, 0x45, 0xab, 0x71, 0xcd, 0xef, 0x89}}

EFI_GUID gModifiedAcpiTableGuid = MODIFIED_ACPI_TABLE_GUID;

VOID FixAcpiChecksum(EFI_ACPI_DESCRIPTION_HEADER *Header) {
  Header->Checksum = 0;
  UINT8 Sum = 0;
  UINT8 *TableData = (UINT8 *)Header;
  for (UINTN i = 0; i < Header->Length; i++) {
    Sum += TableData[i];
  }
  Header->Checksum = (UINT8)(0x100 - Sum);
}

EFI_STATUS EFIAPI StaticChangeACPITable(
    IN UINTN TableIndex, IN EFI_ACPI_DESCRIPTION_HEADER *NewHeader) {
  EFI_STATUS Status;
  EFI_ACPI_SDT_PROTOCOL *AcpiSdt;
  EFI_ACPI_TABLE_PROTOCOL *AcpiTable;

  Status =
      gBS->LocateProtocol(&gEfiAcpiSdtProtocolGuid, NULL, (VOID **)&AcpiSdt);
  if (EFI_ERROR(Status)) return Status;

  Status = gBS->LocateProtocol(&gEfiAcpiTableProtocolGuid, NULL,
                               (VOID **)&AcpiTable);
  if (EFI_ERROR(Status)) return Status;

  EFI_ACPI_DESCRIPTION_HEADER *OldTable;
  UINTN TableKey;
  EFI_ACPI_TABLE_VERSION Version;
  Status = AcpiSdt->GetAcpiTable(TableIndex, (VOID **)&OldTable, &Version,
                                 &TableKey);
  if (EFI_ERROR(Status)) return Status;

  FixAcpiChecksum(NewHeader);

  Status = AcpiTable->UninstallAcpiTable(AcpiTable, TableKey);
  if (EFI_ERROR(Status)) return Status;

  UINTN NewTableKey;
  Status = AcpiTable->InstallAcpiTable(AcpiTable, NewHeader, NewHeader->Length,
                                       &NewTableKey);
  if (EFI_ERROR(Status)) return Status;

  CHAR16 VarName[32];
  UnicodeSPrint(VarName, sizeof(VarName), L"ModifiedACPITable_%d", TableIndex);
  Status = gRT->SetVariable(
      VarName, &gModifiedAcpiTableGuid,
      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
      NewHeader->Length, NewHeader);

  return Status;
}

EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE ImageHandle,
                           IN EFI_SYSTEM_TABLE *SystemTable) {
  // 模拟的示例新 ACPI 表
  EFI_ACPI_DESCRIPTION_HEADER *NewHeader =
      AllocateZeroPool(sizeof(EFI_ACPI_DESCRIPTION_HEADER));
  if (!NewHeader) {
    Print(L"[HackAcpi] Failed to allocate memory for ACPI table.\n");
    return EFI_OUT_OF_RESOURCES;
  }

  NewHeader->Signature = SIGNATURE_32('T', 'E', 'S', 'T');
  NewHeader->Length = sizeof(EFI_ACPI_DESCRIPTION_HEADER);
  NewHeader->Revision = 1;

  EFI_STATUS Status = StaticChangeACPITable(0, NewHeader);  // Index 0
  if (EFI_ERROR(Status)) {
    Print(L"[HackAcpi] Failed to replace ACPI table: %r\n", Status);
  } else {
    Print(L"[HackAcpi] ACPI table replaced successfully.\n");
  }

  FreePool(NewHeader);
  return Status;
}
