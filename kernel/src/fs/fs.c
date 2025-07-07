#include <limine.h>

static volatile struct limine_efi_system_table_request efi_system_table_request = {
    .id = LIMINE_EFI_SYSTEM_TABLE_REQUEST,
    .revision = 0
};

struct limine_efi_system_table_request *get_efi_system_table_request(void) {
    return (struct limine_efi_system_table_request *)&efi_system_table_request;
}