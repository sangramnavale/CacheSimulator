#include <iostream>
#include <stdlib.h>
#include <cmath>
#include <string>
#include <string.h>
#include <fstream>

using namespace std;

int blockSize, addressSize = 48, L1Size, associativityL1, setL1, L2Size, associativityL2, setL2;
int blockOffset, indexW, tagW, indexWL2, tagW2, L;
static int ReadsL1 = 0, ReadMissL1 = 0, WritesL1 = 0, WriteMissL1 = 0, WriteBacksL1 = 0;
static int ReadsL2 = 0, ReadMissL2 = 0, WritesL2 = 0, WriteMissL2 = 0, WriteBacksL2 = 0;
int replPolicy, inclusionPolicy;

int updatetag(long* L1, long long setPosition, int ass, long long tag) {
	*(L1 + setPosition * associativityL1 + ass) = tag;
	return 1;
}
int updatevaliddirty(int* dirtyL1, long long setPosition, int i, int value) {
	*(dirtyL1 + setPosition * associativityL1 + i) = value;

	return 0;
}
int updateStatusArray(int* statusPL1, long long setPosition, int i) {
	
	int maximum;
	int max = 0;
	maximum = *(statusPL1 + setPosition * associativityL1);

	for (int a = 1; a < associativityL1; a++) {
		if (*(statusPL1 + setPosition * associativityL1 + a) > maximum)
		{
			maximum = *(statusPL1 + setPosition * associativityL1 + a);
		}
	}
	*(statusPL1 + setPosition * associativityL1 + i) = maximum + 1;
	
	return 0;

}
int displayL(long* localname, int row, int column) {
	for (int p = row; p <= row; p++) {
		for (int j = 0; j < column; j++)
		{
			cout << '\t' << p << ',' << j << "  " << *(localname + p * column + j);
		}
		cout << endl;
	}
	return 0;
}
int displayint(int* localname, int row, int column) {


	for (int p = 0; p < row; p++) {
		for (int j = 0; j < column; j++)
		{
			cout << '\t' << p << ',' << j << "  " << *(localname + p * column + j);
		}
		cout << endl;
	}
	return 0;
}
int giveReplacePosition(long long tagBits, long long setPosition, int* statusPL1) {
	int minimum;
	int m = 0;
	minimum = *(statusPL1 + setPosition * associativityL1);
	
	for (int a = 1; a < associativityL1; a++) {
		if (*(statusPL1 + setPosition * associativityL1 + a) < minimum)
		{
			minimum = *(statusPL1 + setPosition * associativityL1 + a);
			m = a;
		}
		

	}
	
	return m;
}
long long getIndexPosition(long long address) {
	unsigned long long buffer, indexPosition;

	buffer = 0xffffffffffffffff;
	
	buffer = buffer >> (64 - addressSize);
	
	indexPosition = address << tagW;
	
	indexPosition = indexPosition & buffer;

	indexPosition = indexPosition >> (blockOffset + tagW);
	
	return indexPosition;
}

int checkInL1(char Lcommand, long long setPosition, long long tagBits, long* L1local, int* statusPL1, int* dirtyL1, int* validL1) {
	int hit, ass = 0;

	hit = 0;
	for (int i = 0; i < associativityL1; i++) {

		if (*(L1local + setPosition * associativityL1 + i) == tagBits)   
		{																					
			hit = 1;
			if (Lcommand == 'r') {
				ReadsL1++;  
				updateStatusArray(&(*(statusPL1)), setPosition, i);
				updatevaliddirty((&(*validL1)), setPosition, i, 1);
				
			}
			else if (Lcommand == 'w') {

				WritesL1++;											
				updateStatusArray((&(*statusPL1)), setPosition, i);
				updatevaliddirty((&(*validL1)), setPosition, i, 1);
				updatevaliddirty((&(*dirtyL1)), setPosition, i, 1);

			}
			else cout << "Error in taking r/w command";
			
			break;
		}
	}

	if (hit == 0) 																		
	{
		if (Lcommand == 'r') {
			ReadMissL1++;
			ass = giveReplacePosition(tagBits, setPosition, &(*(statusPL1)));
			if (*(dirtyL1 + setPosition * associativityL1 + ass))
				WriteBacksL1++;											

			
			updatetag(&(*(L1local)), setPosition, ass, tagBits);
			updateStatusArray(&(*(statusPL1)), setPosition, ass);
			updatevaliddirty((&(*validL1)), setPosition, ass, 1);
			updatevaliddirty((&(*dirtyL1)), setPosition, ass, 0);


		}

		else if (Lcommand == 'w') {
			WriteMissL1++;
			
			ass = giveReplacePosition(tagBits, setPosition, &(*(statusPL1)));
			if (*(dirtyL1 + setPosition * associativityL1 + ass))
				WriteBacksL1++;
		

			updatetag(&(*(L1local)), setPosition, ass, tagBits);
			updateStatusArray(&(*(statusPL1)), setPosition, ass);
			updatevaliddirty((&(*validL1)), setPosition, ass, 1);
			updatevaliddirty((&(*dirtyL1)), setPosition, ass, 1);
			
		}
		else cout << "Error in taking r/w command";
		
	}



	return 0;
}


