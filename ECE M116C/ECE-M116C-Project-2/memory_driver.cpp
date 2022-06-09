#include <fstream>
#include <iostream>
#include <cstring>
#include <sstream>
#include <iostream>
#include <vector>
#include <bitset>
#include <string>
#include <tgmath.h>

#define CACHE_SETS 16
#define MEM_SIZE 2048
#define CACHE_WAYS 1
#define BLOCK_SIZE 1 // bytes per block
#define DM 0
#define FA 1
#define SA 2

using namespace std;
struct cache_set
{
	int tag; // you need to compute offset and index to find the tag.
	int lru_position; // for FA and SA only
	int data; // the actual data stored in the cache/memory
	bool updated; // used in LRU to keep track of updated LRU positions 
};

struct trace
{
	bool MemR; 
	bool MemW; 
	int adr; 
	int data; 
};

/*
Either implement your memory_controller here or use a separate .cpp/.c file for memory_controller and all the other functions inside it (e.g., LW, SW, Search, Evict, etc.)
*/

int binary_string_to_decimal(string s);
int memory_controller(bool cur_MemR, bool cur_MemW, int &cur_data, int cur_adr, int status, int &miss, int mode, cache_set myCache[CACHE_SETS], int myMem[MEM_SIZE]);
bool Search(bool cur_MemR, bool cur_MemW, int &cur_adr, int &cur_data, int mode, cache_set myCache[CACHE_SETS]);
void CacheMiss(int &cur_adr, int &cur_data, int myMem[MEM_SIZE], cache_set myCache[CACHE_SETS], int mode);
void Evict(int cur_adr, int cur_data, cache_set myCache[CACHE_SETS], int mode);
void Update(cache_set myCache[CACHE_SETS], int index);
void Adjust(cache_set myCache[CACHE_SETS], int index);

int main (int argc, char* argv[]) // the program runs like this: ./program <filename> <mode>
{
	// input file (i.e., test.txt)
	string filename = argv[1];

	// mode for replacement policy
	int mode;
	
	ifstream fin;

	// opening file
	fin.open(filename.c_str());
	if (!fin){ // making sure the file is correctly opened
		cout << "Error opening " << filename << endl;
		exit(1);
	}
	
	if (argc > 2)  
		mode = stoi(argv[2]); // the input could be either 0 or 1 or 2 (for DM and FA and SA)
	else mode = 0;// the default is DM.
	

	// reading the text file
	string line;
	vector<trace> myTrace;
	int TraceSize = 0;
	string s1,s2,s3,s4;
    while( getline(fin,line) )
      	{
            stringstream ss(line);
            getline(ss,s1,','); 
            getline(ss,s2,','); 
            getline(ss,s3,','); 
            getline(ss,s4,',');
            myTrace.push_back(trace()); 
            myTrace[TraceSize].MemR = stoi(s1);
            myTrace[TraceSize].MemW = stoi(s2);
            myTrace[TraceSize].adr = stoi(s3);
            myTrace[TraceSize].data = stoi(s4);
            TraceSize+=1;
        }


	// defining a fully associative or direct-mapped cache
	cache_set myCache[CACHE_SETS]; // 1 set per line. 1B per Block
	int myMem[MEM_SIZE];


	// initializing
	for (int i = 0; i < CACHE_SETS; i++)
	{
		myCache[i].tag = -1; // -1 indicates that the tag value is invalid. We don't use a separate VALID bit. 
		myCache[i].lru_position = 0;  // 0 means lowest position
		myCache[i].data = 0;
		myCache[i].updated = false;
	}

	// counters for miss rate
	int accessL = 0; // number of LW access
	int accessS = 0; // number of SW access 
	int miss = 0; // this has to be updated inside your memory_controller
	int status = 1; 
	int clock = 0;
	int traceCounter = 0;
	bool cur_MemR; 
	bool cur_MemW; 
	int cur_adr;
	int cur_data;
	bool hit; 
	// this is the main loop of the code
	while (traceCounter < TraceSize){
		
		while (status < 1) {
			status++;
			clock++;
		}

		if (status == 0) {
			cur_data = myMem[cur_adr];
			status = 1; 
		}

		if (status == 1)
		{
			cur_MemR = myTrace[traceCounter].MemR;
			cur_MemW = myTrace[traceCounter].MemW;
			cur_data = myTrace[traceCounter].data;
			cur_adr = myTrace[traceCounter].adr;
			traceCounter += 1;
			hit = false; 
			if (cur_MemR == 1)
				accessL += 1;
			else if (cur_MemW == 1)
				accessS += 1;
		}

		status = memory_controller(cur_MemR, cur_MemW, cur_data, cur_adr, status, miss, mode, myCache, myMem); 
		clock += 1; 

		// clean cache replacement for new instruction 
		for (int i = 0; i < CACHE_SETS; i++) {
				myCache[i].updated = false;
		}	
	}


	// to make sure that the last access is also done
	while (status < 1) 
	{
		status++;
		clock += 1;
	}


	float miss_rate = miss / (float)accessL; 

	cout << "Number of LW: " << accessL << endl;
	cout << "Number of SW: " << accessS << endl;
	cout << "Total number of Instructions: " << TraceSize;

	// printing the final result
	cout << "(" << clock << ", " << miss_rate << ")" << endl;

	// closing the file
	fin.close();

	return 0;
}

