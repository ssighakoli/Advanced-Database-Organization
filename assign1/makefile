CC = gcc
CFLAGS = -I
ALL_FILES = storage_mgr.h dberror.h test_helper.h
STORAGE_FILES = storage_mgr.c storage_mgr.h
ERROR_FILES = dberror.c dberror.h 

test_1: test_assign1_1.o storage_mgr.o dberror.o 
	$(CC) -g -Wall -o test_1 test_assign1_1.o storage_mgr.o dberror.o -lm

test_assign1_1.o: test_assign1_1.c $(ALL_FILES)
	$(CC) -g -Wall -c test_assign1_1.c -lm

storage_mgr.o:  $(STORAGE_FILES)
	$(CC) -g -Wall -c storage_mgr.c -lm

dberror.o: $(ERROR_FILES) 
	$(CC) -g -Wall -c dberror.c

clean: 
	$(RM) test_1 *.o *~

run_test_1:
	./test_1