int main(int argc, char** argv)
{
	int setPosition, hit;
	unsigned long long indexPosition;
	unsigned long long address, tagBits;
	char command;
	float missrateL2 = 0;
	std::string line, addr, trace, repl, incl;
	int b, count = 0;
	L = 0;
	if (argc > 1) {
		blockSize = atoi(argv[1]);
		cout << blockSize;
		L1Size = atoi(argv[2]);
		cout << L1Size;
		associativityL1 = atoi(argv[3]);
		cout << associativityL1;
		L2Size = atoi(argv[4]);
		cout << L2Size;
		associativityL2 = atoi(argv[5]);
		cout << associativityL2;
		replPolicy = atoi(argv[6]);
		cout << replPolicy;
		inclusionPolicy = atoi(argv[7]);
		cout << inclusionPolicy;
		trace = argv[8];
		cout << trace;
	}
	else {
		cout << "Arguments missing";
		return 1;
	}


	setL1 = L1Size / (blockSize * associativityL1);  
	blockOffset = log2(blockSize);     
	indexW = log2(setL1);
	tagW = addressSize - (blockOffset + indexW);

	if (L2Size != 0 && associativityL2 != 0)
		L = 1;
	long L1[setL1][associativityL1];
	int statusL1[setL1][associativityL1];
	int dirtyL1[setL1][associativityL1];
	int validL1[setL1][associativityL1];
	for (int z1 = 0; z1 < setL1; z1++)   					
		for (int z2 = 0; z2 < associativityL1; z2++)
		{
			statusL1[z1][z2] = 0;
			dirtyL1[z1][z2] = 0;
			validL1[z1][z2] = 0;
			L1[z1][z2] = 0;
		}

	ifstream trace_c;
	trace_c.open(trace.c_str());
	while (trace_c >> command >> addr) {
		address = strtol(addr.c_str(), NULL, 16);
		
		indexPosition = getIndexPosition(address);

		tagBits = address >> (blockOffset + indexW);
		

		setPosition = indexPosition % setL1;
	

		checkInL1(command, setPosition, tagBits, &L1[0][0], &statusL1[0][0], &dirtyL1[0][0], &validL1[0][0]);


	}
	trace_c.close();

	if (replPolicy == 0) repl = "LRU";
	else if (replPolicy == 1)repl = "FIFO";
	else repl = "pseudoLRU";


	if (inclusionPolicy == 0) incl = "non-inclusive";
	else if (inclusionPolicy == 1) incl = "inclusive";
	else  incl = "exclusive";


	int memoryTraffic = ReadMissL1 + WriteMissL1 + WriteBacksL1;
	if (L)
		missrateL2 = ((float)ReadMissL2 + WriteMissL2) / ((float)ReadsL2 + ReadMissL2 + WriteMissL2 + WritesL2);

	cout << endl << "===== Simulator configuration =====" << endl
		<< "BLOCKSIZE:             " << blockSize << endl
		<< "L1_SIZE:               " << L1Size << endl
		<< "L1_ASSOC:              " << associativityL1 << endl
		<< "L2_SIZE:               " << L2Size << endl
		<< "L2_ASSOC:              " << associativityL2 << endl
		<< "REPLACEMENT POLICY:    " << repl << endl
		<< "INCLUSION PROPERTY:    " << incl << endl
		<< "trace_file:            " << trace << endl
		<< "===== Simulation results (raw) =====" << endl
		<< endl << "a. number of L1 Reads:" << "\t" << ReadsL1 + ReadMissL1
		<< endl << "b. number of L1 Read Misses:" << "\t" << ReadMissL1
		<< endl << "c. number of L1 Writes:" << "\t" << WritesL1 + WriteMissL1
		<< endl << "d. number of L1 Write Misses:" << "\t" << WriteMissL1
		<< endl << "e. L1 miss rate:" << "\t" << ((float)ReadMissL1 + WriteMissL1) / ((float)ReadsL1 + ReadMissL1 + WriteMissL1 + WritesL1)
		<< endl << "f. number of L1 writebacks:" << "\t" << WriteBacksL1
		<< endl << "g. number of L2 Reads:" << "\t" << ReadsL2 + ReadMissL2
		<< endl << "h. number of L2 Read Misses:" << "\t" << ReadMissL2
		<< endl << "i. number of L2 Writes:" << "\t" << WritesL2 + WriteMissL2
		<< endl << "j. number of L2 Write Misses:" << "\t" << WriteMissL2
		<< endl << "k. L2 miss rate:" << "\t" << missrateL2
		<< endl << "l. number of L2 writebacks:" << "\t" << WriteBacksL2
		<< endl << "m. total memory traffic:" << "\t" << memoryTraffic;

	return 0;
}
