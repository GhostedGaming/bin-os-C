#include <uacpi/kernel_api.h>
#include <uacpi/types.h>
#include <stdint.h>
#include <stddef.h>
#include "../memory.h"
#include "../serial/serial.h"
#include "../cpu/interrupts/idt.h"

// Memory management
void* uacpi_kernel_alloc(size_t size) {
    // Use your kernel's memory allocator
    return malloc(size); // Replace with your allocator
}

void uacpi_kernel_free(void* ptr) {
    // Use your kernel's free function
    free(ptr); // Replace with your free function
}

void* uacpi_kernel_map(uacpi_phys_addr addr, size_t size) {
    // Map physical address to virtual
    // This depends on your memory management
    return (void*)(addr + KERNEL_VIRTUAL_BASE); // Example - adjust for your kernel
}

void uacpi_kernel_unmap(void* addr, size_t size) {
    // Unmap virtual memory
    // Implementation depends on your memory manager
    (void)addr;
    (void)size;
}

// I/O Operations
uacpi_u8 uacpi_kernel_io_read8(uacpi_io_addr port) {
    return inb(port); // Use your port I/O functions
}

uacpi_u16 uacpi_kernel_io_read16(uacpi_io_addr port) {
    return inw(port);
}

uacpi_u32 uacpi_kernel_io_read32(uacpi_io_addr port) {
    return inl(port);
}

void uacpi_kernel_io_write8(uacpi_io_addr port, uacpi_u8 value) {
    outb(port, value);
}

void uacpi_kernel_io_write16(uacpi_io_addr port, uacpi_u16 value) {
    outw(port, value);
}

void uacpi_kernel_io_write32(uacpi_io_addr port, uacpi_u32 value) {
    outl(port, value);
}

void* uacpi_kernel_io_map(uacpi_io_addr base, size_t size) {
    // Map I/O space - may be same as regular mapping on x86
    return uacpi_kernel_map(base, size);
}

void uacpi_kernel_io_unmap(void* addr, size_t size) {
    uacpi_kernel_unmap(addr, size);
}

// PCI Operations
uacpi_u8 uacpi_kernel_pci_read8(uacpi_pci_addr addr, size_t offset) {
    // Use your PCI config space read functions
    return pci_config_read8(addr.segment, addr.bus, addr.device, addr.function, offset);
}

uacpi_u16 uacpi_kernel_pci_read16(uacpi_pci_addr addr, size_t offset) {
    return pci_config_read16(addr.segment, addr.bus, addr.device, addr.function, offset);
}

uacpi_u32 uacpi_kernel_pci_read32(uacpi_pci_addr addr, size_t offset) {
    return pci_config_read32(addr.segment, addr.bus, addr.device, addr.function, offset);
}

void uacpi_kernel_pci_write8(uacpi_pci_addr addr, size_t offset, uacpi_u8 value) {
    pci_config_write8(addr.segment, addr.bus, addr.device, addr.function, offset, value);
}

void uacpi_kernel_pci_write16(uacpi_pci_addr addr, size_t offset, uacpi_u16 value) {
    pci_config_write16(addr.segment, addr.bus, addr.device, addr.function, offset, value);
}

void uacpi_kernel_pci_write32(uacpi_pci_addr addr, size_t offset, uacpi_u32 value) {
    pci_config_write32(addr.segment, addr.bus, addr.device, addr.function, offset, value);
}

uacpi_handle uacpi_kernel_pci_device_open(uacpi_pci_addr addr) {
    // Return a handle to the PCI device - can be a pointer or ID
    return (uacpi_handle)(uintptr_t)((addr.bus << 16) | (addr.device << 8) | addr.function);
}

void uacpi_kernel_pci_device_close(uacpi_handle handle) {
    // Close PCI device handle
    (void)handle;
}

// Synchronization - Spinlocks
uacpi_handle uacpi_kernel_create_spinlock(void) {
    // Create and return spinlock handle
    // For now, return a dummy handle - implement proper spinlocks later
    return (uacpi_handle)1;
}

void uacpi_kernel_free_spinlock(uacpi_handle handle) {
    // Free spinlock
    (void)handle;
}

uacpi_cpu_flags uacpi_kernel_lock_spinlock(uacpi_handle handle) {
    // Lock spinlock and return CPU flags (for interrupt state)
    (void)handle;
    // Disable interrupts and return old state
    return 0; // Return old interrupt flags
}

void uacpi_kernel_unlock_spinlock(uacpi_handle handle, uacpi_cpu_flags flags) {
    // Unlock spinlock and restore CPU flags
    (void)handle;
    (void)flags;
    // Restore interrupt state from flags
}

// Synchronization - Mutexes  
uacpi_handle uacpi_kernel_create_mutex(void) {
    // Create mutex - return dummy handle for now
    return (uacpi_handle)1;
}

void uacpi_kernel_free_mutex(uacpi_handle handle) {
    (void)handle;
}