// helper function to convert a binary string to a decimal 
int binary_string_to_decimal(string s) {
    int decimal = 0; 
    int l = s.length(); 
    for (int i = 0; i < l; i++) {
        if (s[i] == '1') {
            decimal += pow(2, l-i-1);
        }
    }
    return decimal; 
}

int memory_controller(bool cur_MemR, bool cur_MemW, int &cur_data, int cur_adr, int status, int &miss, int mode, cache_set myCache[CACHE_SETS], int myMem[MEM_SIZE]) {
	
	bool hit; 
	int new_status;

	// LW instructions 
	if (cur_MemR == 1) {
		hit = Search(cur_MemR, cur_MemW, cur_adr, cur_data, mode, myCache); 

		// a match was found in the cache 
		if (hit) {
			new_status = 1;
		}

		// no match was found in the cache (cache miss)
		if (!hit) {
			miss++;
			new_status = -3; 
			CacheMiss(cur_adr, cur_data, myMem, myCache, mode);
		}
	}

	// SW instructions 
	if (cur_MemW == 1) {
		hit = Search(cur_MemR, cur_MemW, cur_adr, cur_data, mode, myCache); 

		// update the main memory 
		myMem[cur_adr] = cur_data; 
		new_status = 1;
		cur_data = 0;
	}

	return new_status;
}

bool Search(bool cur_MemR, bool cur_MemW, int &cur_adr, int &cur_data, int mode, cache_set myCache[CACHE_SETS]) {
	string str_index, str_tag, str_offset; 
	int index, tag, offset; 
	int stored_tag;

	// get 32 bit representation of address to calculate tag, index, offset 
	bitset<32> binary(cur_adr);

	// get string representatiom of 32 bit binary 
	string str_addr = binary.to_string();

	// DM (tag = 28, index = 4, offset = 0)
	if (mode == DM) {
		str_tag = str_addr.substr(0, 28);
		str_index = str_addr.substr(28, 4);
		tag = binary_string_to_decimal(str_tag);
		index = binary_string_to_decimal(str_index);
		offset = 0; 

		// go to the index in the cache and check if the tags match up 
		stored_tag = myCache[index].tag;
		if (tag == stored_tag) {
			if (cur_MemR == 1) {
				// LW
				cur_data = myCache[index].data;
				Update(myCache, index);
				return true; 
			}
			if (cur_MemW == 1) {
				// SW 
				myCache[index].data = cur_data;
				return true; 
			}
		}
	}

	// FA (tag = 32, index = 0, offset = 0)
	if (mode == FA) {
		tag = binary_string_to_decimal(str_addr);
		index = 0; 
		offset = 0;

		// search all sets in the cache until there is a set that matches the tag 
		for (int i = 0; i < CACHE_SETS; i++) {
			stored_tag = myCache[i].tag; 
			if (tag == stored_tag) {
				if (cur_MemR == 1) {
					// LW 
					cur_data = myCache[i].data; 
					Update(myCache, i);
					return true;
				}
				if (cur_MemW == 1) {
					// SW 
					myCache[i].data = cur_data; 
					return true;
				}
			}
		}
	}

	// 4 way SA (tag = 30, index = 2, offset = 0)
	if (mode == SA) {
		str_tag = str_addr.substr(0, 30);
		str_index = str_addr.substr(30, 2);
		tag = binary_string_to_decimal(str_tag);
		index = binary_string_to_decimal(str_index);
		offset = 0; 

		// index into the set first using index*4 and serch all sets in that set until there is a set that matches the tag 
		for (int i = index*4; i < index*4 + 4; i++) {
			stored_tag = myCache[i].tag; 
			if (tag == stored_tag) {
				if (cur_MemR == 1) {
					// LW
					cur_data = myCache[i].data;
					Update(myCache, i);
					return true; 
				}
				if (cur_MemW == 1) {
					// SW
					myCache[i].data = cur_data;
					return true; 
				}
			} 
		}
	}
	// no matches were found in the cache
	return false;
}

