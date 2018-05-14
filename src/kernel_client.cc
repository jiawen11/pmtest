#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
//#include <sys/socket.h>

#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include "nvmveri.hh"
// #include "nvmveri_kernel.h"
// TODO: FIX THIS HEADER WHEN WRITING KERNEL MODULE
#define BUFFER_LEN 20

FastVector<Metadata *> allocated;
Metadata buf[BUFFER_LEN];
int start_idx;

// obtain the semaphore and read a single block
/*
void read_datablock()
{
	int fd = open("/dev/nvmveri_dev", O_RDONLY);
	if (fd == -1) {
		assert(0 && "Cannot open NVMVeri file device");
	}
	read(fd, buf, sizeof(Metadata) * BUFFER_LEN);
	close(fd);
}*/

int timer = 0;
Metadata ass, delim, end;

void read_datablock()
{
	if (timer == 0) {
		int i;
		ass.assign.size = 0;
		for (i = 0; i < 15; i++) buf[i] = ass;
		buf[i] = delim; i++;
		ass.assign.size = 1;
		for (; i < BUFFER_LEN; i++) buf[i] = ass; 
	}
	else if (timer == 1) {
		int i;
		for (i = 0; i < 3; i++){
			buf[i] = ass;
		}
		buf[i] = delim; i++;
		ass.assign.size = 2;
		for (; i < 7; i++) buf[i] = ass;
		buf[i] = delim; i++;
		ass.assign.size = 3;
		for (; i < 12; i++) buf[i] = ass;
		buf[i] = delim; i++;
		ass.assign.size = 4;
		for (; i < BUFFER_LEN; i++) buf[i] = ass; 
	
	}
	else if (timer == 2) {
		int i;
		for (i = 0; i < 9; i++) buf[i] = ass;
		buf[i] = delim; i++;
		ass.assign.size = 5;
		for (; i < BUFFER_LEN - 1; i++) buf[i] = ass; 
		buf[i] = delim; i++;
		ass.assign.size = 6;
		printf("wow ");
		Metadata_print(&buf[i - 2]);
	
	}
	else {
		int i;
		for (i = 0; i < 3; i++) buf[i] = ass;
		buf[i] = delim; i++;
		buf[i] = end; i++;
		ass.assign.size = 7;
		for (; i < 6; i++) buf[i] = ass;
		buf[i] = end; i++;
	
	}
	timer++;
}

// assemble a whole transaction and return
int read_transaction(FastVector<Metadata *> *tx)
{
	while (true) {
		if (start_idx >= BUFFER_LEN) {
			read_datablock();
			start_idx = 0;
		}
		for (size_t i = start_idx; i < BUFFER_LEN; i++) {
			if (buf[i].type == _TRANSACTIONDELIM) {
				Metadata *start_ptr = new Metadata[i - start_idx];
				allocated.push_back(start_ptr);
				// insert all Metadata before index i
				for (int j = start_idx; j < i; j++) {
					start_ptr[j - start_idx] = buf[j];
					assert(&start_ptr[j - start_idx] && "Null ptr 1");
					assert(j - start_idx < i - start_idx);
					printf("Transaction 1 %d %d %p %p\n",j,start_idx,start_ptr, &start_ptr[j - start_idx]);
					tx->push_back(&start_ptr[j - start_idx]);
				}
				start_idx = i + 1;
				return 0;
			}
			if (buf[i].type == _ENDING) {
				assert(i == start_idx && "Transaction not wrapped by delimiter");
				return -1;
			}
		}
		Metadata *start_ptr = new Metadata[BUFFER_LEN - start_idx];
		allocated.push_back(start_ptr);
		for (int j = start_idx; j < BUFFER_LEN; j++) {
			start_ptr[j - start_idx] = buf[j];
			assert(&start_ptr[j - start_idx] && "Null ptr 2");
			assert(j - start_idx < BUFFER_LEN - start_idx);
			printf("Transaction 2 %d %d %p, %p\n",j,start_idx, start_ptr, &start_ptr[j - start_idx]);
			tx->push_back(&start_ptr[j - start_idx]);
		}
		start_idx = BUFFER_LEN;
	}

}

int main(int argc, char *argv[])
{
ass.type = _ASSIGN;
ass.assign.size = 1000;
delim.type = _TRANSACTIONDELIM;
end.type = _ENDING;
	NVMVeri veriInstance;
	start_idx = BUFFER_LEN;
	FastVector<FastVector<Metadata *> *> tx_vector;
	while (true) {
		FastVector<Metadata *> *tx = new FastVector<Metadata *>;
		printf("transaction head %p\n", tx);
		int flag = read_transaction(tx);
		tx_vector.push_back(tx);
		for (int i = 0; i < tx->size(); i++) {
			printf("local ");
			Metadata_print((*tx)[i]);
		}
		if (flag == -1) break;
		veriInstance.execVeri(tx);
	}
	FastVector<VeriResult> result;
	veriInstance.getVeri(result);
	for (int i = 0; i < tx_vector.size(); i++)
		delete [] allocated[i];
	for (int i = 0; i < tx_vector.size(); i++)
		delete tx_vector[i];

	return 0;
}
