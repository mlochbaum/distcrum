/*
	Copyright (C) 2014-2022 Igor van den Hoven ivdhoven@gmail.com
*/

/*
	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files (the
	"Software"), to deal in the Software without restriction, including
	without limitation the rights to use, copy, modify, merge, publish,
	distribute, sublicense, and/or sell copies of the Software, and to
	permit persons to whom the Software is furnished to do so, subject to
	the following conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
	CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
	TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/*
	crumsort 1.1.5.2
*/

#define CRUM_AUX 512
#define CRUM_OUT  28

size_t FUNC(crum_analyze)(VAR *array, VAR *swap, size_t swap_size, size_t nmemb, CMPFUNC *cmp)
{
	char loop, dist;
	size_t cnt, balance = 0, streaks = 0;
	VAR *pta, *ptb, tmp;

	pta = array;

	for (cnt = nmemb ; cnt > 16 ; cnt -= 16)
	{
		for (dist = 0, loop = 16 ; loop ; loop--)
		{
			dist += cmp(pta, pta + 1) > 0; pta++;
		}
		streaks += (dist == 0) | (dist == 16);
		balance += dist;
	}

	while (--cnt)
	{
		balance += cmp(pta, pta + 1) > 0;
		pta++;
	}

	if (balance == 0)
	{
		return 1;
	}

	if (balance == nmemb - 1)
	{
		pta = array;
		ptb = array + nmemb;

		cnt = nmemb / 2;

		do
		{
			tmp = *pta; *pta++ = *--ptb; *ptb = tmp;
		}
		while (--cnt);

		return 1;
	}

	if (streaks >= nmemb / 24)
//	if (streaks >= nmemb / 32)
	{
		FUNC(quadsort_swap)(array, swap, swap_size, nmemb, cmp);

		return 1;
	}
	return 0;
}

size_t FUNC(crum_sort_sqrt)(VAR *array, VAR *swap, size_t swap_size, size_t nmemb, size_t npiv, CMPFUNC *cmp)
{
	VAR *ptx, *pta, *pts;
	size_t log2, nt, div, cnt, i;
	unsigned seed, mask;
	nmemb -= npiv;
	pts = ptx = array + nmemb;

	// Set log2 to 1 + floor(log2(nmemb))
	log2 = 11;
	for (nt = nmemb >> log2 ; nt ; nt /= 2) log2++;

	// Set cnt to about sqrt(nt), number of pivots wanted
	nt = nmemb / (2 + log2);
	cnt = 1 << (log2 / 2 - 3);
	for (i = 0 ; i < 5 ; i++) cnt = (cnt + nt / cnt) / 2;
	if (cnt <= npiv) return npiv;
	div = 1 + nmemb / (cnt - npiv);

	seed = nmemb;
	mask = (1 << (1 + log2 / 2)) - 1;  // < div
	for (i = nmemb ; i >= 3 * div ; )
	{
		i -= div; pts--;
		seed ^= seed << 13;
		pta = array + i + (seed & mask); swap[0] = *pts; *pts = *pta; *pta = swap[0];
		i -= div; pts--;
		seed ^= seed >> 17;
		pta = array + i + (seed & mask); swap[0] = *pts; *pts = *pta; *pta = swap[0];
		i -= div; pts--;
		seed ^= seed << 5;
		pta = array + i + (seed & mask); swap[0] = *pts; *pts = *pta; *pta = swap[0];
	}
	cnt = ptx - pts;
	FUNC(quadsort_swap)(pts, swap, swap_size, cnt + npiv, cmp);
//	if (npiv) FUNC(blit_merge_block)(pts, swap, swap_size, cnt, npiv, cmp);

	return cnt + npiv;
}

size_t FUNC(crum_median_of_three)(VAR *array, size_t v0, size_t v1, size_t v2, CMPFUNC *cmp)
{
	size_t v[3] = {v0, v1, v2};
	char x, y, z;

	x = cmp(array + v0, array + v1) > 0;
	y = cmp(array + v0, array + v2) > 0;
	z = cmp(array + v1, array + v2) > 0;

	return v[(x == y) + (y ^ z)];
}

