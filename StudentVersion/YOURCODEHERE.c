#include "YOURCODEHERE.h"

/*
  Testing approach: 
  To be completely transparent, I am not sure how to test this aside from the sample cases given.
  To write my implementation for writeback and fill, 
  I basically looked at the code in the provided helper functions and used it to 
  calculate the correct address to read/write from.
  I kept changing stuff until my output matched the sample output.
*/

unsigned int lg2pow2(uint64_t pow2){ // helper function for integer lg2; using (double) version from math is not safe
  unsigned int retval=0;
  while(pow2 != 1 && retval < 64/* -- should actually check the local VA bits, but, as seen below, if !64, will exit anyway...*/){
    pow2 = pow2 >> 1;
    ++retval;
  }
  return retval;
}

void  setSizesOffsetsAndMaskFields(cache* acache, unsigned int size, unsigned int assoc, unsigned int blocksize){
  unsigned int localVAbits=8*sizeof(uint64_t*);
  if (localVAbits!=64){
    fprintf(stderr,"Running non-portable code on unsupported platform, terminating. Please use designated machines.\n");
    exit(-1);
  }

  // YOUR CODE GOES HERE
  acache->numways=assoc; // sets associativity
  acache->blocksize=blocksize; // sets blocksize
  acache->numsets=size/(blocksize*assoc); // computes total number of sets
  acache->BO = lg2pow2(blocksize); // computes block offset
  acache->TO = acache->BO + lg2pow2(acache->numsets); // computes tag offset
  acache->VAImask= acache->numsets - 1; // computes AND mask for index
  acache->VATmask=(uint64_t)(((uint64_t)1)<<(localVAbits - acache->TO)) - (uint64_t)1; // computes AND mask for tag

}


unsigned long long getindex(cache* acache, unsigned long long address){
  return (address>>acache->BO) & (uint64_t)acache->VAImask; //Returns index bits, masking upper bits
}

unsigned long long gettag(cache* acache, unsigned long long address){
  return (address>>acache->TO) & (uint64_t)acache->VATmask; //Returns tag bits, masking upper bits
}

void writeback(cache* acache, unsigned int index, unsigned int oldestway){

  // YOUR CODE GOES HERE
  //address is computing by shifting the index left by block offset and adding the tag shifted left to the tag offset
  unsigned long long address = (index << acache->BO) | (acache->sets[index].blocks[oldestway].tag << acache->TO); 
  int i;
  //iterate through all words in block
  for (i = 0; i < (acache->blocksize / sizeof(unsigned long long)); i++){
    //get the word to be accessed
    unsigned long long value = acache->sets[index].blocks[oldestway].datawords[oldestway];
    //access the word in the next lower level of cache
    performaccess(acache->nextcache, address, sizeof(unsigned long long), 1, value, i);
    //increment address by size of word
    address += sizeof(unsigned long long);
  }
}

void fill(cache * acache, unsigned int index, unsigned int oldestway, unsigned long long address){

  // YOUR CODE GOES HERE
  //compute address
  address = address & ((acache->VATmask << acache->TO) | (acache->VAImask << acache->BO));
  int i;
  //iterate through all words in block
  for (i = 0; i < (acache->blocksize / sizeof(unsigned long long)); i++){
    //update tag value
    acache->sets[index].blocks[oldestway].tag = gettag(acache, address);
    //fetch value from cache level below
    unsigned long long value = performaccess(acache->nextcache, address, sizeof(unsigned long long), 0, 0, i);
    acache->sets[index].blocks[oldestway].datawords[i] = value;
    //increment address by size of word
    address += sizeof(unsigned long long);
  }
}
