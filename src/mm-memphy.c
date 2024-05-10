//#ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Memory physical module mm/mm-memphy.c
 */

#include "mm.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

pthread_mutex_t lock_mem;

FILE *file;

/*
 *  MEMPHY_mv_csr - move MEMPHY cursor
 *  @mp: memphy struct
 *  @offset: offset
 */
int MEMPHY_mv_csr(struct memphy_struct *mp, int offset)
{
   int numstep = 0;

   mp->cursor = 0;
   while(numstep < offset && numstep < mp->maxsz){
     /* Traverse sequentially */
     mp->cursor = (mp->cursor + 1) % mp->maxsz;
     numstep++;
   }

   return 0;
}

/*
 *  MEMPHY_seq_read - read MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
int MEMPHY_seq_read(struct memphy_struct *mp, int addr, BYTE *value)
{
   if (mp == NULL)
     return -1;

   if (!mp->rdmflg)
     return -1; /* Not compatible mode for sequential read */

   MEMPHY_mv_csr(mp, addr);
   *value = (BYTE) mp->storage[addr];

   return 0;
}

/*
 *  MEMPHY_read read MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
int MEMPHY_read(struct memphy_struct * mp, int addr, BYTE *value)
{
   if (mp == NULL)
     return -1;

   if (mp->rdmflg)
      *value = mp->storage[addr];
   else /* Sequential access device */
      return MEMPHY_seq_read(mp, addr, value);

   return 0;
}

/*
 *  MEMPHY_seq_write - write MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
int MEMPHY_seq_write(struct memphy_struct * mp, int addr, BYTE value)
{

   if (mp == NULL)
     return -1;

   if (!mp->rdmflg)
     return -1; /* Not compatible mode for sequential read */

   MEMPHY_mv_csr(mp, addr);
   mp->storage[addr] = value;

   return 0;
}

/*
 *  MEMPHY_write-write MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
int MEMPHY_write(struct memphy_struct * mp, int addr, BYTE data)
{
  pthread_mutex_lock(&lock_mem);
  if (mp == NULL)
  {
    pthread_mutex_unlock(&lock_mem);
    return -1;
  }

  if (mp->rdmflg)
  {
    mp->storage[addr] = data;
    pthread_mutex_unlock(&lock_mem);
    return 0;
  }
  else /* Sequential access device */
  {
    pthread_mutex_unlock(&lock_mem);
    return MEMPHY_seq_write(mp, addr, data);
  }
}

/*
 *  MEMPHY_format-format MEMPHY device
 *  @mp: memphy struct
 */
int MEMPHY_format(struct memphy_struct *mp, int pagesz)
{
    /* This setting come with fixed constant PAGESZ */
    int numfp = mp->maxsz / pagesz;
    struct framephy_struct *newfst, *fst;
    int iter = 0;

    if (numfp <= 0)
      return -1;

    /* Init head of free framephy list */ 
    fst = malloc(sizeof(struct framephy_struct));
    fst->fpn = iter;
    mp->free_fp_list = fst;

    /* We have list with first element, fill in the rest num-1 element member*/
    for (iter = 1; iter < numfp ; iter++)
    {
       newfst =  malloc(sizeof(struct framephy_struct));
       newfst->fpn = iter;
       newfst->fp_next = NULL;
       fst->fp_next = newfst;
       fst = newfst;
    }

    return 0;
}

int MEMPHY_get_freefp(struct memphy_struct *mp, int *retfpn)
{
    pthread_mutex_lock(&lock_mem);
    struct framephy_struct *fp = mp->free_fp_list;

    if (fp == NULL)
    {
      pthread_mutex_unlock(&lock_mem);
      return -1;
    }

   *retfpn = fp->fpn;
   mp->free_fp_list = fp->fp_next;

  /* MEMPHY is iteratively used up until its exhausted
   * No garbage collector acting then it not been released
   */
  free(fp);
  pthread_mutex_unlock(&lock_mem);
  return 0;
}

int MEMPHY_dump(struct memphy_struct * mp)
{
  /*TODO dump memphy contnt mp->storage
   * for tracing the memory content */
  pthread_mutex_lock(&lock_mem);

  FILE *file = fopen("physical_memory_content.txt", "w");
  if (file == NULL)
  {
    pthread_mutex_unlock(&lock_mem);
    return -1;
  }

  struct framephy_struct *frame = mp->used_fp_list;
  while (frame != NULL)
  {
    #ifdef MEMPHY_DUMP_PASS_EMPTY_FRAME
    int haveValue = 0;
    for (int offset = 0; offset < PAGING_PAGESZ; ++offset)
      if (mp->storage[frame->fpn * PAGING_PAGESZ + offset] != 0){
        haveValue = 1;
        break;
      }
    if (haveValue == 0)
    {
      frame = frame->fp_next;
      continue;
    }
    #endif
    fprintf(file, "Frame %08x", frame->fpn);
    printf("Frame %08x", frame->fpn);
    for (int offset = 0; offset < PAGING_PAGESZ; ++offset)
    {
      if (offset % 32 == 0)
      {
        fprintf(file, "\n");
        printf("\n");
      }
      fprintf(file, "%d ", mp->storage[frame->fpn * PAGING_PAGESZ + offset]);
      printf("%d ", mp->storage[frame->fpn * PAGING_PAGESZ + offset]);
    }
    fprintf(file, "\n");
    printf("\n");
    frame = frame->fp_next;
  }

  fclose(file);
  pthread_mutex_unlock(&lock_mem);
  return 0;
}

int MEMPHY_put_freefp(struct memphy_struct *mp, int fpn)
{
   struct framephy_struct *fp = mp->free_fp_list;
   struct framephy_struct *newnode = malloc(sizeof(struct framephy_struct));

   /* Create new node with value fpn */
   newnode->fpn = fpn;
   newnode->fp_next = fp;
   mp->free_fp_list = newnode;

   return 0;
}

int MEMPHY_put_usedfp(struct memphy_struct *mp, int fpn)
{
  pthread_mutex_lock(&lock_mem);

  struct framephy_struct *newUsedFrame = malloc(sizeof(struct framephy_struct));
  newUsedFrame->fpn = fpn;
  newUsedFrame->fp_next = mp->used_fp_list;
  mp->used_fp_list = newUsedFrame;

  pthread_mutex_unlock(&lock_mem);
  return 0;
}

/*
 *  Init MEMPHY struct
 */
int init_memphy(struct memphy_struct *mp, int max_size, int randomflg)
{
  pthread_mutex_init(&lock_mem, NULL);
   mp->storage = (BYTE *)malloc(max_size*sizeof(BYTE));
   mp->maxsz = max_size;
  mp->used_fp_list = NULL;
  MEMPHY_format(mp, PAGING_PAGESZ);

   mp->rdmflg = (randomflg != 0)?1:0;

   if (!mp->rdmflg )   /* Not Ramdom acess device, then it serial device*/
      mp->cursor = 0;

   return 0;
}

//#endif
