//asm(".intel_syntax noprefix");
#ifndef SEH_H
#define SEH_H
//#include <unwind.h>

#define INT3 asm("int3")
#define NOP asm("nop")


#ifndef _WIN32
#error Only support _WIN32
#endif

#ifdef _x86_64_
#error Not support x64
#endif

#ifndef __GNUC__
#error Only support GCC
#endif

#ifndef __MINGW32__
#error Only support gcc MINGW32
#endif

struct SEH_EXCEPTION {
	PVOID address;
	DWORD code;
};

namespace seh {

	int originalSEH;

	void print_ctx(const CONTEXT *pctx) {
		printf("CONTEXT(%p):\n"
				" ContextFlags=%lx\n"
				" Dr0=%lx\n"
				" Dr1=%lx\n"
				" Dr2=%lx\n"
				" Dr3=%lx\n"
				" Dr6=%lx\n"
				" Dr7=%lx\n"
				" SegGs=%lx\n"
				" SegFs=%lx\n"
				" SegEs=%lx\n"
				" SegDs=%lx\n"
				" Edi=%lx\n"
				" Esi=%lx\n"
				" Ebx=%lx\n"
				" Edx=%lx\n"
				" Ecx=%lx\n"
				" Eax=%lx\n"
				" Ebp=%lx\n"
				" Eip=%lx\n"
				" SegCs=%lx\n"
				" EFlags=%lx\n"
				" Esp=%lx\n"
				" SegSs=%lx\n",
				pctx,
				pctx->ContextFlags,
				pctx->Dr0,
				pctx->Dr1,
				pctx->Dr2,
				pctx->Dr3,
				pctx->Dr6,
				pctx->Dr7,
				pctx->SegGs,
				pctx->SegFs,
				pctx->SegEs,
				pctx->SegDs,
				pctx->Edi,
				pctx->Esi,
				pctx->Ebx,
				pctx->Edx,
				pctx->Ecx,
				pctx->Eax,
				pctx->Ebp,
				pctx->Eip,
				pctx->SegCs,
				pctx->EFlags,
				pctx->Esp,
				pctx->SegSs);
	}

	void print_except(const EXCEPTION_RECORD *pex) {
		printf("EXCEPTION_RECORD(%p):\n"
				" Address=%p\n"
				" Code=%lx\n"
				" Flags=%lx\n",
				pex,
				pex->ExceptionAddress,
				pex->ExceptionCode,
				pex->ExceptionFlags
				);
	}

	void __stdcall landing_throw_unwinder(PVOID exceptionAddress, DWORD exceptionCode) {

		//востанавливаем предыдущий обработчик seh
		//		EXCEPTION_REGISTRATION * pSehExcReg;
		//		asm ("mov %0,fs:[0];" : "=r" (pSehExcReg) :);
		//		asm volatile("mov fs:[0], %0;"::"r"(pSehExcReg->prev) :);

		NOP;
		SEH_EXCEPTION ex = SEH_EXCEPTION();
		ex.address = exceptionAddress;
		ex.code = exceptionCode;
		throw ex;
	}

	EXCEPTION_DISPOSITION __cdecl except_handler(
			PEXCEPTION_RECORD pException,
			PEXCEPTION_REGISTRATION pEstablisherFrame,
			PCONTEXT pContext,
			PEXCEPTION_REGISTRATION *pDispatcherContext) {

		//print_except(pException);
		//print_ctx(pContext);

		if (pException->ExceptionFlags & (EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND)) {
			return ExceptionCollidedUnwind;
		}

		if (pException->ExceptionFlags & EXCEPTION_NONCONTINUABLE) {
			return ExceptionContinueSearch;
		}

		DWORD pLanding = (DWORD) & landing_throw_unwinder;

		//имитация call
		// push параметр DWORD exceptionCode
		pContext->Esp = pContext->Esp - 4;
		*(DWORD *) (pContext->Esp) = pException->ExceptionCode;
		// push параметр exceptionAddress
		pContext->Esp = pContext->Esp - 4;
		*(PVOID *) (pContext->Esp) = pException->ExceptionAddress;
		// push адресс возврата
		pContext->Esp = pContext->Esp - 4;
		*(int *) (pContext->Esp) = pContext->Eip;
		pContext->Eip = pLanding;

		return ExceptionContinueExecution;
	}

	/**
	 * не даёт компилятору вырезать try{..}catch{...} из за отсутсвия методов бросающих исключение
	 * вынуждает компилятор заполнить структуру для перехвата исключения и указать catchIndex
	 * вызов метода будет выглядеть так
	 * mov[esp+20],index
	 * call __throw_magic_link
	 *(push eip; jmp __throw_magic_link)
	 */
	__attribute__((noinline, stdcall)) void __throw_magic_link() {
		int test;
		asm volatile ("mov %0,1;" : "=r" (test)); //чтобы gcc не вырезал не используемый throw
		if (test > 0) {
			return;
		}
		throw SEH_EXCEPTION();
	}

}


#undef __try
#define __try \
				if (bool _try = true) {\
					EXCEPTION_REGISTRATION __seh_ex_reg = EXCEPTION_REGISTRATION();/*размещаем в стеке структуру EXCEPTION_REGISTRATION*/\
					try {\
						int __seh_prev_addr;\
						asm ("mov %0,fs:[0];" : "=r" (__seh_prev_addr) :);\
						__seh_ex_reg.prev = (_EXCEPTION_REGISTRATION_RECORD*) __seh_prev_addr;\
						__seh_ex_reg.handler = (PEXCEPTION_ROUTINE) & seh::except_handler;\
						asm volatile("mov fs:[0], %0;"::"r"(&__seh_ex_reg) :);\
						int catchIndex; asm volatile ("mov %0,[esp+0x20];" : "=r" (catchIndex) :);/*индекс catch блока*/\
						seh::__throw_magic_link();\
						/*begin try bloc*/

#define __except_line(filter, line )\
						asm volatile ("mov [esp+0x20],%0;" ::"r" (catchIndex) :);;\
						asm volatile("mov fs:[0], %0;"::"r"(__seh_ex_reg.prev) :);\
					} catch (filter) {\
						asm volatile("mov fs:[0], %0;"::"r"(__seh_ex_reg.prev) :);\
						_try = false;\
						goto __seh_catch_ ## line;\
					}\
				} else\
					__seh_catch_ ## line:\
					if (!_try)\
					/*begin catch bloc*/

#define __except_line__wrap(filter, line ) __except_line(filter,line)

#undef __except
#define __except(filter) __except_line__wrap(filter,__LINE__)

#define __exceptSEH __except_line__wrap(SEH_EXCEPTION,__LINE__)


#endif


