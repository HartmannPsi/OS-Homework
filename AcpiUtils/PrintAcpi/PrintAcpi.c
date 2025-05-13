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

// VOID FixAcpiChecksum(EFI_ACPI_DESCRIPTION_HEADER *Header) {
//   Header->Checksum = 0;
//   UINT8 Sum = 0;
//   UINT8 *TableData = (UINT8 *)Header;
//   for (UINTN i = 0; i < Header->Length; i++) {
//     Sum += TableData[i];
//   }
//   Header->Checksum = (UINT8)(0x100 - Sum);
// }

// EFI_STATUS EFIAPI ChangeACPITable(IN UINTN TableIndex,
//                                   IN EFI_ACPI_DESCRIPTION_HEADER *NewHeader)
//                                   {
//   EFI_STATUS Status;
//   EFI_ACPI_SDT_PROTOCOL *AcpiSdt;
//   EFI_ACPI_TABLE_PROTOCOL *AcpiTable;

//   // 获取 ACPI 协议
//   Status =
//       gBS->LocateProtocol(&gEfiAcpiSdtProtocolGuid, NULL, (VOID **)&AcpiSdt);
//   if (EFI_ERROR(Status)) return Status;

//   Status = gBS->LocateProtocol(&gEfiAcpiTableProtocolGuid, NULL,
//                                (VOID **)&AcpiTable);
//   if (EFI_ERROR(Status)) return Status;

//   // 定位目标表
//   EFI_ACPI_DESCRIPTION_HEADER *OldTable;
//   UINTN TableKey;
//   EFI_ACPI_TABLE_VERSION Version;
//   Status = AcpiSdt->GetAcpiTable(TableIndex, (VOID **)&OldTable, &Version,
//                                  &TableKey);
//   if (EFI_ERROR(Status)) return Status;

//   // 修复校验和
//   FixAcpiChecksum(NewHeader);

//   // 卸载旧表
//   Status = AcpiTable->UninstallAcpiTable(AcpiTable, TableKey);
//   if (EFI_ERROR(Status)) return Status;

//   // 安装新表
//   UINTN NewTableKey;
//   Status = AcpiTable->InstallAcpiTable(AcpiTable, NewHeader,
//   NewHeader->Length,
//                                        &NewTableKey);
//   if (EFI_ERROR(Status)) return Status;

//   // 持久化到 NVRAM
//   CHAR16 VarName[32];
//   UnicodeSPrint(VarName, sizeof(VarName), L"ModifiedACPITable_%d",
//   TableIndex); Status = gRT->SetVariable(
//       VarName, &gModifiedAcpiTableGuid,
//       EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
//       NewHeader->Length, NewHeader);

//   return Status;
// }

// EFI_STATUS EFIAPI AcpiTablePersistDriverEntry(
//     IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable) {
//   EFI_ACPI_TABLE_PROTOCOL *AcpiTable;
//   EFI_STATUS Status = gBS->LocateProtocol(&gEfiAcpiTableProtocolGuid, NULL,
//                                           (VOID **)&AcpiTable);
//   if (EFI_ERROR(Status)) return Status;

//   // 枚举所有持久化存储的 ACPI 表
//   CHAR16 VarName[128];
//   UINTN VarNameSize = sizeof(VarName);
//   EFI_GUID VendorGuid;
//   UINTN VarSize;
//   UINT8 *Data;

//   for (UINTN Index = 0;; Index++) {
//     VarNameSize = sizeof(VarName);
//     Status = gRT->GetNextVariableName(&VarNameSize, VarName, &VendorGuid);
//     if (Status == EFI_NOT_FOUND) break;

//     if (StrStr(VarName, L"ModifiedACPITable_") == VarName) {
//       // 读取变量数据
//       Status = gRT->GetVariable(VarName, &VendorGuid, NULL, &VarSize, NULL);
//       if (Status != EFI_BUFFER_TOO_SMALL) continue;

//       Data = AllocatePool(VarSize);
//       Status = gRT->GetVariable(VarName, &VendorGuid, NULL, &VarSize, Data);
//       if (EFI_ERROR(Status)) {
//         FreePool(Data);
//         continue;
//       }

//       // 安装持久化的表
//       UINTN NewTableKey;
//       AcpiTable->InstallAcpiTable(AcpiTable, Data, VarSize, &NewTableKey);
//       FreePool(Data);
//     }
//   }

//   return EFI_SUCCESS;
// }

EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE ImageHandle,
                           IN EFI_SYSTEM_TABLE *SystemTable) {
  return PrintAllACPITables();
}
