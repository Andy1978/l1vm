/*
 * This file mem.c is part of L1vm.
 *
 * (c) Copyright Stefan Pietzonke (jay-t@gmx.net), 2020
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

// allocate array memory and access it
// for dynamic allocation of memory

#include <cstdlib>
#include <iostream>
#include <vector>

using namespace std;

#include "../../../include/global.h"
#include "../../../include/stack.h"


// normal C arrays
#define MEMBYTE		0
#define MEMINT16	1
#define MEMINT32	2
#define MEMINT64	3
#define MEMDOUBLE	4

// vector types arrays
#define MEMBYTE_VECT	5
#define MEMINT16_VECT	6
#define MEMINT32_VECT	7
#define MEMINT64_VECT	8
#define MEMDOUBLE_VECT	9



// structures

// allocated memory pointers union
union memptr
{
	U1 *byteptr;
	S2 *int16ptr;
	S4 *int32ptr;
	S8 *int64ptr;
	F8 *doubleptr;
	vector<int8_t> vect_int8ptr;
	vector<int16_t> vect_int16ptr;
	vector<int32_t> vect_int32ptr;
	vector<int64_t> vect_int64ptr;
	vector<double_t> vect_doubleptr;

};

// the memory info structure
struct mem
{
	U1 used;
	U1 type;
	S8 memsize ALIGN;
	union memptr memptr;	// the memory pointers to save the allocations
};

static struct mem *mem = NULL;
static S8 memmax ALIGN = 0;


// mem functions ==============================================================

extern "C" U1 *init_mem (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	S8 i ALIGN;
	S8 maxind ALIGN;

	sp = stpopi ((U1 *) &maxind, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("init_mem: ERROR: stack corrupt!\n");
		return (NULL);
	}

	// allocate gobal mem structure
	mem = (struct mem *) calloc (maxind, sizeof (struct mem));
	if (mem == NULL)
	{
		printf ("init_mem: ERROR can't allocate %lli memory indexes!\n", maxind);

		sp = stpushi (1, sp, sp_bottom);
		if (sp == NULL)
		{
			// error
			printf ("init_mem: ERROR: stack corrupt!\n");
			return (NULL);
		}
		return (sp);
	}

	memmax = maxind;	// save to global var

	// init mem structure
	for (i = 0; i < memmax; i++)
	{
		mem[i].used = 0;
		mem[i].memsize = 0;
	}

	// printf ("init_mem: allocated %lli memory spaces\n", maxind);

	// error code ok
	sp = stpushi (0, sp, sp_bottom);
	if (sp == NULL)
	{
		// error
		printf ("init_mem: ERROR: stack corrupt!\n");
		return (NULL);
	}
	return (sp);
}

extern "C" U1 *free_mem (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	// free global mem
	if (mem) free (mem);
	return (sp);
}

extern "C" S8 get_free_mem (void)
{
	S8 i ALIGN;

	for (i = 0; i < memmax; i++)
	{
		if (mem[i].used == 0)
		{
			// return index of free mem
			return (i);
		}
	}

	// no free memory found
	return (-1);
}

extern "C" U1 *alloc_byte (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	S8 memsize ALIGN = 0;
	U1 *memaddr = NULL;

	S8 memind ALIGN;

	sp = stpopi ((U1 *) &memsize, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("alloc_byte: ERROR: stack corrupt!\n");
		return (NULL);
	}

	memind = get_free_mem ();
	if (memind == -1)
	{
		printf ("alloc_byte: no more memory slot free!\n");
		return (NULL);
	}

	memaddr = (U1 *) calloc (memsize, sizeof (U1));
	if (memaddr == NULL)
	{
		printf ("alloc_byte: out of memory: %lli of size  byte\n", memsize);

		sp = stpushi (-1, sp, sp_bottom);
		if (sp == NULL)
		{
			// error
			printf ("alloc_byte: ERROR: stack corrupt!\n");
			return (NULL);
		}
	}

	// save memory pointer
	mem[memind].memptr.byteptr = memaddr;
	mem[memind].used = 1;
	mem[memind].type = MEMBYTE;
	mem[memind].memsize = memsize;

	// push memory structure handle index
	sp = stpushi (memind, sp, sp_bottom);
	if (sp == NULL)
	{
		// error
		printf ("alloc_byte: ERROR: stack corrupt!\n");
		return (NULL);
	}
	return (sp);
}

extern "C" U1 *alloc_int16 (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	S8 memsize ALIGN = 0;
	S2 *memaddr = NULL;

	S8 memind ALIGN;

	sp = stpopi ((U1 *) &memsize, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("alloc_int16: ERROR: stack corrupt!\n");
		return (NULL);
	}

	memind = get_free_mem ();
	if (memind == -1)
	{
		printf ("alloc_int16: no more memory slot free!\n");
		return (NULL);
	}

	memaddr = (S2 *) calloc (memsize, sizeof (S2));
	if (memaddr == NULL)
	{
		printf ("alloc_int16: out of memory: %lli of size int16!\n", memsize);

		sp = stpushi (-1, sp, sp_bottom);
		if (sp == NULL)
		{
			// error
			printf ("alloc_int16: ERROR: stack corrupt!\n");
			return (NULL);
		}
	}

	// save memory pointer
	mem[memind].memptr.int16ptr = memaddr;
	mem[memind].used = 1;
	mem[memind].type = MEMINT16;
	mem[memind].memsize = memsize;

	// push memory structure handle index
	sp = stpushi (memind, sp, sp_bottom);
	if (sp == NULL)
	{
		// error
		printf ("alloc_int16: ERROR: stack corrupt!\n");
		return (NULL);
	}
	return (sp);
}

extern "C" U1 *alloc_int32 (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	S8 memsize ALIGN = 0;
	S4 *memaddr = NULL;

	S8 memind ALIGN;

	sp = stpopi ((U1 *) &memsize, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("alloc_int32: ERROR: stack corrupt!\n");
		return (NULL);
	}

	memind = get_free_mem ();
	if (memind == -1)
	{
		printf ("alloc_int32: no more memory slot free!\n");
		return (NULL);
	}

	memaddr = (S4 *) calloc (memsize, sizeof (S4));
	if (memaddr == NULL)
	{
		printf ("alloc_int32: out of memory: %lli of size int32!\n", memsize);

		sp = stpushi (-1, sp, sp_bottom);
		if (sp == NULL)
		{
			// error
			printf ("alloc_int32: ERROR: stack corrupt!\n");
			return (NULL);
		}
	}

	// save memory pointer
	mem[memind].memptr.int32ptr = memaddr;
	mem[memind].used = 1;
	mem[memind].type = MEMINT32;
	mem[memind].memsize = memsize;

	// push memory structure handle index
	sp = stpushi (memind, sp, sp_bottom);
	if (sp == NULL)
	{
		// error
		printf ("alloc_int32: ERROR: stack corrupt!\n");
		return (NULL);
	}
	return (sp);
}

extern "C" U1 *alloc_int64 (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	S8 memsize ALIGN = 0;
	S8 *memaddr ALIGN = NULL;

	S8 memind ALIGN;

	sp = stpopi ((U1 *) &memsize, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("alloc_int64: ERROR: stack corrupt!\n");
		return (NULL);
	}

	memind = get_free_mem ();
	if (memind == -1)
	{
		printf ("alloc_int64: no more memory slot free!\n");
		return (NULL);
	}

	memaddr = (S8 *) calloc (memsize, sizeof (S8));
	if (memaddr == NULL)
	{
		printf ("alloc_int64: out of memory: %lli of size int64!\n", memsize);

		sp = stpushi (-1, sp, sp_bottom);
		if (sp == NULL)
		{
			// error
			printf ("alloc_int64: ERROR: stack corrupt!\n");
			return (NULL);
		}
	}

	// save memory pointer
	mem[memind].memptr.int64ptr = memaddr;
	mem[memind].used = 1;
	mem[memind].type = MEMINT64;
	mem[memind].memsize = memsize;

	// push memory structure handle index
	sp = stpushi (memind, sp, sp_bottom);
	if (sp == NULL)
	{
		// error
		printf ("alloc_int64: ERROR: stack corrupt!\n");
		return (NULL);
	}
	return (sp);
}

extern "C" U1 *alloc_double (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	S8 memsize ALIGN = 0;
	F8 *memaddr ALIGN = NULL;

	S8 memind ALIGN;

	sp = stpopi ((U1 *) &memsize, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("alloc_double: ERROR: stack corrupt!\n");
		return (NULL);
	}

	memind = get_free_mem ();
	if (memind == -1)
	{
		printf ("alloc_double: no more memory slot free!\n");
		return (NULL);
	}

	memaddr = (F8 *) calloc (memsize, sizeof (F8));
	if (memaddr == NULL)
	{
		printf ("alloc_double: out of memory: %lli of size double!\n", memsize);

		sp = stpushi (-1, sp, sp_bottom);
		if (sp == NULL)
		{
			// error
			printf ("alloc_double: ERROR: stack corrupt!\n");
			return (NULL);
		}
	}

	// save memory pointer
	mem[memind].memptr.doubleptr = memaddr;
	mem[memind].used = 1;
	mem[memind].type = MEMDOUBLE;
	mem[memind].memsize = memsize;

	// push memory structure handle index
	sp = stpushi (memind, sp, sp_bottom);
	if (sp == NULL)
	{
		// error
		printf ("alloc_double: ERROR: stack corrupt!\n");
		return (NULL);
	}
	return (sp);
}

// dealloc mem ----------------------------------------------------------------

extern "C" U1 *dealloc_mem (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	S8 memind ALIGN = 0;

	sp = stpopi ((U1 *) &memind, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("dealloc_mem: ERROR: stack corrupt!\n");
		return (NULL);
	}

	// do sane check
	if (memind < 0 || memind >= memmax)
	{
		printf ("dealloc_mem: ERROR: memory index out of range!\n");
		return (NULL);
	}

	// free memory if allocated and set to used == 1
	switch (mem[memind].type)
	{
		case MEMBYTE:
			if (mem[memind].memptr.byteptr && mem[memind].used == 1)
			{
				free (mem[memind].memptr.byteptr);
			}
			else
			{
				printf ("dealloc_mem: ERROR byte mem not allocated!\n");
				return (NULL);
			}
			break;

		case MEMINT16:
			if (mem[memind].memptr.int16ptr && mem[memind].used == 1)
			{
				free (mem[memind].memptr.int16ptr);
			}
			else
			{
				printf ("dealloc_mem: ERROR int16 mem not allocated!\n");
				return (NULL);
			}
			break;

		case MEMINT32:
			if (mem[memind].memptr.int32ptr && mem[memind].used == 1)
			{
				free (mem[memind].memptr.int32ptr);
			}
			else
			{
				printf ("dealloc_mem: ERROR int32 mem not allocated!\n");
				return (NULL);
			}
			break;

		case MEMINT64:
			if (mem[memind].memptr.int64ptr && mem[memind].used == 1)
			{
				free (mem[memind].memptr.int64ptr);
			}
			else
			{
				printf ("dealloc_mem: ERROR int64 mem not allocated!\n");
				return (NULL);
			}
			break;

		case MEMDOUBLE:
			if (mem[memind].memptr.doubleptr && mem[memind].used == 1)
			{
				free (mem[memind].memptr.doubleptr);
			}
			else
			{
				printf ("dealloc_mem: ERROR double mem not allocated!\n");
				return (NULL);
			}
			break;
	}
	mem[memind].used = 0;
	return (sp);
}


// access array functions int =================================================

extern "C" U1 *int_to_array (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	// assign int to array

	S8 memind ALIGN = 0;
	S8 arrayind ALIGN = 0;
	S8 value ALIGN;

	sp = stpopi ((U1 *) &value, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("int_to_array: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &arrayind, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("int_to_array: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &memind, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("int_to_array: ERROR: stack corrupt!\n");
		return (NULL);
	}

	#if BOUNDSCHECK
	if (memind < 0 || memind >= memmax)
	{
		printf ("int_to_array: ERROR: memory index out of range!\n");
		return (NULL);
	}

	if (arrayind < 0 || arrayind >= mem[memind].memsize)
	{
		printf ("int_to_array: ERROR: array index out of range!\n");
		return (NULL);
	}
	#endif

	// assign to array
	switch (mem[memind].type)
	{
		case MEMBYTE:
			mem[memind].memptr.byteptr[arrayind] = value;
			break;

		case MEMINT16:
			mem[memind].memptr.int16ptr[arrayind] = value;
			break;

		case MEMINT32:
			mem[memind].memptr.int32ptr[arrayind] = value;
			break;

		case MEMINT64:
			mem[memind].memptr.int64ptr[arrayind] = value;
			break;
	}
	return (sp);
}

extern "C" U1 *array_to_int (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	// assign int array to int

	S8 memind ALIGN = 0;
	S8 arrayind ALIGN = 0;
	S8 value ALIGN;

	sp = stpopi ((U1 *) &arrayind, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("array_to_int: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &memind, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("array_to_int: ERROR: stack corrupt!\n");
		return (NULL);
	}

	#if BOUNDSCHECK
	if (memind < 0 || memind >= memmax)
	{
		printf ("array_to_int: ERROR: memory index out of range!\n");
		return (NULL);
	}

	if (arrayind < 0 || arrayind >= mem[memind].memsize)
	{
		printf ("array_to_int: ERROR: array index out of range!\n");
		return (NULL);
	}
	#endif

	// assign to variable
	switch (mem[memind].type)
	{
		case MEMBYTE:
			value = mem[memind].memptr.byteptr[arrayind];
			break;

		case MEMINT16:
			value = mem[memind].memptr.int16ptr[arrayind];
			break;

		case MEMINT32:
			value = mem[memind].memptr.int32ptr[arrayind];
			break;

		case MEMINT64:
			value = mem[memind].memptr.int64ptr[arrayind];
			break;
	}

	sp = stpushi (value, sp, sp_bottom);
	if (sp == NULL)
	{
		// error
		printf ("array_to_int: ERROR: stack corrupt!\n");
		return (NULL);
	}
	return (sp);
}

// access array functions double ==============================================

extern "C" U1 *double_to_array (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	// assign double to double array

	S8 memind ALIGN = 0;
	S8 arrayind ALIGN = 0;
	F8 value ALIGN;

	sp = stpopd ((U1 *) &value, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("double_to_array: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &arrayind, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("double_to_array: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &memind, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("double_to_array: ERROR: stack corrupt!\n");
		return (NULL);
	}

	#if BOUNDSCHECK
	if (memind < 0 || memind >= memmax)
	{
		printf ("double_to_array: ERROR: memory index out of range!\n");
		return (NULL);
	}

	if (arrayind < 0 || arrayind >= mem[memind].memsize)
	{
		printf ("double_to_array: ERROR: array index out of range!\n");
		return (NULL);
	}
	#endif

	// assign to array
	mem[memind].memptr.doubleptr[arrayind] = value;
	return (sp);
}

extern "C" U1 *array_to_double (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	// assign double array to double

	S8 memind ALIGN = 0;
	S8 arrayind ALIGN = 0;
	F8 value ALIGN;

	sp = stpopi ((U1 *) &arrayind, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("array_to_double: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &memind, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("array_to_double: ERROR: stack corrupt!\n");
		return (NULL);
	}

	#if BOUNDSCHECK
	if (memind < 0 || memind >= memmax)
	{
		printf ("array_to_double: ERROR: memory index out of range!\n");
		return (NULL);
	}

	if (arrayind < 0 || arrayind >= mem[memind].memsize)
	{
		printf ("array_to_double: ERROR: array index out of range!\n");
		return (NULL);
	}
	#endif

	// assign to variable
	value = mem[memind].memptr.doubleptr[arrayind];

	sp = stpushd (value, sp, sp_bottom);
	if (sp == NULL)
	{
		// error
		printf ("array_to_double: ERROR: stack corrupt!\n");
		return (NULL);
	}
	return (sp);
}


// vector C++ code ===========================================================
extern "C" U1 *alloc_byte_vect (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	S8 memsize ALIGN = 0;

	S8 memind ALIGN;

	sp = stpopi ((U1 *) &memsize, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("alloc_byte_vect: ERROR: stack corrupt!\n");
		return (NULL);
	}

	memind = get_free_mem ();
	if (memind == -1)
	{
		printf ("alloc_byte_vect: no more memory slot free!\n");
		return (NULL);
	}

	mem[memind].used = 1;
	mem[memind].type = MEMBYTE_VECT;
	mem[memind].memsize = memsize;

	// push memory structure handle index
	sp = stpushi (memind, sp, sp_bottom);
	if (sp == NULL)
	{
		// error
		printf ("alloc_byte_vect: ERROR: stack corrupt!\n");
		return (NULL);
	}
	return (sp);
}

extern "C" U1 *alloc_int16_vect (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	S8 memsize ALIGN = 0;

	S8 memind ALIGN;

	sp = stpopi ((U1 *) &memsize, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("alloc_int16_vect: ERROR: stack corrupt!\n");
		return (NULL);
	}

	memind = get_free_mem ();
	if (memind == -1)
	{
		printf ("alloc_int16_vect: no more memory slot free!\n");
		return (NULL);
	}

	mem[memind].used = 1;
	mem[memind].type = MEMINT16_VECT;
	mem[memind].memsize = memsize;

	// push memory structure handle index
	sp = stpushi (memind, sp, sp_bottom);
	if (sp == NULL)
	{
		// error
		printf ("alloc_int16_vect: ERROR: stack corrupt!\n");
		return (NULL);
	}
	return (sp);
}

extern "C" U1 *alloc_int32_vect (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	S8 memsize ALIGN = 0;

	S8 memind ALIGN;

	sp = stpopi ((U1 *) &memsize, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("alloc_int32_vect: ERROR: stack corrupt!\n");
		return (NULL);
	}

	memind = get_free_mem ();
	if (memind == -1)
	{
		printf ("alloc_int32_vect: no more memory slot free!\n");
		return (NULL);
	}

	mem[memind].used = 1;
	mem[memind].type = MEMINT32_VECT;
	mem[memind].memsize = memsize;

	// push memory structure handle index
	sp = stpushi (memind, sp, sp_bottom);
	if (sp == NULL)
	{
		// error
		printf ("alloc_int32_vect: ERROR: stack corrupt!\n");
		return (NULL);
	}
	return (sp);
}

extern "C" U1 *alloc_int64_vect (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	S8 memsize ALIGN = 0;

	S8 memind ALIGN;

	sp = stpopi ((U1 *) &memsize, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("alloc_int64_vect: ERROR: stack corrupt!\n");
		return (NULL);
	}

	memind = get_free_mem ();
	if (memind == -1)
	{
		printf ("alloc_int64_vect: no more memory slot free!\n");
		return (NULL);
	}

	mem[memind].used = 1;
	mem[memind].type = MEMINT64_VECT;
	mem[memind].memsize = memsize;

	// push memory structure handle index
	sp = stpushi (memind, sp, sp_bottom);
	if (sp == NULL)
	{
		// error
		printf ("alloc_int64_vect: ERROR: stack corrupt!\n");
		return (NULL);
	}
	return (sp);
}

extern "C" U1 *alloc_double_vect (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	S8 memsize ALIGN = 0;

	S8 memind ALIGN;

	sp = stpopi ((U1 *) &memsize, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("alloc_double_vect: ERROR: stack corrupt!\n");
		return (NULL);
	}

	memind = get_free_mem ();
	if (memind == -1)
	{
		printf ("alloc_double_vect: no more memory slot free!\n");
		return (NULL);
	}

	mem[memind].used = 1;
	mem[memind].type = MEMDOUBLE_VECT;
	mem[memind].memsize = memsize;

	// push memory structure handle index
	sp = stpushi (memind, sp, sp_bottom);
	if (sp == NULL)
	{
		// error
		printf ("alloc_double_vect: ERROR: stack corrupt!\n");
		return (NULL);
	}
	return (sp);
}

// dealloc mem vector ---------------------------------------------------------

extern "C" U1 *dealloc_mem_vect (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	S8 memind ALIGN = 0;

	sp = stpopi ((U1 *) &memind, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("dealloc_mem_vect: ERROR: stack corrupt!\n");
		return (NULL);
	}

	// do sane check
	if (memind < 0 || memind >= memmax)
	{
		printf ("dealloc_mem_vect: ERROR: memory index out of range!\n");
		return (NULL);
	}

	// free memory if allocated and set to used == 1
	switch (mem[memind].type)
	{
		case MEMBYTE_VECT:
			if (mem[memind].memptr.byteptr && mem[memind].used == 1)
			{
				mem[memind].memptr.vect_int8ptr.clear ();
				mem[memind].memptr.vect_int8ptr.shrink_to_fit ();
			}
			else
			{
				printf ("dealloc_mem_vect: ERROR byte mem not allocated!\n");
				return (NULL);
			}
			break;

		case MEMINT16_VECT:
			if (mem[memind].memptr.int16ptr && mem[memind].used == 1)
			{
				mem[memind].memptr.vect_int16ptr.clear ();
				mem[memind].memptr.vect_int16ptr.shrink_to_fit ();
			}
			else
			{
				printf ("dealloc_mem_vect: ERROR int16 mem not allocated!\n");
				return (NULL);
			}
			break;

		case MEMINT32_VECT:
			if (mem[memind].memptr.int32ptr && mem[memind].used == 1)
			{
				mem[memind].memptr.vect_int32ptr.clear ();
				mem[memind].memptr.vect_int32ptr.shrink_to_fit ();
			}
			else
			{
				printf ("dealloc_mem_vect: ERROR int32 mem not allocated!\n");
				return (NULL);
			}
			break;

		case MEMINT64_VECT:
			if (mem[memind].memptr.int64ptr && mem[memind].used == 1)
			{
				mem[memind].memptr.vect_int64ptr.clear ();
				mem[memind].memptr.vect_int64ptr.shrink_to_fit ();
			}
			else
			{
				printf ("dealloc_mem_vect: ERROR int64 mem not allocated!\n");
				return (NULL);
			}
			break;

		case MEMDOUBLE_VECT:
			if (mem[memind].memptr.doubleptr && mem[memind].used == 1)
			{
				mem[memind].memptr.vect_doubleptr.clear ();
				mem[memind].memptr.vect_doubleptr.shrink_to_fit ();
			}
			else
			{
				printf ("dealloc_mem_vect: ERROR double mem not allocated!\n");
				return (NULL);
			}
			break;
	}
	mem[memind].used = 0;
	return (sp);
}

// access vector functions int =================================================

extern "C" U1 *int_to_vect (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	// assign int to array

	S8 memind ALIGN = 0;
	S8 value ALIGN;

	sp = stpopi ((U1 *) &value, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("int_to_vect: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &memind, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("int_to_vect: ERROR: stack corrupt!\n");
		return (NULL);
	}

	#if BOUNDSCHECK
	if (memind < 0 || memind >= memmax)
	{
		printf ("int_to_vect: ERROR: memory index out of range!\n");
		return (NULL);
	}
	#endif

	// assign to array
	switch (mem[memind].type)
	{
		case MEMBYTE_VECT:
			mem[memind].memptr.vect_int8ptr.push_back (value);
			break;

		case MEMINT16_VECT:
			mem[memind].memptr.vect_int16ptr.push_back (value);
			break;

		case MEMINT32_VECT:
			mem[memind].memptr.vect_int32ptr.push_back (value);
			break;

		case MEMINT64_VECT:
			mem[memind].memptr.vect_int64ptr.push_back (value);
			break;
	}
	return (sp);
}

extern "C" U1 *double_to_vect (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	// assign double to double array

	S8 memind ALIGN = 0;
	F8 value ALIGN;

	sp = stpopd ((U1 *) &value, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("double_to_vect: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &memind, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("double_to_vect: ERROR: stack corrupt!\n");
		return (NULL);
	}

	#if BOUNDSCHECK
	if (memind < 0 || memind >= memmax)
	{
		printf ("double_to_vect: ERROR: memory index out of range!\n");
		return (NULL);
	}
	#endif

	// assign to array
	mem[memind].memptr.vect_doubleptr.push_back (value);
	return (sp);
}

// vector to variable

extern "C" U1 *vect_to_int (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	// assign int array to int

	S8 memind ALIGN = 0;
	S8 arrayind ALIGN = 0;
 	S8 value ALIGN;

	sp = stpopi ((U1 *) &arrayind, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("vector_to_int: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &memind, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("vector_to_int: ERROR: stack corrupt!\n");
		return (NULL);
	}

	#if BOUNDSCHECK
	if (memind < 0 || memind >= memmax)
	{
		printf ("vector_to_int: ERROR: memory index out of range!\n");
		return (NULL);
	}

	if (arrayind < 0 || arrayind >= mem[memind].memsize)
	{
		printf ("vector_to_int: ERROR: array index out of range!\n");
		return (NULL);
	}
	#endif

	// assign to variable

	switch (mem[memind].type)
	{
		case MEMBYTE_VECT:
			value = mem[memind].memptr.vect_int8ptr[arrayind];
			break;

		case MEMINT16_VECT:
			value = mem[memind].memptr.vect_int16ptr[arrayind];
			break;

		case MEMINT32_VECT:
			value = mem[memind].memptr.vect_int32ptr[arrayind];
			break;

		case MEMINT64_VECT:
			value = mem[memind].memptr.vect_int64ptr[arrayind];
			break;
	}

	sp = stpushi (value, sp, sp_bottom);
	if (sp == NULL)
	{
		// error
		printf ("vector_to_int: ERROR: stack corrupt!\n");
		return (NULL);
	}
	return (sp);
}

extern "C" U1 *vect_to_double (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	// assign double array to double

	S8 memind ALIGN = 0;
	S8 arrayind ALIGN = 0;
	F8 value ALIGN;

	sp = stpopi ((U1 *) &arrayind, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("vector_to_double: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &memind, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("vector_to_double: ERROR: stack corrupt!\n");
		return (NULL);
	}

	#if BOUNDSCHECK
	if (memind < 0 || memind >= memmax)
	{
		printf ("vector_to_double: ERROR: memory index out of range!\n");
		return (NULL);
	}

	if (arrayind < 0 || arrayind >= mem[memind].memsize)
	{
		printf ("vector_to_double: ERROR: array index out of range!\n");
		return (NULL);
	}
	#endif

	// assign to variable
	value = mem[memind].memptr.vect_doubleptr[arrayind];

	sp = stpushd (value, sp, sp_bottom);
	if (sp == NULL)
	{
		// error
		printf ("vector_to_double: ERROR: stack corrupt!\n");
		return (NULL);
	}
	return (sp);
}

// vector erase index =========================================================
extern "C" U1 *vect_erase (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	S8 memind ALIGN = 0;
	S8 arrayind ALIGN = 0;

	sp = stpopi ((U1 *) &arrayind, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("vector_erase: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &memind, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("vector_erase: ERROR: stack corrupt!\n");
		return (NULL);
	}

	#if BOUNDSCHECK
	if (memind < 0 || memind >= memmax)
	{
		printf ("vector_erase: ERROR: memory index out of range!\n");
		return (NULL);
	}

	if (arrayind < 0 || arrayind >= mem[memind].memsize)
	{
		printf ("vector_erase: ERROR: array index out of range!\n");
		return (NULL);
	}
	#endif

	// assign to variable
	switch (mem[memind].type)
	{
		case MEMBYTE_VECT:
			mem[memind].memptr.vect_int8ptr.erase(mem[memind].memptr.vect_int8ptr.begin() + arrayind);
			mem[memind].memptr.vect_int8ptr.shrink_to_fit ();
			break;

		case MEMINT16_VECT:
			mem[memind].memptr.vect_int16ptr.erase(mem[memind].memptr.vect_int16ptr.begin() + arrayind);
			mem[memind].memptr.vect_int16ptr.shrink_to_fit ();
			break;

		case MEMINT32_VECT:
			mem[memind].memptr.vect_int32ptr.erase(mem[memind].memptr.vect_int32ptr.begin() + arrayind);
			mem[memind].memptr.vect_int32ptr.shrink_to_fit ();
			break;

		case MEMINT64_VECT:
			mem[memind].memptr.vect_int64ptr.erase(mem[memind].memptr.vect_int64ptr.begin() + arrayind);
			mem[memind].memptr.vect_int32ptr.shrink_to_fit ();
			break;

		case MEMDOUBLE_VECT:
			mem[memind].memptr.vect_doubleptr.erase(mem[memind].memptr.vect_doubleptr.begin() + arrayind);
			mem[memind].memptr.vect_doubleptr.shrink_to_fit ();
			break;
	}

	return (sp);
}

// vector insert ==============================================================
extern "C" U1 *insert_int_to_vect (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	S8 memind ALIGN = 0;
	S8 arrayind ALIGN = 0;
	S8 value ALIGN;

	sp = stpopi ((U1 *) &value, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("insert_int_to_vect: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &arrayind, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("insert_int_to_vect: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &memind, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("insert_int_to_vect: ERROR: stack corrupt!\n");
		return (NULL);
	}

	#if BOUNDSCHECK
	if (memind < 0 || memind >= memmax)
	{
		printf ("insert_int_to_vect: ERROR: memory index out of range!\n");
		return (NULL);
	}

	if (arrayind < 0 || arrayind >= mem[memind].memsize)
	{
		printf ("insert_int_to_vect: ERROR: array index out of range!\n");
		return (NULL);
	}
	#endif

	// assign to array
	switch (mem[memind].type)
	{
		case MEMBYTE_VECT:
			mem[memind].memptr.vect_int8ptr.insert (mem[memind].memptr.vect_int8ptr.begin() + arrayind, value);
			break;

		case MEMINT16_VECT:
			mem[memind].memptr.vect_int16ptr.insert (mem[memind].memptr.vect_int16ptr.begin() + arrayind, value);
			break;

		case MEMINT32_VECT:
			mem[memind].memptr.vect_int32ptr.insert (mem[memind].memptr.vect_int32ptr.begin() + arrayind, value);
			break;

		case MEMINT64_VECT:
			mem[memind].memptr.vect_int64ptr.insert (mem[memind].memptr.vect_int64ptr.begin() + arrayind, value);
			break;
	}
	return (sp);
}

extern "C" U1 *insert_double_to_vect (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	S8 memind ALIGN = 0;
	S8 arrayind ALIGN = 0;
	F8 value ALIGN;

	sp = stpopd ((U1 *) &value, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("insert_double_to_vect: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &arrayind, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("insert_double_to_vect: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &memind, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("insert_double_to_vect: ERROR: stack corrupt!\n");
		return (NULL);
	}

	#if BOUNDSCHECK
	if (memind < 0 || memind >= memmax)
	{
		printf ("insert_double_to_vect: ERROR: memory index out of range!\n");
		return (NULL);
	}

	if (arrayind < 0 || arrayind >= mem[memind].memsize)
	{
		printf ("insert_double_to_vect: ERROR: array index out of range!\n");
		return (NULL);
	}
	#endif

	// assign to array
	mem[memind].memptr.vect_doubleptr.insert (mem[memind].memptr.vect_doubleptr.begin() + arrayind, value);
	return (sp);
}
