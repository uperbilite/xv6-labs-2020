// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define BUCKETSIZE 13
#define BUFFERSIZE 5

extern uint ticks;

struct {
  struct spinlock lock;
  struct buf buf[BUFFERSIZE];
} bcache[BUCKETSIZE];

void
binit(void)
{
  for (int i = 0; i < BUCKETSIZE; i++) {
    initlock(&bcache[i].lock, "bcache");
    for (int j = 0; j < BUFFERSIZE; j++) {
      initsleeplock(&bcache[i].buf[j].lock, "buffer");
    }
  }
}

int
bhash(uint blockno)
{
  return blockno % BUCKETSIZE;
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int bucket = bhash(blockno);
  acquire(&bcache[bucket].lock);

  // Is the block already cached?
  for(int i = 0; i < BUFFERSIZE; i++){
    b = &bcache[bucket].buf[i];
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache[bucket].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  uint least = 0xffffffff;
  int leastidx = -1;
  for (int i = 0; i < BUFFERSIZE; i++) {
    b = &bcache[bucket].buf[i];
    if(b->refcnt == 0 && b->least < least) {
      least = b->least;
      leastidx = i;
    }
  }

  if (leastidx == -1) {
    panic("bget: no unused buffer for recycle");
  }

  b = &bcache[bucket].buf[leastidx];
  b->dev = dev;
  b->blockno = blockno;
  b->least = ticks;
  b->valid = 0;
  b->refcnt = 1;

  release(&bcache[bucket].lock);
  acquiresleep(&b->lock);

  return b;
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  int bucket = bhash(b->blockno);
  acquire(&bcache[bucket].lock);
  b->refcnt--;
  release(&bcache[bucket].lock);
  releasesleep(&b->lock);
}

void
bpin(struct buf *b) {
  int bucket = bhash(b->blockno);
  acquire(&bcache[bucket].lock);
  b->refcnt++;
  release(&bcache[bucket].lock);
}

void
bunpin(struct buf *b) {
  int bucket = bhash(b->blockno);
  acquire(&bcache[bucket].lock);
  b->refcnt--;
  release(&bcache[bucket].lock);
}


