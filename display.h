#pragma once


DWORD WINAPI displayThread(LPVOID args);

struct DisplayArgs {
	unsigned int* global_frame_count;
	char* imCache;
	bool* done;
	int imCache_N;
	int imWidth;
	int imHeight;
	unsigned long long imSize;

};