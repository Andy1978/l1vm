/*
 * This file mpfr-head.c is part of L1vm.
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

#include "../../../include/global.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <mpfr.h>
#include <mpreal.h>


#include "../../../include/stack.h"

using mpfr::mpreal;
using std::cout;
using std::endl;


#define MAX_FLOAT_NUM 256

static mpreal mpf_float[MAX_FLOAT_NUM];

char *fgets_uni (char *str, int len, FILE *fptr)
{
    int ch, nextch;
    int i = 0, eol = FALSE;
    char *ret;

    ch = fgetc (fptr);
    if (feof (fptr))
    {
        return (NULL);
    }
    while (! feof (fptr) || i == len - 2)
    {
        switch (ch)
        {
            case '\r':
                /* check for '\r\n\' */

                nextch = fgetc (fptr);
                if (! feof (fptr))
                {
                    if (nextch != '\n')
                    {
                        ungetc (nextch, fptr);
                    }
                }
                str[i] = '\n';
                i++; eol = TRUE;
                break;

            case '\n':
                /* check for '\n\r\' */

                nextch = fgetc (fptr);
                if (! feof (fptr))
                {
                    if (nextch != '\r')
                    {
                        ungetc (nextch, fptr);
                    }
                }
                str[i] = '\n';
                i++; eol = TRUE;
                break;

            default:
				str[i] = ch;
				i++;

                break;
        }

        if (eol)
        {
            break;
        }

        ch = fgetc (fptr);
    }

    if (feof (fptr))
    {
//        str[i] = '\n';
//        i++;
        str[i] = '\0';
    }
    else
    {
        str[i] = '\0';
    }

    ret = str;
    return (ret);
}


size_t strlen_safe (const char * str, int maxlen)
{
	 long long int i = 0;

	 while (1)
	 {
	 	if (str[i] != '\0')
		{
			i++;
		}
		else
		{
			return (i);
		}
		if (i > maxlen)
		{
			return (0);
		}
	}
}