uacpi_status uacpi_kernel_acquire_mutex(uacpi_handle handle, uacpi_u16 timeout) {
    (void)handle;
    (void)timeout;
    return UACPI_STATUS_OK;
}

void uacpi_kernel_release_mutex(uacpi_handle handle) {
    (void)handle;
}

// Events
uacpi_handle uacpi_kernel_create_event(void) {
    return (uacpi_handle)1;
}

void uacpi_kernel_free_event(uacpi_handle handle) {
    (void)handle;
}

uacpi_bool uacpi_kernel_wait_for_event(uacpi_handle handle, uacpi_u16 timeout) {
    (void)handle;
    (void)timeout;
    return UACPI_TRUE;
}

void uacpi_kernel_signal_event(uacpi_handle handle) {
    (void)handle;
}

void uacpi_kernel_reset_event(uacpi_handle handle) {
    (void)handle;
}

// Threading and Work
uacpi_thread_id uacpi_kernel_get_thread_id(void) {
    // Return current thread ID - use 1 for single-threaded kernel
    return 1;
}

uacpi_status uacpi_kernel_schedule_work(uacpi_work_type type, uacpi_work_handler handler, uacpi_handle ctx) {
    // For simple implementation, execute immediately
    handler(ctx);
    return UACPI_STATUS_OK;
}

void uacpi_kernel_wait_for_work_completion(void) {
    // Wait for all scheduled work to complete
    // Nothing to do for immediate execution
}

// Time functions
uacpi_u64 uacpi_kernel_get_nanoseconds_since_boot(void) {
    // Return nanoseconds since boot - use your timer
    return get_system_time_ns(); // Replace with your time function
}

void uacpi_kernel_stall(uacpi_u8 usec) {
    // Busy wait for microseconds
    busy_wait_microseconds(usec); // Replace with your implementation
}

void uacpi_kernel_sleep(uacpi_u64 msec) {
    // Sleep for milliseconds
    sleep_milliseconds(msec); // Replace with your implementation
}

// Interrupts
uacpi_status uacpi_kernel_install_interrupt_handler(uacpi_u32 irq, uacpi_interrupt_handler handler, uacpi_handle ctx, uacpi_handle *out_irq_handle) {
    // Install interrupt handler
    *out_irq_handle = (uacpi_handle)(uintptr_t)irq;
    return install_irq_handler(irq, handler, ctx) ? UACPI_STATUS_OK : UACPI_STATUS_INTERNAL_ERROR;
}

uacpi_status uacpi_kernel_uninstall_interrupt_handler(uacpi_handle irq_handle, uacpi_interrupt_handler handler) {
    uacpi_u32 irq = (uacpi_u32)(uintptr_t)irq_handle;
    return uninstall_irq_handler(irq, handler) ? UACPI_STATUS_OK : UACPI_STATUS_INTERNAL_ERROR;
}

// System functions
uacpi_status uacpi_kernel_handle_firmware_request(uacpi_firmware_request *req) {
    // Handle firmware requests (usually not implemented in simple kernels)
    (void)req;
    return UACPI_STATUS_UNIMPLEMENTED;
}

void uacpi_kernel_log(uacpi_log_level level, const uacpi_char *str) {
    // Log message using your kernel's logging system
    const char* level_str;
    switch (level) {
        case UACPI_LOG_DEBUG: level_str = "DEBUG"; break;
        case UACPI_LOG_TRACE: level_str = "TRACE"; break;
        case UACPI_LOG_INFO: level_str = "INFO"; break;
        case UACPI_LOG_WARN: level_str = "WARN"; break;
        case UACPI_LOG_ERROR: level_str = "ERROR"; break;
        default: level_str = "UNKNOWN"; break;
    }
    
    serial_printf("[ACPI %s] %s\n", level_str, str);
}

uacpi_status uacpi_kernel_get_rsdp(uacpi_phys_addr *out_rsdp_addr) {
    // Find RSDP (Root System Description Pointer)
    // Usually found in EBDA or BIOS area (0xE0000-0xFFFFF)
    
    // Check EBDA first
    uint16_t ebda_seg = *(uint16_t*)0x40E;
    if (ebda_seg) {
        uacpi_phys_addr ebda_addr = ebda_seg * 16;
        // Search first 1KB of EBDA for "RSD PTR "
        for (uacpi_phys_addr addr = ebda_addr; addr < ebda_addr + 1024; addr += 16) {
            if (memcmp((void*)addr, "RSD PTR ", 8) == 0) {
                *out_rsdp_addr = addr;
                return UACPI_STATUS_OK;
            }
        }
    }
    
    // Search BIOS area 0xE0000-0xFFFFF
    for (uacpi_phys_addr addr = 0xE0000; addr < 0x100000; addr += 16) {
        if (memcmp((void*)addr, "RSD PTR ", 8) == 0) {
            *out_rsdp_addr = addr;
            return UACPI_STATUS_OK;
        }
    }
    
    return UACPI_STATUS_NOT_FOUND;
}