size_t FUNC(crum_median_of_nine)(VAR *array, size_t nmemb, CMPFUNC *cmp)
{
	size_t x, y, z, div = nmemb / 16;

	x = FUNC(crum_median_of_three)(array, div * 2, div * 1, div * 4, cmp);
	y = FUNC(crum_median_of_three)(array, div * 8, div * 6, div * 10, cmp);
	z = FUNC(crum_median_of_three)(array, div * 14, div * 12, div * 15, cmp);

	VAR *ptr = array + FUNC(crum_median_of_three)(array, x, y, z, cmp);
	VAR t = array[nmemb - 1]; array[nmemb - 1] = *ptr; *ptr = t;
	return 1;
}

// As per suggestion by Marshall Lochbaum to improve generic data handling

size_t FUNC(fulcrum_reverse_partition)(VAR *array, VAR *swap, VAR *ptx, VAR *piv, size_t swap_size, size_t nmemb, CMPFUNC *cmp)
{
	size_t cnt, val, i, m = 0;
	VAR *ptl, *ptr, *pta, *tpa;

	if (nmemb <= swap_size)
	{
		cnt = nmemb / 8;

		do for (i = 8 ; i ; i--)
		{
			val = cmp(piv, ptx) > 0; swap[-m] = array[m] = *ptx++; m += val; swap++;
		}
		while (--cnt);

		for (cnt = nmemb % 8 ; cnt ; cnt--)
		{
			val = cmp(piv, ptx) > 0; swap[-m] = array[m] = *ptx++; m += val; swap++;
		}
		memcpy(array + m, swap - nmemb, (nmemb - m) * sizeof(VAR));

		return m;
	}

	memcpy(swap, array, 16 * sizeof(VAR));
	memcpy(swap + 16, array + nmemb - 16, 16 * sizeof(VAR));

	ptl = array;
	ptr = array + nmemb - 1;

	pta = array + 16;
	tpa = array + nmemb - 17;

	cnt = nmemb / 16 - 2;

	while (1)
	{
		if (pta - ptl - m <= 16)
		{
			if (cnt-- == 0) break;

			for (i = 16 ; i ; i--)
			{
				val = cmp(piv, pta) > 0; ptl[m] = ptr[m] = *pta++; m += val; ptr--;
			}
		}
		if (pta - ptl - m > 16)
		{
			if (cnt-- == 0) break;

			for (i = 16 ; i ; i--)
			{
				val = cmp(piv, tpa) > 0; ptl[m] = ptr[m] = *tpa--; m += val; ptr--;
			}
		}
	}

	if (pta - ptl - m <= 16)
	{
		for (cnt = nmemb % 16 ; cnt ; cnt--)
		{
			val = cmp(piv, pta) > 0; ptl[m] = ptr[m] = *pta++; m += val; ptr--;
		}
	}
	else
	{
		for (cnt = nmemb % 16 ; cnt ; cnt--)
		{
			val = cmp(piv, tpa) > 0; ptl[m] = ptr[m] = *tpa--; m += val; ptr--;
		}
	}
	pta = swap;

	for (cnt = 32 ; cnt ; cnt--)
	{
		val = cmp(piv, pta) > 0; ptl[m] = ptr[m] = *pta++; m += val; ptr--;
	}
	return m;
}

