#include <Guid/Acpi.h>
#include <IndustryStandard/Acpi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/AcpiSystemDescriptionTable.h>
#include <Protocol/AcpiTable.h>
#include <Uefi.h>

EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE ImageHandle,
                           IN EFI_SYSTEM_TABLE *SystemTable) {
  EFI_STATUS Status;
  EFI_ACPI_TABLE_PROTOCOL *AcpiTable;

  Status = gBS->LocateProtocol(&gEfiAcpiTableProtocolGuid, NULL,
                               (VOID **)&AcpiTable);
  if (EFI_ERROR(Status)) {
    Print(L"[HackAcpi] Failed to locate ACPI Table Protocol: %r\n", Status);
    return Status;
  }

  CHAR16 VarName[128];
  UINTN VarNameSize = sizeof(VarName);
  EFI_GUID VendorGuid;
  UINTN VarSize;
  UINT8 *Data;

  for (UINTN Index = 0;; Index++) {
    VarNameSize = sizeof(VarName);
    Status = gRT->GetNextVariableName(&VarNameSize, VarName, &VendorGuid);
    if (Status == EFI_NOT_FOUND) {
      break;
    }

    if (StrStr(VarName, L"ModifiedACPITable_") == VarName) {
      Status = gRT->GetVariable(VarName, &VendorGuid, NULL, &VarSize, NULL);
      if (Status != EFI_BUFFER_TOO_SMALL) {
        Print(L"[HackAcpi] Skip variable: %s (cannot read size)\n", VarName);
        continue;
      }

      Data = AllocatePool(VarSize);
      if (Data == NULL) {
        Print(L"[HackAcpi] Memory allocation failed for %s\n", VarName);
        continue;
      }

      Status = gRT->GetVariable(VarName, &VendorGuid, NULL, &VarSize, Data);
      if (EFI_ERROR(Status)) {
        Print(L"[HackAcpi] Failed to read variable %s: %r\n", VarName, Status);
        FreePool(Data);
        continue;
      }

      UINTN NewTableKey;
      Status =
          AcpiTable->InstallAcpiTable(AcpiTable, Data, VarSize, &NewTableKey);
      if (EFI_ERROR(Status)) {
        Print(L"[HackAcpi] Failed to install table from %s: %r\n", VarName,
              Status);
      } else {
        Print(L"[HackAcpi] Successfully reinstalled ACPI table: %s\n", VarName);
      }

      FreePool(Data);
    }
  }

  // Print(L"[HackAcpi] Done. ACPI restoration finished.\n");
  return EFI_SUCCESS;
}
