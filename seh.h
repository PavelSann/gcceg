//asm(".intel_syntax noprefix");
#ifndef SEH_H
#define SEH_H
#include <unwind.h>
typedef void (__stdcall *PSEH_HANDLER) (PEXCEPTION_RECORD pException, PEXCEPTION_REGISTRATION pEstablisherFrame, PCONTEXT pContext, PEXCEPTION_REGISTRATION *pDispatcherContext);
//typedef void (__stdcall*PLANDING_THROW)(const CONTEXT *pctx);

#define INT3 asm("int3")
#define NOP asm("nop")

#ifdef __x86_64__
#define DW DWORD64
#else
#define DW DWORD
#endif

struct SEH_EXCEPTION {
	PVOID address;
	DWORD code;
};

namespace seh {

	int originalSEH;

#ifdef __x86_64__

	void print_ctx(const CONTEXT *pctx) {
		printf("CONTEXT(%p):\n"
				" ContextFlags=%lx\n"
				" Dr0=%llx\n"
				" Dr1=%llx\n"
				" Dr2=%llx\n"
				" Dr3=%llx\n"
				" Dr6=%llx\n"
				" Dr7=%llx\n"
				" SegGs=%lx\n"
				" SegFs=%lx\n"
				" SegEs=%lx\n"
				" SegDs=%lx\n"
				" Rdi=%llx\n"
				" Rsi=%llx\n"
				" Rbx=%llx\n"
				" Rdx=%llx\n"
				" Rcx=%llx\n"
				" Rax=%llx\n"
				" Rbp=%llx\n"
				" Rip=%llx\n"
				" SegCs=%lx\n"
				" EFlags=%lx\n"
				" Rsp=%llx\n"
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
				pctx->Rdi,
				pctx->Rsi,
				pctx->Rbx,
				pctx->Rdx,
				pctx->Rcx,
				pctx->Rax,
				pctx->Rbp,
				pctx->Rip,
				pctx->SegCs,
				pctx->EFlags,
				pctx->Rsp,
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

#else
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
#endif

	void __stdcall landing_throw_unwinder(PVOID exceptionAddress, DWORD exceptionCode) {
		//push  ebp
		//mov   ebp, esp

		//востанавливаем предыдущий обработчик seh
		//		EXCEPTION_REGISTRATION * pSehExcReg;
		//		asm ("mov %0,fs:[0];" : "=r" (pSehExcReg) :);
		//		asm volatile("mov fs:[0], %0;"::"r"(pSehExcReg->prev) :);

		NOP;
		SEH_EXCEPTION ex = SEH_EXCEPTION();
		ex.address = exceptionAddress;
		ex.code = exceptionCode;
		throw ex;
		//mov   esp, ebp
		//pop   ebp
		//ret
	}

	EXCEPTION_DISPOSITION __cdecl except_handler(
			PEXCEPTION_RECORD pException,
			PEXCEPTION_REGISTRATION pEstablisherFrame,
			PCONTEXT pContext,
			PEXCEPTION_REGISTRATION *pDispatcherContext) {

		print_except(pException);
		//		print_ctx(pContext);

		//pException->ExceptionCode  //STATUS_
		if (pException->ExceptionFlags & (EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND)) {
			return ExceptionCollidedUnwind;
		}

		if (pException->ExceptionFlags & EXCEPTION_NONCONTINUABLE) {
			return ExceptionContinueSearch;
		}

		auto pLanding = &landing_throw_unwinder;

		//имитация call
#ifdef __x86_64__
		//push параметр DWORD exceptionCode
		pContext->Rsp = pContext->Rsp - 8;
		*(DWORD64 *) (pContext->Rsp) = pException->ExceptionCode;
		// push параметр exceptionAddress
		pContext->Rsp = pContext->Rsp - 8;
		*(PVOID *) (pContext->Rsp) = pException->ExceptionAddress;
		// push адресс возврата
		pContext->Rsp = pContext->Rsp - 8;
		*(long *) (pContext->Rsp) = pContext->Rip;
		pContext->Rip = (DWORD64)pLanding;
#else
		//push параметр DWORD exceptionCode
		pContext->Esp = pContext->Esp - 4;
		*(DWORD *) (pContext->Esp) = pException->ExceptionCode;
		// push параметр exceptionAddress
		pContext->Esp = pContext->Esp - 4;
		*(PVOID *) (pContext->Esp) = pException->ExceptionAddress;
		// push адресс возврата
		pContext->Esp = pContext->Esp - 4;
		*(int *) (pContext->Esp) = pContext->Eip;
		pContext->Eip = (DWORD)pLanding;
#endif


		return ExceptionContinueExecution;
	}

	//mov[esp+20],index
	//call __throw_unwinder_link (push ip+; jmp __throw_unwinder_link)

	/**
	 * не даёт компилятору вырезать try{..}catch{...} из за отсутсвия методов бросающих исключение
	 * вынуждает компилятор заполнить структуру для перехвата исключения и указать catchIndex
	 * возвращает catchIndex
	 */
	__attribute__((noinline, stdcall)) void __throw_magic_link() {
		int test;
		asm volatile ("mov %0,1;" : "=r" (test));
		if (test > 0) {
			return;
		}
		throw SEH_EXCEPTION();
	}

}
//TODO: попробовать реализовать через VEH или RUNTIME_FUNCTION
//http://blog.talosintel.com/2014/06/exceptional-behavior-windows-81-x64-seh.html
//http://www.osronline.com/article.cfm?article=469
#ifdef __x86_64__
#define GET_SEH(var) asm volatile("mov %0,gs:[0];" : "=r" (var) :)
#define SET_SEH(var) asm volatile("mov gs:[0], %0;"::"r"(var) :)
/*индекс catch блока*/
#define GET_CATCH_INDEX(catchIndex)
//asm volatile ("mov %0,[rsp+0x40];" : "=r" (catchIndex) :)
#define SET_CATCH_INDEX(catchIndex)
//asm volatile ("mov [rsp+0x40],%0;" ::"r" (catchIndex) :)
#else
#define GET_SEH(var) asm ("mov %0,fs:[0];" : "=r" (var) :)
#define SET_SEH(var) asm volatile("mov fs:[0], %0;"::"r"(var) :)
/*индекс catch блока*/
#define GET_CATCH_INDEX(catchIndex) asm volatile ("mov %0,[esp+0x20];" : "=r" (catchIndex) :);
#define SET_CATCH_INDEX(catchIndex) asm volatile ("mov [esp+0x20],%0;" ::"r" (catchIndex) :)
#endif


#undef __try
#define __try \
				if (bool _try = true) {\
					EXCEPTION_REGISTRATION __seh_ex_reg = EXCEPTION_REGISTRATION();/*размещаем в стеке структуру EXCEPTION_REGISTRATION*/\
					try {\
						DW __seh_prev_addr;\
						GET_SEH(__seh_prev_addr);\
						__seh_ex_reg.prev = (_EXCEPTION_REGISTRATION_RECORD*) __seh_prev_addr;\
						__seh_ex_reg.handler = (PEXCEPTION_ROUTINE) & seh::except_handler;\
						SET_SEH(&__seh_ex_reg);\
						DW catchIndex; \
						GET_CATCH_INDEX(catchIndex);\
						seh::__throw_magic_link();\
						/*begin try bloc*/

#define __except_line(filter, line )\
						SET_CATCH_INDEX(catchIndex);\
						SET_SEH(__seh_ex_reg.prev);\
					} catch (filter) {\
						SET_SEH(__seh_ex_reg.prev);\
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