void CacheMiss(int &cur_adr, int &cur_data, int myMem[MEM_SIZE], cache_set myCache[CACHE_SETS], int mode) {
	// read the data from the MM
	cur_data = myMem[cur_adr];
	
	// we must evict an entry from the cache 
	Evict(cur_adr, cur_data, myCache, mode); 
}

void Evict(int cur_adr, int cur_data, cache_set myCache[CACHE_SETS], int mode) {

	string str_tag, str_index;
	int tag, index;

	// calculate the tag again to store in the cache 
	bitset<32> binary(cur_adr);
	string str_addr = binary.to_string();

	if (mode == DM) {
		str_tag = str_addr.substr(0, 28);
		str_index = str_addr.substr(28, 4);
		tag = binary_string_to_decimal(str_tag);
		index = binary_string_to_decimal(str_index);

		// we do not need to evict anything, we just add it to the cache 
		if (myCache[index].tag == -1) {
			myCache[index].data = cur_data;
			myCache[index].tag = tag; 
			Update(myCache, index);
			return;
		}

		// for DM, we evict at the index 
		if (mode == DM) {
			myCache[index].data = cur_data;
			myCache[index].tag = tag; 
			Update(myCache, index);
			return;
		}
	}

	if (mode == FA) {
		tag = binary_string_to_decimal(str_addr);

		// obtain the least recently used data (minimum LRU value) 
		for (int i = 0; i < CACHE_SETS; i++) {
			if (myCache[i].lru_position == 0) {
				Update(myCache, i);
				myCache[i].tag = tag;
				myCache[i].data = cur_data; 
				return;
			}
	}

	}

	if (mode == SA) {
		str_tag = str_addr.substr(0, 30);
		str_index = str_addr.substr(30, 2);
		tag = binary_string_to_decimal(str_tag);
		index = binary_string_to_decimal(str_index);

		int min = 15; 
		int index_of_min = 0;
		for (int i = index*4; i < index*4 + 4; i++) {
			// instead of choosing lru_position == 0, we choose the lowest position from the set 
			if (myCache[i].lru_position < min) {
				min = myCache[i].lru_position;
				index_of_min = i; 
			}
		}
		Update(myCache, index_of_min);
		myCache[index_of_min].tag = tag;
		myCache[index_of_min].data = cur_data; 
		return;
	}
}

void Update(cache_set myCache[CACHE_SETS], int index) {
	// set the new line's position to the highest and decrement all others 

	// if we are accessing something in the cache that has been accessed before 
	if (myCache[index].lru_position != 0) {
		Adjust(myCache, index);
		return;
	}

	// no need to modify LRU position if we repeat access to the same data 
	if (myCache[index].lru_position == 15) 
		return; 

	myCache[index].lru_position = 15; 
	for (int i = 0; i < CACHE_SETS; i++) {
		if (i != index) {
			myCache[i].lru_position = max(0, myCache[i].lru_position - 1);
		}
	}
}

void Adjust(cache_set myCache[CACHE_SETS], int index) {
// obtain the old LRU position of the data 
	int old_pos = myCache[index].lru_position;
	int diff = 15 - old_pos;

	for (int j = diff; j > 0; j--) {
		for (int i = 0; i < CACHE_SETS; i++) {
			if (i != index) {
				if (myCache[i].lru_position == (old_pos + j)) {
					// if the position has not been updated yet 
					if (!myCache[i].updated) {
						myCache[i].lru_position = max(0, myCache[i].lru_position - 1);
						myCache[i].updated = true; 
						break;
					}
				}
			}
		}
	}
	myCache[index].lru_position = 15; 
}