size_t FUNC(fulcrum_default_partition)(VAR *array, VAR *swap, VAR *ptx, VAR *piv, size_t swap_size, size_t nmemb, CMPFUNC *cmp)
{
	size_t cnt, val, i, m = 0;
	VAR *ptl, *ptr, *pta, *tpa;

	if (nmemb <= swap_size)
	{
		cnt = nmemb / 8;

		do for (i = 8 ; i ; i--)
		{
			val = cmp(ptx, piv) <= 0; swap[-m] = array[m] = *ptx++; m += val; swap++;
		}
		while (--cnt);

		for (cnt = nmemb % 8 ; cnt ; cnt--)
		{
			val = cmp(ptx, piv) <= 0; swap[-m] = array[m] = *ptx++; m += val; swap++;
		}
		memcpy(array + m, swap - nmemb, sizeof(VAR) * (nmemb - m));

		return m;
	}

	memcpy(swap, array, 16 * sizeof(VAR));
	memcpy(swap + 16, array + nmemb - 16, 16 * sizeof(VAR));

	ptl = array;
	ptr = array + nmemb - 1;

	pta = array + 16;
	tpa = array + nmemb - 17;

	cnt = nmemb / 16 - 2;

	while (1)
	{
		if (pta - ptl - m <= 16)
		{
			if (cnt-- == 0) break;

			for (i = 16 ; i ; i--)
			{
				val = cmp(pta, piv) <= 0; ptl[m] = ptr[m] = *pta++; m += val; ptr--;
			}
		}
		if (pta - ptl - m > 16)
		{
			if (cnt-- == 0) break;

			for (i = 16 ; i ; i--)
			{
				val = cmp(tpa, piv) <= 0; ptl[m] = ptr[m] = *tpa--; m += val; ptr--;
			}
		}
	}

	if (pta - ptl - m <= 16)
	{
		for (cnt = nmemb % 16 ; cnt ; cnt--)
		{
			val = cmp(pta, piv) <= 0; ptl[m] = ptr[m] = *pta++; m += val; ptr--;
		}
	}
	else
	{
		for (cnt = nmemb % 16 ; cnt ; cnt--)
		{
			val = cmp(tpa, piv) <= 0; ptl[m] = ptr[m] = *tpa--; m += val; ptr--;
		}
	}
	pta = swap;

	for (cnt = 32 ; cnt ; cnt--)
	{
		val = cmp(pta, piv) <= 0; ptl[m] = ptr[m] = *pta++; m += val; ptr--;
	}
	return m;
}

void FUNC(fulcrum_partition)(VAR *array, VAR *swap, size_t swap_size, size_t nmemb, size_t npiv, CMPFUNC *cmp)
{
	while (1)
	{
#ifdef IS32
		unsigned int range = array[nmemb]-array[-1];
		if (nmemb<=(1<<16) && range<(1<<16)) return radpack32(array, nmemb, array[-1]);
#endif

		if (nmemb <= 2048)
		{
			npiv = FUNC(crum_median_of_nine)(array, nmemb, cmp);
		}
		else
		{
			npiv = FUNC(crum_sort_sqrt)(array, swap, swap_size, nmemb, npiv, cmp);
		}

		// Pivot candidates are at the end
		size_t npart = nmemb - npiv; // Number to be partitioned
		VAR *pcp = array + npart;    // Pivot candidates position
		VAR piv = pcp[npiv / 2];

		size_t a_size, s_size;

		if (cmp(array + nmemb, &piv) <= 0)
		{
			a_size = npart;
		}
		else
		{
			a_size = FUNC(fulcrum_default_partition)(array, swap, array, &piv, swap_size, npart, cmp);
		}

		if (a_size == npart)
		{
			a_size = FUNC(fulcrum_reverse_partition)(array, swap, array, &piv, swap_size, npart, cmp);
			size_t a0 = a_size;
			while (cmp(&piv, pcp) > 0)
			{
				VAR t = array[a_size]; array[a_size++] = *pcp; *pcp++ = t;
			}

			if (pcp - (array + a_size) <= a_size / 16 || a_size <= CRUM_OUT)
			{
				return FUNC(quadsort_swap)(array, swap, swap_size, a_size, cmp);
			}
			else
			{
				return FUNC(fulcrum_partition)(array, swap, swap_size, a_size, a_size - a0, cmp);
			}
		}

		// Swap candidates <= pivot to the middle
		// Unguarded because case above handles pivot == element after end
		while (cmp(pcp, &piv) <= 0)
		{
			VAR t = array[a_size]; array[a_size++] = *pcp; *pcp++ = t;
		}
		VAR s_piv = (array + nmemb) - pcp;
		npiv -= s_piv;
		pcp = array + a_size;
		s_size = nmemb - a_size;
		a_size--; npiv--; // Exclude a pivot

		if (a_size <= s_size / 16 || s_size <= CRUM_OUT)
		{
			FUNC(quadsort_swap)(pcp, swap, swap_size, s_size, cmp);
		}
		else
		{
			FUNC(fulcrum_partition)(pcp, swap, swap_size, s_size, s_piv, cmp);
		}

		if (s_size <= a_size / 16 || a_size <= CRUM_OUT)
		{
			return FUNC(quadsort_swap)(array, swap, swap_size, a_size, cmp);
		}
		nmemb = a_size;
	}
}

