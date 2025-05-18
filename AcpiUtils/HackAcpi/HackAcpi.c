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

  // 获取 ACPI 协议
  Status =
      gBS->LocateProtocol(&gEfiAcpiSdtProtocolGuid, NULL, (VOID **)&AcpiSdt);
  if (EFI_ERROR(Status)) return Status;

  Status = gBS->LocateProtocol(&gEfiAcpiTableProtocolGuid, NULL,
                               (VOID **)&AcpiTable);
  if (EFI_ERROR(Status)) return Status;

  // 定位目标表
  EFI_ACPI_DESCRIPTION_HEADER *OldTable;
  UINTN TableKey;
  EFI_ACPI_TABLE_VERSION Version;
  Status = AcpiSdt->GetAcpiTable(TableIndex, (VOID **)&OldTable, &Version,
                                 &TableKey);
  if (EFI_ERROR(Status)) return Status;

  // 修复校验和
  FixAcpiChecksum(NewHeader);

  // 卸载旧表
  Status = AcpiTable->UninstallAcpiTable(AcpiTable, TableKey);
  if (EFI_ERROR(Status)) return Status;

  // 安装新表
  UINTN NewTableKey;
  Status = AcpiTable->InstallAcpiTable(AcpiTable, NewHeader, NewHeader->Length,
                                       &NewTableKey);
  if (EFI_ERROR(Status)) return Status;

  // 持久化到 NVRAM
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
  Print(L"[HackAcpi] UEFI application started.\n");

  // 构造一个模拟的新 ACPI 表
  EFI_ACPI_DESCRIPTION_HEADER *NewHeader =
      AllocateZeroPool(sizeof(EFI_ACPI_DESCRIPTION_HEADER));
  if (!NewHeader) {
    Print(L"[HackAcpi] Failed to allocate memory for ACPI table.\n");
    return EFI_OUT_OF_RESOURCES;
  }

  // 填写表内容（这是个示例，你可以换成你真实的表内容）
  NewHeader->Signature = SIGNATURE_32('T', 'E', 'S', 'T');
  NewHeader->Length = sizeof(EFI_ACPI_DESCRIPTION_HEADER);
  NewHeader->Revision = 1;

  EFI_STATUS Status = StaticChangeACPITable(0, NewHeader);  // Index 0
  // 只是演示
  if (EFI_ERROR(Status)) {
    Print(L"[HackAcpi] Failed to replace ACPI table: %r\n", Status);
  } else {
    Print(L"[HackAcpi] ACPI table replaced successfully.\n");
  }

  FreePool(NewHeader);
  return Status;
}

// EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE ImageHandle,
//                            IN EFI_SYSTEM_TABLE *SystemTable) {
//   EFI_STATUS Status;
//   EFI_ACPI_TABLE_PROTOCOL *AcpiTable;

//   // 1. 获取 ACPI 表管理协议
//   Status = gBS->LocateProtocol(&gEfiAcpiTableProtocolGuid, NULL,
//                                (VOID **)&AcpiTable);
//   if (EFI_ERROR(Status)) {
//     Print(L"[HackAcpi] Failed to locate ACPI Table Protocol: %r\n", Status);
//     return Status;
//   }

//   // 2. 初始化变量名缓存、变量数据缓存等
//   CHAR16 VarName[128];
//   UINTN VarNameSize = sizeof(VarName);
//   EFI_GUID VendorGuid;
//   UINTN VarSize;
//   UINT8 *Data;

//   // 3. 枚举所有变量名
//   Print(L"[HackAcpi] Searching for modified ACPI tables in NVRAM...\n");
//   for (UINTN Index = 0;; Index++) {
//     VarNameSize = sizeof(VarName);
//     Status = gRT->GetNextVariableName(&VarNameSize, VarName, &VendorGuid);
//     if (Status == EFI_NOT_FOUND) {
//       break;  // 所有变量都遍历完
//     }

//     // 4. 检查是否是我们自己的变量（前缀匹配）
//     if (StrStr(VarName, L"ModifiedACPITable_") == VarName) {
//       // 5. 获取变量大小（第一次调用只获取大小）
//       Status = gRT->GetVariable(VarName, &VendorGuid, NULL, &VarSize, NULL);
//       if (Status != EFI_BUFFER_TOO_SMALL) {
//         Print(L"[HackAcpi] Skip variable: %s (cannot read size)\n", VarName);
//         continue;
//       }

//       // 6. 分配缓冲区读取数据
//       Data = AllocatePool(VarSize);
//       if (Data == NULL) {
//         Print(L"[HackAcpi] Memory allocation failed for %s\n", VarName);
//         continue;
//       }

//       Status = gRT->GetVariable(VarName, &VendorGuid, NULL, &VarSize, Data);
//       if (EFI_ERROR(Status)) {
//         Print(L"[HackAcpi] Failed to read variable %s: %r\n", VarName,
//         Status); FreePool(Data); continue;
//       }

//       // 7. 安装读取的 ACPI 表
//       UINTN NewTableKey;
//       Status =
//           AcpiTable->InstallAcpiTable(AcpiTable, Data, VarSize,
//           &NewTableKey);
//       if (EFI_ERROR(Status)) {
//         Print(L"[HackAcpi] Failed to install table from %s: %r\n", VarName,
//               Status);
//       } else {
//         Print(L"[HackAcpi] Successfully reinstalled ACPI table: %s\n",
//         VarName);
//       }

//       FreePool(Data);
//     }
//   }

//   Print(L"[HackAcpi] Done. ACPI restoration finished.\n");
//   return EFI_SUCCESS;
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
