/*
 * This file main.c is part of L1vm.
 *
 * (c) Copyright Stefan Pietzonke (jay-t@gmx.net), 2017
 *
 * L1vm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * L1vm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with L1vm.  If not, see <http://www.gnu.org/licenses/>.
 */

//  l1vm RISC VM
//
//  clang main.c load-object.c -o l1vm -O3 -fomit-frame-pointer -s -ldl -lpthread
//


#define JIT_COMPILER 1

#include "../include/global.h"

#if SDL_module
#include <SDL/SDL.h>
#include <SDL/SDL_byteorder.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_image.h>
#endif

#if JIT_COMPILER
#define MAXJITCODE 64
S8 JIT_code_ind ALIGN = -1;

struct JIT_code JIT_code[MAXJITCODE];
#endif

// protos
S2 load_object (U1 *name);
void free_modules (void);


#if JIT_COMPILER
int jit_compiler (U1 *code, U1 *data, S8 *jumpoffs, S8 *regi, F8 *regd, U1 *sp, U1 *sp_top, U1 *sp_bottom, S8 start, S8 end);
int run_jit (S8 code);
int free_jit_code ();
#endif

#if SDL_module
SDL_Surface *surf;
TTF_Font *font;
U1 SDL_use = 0;
#endif

#define EXE_NEXT(); ep = ep + eoffs; goto *jumpt[code[ep]];

//#define EXE_NEXT(); ep = ep + eoffs; printf ("next opcode: %i\n", code[ep]); goto *jumpt[code[ep]];


S8 data_size ALIGN;
S8 code_size ALIGN;
S8 data_mem_size ALIGN;
S8 stack_size ALIGN = STACKSIZE;		// stack size added to data size when dumped to object file

// code
U1 *code = NULL;

// data
U1 *data = NULL;


S8 code_ind ALIGN;
S8 data_ind ALIGN;
S8 modules_ind ALIGN = -1;    // no module loaded = -1

S8 cpu_ind ALIGN = 0;

S8 max_cpu ALIGN = MAXCPUCORES;    // number of threads that can be runned

typedef U1* (*dll_func)(U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data);

struct module
{
    U1 name[512];

#if __linux__
    void *lptr;
#endif

#if _WIN32
    HINSTANCE lptr;
#endif

    dll_func func[MODULES_MAXFUNC];
    S8 func_ind ALIGN;
};

struct module modules[MODULES];

struct data_info data_info[MAXDATAINFO];
S8 data_info_ind ALIGN = -1;

// pthreads data mutex
pthread_mutex_t data_mutex;

// return code of main thread
S8 retcode ALIGN = 0;

// shell arguments
U1 shell_args[MAXSHELLARGS][MAXSHELLARGLEN];
S4 shell_args_ind = -1;

struct threaddata threaddata[MAXCPUCORES];

S2 load_module (U1 *name, S8 ind ALIGN)
{
#if __linux__
    modules[ind].lptr = dlopen ((const char *) name, RTLD_LAZY);
    if (!modules[ind].lptr)
	{
        printf ("error load module %s!\n", (const char *) name);
        return (1);
    }
#endif

#if _WIN32
    modules[ind].lptr = LoadLibrary ((const char *) name);
    if (! modules[ind].lptr)
    {
        printf ("error load module %s!\n", (const char *) name);
        return (1);
    }
#endif

    modules[ind].func_ind = -1;
    strcpy ((char *) modules[ind].name, (const char *) name);

	// print module name:
	printf ("module: %lli %s loaded\n", ind, name);
    return (0);
}

void free_module (S8 ind ALIGN)
{
#if __linux__
    dlclose (modules[ind].lptr);
#endif

#if _WIN32
    FreeLibrary (modules[ind].lptr);
#endif

// mark as free
    strcpy ((char *) modules[ind].name, "");
}

S2 set_module_func (S8 ind ALIGN, S8 func_ind ALIGN, U1 *func_name)
{
#if __linux__
	dlerror ();

    // load the symbols (handle to function)
    modules[ind].func[func_ind] = dlsym (modules[ind].lptr, (const char *) func_name);
    const char* dlsym_error = dlerror ();
    if (dlsym_error)
	{
        printf ("error set module %s, function: '%s'!\n", modules[ind].name, func_name);
		printf ("%s\n", dlsym_error);
        return (1);
    }
    return (0);
#endif

#if _WIN32
    modules[ind].func[func_ind] = GetProcAddress (modules[ind].lptr, (const char *) func_name);
    if (! modules[ind].func[func_ind])
    {
        printf ("error set module %s, function: '%s'!\n", modules[ind].name, func_name);
        return (1);
    }
    return (0);
#endif
}

U1 *call_module_func (S8 ind ALIGN, S8 func_ind ALIGN, U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
    return (*modules[ind].func[func_ind])(sp, sp_top, sp_bottom, data);
}

void cleanup (void)
{
    #if SDL_module
	if (SDL_use == 1)
	{
    	SDL_FreeSurface (surf);
	    SDL_QuitSubSystem (SDL_INIT_AUDIO);
    	SDL_QuitSubSystem (SDL_INIT_VIDEO);
	    if (font)
    	{
        	TTF_CloseFont (font);
    	}
    	TTF_Quit ();
    	SDL_Quit ();
	}
    #endif

    free_jit_code ();
    free_modules ();
	if (data) free (data);
    if (code) free (code);
}