extern "C" U1 *mp_set_float (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	S8 float_index ALIGN;
	S8 numstring_address ALIGN;
	S8 num_base ALIGN;

	sp = stpopi ((U1 *) &float_index, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_set_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &num_base, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_set_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &numstring_address, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_set_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	if (float_index >= MAX_FLOAT_NUM || float_index < 0)
	{
		printf ("mp_set_float: ERROR float index out of range! Must be 0 < %i\n", MAX_FLOAT_NUM);
		return (NULL);
	}

	// printf ("DEBUG: mp_set_float: '%s'\n", (const char *) &data[numstring_address]);

	mpf_float[float_index] = mpfr::mpreal ((const char *) &data[numstring_address]);
	return (sp);
}

// get const ==================================================================

extern "C" U1 *mp_get_pi_float (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	S8 float_index_res ALIGN;
	S8 float_prec ALIGN;

	sp = stpopi ((U1 *) &float_index_res, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_get_pi_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	// floating point precision
	sp = stpopi ((U1 *) &float_prec, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_get_pi_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	mpf_float[float_index_res] = mpfr::const_pi (float_prec, MPFR_RNDN);
	return (sp);
}

extern "C" U1 *mp_get_log2_float (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	S8 float_index_res ALIGN;
	S8 float_prec ALIGN;

	sp = stpopi ((U1 *) &float_index_res, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_get_log2_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	// floating point precision
	sp = stpopi ((U1 *) &float_prec, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_get_log2_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	mpf_float[float_index_res] = mpfr::const_log2 (float_prec, MPFR_RNDN);
	return (sp);
}

extern "C" U1 *mp_get_euler_float (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	S8 float_index_res ALIGN;
	S8 float_prec ALIGN;

	sp = stpopi ((U1 *) &float_index_res, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_get_euler_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	// floating point precision
	sp = stpopi ((U1 *) &float_prec, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_get_euler_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	mpf_float[float_index_res] = mpfr::const_euler (float_prec, MPFR_RNDN);
	return (sp);
}

extern "C" U1 *mp_get_catalan_float (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	S8 float_index_res ALIGN;
	S8 float_prec ALIGN;

	sp = stpopi ((U1 *) &float_index_res, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_get_catalan_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	// floating point precision
	sp = stpopi ((U1 *) &float_prec, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_get_catalan_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	mpf_float[float_index_res] = mpfr::const_catalan (float_prec, MPFR_RNDN);
	return (sp);
}


// print float number =========================================================

extern "C" U1 *mp_print_float (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	S8 float_index_x ALIGN;
	S8 float_format_address ALIGN;
	S8 precision_out ALIGN;

	sp = stpopi ((U1 *) &precision_out, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_print_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &float_format_address, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_print_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &float_index_x, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_print_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	if (float_index_x >= MAX_FLOAT_NUM || float_index_x < 0)
	{
		printf ("mp_print_float: ERROR float index x out of range! Must be 0 < %i\n", MAX_FLOAT_NUM);
		return (NULL);
	}

	cout.precision (precision_out);
	cout << std::fixed;
	cout << mpf_float[float_index_x];

	return (sp);
}

extern "C" U1 *mp_prints_float (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	S8 float_index ALIGN;
	S8 numstring_address_dest ALIGN;
	S8 numstring_len ALIGN;
	S8 float_format_address ALIGN;
	S8 precision ALIGN;

	std::fstream tempfile;
    tempfile.open("temp.txt", std::ios::out);
    std::string line;

	sp = stpopi ((U1 *) &precision, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_prints_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &float_format_address, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_prints_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &numstring_len, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_prints_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &numstring_address_dest, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_prints_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &float_index, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_prints_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	if (float_index >= MAX_FLOAT_NUM || float_index < 0)
	{
		printf ("mp_prints_float: ERROR float index out of range! Must be 0 < %i\n", MAX_FLOAT_NUM);
		return (NULL);
	}

    // Backup streambuffers of  cout
    std::streambuf* stream_buffer_cout = std::cout.rdbuf();
    // std::streambuf* stream_buffer_cin = std::cin.rdbuf();

    // Get the streambuffer of the file
    std::streambuf* stream_buffer_file = tempfile.rdbuf();

    // Redirect cout to file
    cout.rdbuf(stream_buffer_file);
	cout.precision (precision);
	cout << std::fixed;
    cout << mpf_float[float_index];

    // Redirect cout back to screen
    cout.rdbuf(stream_buffer_cout);
    tempfile.close();

	// read string from file
	FILE *tempfilec;
	tempfilec = fopen ("temp.txt", "r");
	if (tempfilec != NULL)
  	{
		if (fgets_uni ((char *) &data[numstring_address_dest], numstring_len, tempfilec) == NULL)
		{
			printf ("mp_prints_float: ERROR output string overflow!\n");
			return (NULL);
		}
		fclose (tempfilec);
	}
	else
	{
		printf ("mp_prints_float: can't open temp file!\n");
		return (NULL);
	}
	return (sp);
}

// cleanup ====================================================================
extern "C" U1 *mp_cleanup (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	mpfr_free_cache ();
	return (sp);
}

// float math =================================================================

// calculate float ============================================================

extern "C" U1 *mp_add_float (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	S8 float_index_x ALIGN;
	S8 float_index_y ALIGN;
	S8 float_index_res ALIGN;

	sp = stpopi ((U1 *) &float_index_res, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_add_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &float_index_y, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_add_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &float_index_x, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_add_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	if (float_index_x >= MAX_FLOAT_NUM || float_index_x < 0)
	{
		printf ("mp_add_float: ERROR float index x out of range! Must be 0 < %i\n", MAX_FLOAT_NUM);
		return (NULL);
	}

	if (float_index_y >= MAX_FLOAT_NUM || float_index_y < 0)
	{
		printf ("mp_add_float: ERROR float index y out of range! Must be 0 < %i\n", MAX_FLOAT_NUM);
		return (NULL);
	}

	if (float_index_res >= MAX_FLOAT_NUM || float_index_res < 0)
	{
		printf ("mp_add_float: ERROR float index res out of range! Must be 0 < %i\n", MAX_FLOAT_NUM);
		return (NULL);
	}

	//mpf_float[float_index_res] = mpf_float[float_index_x] + mpf_float[float_index_y];
	mpfr_add (mpf_float[float_index_res].mpfr_ptr(), mpf_float[float_index_x].mpfr_srcptr(), mpf_float[float_index_y].mpfr_srcptr(), MPFR_RNDN);
	return (sp);
}

extern "C" U1 *mp_sub_float (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	S8 float_index_x ALIGN;
	S8 float_index_y ALIGN;
	S8 float_index_res ALIGN;

	sp = stpopi ((U1 *) &float_index_res, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_sub_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &float_index_y, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_sub_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &float_index_x, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_sub_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	if (float_index_x >= MAX_FLOAT_NUM || float_index_x < 0)
	{
		printf ("mp_sub_float: ERROR float index x out of range! Must be 0 < %i\n", MAX_FLOAT_NUM);
		return (NULL);
	}

	if (float_index_y >= MAX_FLOAT_NUM || float_index_y < 0)
	{
		printf ("mp_sub_float: ERROR float index y out of range! Must be 0 < %i\n", MAX_FLOAT_NUM);
		return (NULL);
	}

	if (float_index_res >= MAX_FLOAT_NUM || float_index_res < 0)
	{
		printf ("mp_sub_float: ERROR float index res out of range! Must be 0 < %i\n", MAX_FLOAT_NUM);
		return (NULL);
	}

	//mpf_float[float_index_res] = mpf_float[float_index_x] - mpf_float[float_index_y];
	mpfr_sub (mpf_float[float_index_res].mpfr_ptr(), mpf_float[float_index_x].mpfr_srcptr(), mpf_float[float_index_y].mpfr_srcptr(), MPFR_RNDN);
	return (sp);
}

extern "C" U1 *mp_mul_float (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	S8 float_index_x ALIGN;
	S8 float_index_y ALIGN;
	S8 float_index_res ALIGN;

	sp = stpopi ((U1 *) &float_index_res, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_mul_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &float_index_y, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_mul_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &float_index_x, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_mul_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	if (float_index_x >= MAX_FLOAT_NUM || float_index_x < 0)
	{
		printf ("mp_mul_float: ERROR float index x out of range! Must be 0 < %i\n", MAX_FLOAT_NUM);
		return (NULL);
	}

	if (float_index_y >= MAX_FLOAT_NUM || float_index_y < 0)
	{
		printf ("mp_mul_float: ERROR float index y out of range! Must be 0 < %i\n", MAX_FLOAT_NUM);
		return (NULL);
	}

	if (float_index_res >= MAX_FLOAT_NUM || float_index_res < 0)
	{
		printf ("mp_mul_float: ERROR float index res out of range! Must be 0 < %i\n", MAX_FLOAT_NUM);
		return (NULL);
	}

	// mpf_float[float_index_res] = mpf_float[float_index_x] * mpf_float[float_index_y];
	mpfr_mul (mpf_float[float_index_res].mpfr_ptr(), mpf_float[float_index_x].mpfr_srcptr(), mpf_float[float_index_y].mpfr_srcptr(), MPFR_RNDN);
	return (sp);
}

extern "C" U1 *mp_div_float (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	S8 float_index_x ALIGN;
	S8 float_index_y ALIGN;
	S8 float_index_res ALIGN;

	sp = stpopi ((U1 *) &float_index_res, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_div_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &float_index_y, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_div_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &float_index_x, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_div_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	if (float_index_x >= MAX_FLOAT_NUM || float_index_x < 0)
	{
		printf ("mp_div_float: ERROR float index x out of range! Must be 0 < %i\n", MAX_FLOAT_NUM);
		return (NULL);
	}

	if (float_index_y >= MAX_FLOAT_NUM || float_index_y < 0)
	{
		printf ("mp_div_float: ERROR float index y out of range! Must be 0 < %i\n", MAX_FLOAT_NUM);
		return (NULL);
	}

	if (float_index_res >= MAX_FLOAT_NUM || float_index_res < 0)
	{
		printf ("mp_div_float: ERROR float index res out of range! Must be 0 < %i\n", MAX_FLOAT_NUM);
		return (NULL);
	}

	// mpf_float[float_index_res] = mpf_float[float_index_x] / mpf_float[float_index_y];
	mpfr_div (mpf_float[float_index_res].mpfr_ptr(), mpf_float[float_index_x].mpfr_srcptr(), mpf_float[float_index_y].mpfr_srcptr(), MPFR_RNDN);
	return (sp);
}


// compare float ==============================================================

extern "C" U1 *mp_less_float (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	S8 float_index_x ALIGN;
	S8 float_index_y ALIGN;
	S8 float_res ALIGN;

	sp = stpopi ((U1 *) &float_index_y, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_less_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &float_index_x, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_less_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	if (float_index_x >= MAX_FLOAT_NUM || float_index_x < 0)
	{
		printf ("mp_less_float: ERROR float index x out of range! Must be 0 < %i\n", MAX_FLOAT_NUM);
		return (NULL);
	}

	if (float_index_y >= MAX_FLOAT_NUM || float_index_y < 0)
	{
		printf ("mp_less_float: ERROR float index y out of range! Must be 0 < %i\n", MAX_FLOAT_NUM);
		return (NULL);
	}

	float_res = mpf_float[float_index_x] < mpf_float[float_index_y];

	// push result on stack
	sp = stpushi (float_res, sp, sp_bottom);
	if (sp == NULL)
	{
		// error
		printf ("mp_less_float: ERROR: stack corrupt!\n");
		return (NULL);
	}
	return (sp);
}

extern "C" U1 *mp_less_equal_float (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	S8 float_index_x ALIGN;
	S8 float_index_y ALIGN;
	S8 float_res ALIGN;

	sp = stpopi ((U1 *) &float_index_y, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_less_equal_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &float_index_x, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_less_equal_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	if (float_index_x >= MAX_FLOAT_NUM || float_index_x < 0)
	{
		printf ("mp_less_equal_float: ERROR float index x out of range! Must be 0 < %i\n", MAX_FLOAT_NUM);
		return (NULL);
	}

	if (float_index_y >= MAX_FLOAT_NUM || float_index_y < 0)
	{
		printf ("mp_less_equal_float: ERROR float index y out of range! Must be 0 < %i\n", MAX_FLOAT_NUM);
		return (NULL);
	}

	float_res = mpf_float[float_index_x] <= mpf_float[float_index_y];

	// push result on stack
	sp = stpushi (float_res, sp, sp_bottom);
	if (sp == NULL)
	{
		// error
		printf ("mp_less_equal_float: ERROR: stack corrupt!\n");
		return (NULL);
	}
	return (sp);
}

extern "C" U1 *mp_greater_float (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	S8 float_index_x ALIGN;
	S8 float_index_y ALIGN;
	S8 float_res ALIGN;

	sp = stpopi ((U1 *) &float_index_y, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_greater_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &float_index_x, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_greater_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	if (float_index_x >= MAX_FLOAT_NUM || float_index_x < 0)
	{
		printf ("mp_greater_float: ERROR float index x out of range! Must be 0 < %i\n", MAX_FLOAT_NUM);
		return (NULL);
	}

	if (float_index_y >= MAX_FLOAT_NUM || float_index_y < 0)
	{
		printf ("mp_greater_float: ERROR float index y out of range! Must be 0 < %i\n", MAX_FLOAT_NUM);
		return (NULL);
	}

	float_res = mpf_float[float_index_x] > mpf_float[float_index_y];

	// push result on stack
	sp = stpushi (float_res, sp, sp_bottom);
	if (sp == NULL)
	{
		// error
		printf ("mp_greater_float: ERROR: stack corrupt!\n");
		return (NULL);
	}
	return (sp);
}

extern "C" U1 *mp_greater_equal_float (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	S8 float_index_x ALIGN;
	S8 float_index_y ALIGN;
	S8 float_res ALIGN;

	sp = stpopi ((U1 *) &float_index_y, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_greater_equal_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &float_index_x, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_greater_equal_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	if (float_index_x >= MAX_FLOAT_NUM || float_index_x < 0)
	{
		printf ("mp_greater_equal_float: ERROR float index x out of range! Must be 0 < %i\n", MAX_FLOAT_NUM);
		return (NULL);
	}

	if (float_index_y >= MAX_FLOAT_NUM || float_index_y < 0)
	{
		printf ("mp_greater_equal_float: ERROR float index y out of range! Must be 0 < %i\n", MAX_FLOAT_NUM);
		return (NULL);
	}

	float_res = mpf_float[float_index_x] >= mpf_float[float_index_y];

	// push result on stack
	sp = stpushi (float_res, sp, sp_bottom);
	if (sp == NULL)
	{
		// error
		printf ("mp_greater_equal_float: ERROR: stack corrupt!\n");
		return (NULL);
	}
	return (sp);
}

extern "C" U1 *mp_equal_float (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	S8 float_index_x ALIGN;
	S8 float_index_y ALIGN;
	S8 float_res ALIGN;

	sp = stpopi ((U1 *) &float_index_y, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_equal_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &float_index_x, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_equal_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	if (float_index_x >= MAX_FLOAT_NUM || float_index_x < 0)
	{
		printf ("mp_equal_float: ERROR float index x out of range! Must be 0 < %i\n", MAX_FLOAT_NUM);
		return (NULL);
	}

	if (float_index_y >= MAX_FLOAT_NUM || float_index_y < 0)
	{
		printf ("mp_equal_float: ERROR float index y out of range! Must be 0 < %i\n", MAX_FLOAT_NUM);
		return (NULL);
	}

	float_res = mpf_float[float_index_x] == mpf_float[float_index_y];

	// push result on stack
	sp = stpushi (float_res, sp, sp_bottom);
	if (sp == NULL)
	{
		// error
		printf ("mp_equal_float: ERROR: stack corrupt!\n");
		return (NULL);
	}
	return (sp);
}

extern "C" U1 *mp_not_equal_float (U1 *sp, U1 *sp_top, U1 *sp_bottom, U1 *data)
{
	S8 float_index_x ALIGN;
	S8 float_index_y ALIGN;
	S8 float_res ALIGN;

	sp = stpopi ((U1 *) &float_index_y, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_not_equal_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	sp = stpopi ((U1 *) &float_index_x, sp, sp_top);
	if (sp == NULL)
	{
		// error
		printf ("mp_not_equal_float: ERROR: stack corrupt!\n");
		return (NULL);
	}

	if (float_index_x >= MAX_FLOAT_NUM || float_index_x < 0)
	{
		printf ("mp_not_equal_float: ERROR float index x out of range! Must be 0 < %i\n", MAX_FLOAT_NUM);
		return (NULL);
	}

	if (float_index_y >= MAX_FLOAT_NUM || float_index_y < 0)
	{
		printf ("mp_not_equal_float: ERROR float index y out of range! Must be 0 < %i\n", MAX_FLOAT_NUM);
		return (NULL);
	}

	float_res = mpf_float[float_index_x] != mpf_float[float_index_y];

	// push result on stack
	sp = stpushi (float_res, sp, sp_bottom);
	if (sp == NULL)
	{
		// error
		printf ("mp_not_equal_float: ERROR: stack corrupt!\n");
		return (NULL);
	}
	return (sp);
}
