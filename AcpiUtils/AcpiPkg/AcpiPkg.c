#include <Guid/Acpi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Uefi.h>

// 函数声明
VOID PrintRsdpInfo(EFI_ACPI_6_3_ROOT_SYSTEM_DESCRIPTION_POINTER *Root);
VOID PrintAcpiHeader(EFI_ACPI_DESCRIPTION_HEADER *Header);
VOID ChangeAcpiTable(IN UINTN TableIndex,
                     IN EFI_ACPI_DESCRIPTION_HEADER *NewTable);
VOID ParseXsdtEntries(EFI_ACPI_DESCRIPTION_HEADER *XSDT, UINTN EntryCount);
VOID HandleFadtTables(EFI_ACPI_6_3_FIXED_ACPI_DESCRIPTION_TABLE *FADT);

/**
  UEFI 程序入口点
**/
EFI_STATUS EFIAPI UefiMain(EFI_HANDLE ImageHandle,
                           EFI_SYSTEM_TABLE *SystemTable) {
  EFI_ACPI_6_3_ROOT_SYSTEM_DESCRIPTION_POINTER *Rsdp = NULL;
  EFI_ACPI_DESCRIPTION_HEADER *Xsdt = NULL;
  UINTN EntryCount = 0;

  // 遍历系统配置表，找到 ACPI 表项
  for (UINTN i = 0; i < gST->NumberOfTableEntries; i++) {
    EFI_CONFIGURATION_TABLE *ConfigTable = &gST->ConfigurationTable[i];
    if (CompareGuid(&gEfiAcpiTableGuid, &ConfigTable->VendorGuid) ||
        CompareGuid(&gEfiAcpi20TableGuid, &ConfigTable->VendorGuid)) {
      Print(L"ACPI Table:\n");

      Rsdp = (EFI_ACPI_6_3_ROOT_SYSTEM_DESCRIPTION_POINTER *)
                 ConfigTable->VendorTable;
      Xsdt = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)(Rsdp->XsdtAddress);

      PrintRsdpInfo(Rsdp);
      PrintAcpiHeader(Xsdt);

      EntryCount =
          (Xsdt->Length - sizeof(EFI_ACPI_DESCRIPTION_HEADER)) / sizeof(UINT64);

      ParseXsdtEntries(Xsdt, EntryCount);
      break;
    }
  }

  Print(L"----------------MODIFY-----------------\n");

  // 尝试修改两个 ACPI 表（占位）
  EFI_ACPI_DESCRIPTION_HEADER DummyTable = {0};
  ChangeAcpiTable(1, &DummyTable);
  ChangeAcpiTable(2, &DummyTable);

  Print(L"Done Successfully.\n");
  return EFI_SUCCESS;
}

/**
  打印 RSDP 信息
**/
VOID PrintRsdpInfo(EFI_ACPI_6_3_ROOT_SYSTEM_DESCRIPTION_POINTER *Root) {
  Print(L"RSDP:\n");
  Print(L"Addr: 0x%x\n", Root);
  Print(L"Length: %d\n", Root->Length);
  Print(L"OEM_ID: ");
  for (UINTN i = 0; i < 6; i++) {
    Print(L"%c", Root->OemId[i]);
  }
  Print(L"\nChecksum: 0x%x\n", Root->Checksum);
}

/**
  打印 ACPI 表头信息
**/
VOID PrintAcpiHeader(EFI_ACPI_DESCRIPTION_HEADER *Header) {
  Print(L"Signature: ");
  for (UINTN i = 0; i < 4; i++) {
    Print(L"%c", (Header->Signature >> (i * 8)) & 0xff);
  }
  Print(L"\n");
  Print(L"Addr: 0x%x\n", Header);
  Print(L"Length: %d\n", Header->Length);
  Print(L"OEM_ID: ");
  for (UINTN i = 0; i < 6; i++) {
    Print(L"%c", Header->OemId[i]);
  }
  Print(L"\nChecksum: 0x%x\n", Header->Checksum);
}

/**
  遍历并打印 XSDT 中每个表的头信息，如果是 FADT 则进一步处理 FACS 和 DSDT
**/
VOID ParseXsdtEntries(EFI_ACPI_DESCRIPTION_HEADER *XSDT, UINTN EntryCount) {
  UINT64 *EntryPtr = (UINT64 *)(XSDT + 1);
  for (UINTN j = 0; j < EntryCount; j++, EntryPtr++) {
    EFI_ACPI_DESCRIPTION_HEADER *Entry =
        (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)(*EntryPtr);
    if (Entry == NULL) continue;

    PrintAcpiHeader(Entry);

    // 如果当前表是 FADT，则打印其关联的 FACS 和 DSDT
    if (Entry->Signature == SIGNATURE_32('F', 'A', 'C', 'P')) {
      HandleFadtTables((EFI_ACPI_6_3_FIXED_ACPI_DESCRIPTION_TABLE *)Entry);
    }
  }
}

/**
  打印 FADT 中的 FACS 和 DSDT 表头信息
**/
VOID HandleFadtTables(EFI_ACPI_6_3_FIXED_ACPI_DESCRIPTION_TABLE *FADT) {
  EFI_ACPI_DESCRIPTION_HEADER *FACS =
      (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)(FADT->FirmwareCtrl);
  EFI_ACPI_DESCRIPTION_HEADER *DSDT =
      (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)(FADT->XDsdt);

  if (FACS) PrintAcpiHeader(FACS);
  if (DSDT) PrintAcpiHeader(DSDT);
}

/**
  替换指定索引的 ACPI 表
**/
VOID ChangeAcpiTable(IN UINTN TableIndex,
                     IN EFI_ACPI_DESCRIPTION_HEADER *NewTable) {
  Print(L"Changing ACPI Table at index %d...\n", TableIndex);

  EFI_ACPI_DESCRIPTION_HEADER *TargetTable = NULL;

  for (UINTN i = 0; i < gST->NumberOfTableEntries; i++) {
    EFI_CONFIGURATION_TABLE *ConfigTable = &gST->ConfigurationTable[i];

    if (CompareGuid(&gEfiAcpiTableGuid, &ConfigTable->VendorGuid) ||
        CompareGuid(&gEfiAcpi20TableGuid, &ConfigTable->VendorGuid)) {
      if (TableIndex == 0) {
        TargetTable = (EFI_ACPI_DESCRIPTION_HEADER *)ConfigTable->VendorTable;
        break;
      }
      TableIndex--;
    }
  }

  if (!TargetTable) return;

  if (NewTable) {
    CopyMem(TargetTable, NewTable, NewTable->Length);
  }

  // 校验校验和
  UINT8 Checksum = 0;
  UINT8 *Bytes = (UINT8 *)TargetTable;
  for (UINTN i = 0; i < TargetTable->Length; i++) {
    Checksum += Bytes[i];
  }

  if (Checksum != 0) Print(L"Invalid Checksum: %d\n", Checksum);
}
