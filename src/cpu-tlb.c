/*
 * Copyright (C) 2024 pdnguyen of the HCMC University of Technology
 */
/*
 * Source Code License Grant: Authors hereby grants to Licensee 
 * a personal to use and modify the Licensed Source Code for 
 * the sole purpose of studying during attending the course CO2018.
 */
//#ifdef CPU_TLB
/*
 * CPU TLB
 * TLB module cpu/cpu-tlb.c
 */
 
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>

uint32_t get_frame_num(struct pcb_t *proc, uint32_t vaddr)
{
  if (proc && proc->mm && proc->mm->page_table)
  {
    // Implement logic to get the frame number from the virtual address
    // Example implementation (assuming a simple linear mapping):
    uint32_t frame_num = vaddr / PAGESIZE;
    return frame_num;
  }
  return INVALID_FRAME_NUM;
}

struct vm_rg_struct *get_rg_struct(struct pcb_t *proc, uint32_t reg_index)
{
  if (proc && proc->mm && reg_index < PAGING_MAX_SYMTBL_SZ)
  {
    return &proc->mm->symrgtbl[reg_index];
  }
  else
  {
    return NULL;
  }
}

int tlb_change_all_page_tables_of(struct pcb_t *proc, struct memphy_struct *mp)
{
  /* Update all page table directory info and flush or wipe TLB (if needed) */
  if (proc && proc->mm && proc->mm->page_table && proc->tlb)
  {
    // Iterate over each page table entry
    for (int i = 0; i < proc->mm->page_table->size; i++)
    {
      // Update v_index with the page table index
      proc->mm->page_table->table[i].v_index = i;
    }

    // Flush the TLB if necessary
    if (proc->tlb)
    {
      tlb_flush_tlb_of(proc, mp);
    }
  }

  return 0;
}

int tlb_flush_tlb_of(struct pcb_t *proc, struct memphy_struct *mp)
{
  /* Flush TLB cached */
  if (proc && proc->tlb)
  {
    // Iterate over each TLB entry and invalidate it
    for (int i = 0; i < proc->tlb->maxsz; i++)
    {
      proc->tlb->storage[i] = 0; // Invalidate the entry
    }
  }

  return 0;
}

/*tlballoc - CPU TLB-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int tlballoc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
{
  int addr, val;

  /* By default using vmaid = 0 */
  val = __alloc(proc, 0, reg_index, size, &addr);

  /* TODO update TLB CACHED frame num of the new allocated page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/

  if (val == 0)
  {
    for (uint32_t i = addr; i < addr + size; i += PAGESIZE)
    {
      uint32_t frame_num = get_frame_num(proc, i);
      if (frame_num != INVALID_FRAME_NUM)
      {
        tlb_cache_write(proc->tlb, i, frame_num);
      }
    }
  }
  return val;
}

/*pgfree - CPU TLB-based free a region memory
 *@proc: Process executing the instruction
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int tlbfree_data(struct pcb_t *proc, uint32_t reg_index)
{
  __free(proc, 0, reg_index);

  /* TODO update TLB CACHED frame num of freed page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/

  struct vm_rg_struct *rg = get_rg_struct(proc, reg_index);
  if (rg != NULL)
  {
    for (uint32_t addr = rg->rg_start; addr < rg->rg_end; addr += PAGESIZE)
    {
      tlb_cache_write(proc->tlb, addr, INVALID_FRAME_NUM);
    }
  }
  return 0;
}

/*tlbread - CPU TLB-based read a region memory
 *@proc: Process executing the instruction
 *@source: index of source register
 *@offset: source address = [source] + [offset]
 *@destination: destination storage
 */
int tlbread(struct pcb_t *proc, uint32_t source, uint32_t offset, uint32_t *destination)
{
  /* TODO retrieve TLB CACHED frame num of accessing page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/
  /* frmnum is return value of tlb_cache_read/write value*/
  
  // Kiểm tra tính hợp lệ của proc, destination
  if (proc == NULL || destination == NULL)
  {
    return -1; // Trả về giá trị lỗi
  }

  BYTE data;
  uint32_t frame_num = INVALID_FRAME_NUM;
  uint32_t addr = proc->regs[source] + offset;
  frame_num = tlb_cache_read(proc->tlb, addr);

#ifdef IODUMP
  if (frame_num != INVALID_FRAME_NUM)
    printf("TLB hit at read region=%d offset=%d\n", source, offset);
  else
    printf("TLB miss at read region=%d offset=%d\n", source, offset);

#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif

  int val = __read(proc, 0, source, offset, &data);

  /* TODO update TLB CACHED with frame num of recent accessing page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/

  if (frame_num == INVALID_FRAME_NUM)
  {
    frame_num = get_frame_num(proc, addr);
    if (frame_num != INVALID_FRAME_NUM)
    {
      tlb_cache_write(proc->tlb, addr, frame_num);
    }
  }
  return val;
}

/*tlbwrite - CPU TLB-based write a region memory
 *@proc: Process executing the instruction
 *@data: data to be wrttien into memory
 *@destination: index of destination register
 *@offset: destination address = [destination] + [offset]
 */
int tlbwrite(struct pcb_t *proc, BYTE data, uint32_t destination, uint32_t offset)
{

  /* TODO retrieve TLB CACHED frame num of accessing page(s))*/
  /* by using tlb_cache_read()/tlb_cache_write()
  frmnum is return value of tlb_cache_read/write value*/

  uint32_t frame_num = INVALID_FRAME_NUM;
  uint32_t addr = proc->regs[destination] + offset;
  frame_num = tlb_cache_read(proc->tlb, addr);

#ifdef IODUMP
  if (frame_num != INVALID_FRAME_NUM)
    printf("TLB hit at write region=%d offset=%d value=%d\n", destination, offset, data);
  else
    printf("TLB miss at write region=%d offset=%d value=%d\n", destination, offset, data);

#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); // print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif

  int val = __write(proc, 0, destination, offset, data);

  /* TODO update TLB CACHED with frame num of recent accessing page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/

  if (frame_num == INVALID_FRAME_NUM)
  {
    frame_num = get_frame_num(proc, addr);
    if (frame_num != INVALID_FRAME_NUM)
    {
      tlb_cache_write(proc->tlb, addr, frame_num);
    }
  }
  return val;
}

//#endif