S2 run (void *arg)
{
	S8 cpu_core ALIGN = (S8) arg;
	S8 i ALIGN;
	U1 eoffs;                  // offset to next opcode
	S8 regi[MAXREG];   // integer registers
	F8 regd[MAXREG];          // double registers
	S8 arg1 ALIGN;
	S8 arg2 ALIGN;
	S8 arg3 ALIGN;
	S8 arg4 ALIGN;	   // opcode arguments

	S8 ep ALIGN = 0; // execution pointer in code segment
	S8 startpos ALIGN;

	U1 *sp;   // stack pointer
	U1 *sp_top;    // stack pointer start address
	U1 *sp_bottom;  // stack bottom
	U1 *srcptr, *dstptr;

	U1 *bptr;

	// jump call stack for jsr, jsra
	S8 jumpstack[MAXSUBJUMPS];
	S8 jumpstack_ind ALIGN = -1;		// empty

	// threads
	S8 new_cpu ALIGN;

    // thread attach to CPU core
	cpu_set_t cpuset;

	// for data input
	U1 input_str[MAXINPUT];

	// jumpoffsets
	S8 *jumpoffs ALIGN;
	S8 offset ALIGN;
	jumpoffs = (S8 *) calloc (code_size, sizeof (S8));
	if (jumpoffs == NULL)
	{
		printf ("ERROR: can't allocate %lli bytes for jumpoffsets!\n", code_size);
		pthread_exit ((void *) 1);
	}

	sp_top = threaddata[cpu_core].sp_top_thread;
	sp_bottom = threaddata[cpu_core].sp_bottom_thread;
	sp = threaddata[cpu_core].sp_thread;

	printf ("%lli sp top: %lli\n", cpu_core, (S8) sp_top);
	printf ("%lli sp bottom: %lli\n", cpu_core, (S8) sp_bottom);
	printf ("%lli sp: %lli\n", cpu_core, (S8) sp);

	printf ("%lli sp caller top: %lli\n", cpu_core, (S8) threaddata[cpu_core].sp_top);
	printf ("%lli sp caller bottom: %lli\n", cpu_core, (S8) threaddata[cpu_core].sp_bottom);

	startpos = threaddata[cpu_core].ep_startpos;
	if (threaddata[cpu_core].sp != threaddata[cpu_core].sp_top)
	{
		// something on mother thread stack, copy it

		srcptr = threaddata[cpu_core].sp_top;
		dstptr = threaddata[cpu_core].sp_top_thread;

		while (srcptr >= threaddata[cpu_core].sp)
		{
			// printf ("dstptr stack: %lli\n", (S8) dstptr);
			*dstptr-- = *srcptr--;
		}
	}

	cpu_ind = cpu_core;

	// jumptable for indirect threading execution
	static void *jumpt[] =
	{
		&&pushb, &&pushw, &&pushdw, &&pushqw, &&pushd,
		&&pullb, &&pullw, &&pulldw, &&pullqw, &&pulld,
		&&addi, &&subi, &&muli, &&divi,
		&&addd, &&subd, &&muld, &&divd,
		&&smuli, &&sdivi,
		&&andi, &&ori, &&bandi, &&bori, &&bxori, &&modi,
		&&eqi, &&neqi, &&gri, &&lsi, &&greqi, &&lseqi,
		&&eqd, &&neqd, &&grd, &&lsd, &&greqd, &&lseqd,
		&&jmp, &&jmpi,
		&&stpushb, &&stpopb, &&stpushi, &&stpopi, &&stpushd, &&stpopd,
		&&loada, &&loadd,
		&&intr0, &&intr1, &&inclsijmpi, &&decgrijmpi,
		&&movi, &&movd, &&loadl, &&jmpa,
		&&jsr, &&jsra, &&rts, &&load,
        &&noti
	};

	//printf ("setting jump offset table...\n");

	// setup jump offset table
	for (i = 16; i < code_size; i = i + offset)
	{
		//printf ("opcode: %i\n", code[i]);
		offset = 0;
		if (code[i] <= LSEQD)
		{
			offset = 4;
		}
		if (code[i] == JMP)
		{
			bptr = (U1 *) &arg1;

			*bptr = code[i + 1];
			bptr++;
			*bptr = code[i + 2];
			bptr++;
			*bptr = code[i + 3];
			bptr++;
			*bptr = code[i + 4];
			bptr++;
			*bptr = code[i + 5];
			bptr++;
			*bptr = code[i + 6];
			bptr++;
			*bptr = code[i + 7];
			bptr++;
			*bptr = code[i + 8];

			jumpoffs[i] = arg1;
			offset = 9;
		}

		if (code[i] == JMPI)
		{
			bptr = (U1 *) &arg1;

			*bptr = code[i + 2];
			bptr++;
			*bptr = code[i + 3];
			bptr++;
			*bptr = code[i + 4];
			bptr++;
			*bptr = code[i + 5];
			bptr++;
			*bptr = code[i + 6];
			bptr++;
			*bptr = code[i + 7];
			bptr++;
			*bptr = code[i + 8];
			bptr++;
			*bptr = code[i + 9];

			jumpoffs[i] = arg1;
			offset = 10;
		}

		if (code[i] == INCLSIJMPI || code[i] == DECGRIJMPI)
		{
			bptr = (U1 *) &arg1;

			*bptr = code[i + 3];
			bptr++;
			*bptr = code[i + 4];
			bptr++;
			*bptr = code[i + 5];
			bptr++;
			*bptr = code[i + 6];
			bptr++;
			*bptr = code[i + 7];
			bptr++;
			*bptr = code[i + 8];
			bptr++;
			*bptr = code[i + 9];
			bptr++;
			*bptr = code[i + 10];

			jumpoffs[i] = arg1;
			offset = 11;
		}

		if (code[i] == JSR)
		{
			bptr = (U1 *) &arg1;

			*bptr = code[i + 1];
			bptr++;
			*bptr = code[i + 2];
			bptr++;
			*bptr = code[i + 3];
			bptr++;
			*bptr = code[i + 4];
			bptr++;
			*bptr = code[i + 5];
			bptr++;
			*bptr = code[i + 6];
			bptr++;
			*bptr = code[i + 7];
			bptr++;
			*bptr = code[i + 8];

			jumpoffs[i] = arg1;
			offset = 9;
		}

		if (offset == 0)
		{
			// not set yet, do it!

			switch (code[i])
			{
				case STPUSHB:
				case STPOPB:
				case STPUSHI:
				case STPOPI:
				case STPUSHD:
				case STPOPD:
					offset = 2;
					break;

				case LOADA:
				case LOADD:
					offset = 18;
					break;

				case INTR0:
				case INTR1:
					offset = 5;
					break;

				case MOVI:
				case MOVD:
					offset = 3;
					break;

				case LOADL:
					offset = 10;
					break;

				case JMPA:
					offset = 2;
					break;

				case JSRA:
					offset = 2;
					break;

				case RTS:
					offset = 1;
					break;

				case LOAD:
					offset = 18;
					break;

                case NOTI:
                    offset = 3;
                    break;
			}
		}
		if (offset == 0)
		{
			printf ("FATAL error: setting jump offset failed! opcode: %i\n", code[i]);
			free (jumpoffs);
			pthread_exit ((void *) 1);
		}

		if (i >= code_size) break;
	}

	// debug
#if DEBUG
	printf ("code DUMP:\n");
	for (i = 0; i < code_size; i++)
	{
		printf ("code %lli: %02x\n", i, code[i]);
	}
	printf ("DUMP END\n");
#endif

	// init registers
	for (i = 0; i < 256; i++)
	{
		regi[i] = 0;
		regd[i] = 0.0;
	}

	printf ("ready...\n");
	// printf ("modules loaded: %lli\n", modules_ind + 1);
	printf ("CPU %lli ready\n", cpu_ind);
	printf ("codesize: %lli\n", code_size);
	printf ("datasize: %lli\n", data_mem_size);
	printf ("ep: %lli\n\n", startpos);

#if DEBUG
	printf ("stack pointer sp: %lli\n", (S8) sp);
#endif

	ep = startpos; eoffs = 0;

	EXE_NEXT();

	// arg2 = data offset
	pushb:
	#if DEBUG
	printf ("%lli PUSHB\n", cpu_core);
	#endif
	arg1 = regi[code[ep + 1]];
	arg2 = regi[code[ep + 2]];
	arg3 = code[ep + 3];

    #if MEMCHECK
    if (arg1 + arg2 < 0 || arg1 + arg2 >= data_size)
    {
        printf ("FATAL ERROR: data access out of range!\n");
        printf ("address: %lli, offset: %lli\n", arg1, arg2);
        free (jumpoffs);
        pthread_exit ((void *) 1);
    }
    #endif

	regi[arg3] = data[arg1 + arg2];

	eoffs = 4;
	EXE_NEXT();

	pushw:
	#if DEBUG
	printf ("%lli PUSHW\n", cpu_core);
	#endif
	arg1 = regi[code[ep + 1]];
	arg2 = regi[code[ep + 2]];
	arg3 = code[ep + 3];

    #if MEMCHECK
    if (arg1 + arg2 < 0 || arg1 + arg2 >= data_size)
    {
        printf ("FATAL ERROR: data access out of range!\n");
        printf ("address: %lli, offset: %lli\n", arg1, arg2);
        free (jumpoffs);
        pthread_exit ((void *) 1);
    }
    #endif

	bptr = (U1 *) &regi[arg3];

	*bptr = data[arg1 + arg2];
	bptr++;
	*bptr = data[arg1 + arg2 + 1];

	eoffs = 4;
	EXE_NEXT();

	pushdw:
	#if DEBUG
	printf ("%lli PUSHDW\n", cpu_core);
	#endif
	arg1 = regi[code[ep + 1]];
	arg2 = regi[code[ep + 2]];
	arg3 = code[ep + 3];

    #if MEMCHECK
    if (arg1 + arg2 < 0 || arg1 + arg2 >= data_size)
    {
        printf ("FATAL ERROR: data access out of range!\n");
        printf ("address: %lli, offset: %lli\n", arg1, arg2);
        free (jumpoffs);
        pthread_exit ((void *) 1);
    }
    #endif

	bptr = (U1 *) &regi[arg3];

	*bptr = data[arg1 + arg2];
	bptr++;
	*bptr = data[arg1 + arg2 + 1];
	bptr++;
	*bptr = data[arg1 + arg2 + 2];
	bptr++;
	*bptr = data[arg1 + arg2 + 3];

	eoffs = 4;
	EXE_NEXT();

	pushqw:
	#if DEBUG
	printf ("%lli PUSHQW\n", cpu_core);
	#endif
	arg1 = regi[code[ep + 1]];
	arg2 = regi[code[ep + 2]];
	arg3 = code[ep + 3];

    #if MEMCHECK
    if (arg1 + arg2 < 0 || arg1 + arg2 >= data_size)
    {
        printf ("FATAL ERROR: data access out of range!\n");
        printf ("address: %lli, offset: %lli\n", arg1, arg2);
        free (jumpoffs);
        pthread_exit ((void *) 1);
    }
    #endif

	//printf ("PUSHQW: ep: %li\n", ep);
	//printf ("arg1: %li: asm: %li\n", arg1, code[ep + 1]);
	//printf ("arg2: %li\n", arg2);

	bptr = (U1 *) &regi[arg3];

	*bptr = data[arg1 + arg2];
	bptr++;
	*bptr = data[arg1 + arg2 + 1];
	bptr++;
	*bptr = data[arg1 + arg2 + 2];
	bptr++;
	*bptr = data[arg1 + arg2 + 3];
	bptr++;
	*bptr = data[arg1 + arg2 + 4];
	bptr++;
	*bptr = data[arg1 + arg2 + 5];
	bptr++;
	*bptr = data[arg1 + arg2 + 6];
	bptr++;
	*bptr = data[arg1 + arg2 + 7];

	eoffs = 4;
	EXE_NEXT();

	pushd:
	#if DEBUG
	printf ("%lli PUSHD\n", cpu_core);
	#endif
	arg1 = regi[code[ep + 1]];
	arg2 = regi[code[ep + 2]];
	arg3 = code[ep + 3];

    #if MEMCHECK
    if (arg1 + arg2 < 0 || arg1 + arg2 >= data_size)
    {
        printf ("FATAL ERROR: data access out of range!\n");
        printf ("address: %lli, offset: %lli\n", arg1, arg2);
        free (jumpoffs);
        pthread_exit ((void *) 1);
    }
    #endif

	bptr = (U1 *) &regd[arg3];

	*bptr = data[arg1 + arg2];
	bptr++;
	*bptr = data[arg1 + arg2 + 1];
	bptr++;
	*bptr = data[arg1 + arg2 + 2];
	bptr++;
	*bptr = data[arg1 + arg2 + 3];
	bptr++;
	*bptr = data[arg1 + arg2 + 4];
	bptr++;
	*bptr = data[arg1 + arg2 + 5];
	bptr++;
	*bptr = data[arg1 + arg2 + 6];
	bptr++;
	*bptr = data[arg1 + arg2 + 7];

	eoffs = 4;
	EXE_NEXT();


	pullb:
	#if DEBUG
	printf ("%lli PULLB\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = regi[code[ep + 2]];
	arg3 = regi[code[ep + 3]];

    #if MEMCHECK
    if (arg2 + arg3 < 0 || arg2 + arg3 >= data_size)
    {
        printf ("FATAL ERROR: data access out of range!\n");
        printf ("address: %lli, offset: %lli\n", arg2, arg3);
        free (jumpoffs);
        pthread_exit ((void *) 1);
    }
    #endif

	data[arg2 + arg3] = regi[arg1];

	eoffs = 4;
	EXE_NEXT();

	pullw:
	#if DEBUG
	printf ("%lli PULLW\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = regi[code[ep + 2]];
	arg3 = regi[code[ep + 3]];

    #if MEMCHECK
    if (arg2 + arg3 < 0 || arg2 + arg3 >= data_size)
    {
        printf ("FATAL ERROR: data access out of range!\n");
        printf ("address: %lli, offset: %lli\n", arg2, arg3);
        free (jumpoffs);
        pthread_exit ((void *) 1);
    }
    #endif

	bptr = (U1 *) &regi[arg1];

	data[arg2 + arg3] = *bptr;
	bptr++;
	data[arg2 + arg3 + 1] = *bptr;

	eoffs = 4;
	EXE_NEXT();

	pulldw:
	#if DEBUG
	printf ("%lli PULLDW\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = regi[code[ep + 2]];
	arg3 = regi[code[ep + 3]];

    #if MEMCHECK
    if (arg2 + arg3 < 0 || arg2 + arg3 >= data_size)
    {
        printf ("FATAL ERROR: data access out of range!\n");
        printf ("address: %lli, offset: %lli\n", arg2, arg3);
        free (jumpoffs);
        pthread_exit ((void *) 1);
    }
    #endif

	bptr = (U1 *) &regi[arg1];

	data[arg2 + arg3] = *bptr;
	bptr++;
	data[arg2 + arg3 + 1] = *bptr;
	bptr++;
	data[arg2 + arg3 + 2] = *bptr;
	bptr++;
	data[arg2 + arg3 + 3] = *bptr;

	eoffs = 4;
	EXE_NEXT();

	pullqw:
	#if DEBUG
	printf ("%lli PULLQW\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = regi[code[ep + 2]];
	arg3 = regi[code[ep + 3]];

    #if MEMCHECK
    if (arg2 + arg3 < 0 || arg2 + arg3 >= data_size)
    {
        printf ("FATAL ERROR: data access out of range!\n");
        printf ("address: %lli, offset: %lli\n", arg2, arg3);
        free (jumpoffs);
        pthread_exit ((void *) 1);
    }
    #endif

	//printf ("PULLQW\n");
	//printf ("data: %li\n", arg2);
	//printf ("offset: %li\n", arg3);

	bptr = (U1 *) &regi[arg1];

	data[arg2 + arg3] = *bptr;
	bptr++;
	data[arg2 + arg3 + 1] = *bptr;
	bptr++;
	data[arg2 + arg3 + 2] = *bptr;
	bptr++;
	data[arg2 + arg3 + 3] = *bptr;
	bptr++;
	data[arg2 + arg3 + 4] = *bptr;
	bptr++;
	data[arg2 + arg3 + 5] = *bptr;
	bptr++;
	data[arg2 + arg3 + 6] = *bptr;
	bptr++;
	data[arg2 + arg3 + 7] = *bptr;

	eoffs = 4;
	EXE_NEXT();

	pulld:
	#if DEBUG
	printf ("%lli PULLD\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = regi[code[ep + 2]];
	arg3 = regi[code[ep + 3]];

    #if MEMCHECK
    if (arg2 + arg3 < 0 || arg2 + arg3 >= data_size)
    {
        printf ("FATAL ERROR: data access out of range!\n");
        printf ("address: %lli, offset: %lli\n", arg2, arg3);
        free (jumpoffs);
        pthread_exit ((void *) 1);
    }
    #endif

	bptr = (U1 *) &regd[arg1];

	data[arg2 + arg3] = *bptr;
	bptr++;
	data[arg2 + arg3 + 1] = *bptr;
	bptr++;
	data[arg2 + arg3 + 2] = *bptr;
	bptr++;
	data[arg2 + arg3 + 3] = *bptr;
	bptr++;
	data[arg2 + arg3 + 4] = *bptr;
	bptr++;
	data[arg2 + arg3 + 5] = *bptr;
	bptr++;
	data[arg2 + arg3 + 6] = *bptr;
	bptr++;
	data[arg2 + arg3 + 7] = *bptr;

	eoffs = 4;
	EXE_NEXT();


	addi:
	#if DEBUG
	printf ("%lli ADDI\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = code[ep + 2];
	arg3 = code[ep + 3];

	regi[arg3] = regi[arg1] + regi[arg2];

	eoffs = 4;
	EXE_NEXT();

	subi:
	#if DEBUG
	printf ("%lli SUBI\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = code[ep + 2];
	arg3 = code[ep + 3];

	regi[arg3] = regi[arg1] - regi[arg2];

	eoffs = 4;
	EXE_NEXT();

	muli:
	#if DEBUG
	printf ("%lli MULI\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = code[ep + 2];
	arg3 = code[ep + 3];

	regi[arg3] = regi[arg1] * regi[arg2];

	eoffs = 4;
	EXE_NEXT();

	divi:
	#if DEBUG
	printf ("%lli DIVI\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = code[ep + 2];
	arg3 = code[ep + 3];

    #if DIVISIONCHECK
    if (iszero (regi[arg2]))
    {
        printf ("FATAL ERROR: division by zero!\n");
		free (jumpoffs);
		pthread_exit ((void *) 1);
    }
    #endif

	regi[arg3] = regi[arg1] / regi[arg2];

	eoffs = 4;
	EXE_NEXT();

	addd:
	#if DEBUG
	printf ("%lli ADDD\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = code[ep + 2];
	arg3 = code[ep + 3];

	regd[arg3] = regd[arg1] + regd[arg2];

	eoffs = 4;
	EXE_NEXT();

	subd:
	#if DEBUG
	printf ("%lli SUBD\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = code[ep + 2];
	arg3 = code[ep + 3];

	regd[arg3] = regd[arg1] - regd[arg2];

	eoffs = 4;
	EXE_NEXT();

	muld:
	#if DEBUG
	printf ("%lli MULD\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = code[ep + 2];
	arg3 = code[ep + 3];

	regd[arg3] = regd[arg1] * regd[arg2];

	eoffs = 4;
	EXE_NEXT();

	divd:
	#if DEBUG
	printf ("%lli DIVD\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = code[ep + 2];
	arg3 = code[ep + 3];

    #if DIVISIONCHECK
    if (iszero (regd[arg2]))
    {
        printf ("FATAL ERROR: division by zero!\n");
		free (jumpoffs);
		pthread_exit ((void *) 1);
    }
    #endif

	regd[arg3] = regd[arg1] / regd[arg2];

	eoffs = 4;
	EXE_NEXT();

	smuli:
	#if DEBUG
	printf ("%lli SMULI\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = code[ep + 2];
	arg3 = code[ep + 3];

	regi[arg3] = regi[arg1] << regi[arg2];

	eoffs = 4;
	EXE_NEXT();

	sdivi:
	#if DEBUG
	printf ("%lli SDIVI\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = code[ep + 2];
	arg3 = code[ep + 3];

	regi[arg3] = regi[arg1] >> regi[arg2];

	eoffs = 4;
	EXE_NEXT();

	andi:
	#if DEBUG
	printf ("%lli ANDI\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = code[ep + 2];
	arg3 = code[ep + 3];

	regi[arg3] = regi[arg1] && regi[arg2];

	eoffs = 4;
	EXE_NEXT();

	ori:
	#if DEBUG
	printf ("%lli ORI\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = code[ep + 2];
	arg3 = code[ep + 3];

	regi[arg3] = regi[arg1] || regi[arg2];

	eoffs = 4;
	EXE_NEXT();


	bandi:
	#if DEBUG
	printf ("%lli BANDI\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = code[ep + 2];
	arg3 = code[ep + 3];

	regi[arg3] = regi[arg1] & regi[arg2];

	eoffs = 4;
	EXE_NEXT();

	bori:
	#if DEBUG
	printf ("%lli BORI\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = code[ep + 2];
	arg3 = code[ep + 3];

	regi[arg3] = regi[arg1] | regi[arg2];

	eoffs = 4;
	EXE_NEXT();

	bxori:
	#if DEBUG
	printf ("%lli BXORI\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = code[ep + 2];
	arg3 = code[ep + 3];

	regi[arg3] = regi[arg1] ^ regi[arg2];

	eoffs = 4;
	EXE_NEXT();

	modi:
	#if DEBUG
	printf ("%lli MODI\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = code[ep + 2];
	arg3 = code[ep + 3];

	regi[arg3] = regi[arg1] % regi[arg2];

	eoffs = 4;
	EXE_NEXT();


	eqi:
	#if DEBUG
	printf ("%lli EQI\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = code[ep + 2];
	arg3 = code[ep + 3];

	regi[arg3] = regi[arg1] == regi[arg2];

	eoffs = 4;
	EXE_NEXT();

	neqi:
	#if DEBUG
	printf ("%lli NEQI\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = code[ep + 2];
	arg3 = code[ep + 3];

	regi[arg3] = regi[arg1] != regi[arg2];

	eoffs = 4;
	EXE_NEXT();

	gri:
	#if DEBUG
	printf ("%lli GRI\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = code[ep + 2];
	arg3 = code[ep + 3];

	regi[arg3] = regi[arg1] > regi[arg2];

	eoffs = 4;
	EXE_NEXT();

	lsi:
	#if DEBUG
	printf ("%lli LSI\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = code[ep + 2];
	arg3 = code[ep + 3];

	regi[arg3] = regi[arg1] < regi[arg2];

	eoffs = 4;
	EXE_NEXT();

	greqi:
	#if DEBUG
	printf ("%lli GREQI\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = code[ep + 2];
	arg3 = code[ep + 3];

	regi[arg3] = regi[arg1] >= regi[arg2];

	eoffs = 4;
	EXE_NEXT();

	lseqi:
	#if DEBUG
	printf ("%lli LSEQI\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = code[ep + 2];
	arg3 = code[ep + 3];

	regi[arg3] = regi[arg1] <= regi[arg2];

	eoffs = 4;
	EXE_NEXT();


	eqd:
	#if DEBUG
	printf ("%lli EQD\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = code[ep + 2];
	arg3 = code[ep + 3];

	regi[arg3] = regd[arg1] == regd[arg2];

	eoffs = 4;
	EXE_NEXT();

	neqd:
	#if DEBUG
	printf ("%lli NEQD\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = code[ep + 2];
	arg3 = code[ep + 3];

	regi[arg3] = regd[arg1] != regd[arg2];

	eoffs = 4;
	EXE_NEXT();

	grd:
	#if DEBUG
	printf ("%lli GRD\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = code[ep + 2];
	arg3 = code[ep + 3];

	regi[arg3] = regd[arg1] > regd[arg2];

	eoffs = 4;
	EXE_NEXT();

	lsd:
	#if DEBUG
	printf ("%lli LSD\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = code[ep + 2];
	arg3 = code[ep + 3];

	regi[arg3] = regd[arg1] < regd[arg2];

	eoffs = 4;
	EXE_NEXT();

	greqd:
	#if DEBUG
	printf ("%lli GREQD\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = code[ep + 2];
	arg3 = code[ep + 3];

	regi[arg3] = regd[arg1] >= regd[arg2];

	eoffs = 4;
	EXE_NEXT();

	lseqd:
	#if DEBUG
	printf ("%lli LSEQD\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = code[ep + 2];
	arg3 = code[ep + 3];

	regi[arg3] = regd[arg1] <= regd[arg2];

	eoffs = 4;
	EXE_NEXT();


	jmp:
	#if DEBUG
	printf ("%lli JMP\n", cpu_core);
	#endif
	arg1 = jumpoffs[ep];

	eoffs = 0;
	ep = arg1;

	EXE_NEXT();

	jmpi:
	#if DEBUG
	printf ("%lli JMPI\n", cpu_core);
	#endif
	arg1 = code[ep + 1];

	arg2 = jumpoffs[ep];

	if (regi[arg1] != 0)
	{
		eoffs = 0;
		ep = arg2;
		#if DEBUG
		printf ("%lli JUMP TO %lli\n", cpu_core, ep);
		#endif
		EXE_NEXT();
	}

	eoffs = 10;

	EXE_NEXT();


	stpushb:
	#if DEBUG
	printf("%lli STPUSHB\n", cpu_core);
	#endif
	arg1 = code[ep + 1];

	if (sp >= sp_bottom)
	{
		sp--;

		bptr = (U1 *) &regi[arg1];

		*sp = *bptr;
	}
	else
	{
		// fatal ERROR: stack pointer can't go below address ZERO!

		printf ("FATAL ERROR: stack pointer can't go below address 0!\n");
		free (jumpoffs);
		pthread_exit ((void *) 1);
	}
	eoffs = 2;

	EXE_NEXT();

	stpopb:
	#if DEBUG
	printf("%lli STPOPB\n", cpu_core);
	#endif
	if (sp == sp_top)
	{
		// nothing on stack!! can't pop!!

		printf ("FATAL ERROR: stack pointer can't pop empty stack!\n");
		free (jumpoffs);
		pthread_exit ((void *) 1);
	}

	bptr = (U1 *) &regi[arg1];

	*bptr = *sp;

	sp++;
	eoffs = 2;

	EXE_NEXT();

	stpushi:
	#if DEBUG
	printf("%lli STPUSHI\n", cpu_core);
	#endif

	arg1 = code[ep + 1];

	if (sp >= sp_bottom + 8)
	{
		// set stack pointer to lower address

		bptr = (U1 *) &regi[arg1];

		sp--;
		*sp-- = *bptr;
		bptr++;
		*sp-- = *bptr;
		bptr++;
		*sp-- = *bptr;
		bptr++;
		*sp-- = *bptr;
		bptr++;
		*sp-- = *bptr;
		bptr++;
		*sp-- = *bptr;
		bptr++;
		*sp-- = *bptr;
		bptr++;
		*sp = *bptr;
	}
	else
	{
		// fatal ERROR: stack pointer can't go below address ZERO!

		printf ("FATAL ERROR: stack pointer can't go below address 0!\n");
		free (jumpoffs);
		pthread_exit ((void *) 1);
	}

	eoffs = 2;

	EXE_NEXT();

	stpopi:
	#if DEBUG
	printf("%lli STPOPI\n", cpu_core);
	#endif
	arg1 = code[ep + 1];

	if (sp == sp_top)
	{
		// nothing on stack!! can't pop!!

		printf ("FATAL ERROR: stack pointer can't pop empty stack!\n");
		free (jumpoffs);
		pthread_exit ((void *) 1);
	}

	bptr = (U1 *) &regi[arg1];
	bptr += 7;

	*bptr = *sp++;
	bptr--;
	*bptr = *sp++;
	bptr--;
	*bptr = *sp++;
	bptr--;
	*bptr = *sp++;
	bptr--;
	*bptr = *sp++;
	bptr--;
	*bptr = *sp++;
	bptr--;
	*bptr = *sp++;
	bptr--;
	*bptr = *sp++;

	eoffs = 2;

	EXE_NEXT();

	stpushd:
	#if DEBUG
	printf("%lli STPUSHD\n", cpu_core);
	#endif
	arg1 = code[ep + 1];

	if (sp >= sp_bottom + 8)
	{
		// set stack pointer to lower address

		bptr = (U1 *) &regd[arg1];

		sp--;
		*sp-- = *bptr;
		bptr++;
		*sp-- = *bptr;
		bptr++;
		*sp-- = *bptr;
		bptr++;
		*sp-- = *bptr;
		bptr++;
		*sp-- = *bptr;
		bptr++;
		*sp-- = *bptr;
		bptr++;
		*sp-- = *bptr;
		bptr++;
		*sp = *bptr;
	}
	else
	{
		// fatal ERROR: stack pointer can't go below address ZERO!

		printf ("FATAL ERROR: stack pointer can't go below address 0!\n");
		free (jumpoffs);
		pthread_exit ((void *) 1);
	}
	eoffs = 2;

	EXE_NEXT();

	stpopd:
	#if DEBUG
	printf("%lli STPOPD\n", cpu_core);
	#endif
	arg1 = code[ep + 1];

	if (sp == sp_top)
	{
		// nothing on stack!! can't pop!!

		printf ("FATAL ERROR: stack pointer can't pop empty stack!\n");
		free (jumpoffs);
		pthread_exit ((void *) 1);
	}

	bptr = (U1 *) &regd[arg1];
	bptr += 7;

	*bptr = *sp++;
	bptr--;
	*bptr = *sp++;
	bptr--;
	*bptr = *sp++;
	bptr--;
	*bptr = *sp++;
	bptr--;
	*bptr = *sp++;
	bptr--;
	*bptr = *sp++;
	bptr--;
	*bptr = *sp++;
	bptr--;
	*bptr = *sp++;

	eoffs = 2;

	EXE_NEXT();

	loada:
	#if DEBUG
	printf ("%lli LOADA\n", cpu_core);
	#endif
	// data
	bptr = (U1 *) &arg1;

	*bptr = code[ep + 1];
	bptr++;
	*bptr = code[ep + 2];
	bptr++;
	*bptr = code[ep + 3];
	bptr++;
	*bptr = code[ep + 4];
	bptr++;
	*bptr = code[ep + 5];
	bptr++;
	*bptr = code[ep + 6];
	bptr++;
	*bptr = code[ep + 7];
	bptr++;
	*bptr = code[ep + 8];

	// offset

	//printf ("arg1: %li\n", arg1);

	bptr = (U1 *) &arg2;

	*bptr = code[ep + 9];
	bptr++;
	*bptr = code[ep + 10];
	bptr++;
	*bptr = code[ep + 11];
	bptr++;
	*bptr = code[ep + 12];
	bptr++;
	*bptr = code[ep + 13];
	bptr++;
	*bptr = code[ep + 14];
	bptr++;
	*bptr = code[ep + 15];
	bptr++;
	*bptr = code[ep + 16];

	//printf ("arg2: %li\n", arg2);

	arg3 = code[ep + 17];

	bptr = (U1 *) &regi[arg3];

	*bptr = data[arg1 + arg2];
	bptr++;
	*bptr = data[arg1 + arg2 + 1];
	bptr++;
	*bptr = data[arg1 + arg2 + 2];
	bptr++;
	*bptr = data[arg1 + arg2 + 3];
	bptr++;
	*bptr = data[arg1 + arg2 + 4];
	bptr++;
	*bptr = data[arg1 + arg2 + 5];
	bptr++;
	*bptr = data[arg1 + arg2 + 6];
	bptr++;
	*bptr = data[arg1 + arg2 + 7];

	//printf ("LOADA: %li\n", regi[arg3]);

	eoffs = 18;
	EXE_NEXT();

	loadd:
	#if DEBUG
	printf ("%lli LOADD\n", cpu_core);
	#endif
	// data
	bptr = (U1 *) &arg1;

	*bptr = code[ep + 1];
	bptr++;
	*bptr = code[ep + 2];
	bptr++;
	*bptr = code[ep + 3];
	bptr++;
	*bptr = code[ep + 4];
	bptr++;
	*bptr = code[ep + 5];
	bptr++;
	*bptr = code[ep + 6];
	bptr++;
	*bptr = code[ep + 7];
	bptr++;
	*bptr = code[ep + 8];

	// offset

	//printf ("arg1: %li\n", arg1);

	bptr = (U1 *) &arg2;

	*bptr = code[ep + 9];
	bptr++;
	*bptr = code[ep + 10];
	bptr++;
	*bptr = code[ep + 11];
	bptr++;
	*bptr = code[ep + 12];
	bptr++;
	*bptr = code[ep + 13];
	bptr++;
	*bptr = code[ep + 14];
	bptr++;
	*bptr = code[ep + 15];
	bptr++;
	*bptr = code[ep + 16];

	//printf ("arg2: %li\n", arg2);

	arg3 = code[ep + 17];

	bptr = (U1 *) &regd[arg3];

	*bptr = data[arg1 + arg2];
	bptr++;
	*bptr = data[arg1 + arg2 + 1];
	bptr++;
	*bptr = data[arg1 + arg2 + 2];
	bptr++;
	*bptr = data[arg1 + arg2 + 3];
	bptr++;
	*bptr = data[arg1 + arg2 + 4];
	bptr++;
	*bptr = data[arg1 + arg2 + 5];
	bptr++;
	*bptr = data[arg1 + arg2 + 6];
	bptr++;
	*bptr = data[arg1 + arg2 + 7];

	//printf ("LOADD: %li\n", regd[arg3]);

	eoffs = 18;
	EXE_NEXT();

	intr0:

	arg1 = code[ep + 1];
	#if DEBUG
	printf ("%lli INTR0: %lli\n", cpu_core, arg1);
	#endif
	switch (arg1)
	{
		case 0:
			//printf ("LOADMODULE\n");
			arg2 = code[ep + 2];
			arg3 = code[ep + 3];

			if (load_module ((U1 *) &data[regi[arg2]], regi[arg3]) != 0)
			{
				printf ("EXIT!\n");
				free (jumpoffs);
				pthread_exit ((void *) 1);
			}
			eoffs = 5;
			break;

		case 1:
			//printf ("FREEMODULE\n");
			arg2 = code[ep + 2];

			free_module (regi[arg2]);
			eoffs = 5;
			break;

		case 2:
			//printf ("SETMODULEFUNC\n");
			arg2 = code[ep + 2];
			arg3 = code[ep + 3];
			arg4 = code[ep + 4];

			if (set_module_func (regi[arg2], regi[arg3], (U1 *) &data[regi[arg4]]) != 0)
			{
				printf ("EXIT!\n");
				free (jumpoffs);
				pthread_exit ((void *) 1);
			}
			eoffs = 5;
			break;

		case 3:
			//printf ("CALLMODULEFUNC\n");
			arg2 = code[ep + 2];
			arg3 = code[ep + 3];

			sp = call_module_func (regi[arg2], regi[arg3], (U1 *) sp, sp_top, sp_bottom, (U1 *) data);
			eoffs = 5;
			break;

		case 4:
			//printf ("PRINTI\n");
			arg2 = code[ep + 2];
			printf ("%lli", regi[arg2]);
			eoffs = 5;
			break;

		case 5:
			//printf ("PRINTD\n");
			arg2 = code[ep + 2];
			printf ("%.10lf", regd[arg2]);
			eoffs = 5;
			break;

		case 6:
			//printf ("PRINTSTR\n");
			arg2 = code[ep + 2];
			printf ("%s", (char *) &data[regi[arg2]]);
			eoffs = 5;
			break;

		case 7:
			//printf ("PRINTNEWLINE\n");
			printf ("\n");
			eoffs = 5;
			break;

		case 8:
			printf ("DELAY\n");
			arg2 = code[ep + 2];
			usleep (regi[arg2] * 1000);
			#if DEBUG
				printf ("delay: %lli\n", regi[arg2]);
			#endif
			eoffs = 5;
			break;

		case 9:
			//printf ("INPUTI\n");
			arg2 = code[ep + 2];
			if (fgets ((char *) input_str, MAXINPUT - 1, stdin) != NULL)
			{
				sscanf ((const char *) input_str, "%lli", &regi[arg2]);
			}
			else
			{
				printf ("input integer: can't read!\n");
			}
			eoffs = 5;
			break;

		case 10:
			//printf ("INPUTD\n");
			arg2 = code[ep + 2];
			if (fgets ((char *) input_str, MAXINPUT - 1, stdin) != NULL)
			{
				sscanf ((const char *) input_str, "%lf", &regd[arg2]);
			}
			else
			{
				printf ("input double: can't read!\n");
			}
			eoffs = 5;
			break;

		case 11:
			//printf ("INPUTS\n");
			arg2 = code[ep + 2];
			arg3 = code[ep + 3];
			if (fgets ((char *) &data[regi[arg3]], regi[arg2], stdin) != NULL)
			{
				// set END OF STRING mark
				i = strlen ((const char *) &data[regi[arg3]]);
				data[regi[arg3] + (i - 1)] = '\0';
			}
			else
			{
				printf ("input string: can't read!\n");
			}
			eoffs = 5;
			break;

		case 12:
			//printf ("SHELLARGSNUM\n");
			arg2 = code[ep + 2];
			regi[arg2] = shell_args_ind + 1;
			eoffs = 5;
			break;

		case 13:
			//printf ("GETSHELLARG\n");
			arg2 = code[ep + 2];
			arg3 = code[ep + 3];
			if (regi[arg2] > shell_args_ind)
			{
				printf ("ERROR: shell argument index out of range!\n");
				free (jumpoffs);
				pthread_exit ((void *) 1);
			}
			strncpy ((char *) &data[regi[arg3]], (const char *) shell_args[regi[arg2]], MAXSHELLARGLEN);
			eoffs = 5;
			break;

		case 14:
			//printf("SHOWSTACKPOINTER\n");
			printf ("stack pointer sp: %lli\n", (S8) sp);
			eoffs = 5;
			break;

        case 15:
            // return number of CPU cores available
            arg2 = code[ep + 2];
            regi[arg2] = max_cpu;
            eoffs = 5;
            break;

        case 16:
            // return endianess of host machine
            arg2 = code[ep + 2];
#if MACHINE_BIG_ENDIAN
            regi[arg2] = 1;
#else
            regi[arg2] = 0;
#endif
            eoffs = 5;
            break;

#if JIT_COMPILER
        case 253:
        // run JIT compiler
            arg2 = code[ep + 2];
            arg3 = code[ep + 3];

            if (jit_compiler ((U1 *) code, (U1 *) data, (S8 *) jumpoffs, (S8 *) &regi, (F8 *) &regd, (U1 *) sp, sp_top, sp_bottom, regi[arg2], regi[arg3]) != 0)
            {
                printf ("FATAL ERROR: JIT compiler: can't compile!\n");
                free (jumpoffs);
            	pthread_exit ((void *) 1);
            }

            eoffs = 5;
            break;

        case 254:
            arg2 = code[ep + 2];
            // printf ("intr0: 254: RUN JIT CODE: %i\n", arg2);
            run_jit (regi[arg2]);

            eoffs = 5;
            break;
#endif


		case 255:
			printf ("EXIT\n");
			arg2 = code[ep + 2];
			retcode = regi[arg2];
			free (jumpoffs);
			pthread_mutex_lock (&data_mutex);
			threaddata[cpu_core].status = STOP;
			pthread_mutex_unlock (&data_mutex);
			pthread_exit ((void *) retcode);
	}
	EXE_NEXT();

	intr1:
	#if DEBUG
	printf("%lli INTR1\n", cpu_core);
	#endif
	// special interrupt
	arg1 = code[ep + 1];

	switch (arg1)
	{
		case 0:
			// run new CPU instance
			arg2 = code[ep + 2];
			arg2 = regi[arg2];

            // search for a free CPU core
            // if none free found set new_cpu to -1, to indicate all CPU cores are used!!
            new_cpu = -1;
            pthread_mutex_lock (&data_mutex);
            for (i = 0; i < max_cpu; i++)
            {
                if (threaddata[i].status == STOP)
                {
                    new_cpu = i;
                    break;
                }
            }
	        pthread_mutex_unlock (&data_mutex);
			if (new_cpu == -1)
			{
				// maximum of CPU cores used, no new core possible

				printf ("ERROR: can't start new CPU core!\n");
				free (jumpoffs);
				pthread_exit ((void *) 1);
			}

			// sp_top = threaddata[cpu_core].sp_top_thread;
			// sp_bottom = threaddata[cpu_core].sp_bottom_thread;
			// sp = threaddata[cpu_core].sp_thread;

            // run new CPU core
            // set threaddata

			printf ("current CPU: %lli, starts new CPU: %lli\n", cpu_core, new_cpu);
			pthread_mutex_lock (&data_mutex);
			threaddata[new_cpu].sp = sp;
			threaddata[new_cpu].sp_top = sp_top;
			threaddata[new_cpu].sp_bottom = sp_bottom;

			threaddata[new_cpu].sp_top_thread = sp_top + (new_cpu * stack_size);
			threaddata[new_cpu].sp_bottom_thread = sp_bottom + (new_cpu * stack_size);
			threaddata[new_cpu].sp_thread = threaddata[new_cpu].sp_top_thread - (sp_top - sp);
			threaddata[new_cpu].ep_startpos = arg2;
			pthread_mutex_unlock (&data_mutex);

            // create new POSIX thread

			if (pthread_create (&threaddata[new_cpu].id, NULL, (void *) run, (void*) new_cpu) != 0)
			{
				printf ("ERROR: can't start new thread!\n");
				free (jumpoffs);
				pthread_exit ((void *) 1);
			}


            #if CPU_SET_AFFINITY
            // LOCK thread to CPU core

            CPU_ZERO (&cpuset);
            CPU_SET (new_cpu, &cpuset);

            if (pthread_setaffinity_np (threaddata[new_cpu].id, sizeof(cpu_set_t), &cpuset) != 0)
            {
                    printf ("ERROR: setting pthread affinity of thread: %lli\n", new_cpu);
            }
            #endif

			pthread_mutex_lock (&data_mutex);
			threaddata[new_cpu].status = RUNNING;
			pthread_mutex_unlock (&data_mutex);
			eoffs = 5;
			break;

		case 1:
			// join threads
			printf ("JOINING THREADS...\n");
			U1 wait = 1, running;
			while (wait == 1)
			{
				running = 0;
				pthread_mutex_lock (&data_mutex);
				for (i = 1; i < MAXCPUCORES; i++)
				{
					if (threaddata[i].status == RUNNING)
					{
						// printf ("CPU: %lli running\n", i);
						running = 1;
					}
				}
				pthread_mutex_unlock (&data_mutex);
				if (running == 0)
				{
					// no child threads running, joining done
					wait = 0;
				}

				usleep (200);
			}

			eoffs = 5;
			break;

		case 2:
			// lock data_mutex
			pthread_mutex_lock (&data_mutex);
			eoffs = 5;
			break;

		case 3:
			// unlock data_mutex
			pthread_mutex_unlock (&data_mutex);
			eoffs = 5;
			break;

		case 4:
			// return number of current CPU core
			arg2 = code[ep + 2];
			regi[arg2] = cpu_core;

			eoffs = 5;
			break;

		case 255:
			printf ("thread EXIT\n");
			arg2 = code[ep + 2];
			retcode = regi[arg2];
			pthread_mutex_lock (&data_mutex);
			threaddata[cpu_core].status = STOP;
			pthread_mutex_unlock (&data_mutex);
			pthread_exit ((void *) retcode);
	}
	EXE_NEXT();

	//  superopcodes for counter loops
	inclsijmpi:
	#if DEBUG
	printf ("%lli INCLSIJMPI\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = code[ep + 2];

	arg3 = jumpoffs[ep];

	//printf ("jump to: %li\n", arg3);

	regi[arg1]++;
	if (regi[arg1] < regi[arg2])
	{
		eoffs = 0;
		ep = arg3;
		EXE_NEXT();
	}

	eoffs = 11;

	EXE_NEXT();

	decgrijmpi:
	#if DEBUG
	printf ("%lli DECGRIJMPI\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = code[ep + 2];

	arg3 = jumpoffs[ep];

	regi[arg1]--;
	if (regi[arg1] > regi[arg2])
	{
		eoffs = 0;
		ep = arg3;
		EXE_NEXT();
	}

	eoffs = 11;

	EXE_NEXT();

	movi:
	#if DEBUG
	printf ("%lli MOVI\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = code[ep + 2];

	regi[arg2] = regi[arg1];

	eoffs = 3;
	EXE_NEXT();

	movd:
	#if DEBUG
	printf ("%lli MOVD\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = code[ep + 2];

	regd[arg2] = regd[arg1];

	eoffs = 3;
	EXE_NEXT();

	loadl:
	#if DEBUG
	printf ("%lli LOADL\n", cpu_core);
	#endif
	// data
	bptr = (U1 *) &arg1;
	arg2 = code[ep + 9];

	*bptr = code[ep + 1];
	bptr++;
	*bptr = code[ep + 2];
	bptr++;
	*bptr = code[ep + 3];
	bptr++;
	*bptr = code[ep + 4];
	bptr++;
	*bptr = code[ep + 5];
	bptr++;
	*bptr = code[ep + 6];
	bptr++;
	*bptr = code[ep + 7];
	bptr++;
	*bptr = code[ep + 8];

	regi[arg2] = arg1;

	eoffs = 10;
	EXE_NEXT();

	jmpa:
	#if DEBUG
	printf ("%lli JMPA\n", cpu_core);
	#endif
	arg1 = code[ep + 1];

	eoffs = 0;
	ep = regi[arg1];
	#if DEBUG
	printf ("%lli JUMP TO %lli\n", cpu_core, ep);
	#endif
	EXE_NEXT();

	jsr:
	#if DEBUG
	printf ("%lli JSR\n", cpu_core);
	#endif
	arg1 = jumpoffs[ep];

	if (jumpstack_ind == MAXSUBJUMPS - 1)
	{
		printf ("ERROR: jumpstack full, no more jsr!\n");
		free (jumpoffs);
		pthread_exit ((void *) 1);
	}

	jumpstack_ind++;
	jumpstack[jumpstack_ind] = ep + 9;

	eoffs = 0;
	ep = arg1;

	EXE_NEXT();

	jsra:
	#if DEBUG
	printf ("%lli JSRA\n", cpu_core);
	#endif

	arg1 = code[ep + 1];

	#if DEBUG
	printf ("%lli JUMP TO %lli\n", cpu_core, ep);
	#endif

	if (jumpstack_ind == MAXSUBJUMPS - 1)
	{
		printf ("ERROR: jumpstack full, no more jsr!\n");
		free (jumpoffs);
		pthread_exit ((void *) 1);
	}

	jumpstack_ind++;
	jumpstack[jumpstack_ind] = ep + 2;

	eoffs = 0;
	ep = regi[arg1];

	EXE_NEXT();

	rts:
	#if DEBUG
	printf ("%lli RTS\n", cpu_core);
	#endif

	ep = jumpstack[jumpstack_ind];
	eoffs = 0;
	jumpstack_ind--;

	// printf ("RTS: return to: %lli\n\n", ep);

	EXE_NEXT();

	load:
	#if DEBUG
	printf ("%lli LOAD\n", cpu_core);
	#endif
	// data
	bptr = (U1 *) &arg1;

	*bptr = code[ep + 1];
	bptr++;
	*bptr = code[ep + 2];
	bptr++;
	*bptr = code[ep + 3];
	bptr++;
	*bptr = code[ep + 4];
	bptr++;
	*bptr = code[ep + 5];
	bptr++;
	*bptr = code[ep + 6];
	bptr++;
	*bptr = code[ep + 7];
	bptr++;
	*bptr = code[ep + 8];

	// offset

	//printf ("arg1: %li\n", arg1);

	bptr = (U1 *) &arg2;

	*bptr = code[ep + 9];
	bptr++;
	*bptr = code[ep + 10];
	bptr++;
	*bptr = code[ep + 11];
	bptr++;
	*bptr = code[ep + 12];
	bptr++;
	*bptr = code[ep + 13];
	bptr++;
	*bptr = code[ep + 14];
	bptr++;
	*bptr = code[ep + 15];
	bptr++;
	*bptr = code[ep + 16];

	//printf ("arg2: %li\n", arg2);

	arg3 = code[ep + 17];

	regi[arg3] = arg1 + arg2;

	//printf ("LOAD: %li\n", regi[arg3]);

	eoffs = 18;
	EXE_NEXT();


    noti:
	#if DEBUG
	printf ("%lli NOTI\n", cpu_core);
	#endif
	arg1 = code[ep + 1];
	arg2 = code[ep + 2];

	regi[arg2] = ! regi[arg1];

	eoffs = 3;
	EXE_NEXT();
}

void break_handler (void)
{
	/* break - handling
	 *
	 * if user answer is 'y', the engine will shutdown
	 *
	 */

	U1 answ[2];

	printf ("\nexe: break,  exit now (y/n)? ");
	scanf ("%1s", answ);

	if (strcmp ((const char *) answ, "y") == 0 || strcmp ((const char *) answ, "Y") == 0)
	{
		cleanup ();
		exit (1);
	}
}

void init_modules (void)
{
    S8 i ALIGN;

    for (i = 0; i < MODULES; i++)
    {
        strcpy ((char *) modules[i].name, "");
    }
}

void free_modules (void)
{
    S8 i ALIGN;

    for (i = MODULES - 1; i >= 0; i--)
    {
        if (modules[i].name[0] != '\0')
        {
            printf ("free_modules: module %lli free.\n", i);
            free_module (i);
        }
    }
}

int main (int ac, char *av[])
{
	S8 i ALIGN;
	S8 arglen ALIGN;
	S8 strind ALIGN;
	S8 avind ALIGN;
	pthread_t id;

	S8 new_cpu ALIGN;

    printf ("l1vm - 0.9.1 - (C) 2017-2019 Stefan Pietzonke\n");

#if MAXCPUCORES == 0
    max_cpu = sysconf (_SC_NPROCESSORS_ONLN);
    printf ("CPU cores: %lli (autoconfig)\n", max_cpu);
#else
    printf ("CPU cores: %lli (STATIC)\n", max_cpu);
#endif

#if JIT_COMPILER
    printf ("JIT-compiler inside: lib asmjit\n");
#endif

    if (ac > 1)
    {
        for (i = 2; i < ac; i++)
        {
            arglen = strlen (av[i]);
            if (arglen > 2)
            {
                if (av[i][0] == '-' && av[i][1] == 'M')
                {
                    // try load module (shared library)

                    if (modules_ind < MODULES)
                    {
                        modules_ind++;
                        strind = 0; avind = 2;
                        for (avind = 2; avind < arglen; avind++)
                        {
                            modules[modules_ind].name[strind] = av[i][avind];
                            strind++;
                        }
                        modules[modules_ind].name[strind] = '\0';

                        if (load_module (modules[modules_ind].name, modules_ind) == 0)
                        {
                            printf ("module: %s loaded\n", modules[modules_ind].name);
                        }
                        else
                        {
                            printf ("EXIT!\n");
                            exit (1);
                        }
                    }
                }
                else
				{
                    if (av[i][0] == '-' && av[i][1] == 'S' && av[i][2] == 'D' && av[i][3] == 'L')
                    {
                        // initialize SDL, set SDL flag!
                        SDL_use = 1;
                    }
                    else
                    {
			            shell_args_ind++;
					    if (shell_args_ind >= MAXSHELLARGS)
					    {
                            printf ("ERROR: too many shell arguments!\n");
						    exit (1);
					    }
					    strncpy ((char *) shell_args[shell_args_ind], av[i], MAXSHELLARGLEN);
                    }
				}
            }
        }
    }
    else
	{
		printf ("l1vm <program>\n");
		exit (1);
	}
    if (load_object ((U1 *) av[1]))
    {
        exit (1);
    }

    init_modules ();
	signal (SIGINT, (void *) break_handler);

#if SDL_module
    if (SDL_use == 1)
    {
        if (SDL_Init (SDL_INIT_EVERYTHING) != 0)
	    {
			printf ("ERROR SDL_Init!!!");
			exit (1);
		}

		/* key input settings */

	    SDL_EnableUNICODE (SDL_ENABLE);
	    SDL_EnableKeyRepeat (500, 125);

		if (TTF_Init () < 0)
		{
			printf ("ERROR TTF_Init!!!");
			exit (1);
		}

		printf ("SDL initialized...\n");
	}
#endif

	// set all higher threads as STOPPED = unused
	for (i = 1; i < max_cpu; i++)
	{
		threaddata[i].status = STOP;
	}
	threaddata[0].status = RUNNING;		// main thread will run

	new_cpu = 0;
	// threaddata[new_cpu].sp = (U1 *) &data[data_mem_size - (max_cpu * stack_size) - 1];
	threaddata[new_cpu].sp = (U1 *) &data + (data_mem_size - ((max_cpu - 1) * stack_size) - 1);
	threaddata[new_cpu].sp_top = threaddata[new_cpu].sp;
	threaddata[new_cpu].sp_bottom = threaddata[new_cpu].sp_top - stack_size + 1;

	threaddata[new_cpu].sp_thread = threaddata[new_cpu].sp + (new_cpu * stack_size);
	threaddata[new_cpu].sp_top_thread = threaddata[new_cpu].sp_top + (new_cpu * stack_size);
	threaddata[new_cpu].sp_bottom_thread = threaddata[new_cpu].sp_bottom + (new_cpu * stack_size);
	threaddata[new_cpu].ep_startpos = 16;

    if (pthread_create (&id, NULL, (void *) run, (void *) new_cpu) != 0)
	{
		printf ("ERROR: can't start new thread!\n");
		cleanup ();
		exit (1);
	}
    pthread_join (id, NULL);
	cleanup ();
	exit (retcode);
}
