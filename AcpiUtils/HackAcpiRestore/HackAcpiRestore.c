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

  // 1. 获取 ACPI 表管理协议
  Status = gBS->LocateProtocol(&gEfiAcpiTableProtocolGuid, NULL,
                               (VOID **)&AcpiTable);
  if (EFI_ERROR(Status)) {
    Print(L"[HackAcpi] Failed to locate ACPI Table Protocol: %r\n", Status);
    return Status;
  }

  // 2. 初始化变量名缓存、变量数据缓存等
  CHAR16 VarName[128];
  UINTN VarNameSize = sizeof(VarName);
  EFI_GUID VendorGuid;
  UINTN VarSize;
  UINT8 *Data;

  // 3. 枚举所有变量名
  // Print(L"[HackAcpi] Searching for modified ACPI tables in NVRAM...\n");
  for (UINTN Index = 0;; Index++) {
    VarNameSize = sizeof(VarName);
    Status = gRT->GetNextVariableName(&VarNameSize, VarName, &VendorGuid);
    if (Status == EFI_NOT_FOUND) {
      break;  // 所有变量都遍历完
    }

    // 4. 检查是否是我们自己的变量（前缀匹配）
    if (StrStr(VarName, L"ModifiedACPITable_") == VarName) {
      // 5. 获取变量大小（第一次调用只获取大小）
      Status = gRT->GetVariable(VarName, &VendorGuid, NULL, &VarSize, NULL);
      if (Status != EFI_BUFFER_TOO_SMALL) {
        // Print(L"[HackAcpi] Skip variable: %s (cannot read size)\n", VarName);
        continue;
      }

      // 6. 分配缓冲区读取数据
      Data = AllocatePool(VarSize);
      if (Data == NULL) {
        // Print(L"[HackAcpi] Memory allocation failed for %s\n", VarName);
        continue;
      }

      Status = gRT->GetVariable(VarName, &VendorGuid, NULL, &VarSize, Data);
      if (EFI_ERROR(Status)) {
        // Print(L"[HackAcpi] Failed to read variable %s: %r\n", VarName,
        // Status);
        FreePool(Data);
        continue;
      }

      // 7. 安装读取的 ACPI 表
      UINTN NewTableKey;
      Status =
          AcpiTable->InstallAcpiTable(AcpiTable, Data, VarSize, &NewTableKey);
      if (EFI_ERROR(Status)) {
        // Print(L"[HackAcpi] Failed to install table from %s: %r\n", VarName,
        // Status);
      } else {
        Print(L"[HackAcpi] Successfully reinstalled ACPI table: %s\n", VarName);
      }

      FreePool(Data);
    }
  }

  // Print(L"[HackAcpi] Done. ACPI restoration finished.\n");
  return EFI_SUCCESS;
}
