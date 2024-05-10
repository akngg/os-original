/*
 * Copyright (C) 2024 pdnguyen of the HCMC University of Technology
 */
/*
 * Source Code License Grant: Authors hereby grants to Licensee 
 * a personal to use and modify the Licensed Source Code for 
 * the sole purpose of studying during attending the course CO2018.
 */
//#ifdef MM_TLB
/*
 * Memory physical based TLB Cache
 * TLB cache module tlb/tlbcache.c
 *
 * TLB cache is physically memory phy
 * supports random access 
 * and runs at high speed
 */


#include "mm.h"
#include <stdlib.h>

#define init_tlbcache(mp,sz,...) init_memphy(mp, sz, (1, ##__VA_ARGS__))

/*
 * tlb_cache_read read TLB cache device
 * @mp: memphy struct
 * @addr: virtual address
 * @value: obtained value
 */
int tlb_cache_read(struct memphy_struct *mp, uint32_t addr)
{
   if (mp && mp->storage)
   {
      /* TODO: the identify info is mapped to
       * cache line by employing:
       * direct mapped, associated mapping etc.
       */
      int index = (addr / PAGING_PAGESZ) % mp->maxsz; // Direct-mapped cache
      int val = mp->storage[index];
      if (val >= 0)
         return val;
      else
         return -1;
   }
   return -1;
}

/*
 * tlb_cache_write write TLB cache device
 * @mp: memphy struct
 * @addr: virtual address
 * @value: obtained value
 */
int tlb_cache_write(struct memphy_struct *mp, uint32_t addr, uint32_t value)
{
   if (mp == NULL || mp->storage == NULL){
      return -1;
   }
      else{
      /* TODO: the identify info is mapped to
       * cache line by employing:
       * direct mapped, associated mapping etc.
       */
         int index = (addr / PAGING_PAGESZ) % mp->maxsz; // Direct-mapped cache
         mp->storage[index] = value;
         return 0;
      }
}

/*
 * TLBMEMPHY_read natively supports MEMPHY device interfaces
 * @mp: memphy struct
 * @addr: address
 * @value: obtained value
 */
int TLBMEMPHY_read(struct memphy_struct *mp, int addr, BYTE *value)
{
   if (mp == NULL || mp->storage == NULL)
      return -1;
      else{
      /* TLB cached is random access by native */
      *value = mp->storage[addr];
      return 0;
      }
}

/*
 * TLBMEMPHY_write natively supports MEMPHY device interfaces
 * @mp: memphy struct
 * @addr: address
 * @data: written data
 */
int TLBMEMPHY_write(struct memphy_struct *mp, int addr, BYTE data)
{
   if (mp == NULL || mp->storage == NULL)
      return -1;
   else{
      /* TLB cached is random access by native */
      mp->storage[addr] = data;
      return 0;
      }
}

/*
 * TLBMEMPHY_dump natively supports MEMPHY device interfaces
 * @mp: memphy struct
 */
int TLBMEMPHY_dump(struct memphy_struct *mp)
{
   /*TODO dump memphy contnt mp->storage 
    *     for tracing the memory content
    */
   if (mp && mp->storage)
   {
      for (int i = 0; i < mp->maxsz; i++)
      {
         printf("%02x", mp->storage[i]);
         if ((i + 1) % 16 == 0)
            printf("\n");
      }
      return 0;
   }
   else
      return -1;
}

/*
 * Init TLBMEMPHY struct
 */
int init_tlbmemphy(struct memphy_struct *mp, int max_size)
{
   if (mp)
   {
      mp->storage = (BYTE *)malloc(max_size * sizeof(BYTE));
      mp->maxsz = max_size;
      mp->rdmflg = 1;
      mp->cursor = 0;
      free(mp->free_fp_list);
      free(mp->used_fp_list);
      mp->free_fp_list = NULL;
      mp->used_fp_list = NULL;
      return 0;
   }
   else
      return -1;
}
// #endif