/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.cpp
 * Author: ExIngus (ExIngus@gmail.com)
 *
 * Created on 21 марта 2016 г., 9:51
 */
#include <cstdlib>
#include <windows.h>
#include<iostream>
//#include <Dbghelp.h>
//#include <stdio.h>
//#include "seh.cpp"
#include "seh.h"

using namespace std;

void recur_test() {
	recur_test();
}

LONG WINAPI ExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo) {
	PCONTEXT pContext = ExceptionInfo->ContextRecord;
	PEXCEPTION_RECORD pExceptionRecord = ExceptionInfo->ExceptionRecord;
	cout << endl << "Exception " << pExceptionRecord->ExceptionCode << " in " << pExceptionRecord->ExceptionAddress << endl;

	//	MiniDumpWriteDump
	SetErrorMode(SEM_FAILCRITICALERRORS);
	return 1;
}

void throw_test() {
	throw SEH_EXCEPTION();
}

void NPE_deep_test(int deep) {
	if (deep == 0) {
		*(char *) 0 = 0;
	} else {
		deep--;
		NPE_deep_test(deep);
	}

}

void NPE_test() {
	*(char *) 0 = 0;
	//	throw_test();
}

void catch_test() {


	try {
		throw_test();
	} catch (...) {
		printf("catch_test_0");
	}

	try {
		throw_test();
	} catch (...) {
		printf("catch_test");
	}
}

void test_2() {
	printf("\ntest2!\n");
	__try{
		__try
		{
			printf("try2_1!\n");
		}
		__exceptSEH
		{
			printf("except2_1!\n");
		}
		NPE_test();
	}
	__exceptSEH{
		printf("except2_0!\n");
	}
}

int main(int argc, char** argv) {
	NOP;
	NOP;
	NOP;
	//	catch_test();
	//	SetUnhandledExceptionFilter(&ExceptionFilter);
	//	cout << "SetUnhandledExceptionFilter" << endl;
	//	cout << "recursive .";

	printf("\ntest1!\n");
	__try{
		throw_test();
	}
	__exceptSEH{
		printf("except1!\n");
	}


	//try2_1!
	//EXCEPTION_RECORD(0028f8e4):
	// Address=004019ec
	// Code=c0000005
	// Flags=0
	//except2_1!
	//EXCEPTION_RECORD(0028f8e4):
	// Address=004019ec
	// Code=c0000005
	// Flags=0
	//except2_0!

	test_2();


	printf("\ntest3:!\n");
	__try{
		__try
		{
			__try
			{
				__try
				{
					printf("try3_3!\n");
				}
				__exceptSEH
				{
					printf("except3_3!\n");
				}
				NPE_test();
			}
			__exceptSEH
			{
				printf("except3_2!\n");
			}
			NPE_test();
		}
		__exceptSEH
		{
			printf("except3_1!\n");
		}
		NPE_test();
	}
	__exceptSEH{
		printf("except3_0!\n");
	}



}



