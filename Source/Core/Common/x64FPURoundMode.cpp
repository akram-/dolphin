// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include <cfenv>

#include "Common/Common.h"
#include "Common/CPUDetect.h"
#include "Common/FPURoundMode.h"

#ifdef _WIN32
#	include <mmintrin.h>
#else
#	include <xmmintrin.h>
#endif

namespace FPURoundMode
{
	// Get the default SSE states here.
	static u32 saved_sse_state = _mm_getcsr();
	static const u32 default_sse_state = _mm_getcsr();

	void SetRoundMode(int mode)
	{
		// Convert PowerPC to native rounding mode.
		const int rounding_mode_lut[] = {
			FE_TONEAREST,
			FE_TOWARDZERO,
			FE_UPWARD,
			FE_DOWNWARD
		};
		fesetround(rounding_mode_lut[mode]);
	}

	void SetPrecisionMode(PrecisionMode mode)
	{
		#ifdef _WIN32
			_control87(_PC_53, MCW_PC);
		#else
			const unsigned short PRECISION_MASK = 3 << 8;
			const unsigned short precision_table[] = {
				0 << 8, // 24 bits
				2 << 8, // 53 bits
				3 << 8, // 64 bits
			};
			unsigned short cw;
			asm ("fnstcw %0" : "=m" (cw));
			cw = (cw & ~PRECISION_MASK) | precision_table[mode];
			asm ("fldcw %0" : : "m" (cw));
		#endif
	}

	void SetSIMDMode(int rounding_mode, bool non_ieee_mode)
	{
		// OR-mask for disabling FPU exceptions (bits 7-12 in the MXCSR register)
		const u32 EXCEPTION_MASK = 0x1F80;
		// Denormals-Are-Zero (non-IEEE mode: denormal inputs are set to +/- 0)
		const u32 DAZ = 0x40;
		// Flush-To-Zero (non-IEEE mode: denormal outputs are set to +/- 0)
		const u32 FTZ = 0x8000;
		// lookup table for FPSCR.RN-to-MXCSR.RC translation
		static const u32 simd_rounding_table[] =
		{
			(0 << 13) | EXCEPTION_MASK, // nearest
			(3 << 13) | EXCEPTION_MASK, // -inf
			(2 << 13) | EXCEPTION_MASK, // +inf
			(1 << 13) | EXCEPTION_MASK, // zero
		};
		u32 csr = simd_rounding_table[rounding_mode];

		// Some initial steppings of Pentium 4 CPUs support FTZ but not DAZ.
		// They will not flush input operands but flushing outputs only is better than nothing.
		static const u32 denormalLUT[2] =
		{
			FTZ,       // flush-to-zero only
			FTZ | DAZ, // flush-to-zero and denormals-are-zero (may not be supported)
		};
		if (non_ieee_mode)
		{
			csr |= denormalLUT[cpu_info.bFlushToZero];
		}
		_mm_setcsr(csr);
	}

	void SaveSIMDState()
	{
		saved_sse_state = _mm_getcsr();
	}
	void LoadSIMDState()
	{
		_mm_setcsr(saved_sse_state);
	}
	void LoadDefaultSIMDState()
	{
		_mm_setcsr(default_sse_state);
	}
}
