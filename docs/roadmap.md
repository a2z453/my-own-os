# Kernel Roadmap (First 5 Modules)

1. **Bootstrapping & GDT**
   - Establish long mode entry assumptions from Limine.
   - Install a minimal GDT (code/data segments) and load with `lgdt`.
   - Confirm `CR0`, `CR4`, and `EFER` are configured by the bootloader for long mode.

2. **Interrupt Handling (IDT)**
   - Build IDT entries for exceptions and IRQs.
   - Load with `lidt`, enable `sti` after handlers are ready.

3. **Physical/Virtual Memory Management**
   - Parse the bootloader memory map; build a physical allocator.
   - Stand up paging structures (PML4/PDPT/PD/PT), manage `CR3` updates.

4. **Process Scheduling**
   - Define task structs and context switching.
   - Use timer IRQ for preemption; save/restore registers and stack pointers.

5. **Virtual File System (VFS)**
   - Define vnode/inode interfaces and mount table.
   - Implement a ramfs for early boot; add path resolution.
