#include "my_vm.h"
#include "defines.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "string.h"
#include <stdbool.h>

#define get_pde_bits(x, pde_width) ((x) >> (ADDRESS_BITS - pde_width))
#define get_pte_bits(x, pte_width, offset) ((x >> (offset)) & ((1UL << (pte_width)) - 1))
#define get_offset_bits(x, offset) ((x) & ((1UL << offset) - 1))

#define DISK_MEMORY 1024*1024*4 //4GB
bool memory_initialized = false;
void* disk_memory = NULL;
void* physical_memory = NULL;
uint8_t* disk_bitmap = NULL;
uint8_t* pm_bitmap = NULL;
uint8_t* vm_bitmap = NULL;
uint8_t* global_pgdir = NULL;
unsigned long num_vm_pages = 0;
unsigned long num_pm_pages = 0;
/*
Function responsible for allocating and setting your simulated physical memory and disk space
*/
void initMemoryAndDisk() {

    //Allocate physical memory and disk space using mmap or malloc; this is the total size of
    //your memory/disk you are simulating
    disk_memory = malloc(DISK_SIZE);
    physical_memory = malloc(PM_SIZE);

    num_vm_pages = VM_SIZE / PAGE_SIZE;
    int pm_page_num = PM_SIZE / PAGE_SIZE;
    int disk_page_num = DISK_SIZE / PAGE_SIZE;
    
    disk_bitmap = malloc((disk_page_num + 7)/8);
    pm_bitmap = malloc((pm_page_num + 7) / 8);
    vm_bitmap = malloc((num_vm_pages + 7) / 8);

    memset(disk_bitmap,0, (disk_page_num + 7)/8);
    memset(pm_bitmap, 0, (pm_page_num + 7) / 8);
    memset(vm_bitmap, 0, (num_vm_pages + 7) / 8);
    
    printf("Memory and disk initialized: %d physical pages, %d disk pages\n",
        pm_page_num, disk_page_num);
    //HINT: Also calculate the number of physical and virtual pages and allocate
    //virtual and physical bitmaps and initialize them

}

void* get_physical_address(uint32_t phys_addr) {
    return physical_memory + phys_addr;
}

/*
The function takes a virtual address and page directories starting address and
performs translation to return the physical address
*/

/*
pde: [31]valid ; [19:0]page table index
pte: [31]valid ; [17:0]physical page number    
*/

pte_t * translate(pde_t *pgdir, void *va) {
    //HINT: Get the Page directory index (1st level) Then get the
    //2nd-level-page table index using the virtual address.  Using the page
    //directory index and page table index get the physical address
    
    //If translation not successfull
    if(pgdir == NULL || va == NULL) return NULL; 
    uint32_t pd_index = get_pde_bits((uintptr_t)va, 10);
    uint32_t pt_index = get_pte_bits((uintptr_t)va, 10, 12);
    uint32_t offset = get_offset_bits((uintptr_t)va, 12);

    pde_t pde = pgdir[pd_index];
    
    if((pde & 0x80000000) == 0) return NULL;

    pte_t* page_table = (pte_t*) get_physical_address((pde & 0xFFFFF) << 12);
    pte_t pte = page_table[pt_index];

    if ((pte & 0x80000000) == 0) return NULL;
    pte_t* phy_addr = ((pte_t*) get_physical_address((pte & 0x3FFFF) << 12) + offset);
    return phy_addr;   
}

void set_bit(uint8_t* bitmap, int n){ bitmap[n/8] |= (1 << (n % 8));}
int get_bit(uint8_t* bitmap, int n){ return (bitmap[n/8] >> (n%8)) & 1;}
void* allocate_physical_page()
{
    int pm_page_num = PM_SIZE / PAGE_SIZE;
    if (!pm_bitmap || !physical_memory) {
        fprintf(stderr, "Error: Physical memory or bitmap not initialized.\n");
        return NULL;
    }

    for(int i = 0; i < pm_page_num; i++)
    {
        if (get_bit(pm_bitmap, i) == 0) {
            set_bit(pm_bitmap, i);
            return (void*)((uintptr_t)physical_memory + i * PAGE_SIZE);
        }
    }
    fprintf(stderr, "Error: Out of physical memory.\n");
    return NULL; // 没有空闲页
}

/*
The function takes a page directory address, virtual address, physical address
as an argument, and sets a page table entry. This function will walk the page
directory to see if there is an existing mapping for a virtual address. If the
virtual address is not present, then a new entry will be added
*/
int
pageMap(pde_t *pgdir, void *va, void *pa)
{

    /*HINT: Similar to translate(), find the page directory (1st level)
    and page table (2nd-level) indices. If no mapping exists, set the
    virtual to physical mapping */
    if(pgdir == NULL || va == NULL || pa == NULL) return -1;
    
    uint32_t pd_index = get_pde_bits((uintptr_t)va, 10);
    uint32_t pt_index = get_pte_bits((uintptr_t)va, 10, 12);

    pde_t pde = pgdir[pd_index];
    pte_t *page_table;
    
    if ((pde & 0x80000000) == 0) {
        // page directory is invalid
        void* new_pt_phys_addr = allocate_physical_page();
        if (new_pt_phys_addr == NULL) {
            fprintf(stderr, "pageMap: Failed to allocate physical page for page table.\n");
            return -1; // 物理内存不足
        }

        memset(new_pt_phys_addr, 0, PAGE_SIZE);

        uint32_t pt_pfn = ((uintptr_t) new_pt_phys_addr - (uintptr_t)physical_memory) >> 12;
        pde = 0x80000000 | (pt_pfn & 0xFFFFF);
        page_table = (pte_t*) new_pt_phys_addr;
    } else {
        uint32_t pt_pfn = (pde & 0xFFFFF);
        page_table = (pte_t*) get_physical_address(pt_pfn << 12);
    }
    uint32_t pa_pfn = ((uintptr_t)pa - (uintptr_t)physical_memory) >> 12;
    page_table[pt_index] = 0x80000000 | (pa_pfn & 0x3FFFF);
    return 0;
}