// Swap a minimum to the beginning and a maximum to the end.
size_t FUNC(crum_range)(VAR *array, size_t nmemb, CMPFUNC *cmp)
{
	size_t block = 1024;
	VAR min = array[0], max = min;
	size_t jmin = 0, jmax = 0, j = 0, i = 0;

	do
	{
		size_t j0 = j;
		j += block; if (j > nmemb) j = nmemb;
		VAR m0 = array[i], m1 = m0;
		for ( ; i<j; i++)
		{
			if (cmp(&m0,array+i)>0) m0=array[i];
			if (cmp(array+i,&m1)>0) m1=array[i];
		}
		if (cmp(&min, &m0) >  0) { min=m0; jmin=j0; }
		if (cmp(&max, &m1) <= 0) { max=m1; jmax=j0; }
	}
	while (i < nmemb);

	if (cmp(&max, &min) <= 0) return 1;  // All elements equal

	// Swapping
	// Careful: min/max might already be at the ends
	VAR t = array[0];
	if (cmp(&t, &min)>0)
	{
		for (i = jmin; cmp(array+i, &min)>0; i++);
		array[0] = array[i];
		if (cmp(&max, &t)<=0)
		{
			array[i] = array[nmemb-1];
			array[nmemb-1] = t;
			return 0;
		}
		array[i] = t;
	}
	t = array[nmemb-1];
	if (cmp(&max, &t)>0)
	{
		for (i = jmax; cmp(&max, array+i)>0; i++);
		array[nmemb-1] = array[i];
		array[i] = t;
	}
	return 0;
}

void FUNC(crumsort_main)(VAR *array, VAR *swap, size_t swap_size, size_t nmemb, CMPFUNC *cmp)
{
#if ANALYZE // Off by default, to benchmark crumsort portion better
	if (FUNC(crum_analyze)(array, swap, swap_size, nmemb, cmp)) return;
#endif

	if (FUNC(crum_range)(array, nmemb, cmp)) return;
	array++; nmemb-=2;

	FUNC(fulcrum_partition)(array, swap, swap_size, nmemb, 0, cmp);

}

void FUNC(crumsort)(VAR *array, size_t nmemb, CMPFUNC *cmp)
{
	if (nmemb < 32)
	{
		return FUNC(tail_swap)(array, nmemb, cmp);
	}
#if CRUM_AUX
	size_t swap_size = CRUM_AUX;
#else
	size_t swap_size = 32;

	while (swap_size * swap_size <= nmemb)
	{
		swap_size *= 4;
	}
#endif
	VAR swap[swap_size];

	FUNC(crumsort_main)(array, swap, swap_size, nmemb, cmp);
}

void FUNC(crumsort_swap)(VAR *array, VAR *swap, size_t swap_size, size_t nmemb, CMPFUNC *cmp)
{
	if (nmemb < 32)
	{
		FUNC(tail_swap)(array, nmemb, cmp);
	}
	else
	{
		FUNC(crumsort_main)(array, swap, swap_size, nmemb, cmp);
	}
}
