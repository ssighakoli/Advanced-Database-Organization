# Assignment 2: BUFFER MANAGER

- The goal of this assignment is to implement a simple buffer manager - which manages a fixed number of pages in memory that represent pages from a page file managed by the storage manager implemented in the previous assignment. <br>
- The Buffer manager should be able to handle more than one open buffer pool at the same time. However, there can only be one buffer pool for each page file. <br>
- It uses one page replacement strategy whenever the buffer pool is initialized.




## For extra credit:

- We made sure that all the buffer pool functions are thread safe.
- In addition to the two minimum page replacement strategies FIFO and LRU, we have also implemented CLOCK, LFU and LFU_K.