int initialize_vm_system()
{
    if(memory_initialized) return 0;
    initMemoryAndDisk();
    num_pm_pages = PM_SIZE / PAGE_SIZE;
   
    // 分配并初始化页目录 (需要一个物理页)
    global_pgdir = (pde_t *)allocate_physical_page();
    if (global_pgdir == NULL) {
        fprintf(stderr, "Initialization Error: Failed to allocate page directory.\n");
        free(vm_bitmap); // 清理
        vm_bitmap = NULL;
        return -1;
    }
    memset(global_pgdir, 0, PAGE_SIZE);

    memory_initialized = true;
    printf("VM System Initialized. PGDIR at PA: %p\n", (void*)global_pgdir);
    return 0;
}

int find_free_virtual_pages(unsigned int num_pages)
{
    int consecutive_free = 0;
    for(int i = 0; i < num_vm_pages; i++)
    {
        if (get_bit(vm_bitmap, i) == 0) {
            consecutive_free++;
        } else {
            consecutive_free = 0;
        }
        if(consecutive_free >= num_pages){
            return i - num_pages + 1;
        }
    }
    return -1;
}

/* Function responsible for allocating pages
and used by the benchmark
*/
void *myMalloc(unsigned int num_bytes) {

    //HINT: If the physical memory is not yet initialized, then allocate and initialize.
    if (!memory_initialized) {
        if (initialize_vm_system() != 0) {
            return NULL;
        }
    }

    if (num_bytes == 0) {
        return NULL;
    }
   /* HINT: If the page directory is not initialized, then initialize the
   page directory. Next, using get_next_avail(), check if there are free pages. If
   free pages are available, set the bitmaps and map a new page. Note, you will 
   have to mark which physical pages are used. */
    unsigned int num_pages_needed = (num_bytes + PAGE_SIZE - 1) / PAGE_SIZE;

    int start_vpn = find_free_virtual_pages(num_pages_needed);
     if (start_vpn == -1) {
        fprintf(stderr, "myMalloc: Not enough contiguous virtual address space for %u bytes.\n", num_bytes);
        return NULL; // 没有足够的连续虚拟空间
    }
    void* start_va = (void *)((uintptr_t) start_vpn * PAGE_SIZE);
    void **allocated_pas = malloc(num_pages_needed * sizeof(void*)); // 临时存储 PA，以便清理
    if (!allocated_pas) {
        fprintf(stderr, "myMalloc: Failed to allocate temporary PA list.\n");
        return NULL;
    }
    for(int i = 0; i < num_pages_needed; i++)
    {
        void* current_va = (void *)((uintptr_t)start_va + i*PAGE_SIZE);
        int current_vpn = start_vpn + i;

        set_bit(vm_bitmap, current_vpn);
        void *pa = allocate_physical_page();
        if (pa == NULL) {
            fprintf(stderr, "myMalloc: Out of physical memory. Cleaning up.\n");
            // 清理已分配的资源
            for (int j = 0; j < i; j++) {
                free_physical_page(allocated_pas[j]);
                clear_bit(vm_bitmap, start_vpn + j);
                // 注意: 我们还没有映射，所以不需要取消映射
            }
            clear_bit(vm_bitmap, current_vpn); // 清理当前失败的虚拟页标记
            free(allocated_pas);
            return NULL;
        }
        allocated_pas[i] = pa; // 存储 PA
    }
    return NULL;
}

/* Responsible for releasing one or more memory pages using virtual address (va)
*/
void myFree(void *va, int size) {

    //Free the page table entries starting from this virtual address (va)
    // Also mark the pages free in the bitmap
    //Only free if the memory from "va" to va+size is valid
}


/* The function copies data pointed by "val" to physical
 * memory pages using virtual address (va)
*/
void myWrite(void *va, void *val, int size) {

    /* HINT: Using the virtual address and translate(), find the physical page. Copy
       the contents of "val" to a physical page. NOTE: The "size" value can be larger
       than one page. Therefore, you may have to find multiple pages using translate()
       function.*/

}


/*Given a virtual address, this function copies the contents of the page to val*/
void myRead(void *va, void *val, int size) {

    /* HINT: put the values pointed to by "va" inside the physical memory at given
    "val" address. Assume you can access "val" directly by derefencing them.
    If you are implementing TLB,  always check first the presence of translation
    in TLB before proceeding forward */


}
