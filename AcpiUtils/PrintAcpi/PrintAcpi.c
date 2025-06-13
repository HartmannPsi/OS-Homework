#include <Guid/Acpi.h>
#include <IndustryStandard/Acpi.h>
// #include <Library/AcpiLib.h>
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

EFI_STATUS PrintAllACPITables() {
  EFI_ACPI_SDT_PROTOCOL *AcpiSdt;
  EFI_STATUS Status =
      gBS->LocateProtocol(&gEfiAcpiSdtProtocolGuid, NULL, (VOID **)&AcpiSdt);
  if (EFI_ERROR(Status)) {
    Print(L"Error: Could not find ACPI SDT Protocol.\n");
    return Status;
  }

  UINTN Index = 0;
  while (TRUE) {
    EFI_ACPI_SDT_HEADER *Table;
    UINTN TableKey;
    EFI_ACPI_TABLE_VERSION Version;

    Status = AcpiSdt->GetAcpiTable(Index, &Table, &Version, &TableKey);
    if (Status == EFI_NOT_FOUND) {
      break;
    }
    if (EFI_ERROR(Status)) {
      Print(L"Error: Failed to get ACPI table at index %d.\n", Index);
      return Status;
    }

    Print(
        L"Physical Address: 0x%016lx, "
        L"Length: %u, "
        L"OEM ID: %c%c%c%c%c%c, "
        L"Checksum: 0x%02x\n",
        (UINT64)(UINTN)Table, Table->Length, Table->OemId[0], Table->OemId[1],
        Table->OemId[2], Table->OemId[3], Table->OemId[4], Table->OemId[5],
        Table->Checksum);

    Index++;
  }

  return EFI_SUCCESS;
}

EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE ImageHandle,
                           IN EFI_SYSTEM_TABLE *SystemTable) {
  return PrintAllACPITables();
}
