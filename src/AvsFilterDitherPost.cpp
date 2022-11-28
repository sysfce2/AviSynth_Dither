/*****************************************************************************

        AvsFilterDitherPost.cpp
        Author: Laurent de Soras, 2010

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if defined (_MSC_VER)
	#pragma warning (1 : 4130 4223 4705 4706)
	#pragma warning (4 : 4355 4786 4800)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include	"fmtcl/ErrDifBuf.h"
#include	"fstb/fnc.h"
#include	"AvsFilterDitherPost.h"

#include	<algorithm>

#include	<cassert>



#if ! defined (NDEBUG)
	// Define this symbol to check that the optimised code matches
	// the reference code (slow, debug purpose only).
	#define	AvsFilterDitherPost_CHECK_REF_CODE
#endif



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



AvsFilterDitherPost::AvsFilterDitherPost (::IScriptEnvironment *env_ptr, ::PClip msb_clip_sptr, ::PClip lsb_clip_sptr, int mode, double ampo, double ampn, int pat, bool dyn_flag, bool prot_flag, ::PClip mask_clip_sptr, double thr, bool stacked_flag, bool interlaced_flag, int y, int u, int v, bool staticnoise_flag, bool slice_flag)
:	GenericVideoFilter (msb_clip_sptr)
,	_msb_clip_sptr (msb_clip_sptr)
,	_lsb_clip_sptr (lsb_clip_sptr)
,	_mask_clip_sptr (mask_clip_sptr)
,	_vi_src (vi)
#if defined (_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4355)
#endif // 'this': used in base member initializer list
,	_plane_proc (vi, vi, *this)
#if defined (_MSC_VER)
#pragma warning (pop)
#endif
,	_mode (mode)
,	_ampo (ampo)
,	_ampn (ampn)
,	_pat (pat)
,	_dyn_flag (dyn_flag)
,	_prot_flag (prot_flag)
,	_thr (thr)
,	_stacked_flag (stacked_flag)
,	_interlaced_flag (interlaced_flag)
,	_dither_pat_arr ()
,	_ampo_i (0)
,	_ampn_i (0)
,	_ampe_i (0)
,	_thr_a (0)
,	_thr_b (0)
,	_threshold_flag (false)
,	_dith_type (Type (-1))
,	_edb_pool ()
,	_edb_factory (vi.width)
,	_protect_segment_ptr (0)
,	_process_segment_ptr (0)
,	_process_segment_errdif_ptr (0)
,	_apply_mask_ptr (0)
,	_staticnoise_flag (staticnoise_flag)
,	_slice_flag (slice_flag)
{
	assert (env_ptr != 0);

	// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
	// Parameter check

	if (! vi.IsPlanar ())
	{
		env_ptr->ThrowError ("DitherPost: input must be planar.");
	}

	if (lsb_clip_sptr)
	{
		PlaneProcessor::check_same_format (env_ptr, vi, lsb_clip_sptr, "DitherPost", "rem");
	}
	else
	{
		if (_stacked_flag)
		{
			if (! PlaneProcessor::check_stack16_height (vi))
			{
				env_ptr->ThrowError (
					"DitherPost: stacked MSB/LSB data must have\n"
					"a height multiple of twice the maximum subsampling."
				);
			}
			vi.height /= 2;
		}
		else
		{
			if ((vi.num_frames & 1) != 0)
			{
				env_ptr->ThrowError (
					"DitherPost: main clip used alone must have\n"
					"an even number of frames (MSB/LSB pairs)."
				);
			}
			vi.num_frames /= 2;
			vi.MulDivFPS (1, 2);
		}
	}

	// From this point, vi.height is the final clip height (8-bit)

	if (_interlaced_flag)
	{
		int			h_mult = 2;
		if (vi.IsYUV () && ! vi.IsY8 ())
		{
			int			ss_vert = vi.GetPlaneHeightSubsampling (PLANAR_U);
			h_mult <<= ss_vert;
		}
		if ((vi.height & (h_mult - 1)) != 0)
		{
			env_ptr->ThrowError (
				"DitherPost: interlaced clips must have\n"
				"an even height for all their planes."
			);
		}
	}

	if (mask_clip_sptr)
	{
		PlaneProcessor::check_same_format (env_ptr, vi, mask_clip_sptr, "DitherPost", "mask");
	}

	if (ampo < 0)
	{
		env_ptr->ThrowError (
			"DitherPost: \"ampo\" cannot be negative."
		);
	}
	if (ampn < 0)
	{
		env_ptr->ThrowError (
			"DitherPost: \"ampn\" cannot be negative."
		);
	}

	if (pat < 0 || pat >= Pattern_NBR_ELT)
	{
		env_ptr->ThrowError (
			"DitherPost: Wrong value for \"pat\"."
		);
	}

	if (thr > 0.5)
	{
		env_ptr->ThrowError (
			"DitherPost: Wrong value for \"thr\"."
		);
	}

	if (mode >= Mode_NBR_ELT)
	{
		env_ptr->ThrowError (
			"DitherPost: Wrong value for \"mode\"."
		);
	}

	build_dither_pat ();

	if (_dith_type == Type_ERRDIF)
	{
		_edb_pool.set_factory (_edb_factory);
	}

	const int		amp_mul = 1 << AMP_BITS;
	_ampo_i = std::min (fstb::round_int (_ampo * amp_mul), 127);
	_ampn_i = std::min (fstb::round_int (_ampn * amp_mul), 127);

	_ampe_i = std::max (std::min (fstb::round_int ((_ampo - 1) * 256), 2047), 0);

	if (thr < 0.01)
	{
		_threshold_flag = false;
	}
	else if (thr > 0.49)
	{
		_threshold_flag = false;
		_ampo_i = 0;
		_ampn_i = 0;
	}
	else
	{
		_threshold_flag = true;
		const double	a = 1 / (_thr + 0.01) + 1 / (0.51 - _thr);
		_thr_a = fstb::round_int (-a);
		_thr_b = fstb::round_int ((0.5 - _thr) * 256 * a);
	}

	_plane_proc.set_proc_mode (y, u, v);
	_plane_proc.set_clip_info (0, PlaneProcessor::ClipType_NORMAL_8);
	if (lsb_clip_sptr)
	{
		_plane_proc.set_clip_info (1, PlaneProcessor::ClipType_MSB);
	}
	else if (_stacked_flag)
	{
		_plane_proc.set_clip_info (1, PlaneProcessor::ClipType_STACKED_16);
	}
	else
	{
		_plane_proc.set_clip_info (1, PlaneProcessor::ClipType_INTERLEAVED_16);
	}
	_plane_proc.set_clip_info (2, PlaneProcessor::ClipType_LSB);
	_plane_proc.set_clip_info (3, PlaneProcessor::ClipType_NORMAL_8);

	init_fnc (*env_ptr);
}



AvsFilterDitherPost::~AvsFilterDitherPost ()
{
	// Nothing
}



::PVideoFrame __stdcall	AvsFilterDitherPost::GetFrame (int n, ::IScriptEnvironment *env_ptr)
{
	::PVideoFrame	dst_sptr = env_ptr->NewVideoFrame (vi);
	FrameContext   frame_context;

	_plane_proc.process_frame (
		dst_sptr, n, *env_ptr,
		&_msb_clip_sptr, &_lsb_clip_sptr, &_mask_clip_sptr,
		&frame_context
	);

	for (auto &plane : frame_context._pc_arr)
	{
		for (auto &field : plane)
		{
			if (field._global_data._this_ptr != nullptr)
			{
				field._slicer.wait ();
			}
		}
	}

	return (dst_sptr);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	AvsFilterDitherPost::do_process_plane (::PVideoFrame &dst_sptr, int n, ::IScriptEnvironment &env, int plane_index, int plane_id, void *ctx_ptr)
{
	FrameContext & frame_context = *reinterpret_cast <FrameContext *> (ctx_ptr);

	::PVideoFrame	msb_sptr;
	::PVideoFrame	lsb_sptr;
	::PVideoFrame	msk_sptr;
	{
		std::lock_guard <std::mutex>	lock (frame_context._mutex);
		if (! frame_context._filled_flag)
		{
			frame_context._filled_flag = true;

			if (_lsb_clip_sptr)
			{
				frame_context._msb_sptr = _msb_clip_sptr->GetFrame (n, &env);
				frame_context._lsb_sptr = _lsb_clip_sptr->GetFrame (n, &env);
			}
			else if (_stacked_flag)
			{
				frame_context._msb_sptr = _msb_clip_sptr->GetFrame (n, &env);
			}
			else
			{
				frame_context._msb_sptr = _msb_clip_sptr->GetFrame (n * 2    , &env);
				frame_context._lsb_sptr = _msb_clip_sptr->GetFrame (n * 2 + 1, &env);
			}

			if (_mask_clip_sptr)
			{
				frame_context._msk_sptr = _mask_clip_sptr->GetFrame (n, &env);
			}
		}

		msb_sptr = frame_context._msb_sptr;
		lsb_sptr = frame_context._lsb_sptr;
		msk_sptr = frame_context._msk_sptr;
	}

	uint8_t *		data_dst_ptr = dst_sptr->GetWritePtr (plane_id);
	const uint8_t*	data_msb_ptr = msb_sptr->GetReadPtr (plane_id);
	const int		w            = _plane_proc.get_width (dst_sptr, plane_id);
	const int		h            = _plane_proc.get_height (dst_sptr, plane_id);
	const int		stride_dst   = dst_sptr->GetPitch (plane_id);
	const int		stride_msb   = msb_sptr->GetPitch (plane_id);

	const uint8_t*	data_lsb_ptr = 0;
	int				stride_lsb   = 0;
	if (! _lsb_clip_sptr && _stacked_flag)
	{
		data_lsb_ptr = data_msb_ptr + h * stride_msb;
		stride_lsb   = stride_msb;
	}
	else
	{
		data_lsb_ptr = lsb_sptr->GetReadPtr (plane_id);
		stride_lsb   = lsb_sptr->GetPitch (plane_id);
	}

	const uint8_t*	data_msk_ptr = 0;
	int				stride_msk   = 0;
	if (_mask_clip_sptr)
	{
		data_msk_ptr = msk_sptr->GetReadPtr (plane_id);
		stride_msk   = msk_sptr->GetPitch (plane_id);
	}

	if (_interlaced_flag && _dith_type == Type_ERRDIF)
	{
		process_plane (
			data_dst_ptr, data_msb_ptr, data_lsb_ptr, data_msk_ptr,
			w, h >> 1,
			stride_dst << 1, stride_msb << 1, stride_lsb << 1, stride_msk << 1,
			n << 1, plane_index,
			env,
			frame_context._pc_arr [plane_index] [0]
		);
		process_plane (
			data_dst_ptr + stride_dst,
			data_msb_ptr + stride_msb,
			data_lsb_ptr + stride_lsb,
			data_msk_ptr + stride_msk,
			w, h >> 1,
			stride_dst << 1, stride_msb << 1, stride_lsb << 1, stride_msk << 1,
			(n << 1) + 1, plane_index,
			env,
			frame_context._pc_arr [plane_index] [1]
		);
	}
	else
	{
		process_plane (
			data_dst_ptr, data_msb_ptr, data_lsb_ptr, data_msk_ptr,
			w, h,
			stride_dst, stride_msb, stride_lsb, stride_msk,
			n, plane_index,
			env,
			frame_context._pc_arr [plane_index] [0]
		);
	}
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	AvsFilterDitherPost::build_dither_pat ()
{
	_dith_type = Type_ORDERED;

	switch (_mode)
	{
	case	Mode_ORDERED:
		build_dither_pat_ordered ();
		break;

	case	Mode_B1:
		build_dither_pat_b1 ();
		break;

	case	Mode_B2L:
		build_dither_pat_b2 (0x00000001L, 0x00010001L, 0x01010100L, 0x101);
		_dith_type = Type_2B;
		break;

	case	Mode_B2M:
		build_dither_pat_b2 (0x00000001L, 0x00000002L, 0x01010100L, 0x111);
		_dith_type = Type_2B;
		break;

	case	Mode_B2S:
		build_dither_pat_b2 (0xFF01FF02L, 0xFF02FF02L, 0xFF020002L, 0x101);
		_dith_type = Type_2B;
		break;

	case	Mode_B2SS:
		build_dither_pat_b2 (0xFFFFFF04L, 0xFF02FF02L, 0x020202FDL, 0x101);
		_dith_type = Type_2B;
		break;

	case	Mode_FLOYD:
	case	Mode_STUCKI:
	case	Mode_ATKINSON:
		_dith_type = Type_ERRDIF;
		break;

	case	Mode_ROUND:
	default:
		build_dither_pat_round ();
		break;
	}
}



void	AvsFilterDitherPost::build_dither_pat_round ()
{
	PatData &		pat_data = _dither_pat_arr [0];
	for (int y = 0; y < PAT_WIDTH; ++y)
	{
		for (int x = 0; x < PAT_WIDTH; ++x)
		{
			_dither_pat_arr [0] [y] [0] [x] = 0;
		}
	}

	copy_dither_level (pat_data);
	build_next_dither_pat ();
}



void	AvsFilterDitherPost::build_dither_pat_ordered ()
{
	assert (fstb::is_pow_2 (int (PAT_WIDTH)));

	for (int y = 0; y < PAT_WIDTH; ++y)
	{
		for (int x = 0; x < PAT_WIDTH; ++x)
		{
			_dither_pat_arr [0] [y] [0] [x] = -128;
		}
	}

	for (int dith_size = 2; dith_size <= PAT_WIDTH; dith_size <<= 1)
	{
		for (int y = 0; y < PAT_WIDTH; y += 2)
		{
			for (int x = 0; x < PAT_WIDTH; x += 2)
			{
				const int		xx = (x >> 1) + (PAT_WIDTH >> 1);
				const int		yy = (y >> 1) + (PAT_WIDTH >> 1);
				const int		val = (_dither_pat_arr [0] [yy] [0] [xx] + 128) >> 2;
				_dither_pat_arr [0] [y    ] [0] [x    ] = int16_t (val +   0-128);
				_dither_pat_arr [0] [y    ] [0] [x + 1] = int16_t (val + 128-128);
				_dither_pat_arr [0] [y + 1] [0] [x    ] = int16_t (val + 192-128);
				_dither_pat_arr [0] [y + 1][0]  [x + 1] = int16_t (val +  64-128);
			}
		}
	}

	copy_dither_level (_dither_pat_arr [0]);
	build_next_dither_pat ();
}



void	AvsFilterDitherPost::build_dither_pat_b1 ()
{
	for (int y = 0; y < PAT_WIDTH; y += 2)
	{
		for (int x = 0; x < PAT_WIDTH; x += 2)
		{
			_dither_pat_arr [0] [y    ] [0] [x    ] =  64;
			_dither_pat_arr [0] [y    ] [0] [x + 1] = -64;
			_dither_pat_arr [0] [y + 1] [0] [x    ] = -64;
			_dither_pat_arr [0] [y + 1] [0] [x + 1] =  64;
		}
	}

	copy_dither_level (_dither_pat_arr [0]);
	build_next_dither_pat ();
}



void	AvsFilterDitherPost::build_dither_pat_b2 (long p_0, long p_1, long p_2, int map_alt)
{
	static const int	pos_map [2*2] [8] =
	{
		// x
		{ 0, 1, 1, 0, 2, 3, 3, 2 },
		{ 0, 1, 3, 2, 2, 3, 1, 0 },
		// y
		{ 0, 0, 1, 1, 0, 0, 1, 1 },
		{ 0, 0, 0, 0, 1, 1, 1, 1 }
	};

	const int		x_inc = (_pat == Pattern_ALT_H) ? 2 : 4;
	const int		y_inc = (_pat == Pattern_ALT_H) ? 4 : 2;
	const long		lvl_arr [PAT_LEVELS] = { p_0, p_1, p_2 };

	for (int seq = 0; seq < PAT_PERIOD; ++seq)
	{
		PatData &		pat_data = _dither_pat_arr [seq];
		const int		angle = (_dyn_flag) ? seq & 3 : 0;

		for (int y = 0; y < PAT_WIDTH; y += y_inc)
		{
			for (int x = 0; x < PAT_WIDTH; x += x_inc)
			{
				for (int lvl = 0; lvl < PAT_LEVELS; ++lvl)
				{
					int			p_index_x = 0;
					int			p_index_y = 2;
					if (_pat != Pattern_REGULAR)
					{
						const int		ofs = (map_alt >> (lvl * 4)) & 15;
						p_index_x += ofs;
						p_index_y += ofs;
					}
					if (_pat == Pattern_ALT_H)
					{
						std::swap (p_index_x, p_index_y);
					}

					for (int pos = 0; pos < 8; ++pos)
					{
						const int		xx = pos_map [p_index_x] [(pos + angle) & 7];
						const int		yy = pos_map [p_index_y] [(pos + angle) & 7];

						const int		shift = (pos & 3) * 8;
						const int		val = int8_t (lvl_arr [lvl] >> shift);

						pat_data [y + yy] [lvl] [x + xx] = int16_t (val);
					}
				}
			}
		}
	}
}



void	AvsFilterDitherPost::build_next_dither_pat ()
{
	for (int seq = 1; seq < PAT_PERIOD; ++seq)
	{
		const int		angle = (_dyn_flag) ? seq & 3 : 0;
		copy_dither_pat_rotate (
			_dither_pat_arr [seq],
			_dither_pat_arr [0],
			angle
		);
	}
}



void	AvsFilterDitherPost::copy_dither_level (PatData &pat)
{
	assert (pat != 0);

	for (int y = 0; y < PAT_WIDTH; ++y)
	{
		for (int lvl = 1; lvl < PAT_LEVELS; ++lvl)
		{
			for (int x = 0; x < PAT_WIDTH; ++x)
			{
				pat [y] [lvl] [x] = pat [y] [0] [x];
			}
		}
	}
}



// Clockwise rotation
void	AvsFilterDitherPost::copy_dither_pat_rotate (PatData &dst, const PatData &src, int angle)
{
	assert (dst != 0);
	assert (src != 0);
	assert (angle >= 0);
	assert (angle < 4);

	static const int		sin_arr [4] = { 0, 1, 0, -1 };
	const int		s = sin_arr [ angle         ];
	const int		c = sin_arr [(angle + 1) & 3];

	assert (fstb::is_pow_2 (int (PAT_WIDTH)));
	const int		mask = PAT_WIDTH - 1;

	for (int y = 0; y < PAT_WIDTH; ++y)
	{
		for (int x = 0; x < PAT_WIDTH; ++x)
		{
			const int		xs = (x * c - y * s) & mask;
			const int		ys = (x * s + y * c) & mask;

			for (int lvl = 0; lvl < PAT_LEVELS; ++lvl)
			{
				dst [y] [lvl] [x] = src [ys] [lvl] [xs];
			}
		}
	}
}



void	AvsFilterDitherPost::init_fnc (::IScriptEnvironment &env)
{
	assert (_dith_type >= 0);

	_protect_segment_ptr = &AvsFilterDitherPost::protect_segment_cpp;
	_apply_mask_ptr = &AvsFilterDitherPost::apply_mask_cpp;
	switch (_dith_type)
	{
	case	Type_2B:
		_process_segment_ptr = &AvsFilterDitherPost::process_segment_2b_cpp;
		break;
	case	Type_ERRDIF:
		switch (_mode)
		{
		case	Mode_STUCKI:
			_process_segment_errdif_ptr = &AvsFilterDitherPost::process_segment_stucki_cpp;
			break;
		case	Mode_ATKINSON:
			_process_segment_errdif_ptr = &AvsFilterDitherPost::process_segment_atkinson_cpp;
			break;
		case	Mode_FLOYD:
		default:
			_process_segment_errdif_ptr = &AvsFilterDitherPost::process_segment_floydsteinberg_cpp;
			break;
		}
		break;
	default:
		_process_segment_ptr = &AvsFilterDitherPost::process_segment_ord_cpp;
		break;
	}

	const long		cpu_flags = env.GetCPUFlags ();
	if ((cpu_flags & CPUF_SSE2) != 0)
	{
		_protect_segment_ptr = &AvsFilterDitherPost::protect_segment_sse2;
		_apply_mask_ptr = &AvsFilterDitherPost::apply_mask_sse2;
		switch (_dith_type)
		{
		case	Type_2B:
			_process_segment_ptr = &AvsFilterDitherPost::process_segment_2b_sse2;
			break;
		case	Type_ERRDIF:
			// Nothing
			break;
		default:
			if (_ampo_i == 1 << AMP_BITS && _ampn_i == 0 && ! _threshold_flag)
			{
				_process_segment_ptr = &AvsFilterDitherPost::process_segment_ord_sse2_simple;
			}
			else
			{
				_process_segment_ptr = &AvsFilterDitherPost::process_segment_ord_sse2;
			}
			break;
		}
	}
}



/*

Original formula (script version)
---------------------------------

On input:
lsb in [0 ; 255]
dith_? in [0 ; 1[

If the pixel is excluded by the mask

	pix = msb

Else

	If prot
		lsb = ?
		msb = ?
	End If

	If thr < 0 (no threshold)
		f = 1
	Else If thr >= 0.499 (no dithering)
		f = 0
	Else
		r0 = max ((0.5 - thr) / 3, 0.375 - thr)
		r1 = 0.5 - thr
		a = r0 * 256
		b = (r1 - r0) * 256
		cy = clip ((abs (lsb - 128) - a) / b, 0, 1)
		f = 1 - cy^2
	End If

	noi = ampn * f * (dith_n - 0.5)
	ord = ampo * f * (dith_o - 0.5)

	sum = lsb/256 + noi + ord
	dif = round (sum)
	pix = msb + dif

End If


f optimisation
--------------

f is a function of the LSB that equals 0 for LSB == 0, 1 on half-way
and 0 again when the LSB reach the next MSB unit.
Smooth change in f occurs just above thr and just below 1-thr.
We can simplify the formula as below (transition as straight lines
instead of S-shaped curves):

a = 1 / (thr + 0.01) + 1 / (0.51 - thr);	[8; 100]
b = (thr - 0.5) * a;								[0;  50]
f = clip (b + abs (lsb - 0.5) * a, 0, 1);
(with a lsb in [0 ; 1[)

Value ranges:

- Dithers are values between -128 and +127 (once centered).
- ampo and ampn are values in [0; 127], 1<<AMP_BITS (32) being the unity.
- f is a value in [0; 255]
- We can compose first amp? * f then shift them in order to obtain something
in the [0; 127] range (with 1.0 at 32). Mult u8xu8->u16->shift8->u8->s8
- noi and ord ranges are [-512; 512]. Mult s8xs8->s16->shift5

*/

// msk_ptr can be 0
void	AvsFilterDitherPost::process_plane (uint8_t *dst_ptr, const uint8_t *msb_ptr, const uint8_t *lsb_ptr, const uint8_t *msk_ptr, int w, int h, int stride_dst, int stride_msb, int stride_lsb, int stride_msk, int frame_index, int plane_index, ::IScriptEnvironment &env, PlaneContext &plane_context)
{
	uint32_t			rnd_state = plane_index << 16;
	if (_staticnoise_flag)
	{
		rnd_state += 55555;
	}
	else
	{
		rnd_state += frame_index;
	}

	const int		pat_index = (frame_index + plane_index) & (PAT_PERIOD - 1);
	const PatData&	pattern = _dither_pat_arr [pat_index];

	TaskDataGlobal &	tdg = plane_context._global_data;
	tdg._this_ptr    = this;
	tdg._env_ptr     = &env;
	tdg._w           = w;
	tdg._h           = h;
	tdg._dst_ptr     = dst_ptr;
	tdg._msb_ptr     = msb_ptr;
	tdg._lsb_ptr     = lsb_ptr;
	tdg._msk_ptr     = msk_ptr;
	tdg._stride_dst  = stride_dst;
	tdg._stride_msb  = stride_msb;
	tdg._stride_lsb  = stride_lsb;
	tdg._stride_msk  = stride_msk;
	tdg._pattern_ptr = &pattern;
	tdg._rnd_state   = rnd_state;

	Slicer &			slicer = plane_context._slicer;
	const int      min_slice_h = (_slice_flag) ? 1 : h;
	slicer.start (h, tdg, &AvsFilterDitherPost::process_subplane, min_slice_h);
}



void	AvsFilterDitherPost::process_subplane (Slicer::TaskData &td)
{
	assert (&td != 0);

	const int		y_beg = td._y_beg;
	const int		y_end = td._y_end;
	const TaskDataGlobal &	tdg = *(td._glob_data_ptr);
	assert (this == tdg._this_ptr);

	uint8_t *		dst_ptr = tdg._dst_ptr + y_beg * tdg._stride_dst;
	const uint8_t*	msb_ptr = tdg._msb_ptr + y_beg * tdg._stride_msb;
	const uint8_t*	lsb_ptr = tdg._lsb_ptr + y_beg * tdg._stride_lsb;
	const uint8_t*	msk_ptr = tdg._msk_ptr + y_beg * tdg._stride_msk;
	const PatData&	pattern = *(tdg._pattern_ptr);
	uint32_t			rnd_state = tdg._rnd_state + (td._y_beg << 8);

	fmtcl::ErrDifBuf *   edb_ptr = 0;
	if (_dith_type == Type_ERRDIF)
	{
		edb_ptr = _edb_pool.take_obj ();
		if (edb_ptr == 0)
		{
			tdg._env_ptr->ThrowError (
				"DitherPost: "
				"Cannot allocate a buffer for the error diffusion dithering."
			);
		}
		edb_ptr->clear (sizeof (uint16_t));
	}

//	fstb::ArrayAlign <uint8_t, MAX_SEG_LEN, 16>	tmp_msb_arr;
//	fstb::ArrayAlign <uint8_t, MAX_SEG_LEN, 16>	tmp_lsb_arr;
	__declspec (align (16)) uint8_t	tmp_msb_arr [MAX_SEG_LEN];
	__declspec (align (16)) uint8_t	tmp_lsb_arr [MAX_SEG_LEN];

	const int		nbr_seg = (tdg._w + MAX_SEG_LEN - 1) / MAX_SEG_LEN;
	int				seg_beg = 0;
	int				seg_end = nbr_seg * MAX_SEG_LEN;
	int				seg_stride = MAX_SEG_LEN;

	for (int y = td._y_beg; y < td._y_end; ++y)
	{
		const int16_t (*	pat_ptr) [PAT_WIDTH] = pattern [y & (PAT_WIDTH-1)];

		for (int s = seg_beg; s != seg_end; s += seg_stride)
		{
			const int		seg_len = std::min (tdg._w - s, int (MAX_SEG_LEN));

			const uint8_t*	xmsb_ptr = msb_ptr + s;
			const uint8_t*	xlsb_ptr = lsb_ptr + s;

			if (_prot_flag)
			{
				(this->*_protect_segment_ptr) (
					&tmp_msb_arr [0],
					&tmp_lsb_arr [0],
					xmsb_ptr,
					xlsb_ptr,
					seg_len,
					tdg._stride_msb,
					(s == 0),
					(s + MAX_SEG_LEN >= tdg._w),
					(y == 0),
					(y == tdg._h - 1)
				);
				xmsb_ptr = &tmp_msb_arr [0];
				xlsb_ptr = &tmp_lsb_arr [0];
			}

			if (_dith_type == Type_ERRDIF)
			{
				(this->*_process_segment_errdif_ptr) (
					dst_ptr + s,
					xmsb_ptr,
					xlsb_ptr,
					*edb_ptr,
					seg_len,
					s,
					(seg_stride < 0),
					rnd_state
				);
			}

			else
			{
				(this->*_process_segment_ptr) (
					dst_ptr + s,
					xmsb_ptr,
					xlsb_ptr,
					pat_ptr,
					seg_len,
					rnd_state
				);
			}
		}

		if (msk_ptr != 0)
		{
			(this->*_apply_mask_ptr) (dst_ptr, msb_ptr, msk_ptr, tdg._w);
		}

		dst_ptr += tdg._stride_dst;
		msb_ptr += tdg._stride_msb;
		lsb_ptr += tdg._stride_lsb;
		msk_ptr += tdg._stride_msk;

		if (_dith_type == Type_ERRDIF)
		{
			const int		delta = (nbr_seg - 1) * MAX_SEG_LEN;
			seg_beg = delta - seg_beg;
			seg_end = delta - seg_end;
			seg_stride = -seg_stride;
		}
	}

	if (edb_ptr != 0)
	{
		_edb_pool.return_obj (*edb_ptr);
		edb_ptr = 0;
	}
}



// Flags indicate that we are at the boundaries of the picture, therefore
// some lines or columns must be ignored.
void	AvsFilterDitherPost::protect_segment_cpp (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int w, int stride_msb, bool l_flag, bool r_flag, bool t_flag, bool b_flag) const
{
	assert (dst_msb_ptr != 0);
	assert (dst_lsb_ptr != 0);
	assert (src_msb_ptr != 0);
	assert (src_lsb_ptr != 0);
	assert (w > 0);
	assert (w <= MAX_SEG_LEN);

#if defined (AvsFilterDitherPost_CHECK_REF_CODE)
	uint8_t			dst_msb_ref_arr [MAX_SEG_LEN];
	uint8_t			dst_lsb_ref_arr [MAX_SEG_LEN];
	protect_segment_ref (
		dst_msb_ref_arr, dst_lsb_ref_arr,
		src_msb_ptr, src_lsb_ptr,
		w, stride_msb,
		l_flag, r_flag, t_flag, b_flag
	);
#endif

	// Leftmost pixel
	protect_pix_generic (
		dst_msb_ptr, dst_lsb_ptr,
		src_msb_ptr, src_lsb_ptr,
		0,
		stride_msb,
		l_flag, r_flag && (w == 1),
		t_flag, b_flag
	);

	// Loops for the main segment part
	const int		end = w - 1;

	if (t_flag)
	{
		if (b_flag)
		{
			for (int pos = 1; pos < end; ++pos)
			{
				int				msb = src_msb_ptr [pos];
				int				mi = msb;
				int				ma = msb;

				update_min_max (mi, ma, src_msb_ptr [pos - 1]);
				update_min_max (mi, ma, src_msb_ptr [pos + 1]);

				protect_pix_decide (
					dst_msb_ptr, dst_lsb_ptr,
					msb, src_lsb_ptr,
					pos, mi, ma
				);
			}
		}

		else
		{
			for (int pos = 1; pos < end; ++pos)
			{
				int				msb = src_msb_ptr [pos];
				int				mi = msb;
				int				ma = msb;

				update_min_max (mi, ma, src_msb_ptr [pos - 1]);
				update_min_max (mi, ma, src_msb_ptr [pos + 1]);
				update_min_max (mi, ma, src_msb_ptr [pos + stride_msb]);
				update_min_max (mi, ma, src_msb_ptr [pos + stride_msb - 1]);
				update_min_max (mi, ma, src_msb_ptr [pos + stride_msb + 1]);

				protect_pix_decide (
					dst_msb_ptr, dst_lsb_ptr,
					msb, src_lsb_ptr,
					pos, mi, ma
				);
			}
		}
	}

	else 
	{
		if (b_flag)
		{
			for (int pos = 1; pos < end; ++pos)
			{
				int				msb = src_msb_ptr [pos];
				int				mi = msb;
				int				ma = msb;

				update_min_max (mi, ma, src_msb_ptr [pos - 1]);
				update_min_max (mi, ma, src_msb_ptr [pos + 1]);
				update_min_max (mi, ma, src_msb_ptr [pos - stride_msb]);
				update_min_max (mi, ma, src_msb_ptr [pos - stride_msb - 1]);
				update_min_max (mi, ma, src_msb_ptr [pos - stride_msb + 1]);

				protect_pix_decide (
					dst_msb_ptr, dst_lsb_ptr,
					msb, src_lsb_ptr,
					pos, mi, ma
				);
			}
		}

		else
		{
			for (int pos = 1; pos < end; ++pos)
			{
				int				msb = src_msb_ptr [pos];
				int				mi = msb;
				int				ma = msb;

				update_min_max (mi, ma, src_msb_ptr [pos - 1]);
				update_min_max (mi, ma, src_msb_ptr [pos + 1]);
				update_min_max (mi, ma, src_msb_ptr [pos - stride_msb]);
				update_min_max (mi, ma, src_msb_ptr [pos - stride_msb - 1]);
				update_min_max (mi, ma, src_msb_ptr [pos - stride_msb + 1]);
				update_min_max (mi, ma, src_msb_ptr [pos + stride_msb]);
				update_min_max (mi, ma, src_msb_ptr [pos + stride_msb - 1]);
				update_min_max (mi, ma, src_msb_ptr [pos + stride_msb + 1]);

				protect_pix_decide (
					dst_msb_ptr, dst_lsb_ptr,
					msb, src_lsb_ptr,
					pos, mi, ma
				);
			}
		}
	}

	// Rightmost pixel
	if (w > 1)
	{
		protect_pix_generic (
			dst_msb_ptr, dst_lsb_ptr,
			src_msb_ptr, src_lsb_ptr,
			w - 1,
			stride_msb,
			false, r_flag,
			t_flag, b_flag
		);
	}

#if defined (AvsFilterDitherPost_CHECK_REF_CODE)
	for (int k = 0; k < w; ++k)
	{
		assert (dst_msb_ptr [k] == dst_msb_ref_arr [k]);
		assert (dst_lsb_ptr [k] == dst_lsb_ref_arr [k]);
	}
#endif
}



void	AvsFilterDitherPost::protect_segment_sse2 (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int w, int stride_msb, bool l_flag, bool r_flag, bool t_flag, bool b_flag) const
{
	assert (dst_msb_ptr != 0);
	assert (dst_lsb_ptr != 0);
	assert (src_msb_ptr != 0);
	assert (src_lsb_ptr != 0);
	assert (w > 0);
	assert (w <= MAX_SEG_LEN);

#if defined (AvsFilterDitherPost_CHECK_REF_CODE)
	uint8_t			dst_msb_ref_arr [MAX_SEG_LEN];
	uint8_t			dst_lsb_ref_arr [MAX_SEG_LEN];
	protect_segment_ref (
		dst_msb_ref_arr, dst_lsb_ref_arr,
		src_msb_ptr, src_lsb_ptr,
		w, stride_msb,
		l_flag, r_flag, t_flag, b_flag
	);
#endif

	int				beg_pos = 0;
	if (l_flag)
	{
		const int		head_len = 16;
		protect_segment_cpp (
			dst_msb_ptr, dst_lsb_ptr,
			src_msb_ptr, src_lsb_ptr,
			std::min (w, head_len),
			stride_msb,
			l_flag, (r_flag && w <= head_len),
			t_flag, b_flag
		);
		beg_pos = head_len;
	}

	if (w > beg_pos)
	{
		int				end_pos_n = w;
		if (r_flag)
		{
			-- end_pos_n;
		}
		const int		end_pos = end_pos_n & -16;

		const __m128i	zero = _mm_setzero_si128 ();
		const __m128i	c1   = _mm_set1_epi8 (1);

		for (int pos = beg_pos; pos < end_pos; pos += 16)
		{
			// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
			// Finds the local MSB min/max

			__m128i			msb = _mm_loadu_si128 (
				reinterpret_cast <const __m128i *> (src_msb_ptr + pos)
			);
			__m128i			msb_0 = _mm_unpacklo_epi8 (msb, zero);
			__m128i			msb_1 = _mm_unpackhi_epi8 (msb, zero);

			__m128i			mi_0 = msb_0;
			__m128i			mi_1 = msb_1;
			__m128i			ma_0 = msb_0;
			__m128i			ma_1 = msb_1;

			update_min_max (mi_0, mi_1, ma_0, ma_1, src_msb_ptr + pos - 1);
			update_min_max (mi_0, mi_1, ma_0, ma_1, src_msb_ptr + pos + 1);
			if (! t_flag)
			{
				update_min_max (mi_0, mi_1, ma_0, ma_1, src_msb_ptr + pos - stride_msb);
				update_min_max (mi_0, mi_1, ma_0, ma_1, src_msb_ptr + pos - stride_msb - 1);
				update_min_max (mi_0, mi_1, ma_0, ma_1, src_msb_ptr + pos - stride_msb + 1);
			}
			if (! b_flag)
			{
				update_min_max (mi_0, mi_1, ma_0, ma_1, src_msb_ptr + pos + stride_msb);
				update_min_max (mi_0, mi_1, ma_0, ma_1, src_msb_ptr + pos + stride_msb - 1);
				update_min_max (mi_0, mi_1, ma_0, ma_1, src_msb_ptr + pos + stride_msb + 1);
			}

			__m128i			mi = _mm_packus_epi16 (mi_0, mi_1);
			__m128i			ma = _mm_packus_epi16 (ma_0, ma_1);

			// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
			// Value change

			__m128i			lsb = _mm_loadu_si128 (
				reinterpret_cast <const __m128i *> (src_lsb_ptr + pos)
			);

			const __m128i	msb_mi_mask = _mm_cmpeq_epi8 (mi, msb);
			const __m128i	msb_inc = _mm_adds_epu8 (msb, c1);
			const __m128i	msb_ma_mask = _mm_cmpeq_epi8 (ma, msb_inc);

#if 1	// Quicker solution

			lsb = _mm_or_si128 (lsb, msb_ma_mask);

#else

			msb = _mm_or_si128 (
				_mm_and_si128 (msb_ma_mask, ma),
				_mm_andnot_si128 (msb_ma_mask, msb),
			);
			lsb = _mm_andnot_si128 (msb_ma_mask, lsb);

#endif

			lsb = _mm_and_si128 (lsb, msb_mi_mask);
			_mm_storeu_si128 (
				reinterpret_cast <__m128i *> (dst_lsb_ptr + pos),
				lsb
			);
			_mm_storeu_si128 (
				reinterpret_cast <__m128i *> (dst_msb_ptr + pos),
				msb
			);
		}

		if (end_pos < w)
		{
			protect_segment_cpp (
				dst_msb_ptr + end_pos, dst_lsb_ptr + end_pos,
				src_msb_ptr + end_pos, src_lsb_ptr + end_pos,
				w - end_pos,
				stride_msb,
				l_flag && (end_pos == 0), r_flag,
				t_flag, b_flag
			);
		}
	}

#if defined (AvsFilterDitherPost_CHECK_REF_CODE)
	for (int k = 0; k < w; ++k)
	{
		assert (dst_msb_ptr [k] == dst_msb_ref_arr [k]);
		assert (dst_lsb_ptr [k] == dst_lsb_ref_arr [k]);
	}
#endif
}



// Reference implementation (slow)
void	AvsFilterDitherPost::protect_segment_ref (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int w, int stride_msb, bool l_flag, bool r_flag, bool t_flag, bool b_flag) const
{
	assert (dst_msb_ptr != 0);
	assert (dst_lsb_ptr != 0);
	assert (src_msb_ptr != 0);
	assert (src_lsb_ptr != 0);
	assert (w > 0);
	assert (w <= MAX_SEG_LEN);

	for (int pos = 0; pos < w; ++pos)
	{
		protect_pix_generic (
			dst_msb_ptr, dst_lsb_ptr,
			src_msb_ptr, src_lsb_ptr,
			pos,
			stride_msb,
			(l_flag && pos == 0), (r_flag && pos == w - 1),
			t_flag, b_flag
		);
	}
}



void	AvsFilterDitherPost::process_segment_ord_cpp (uint8_t *dst_ptr, const uint8_t *msb_ptr, const uint8_t *lsb_ptr, const int16_t pattern [PAT_LEVELS] [PAT_WIDTH], int w, uint32_t &rnd_state) const
{
	for (int pos = 0; pos < w; ++pos)
	{
		generate_rnd (rnd_state);

		int				ao = _ampo_i;           // s8
		int				an = _ampn_i;           // s8
		const int		lsb = lsb_ptr [pos];    // u8
		const int		msb = msb_ptr [pos];    // u8

		if (_threshold_flag)
		{
			int				f = abs (lsb - 128); // ?8
			f *= _thr_a;                        // s16 = ?8 * s8
			f += _thr_b;                        // s16 = s16 + ?16
			f = fstb::limit (f, 0, 256);        // ?16
			ao = (ao * f) >> 8;                 // s16 = ?16 * s8 # s8 = s16 >> 8
			an = (an * f) >> 8;
		}

		const int		dith_o = pattern [0] [pos & (PAT_WIDTH-1)];	// s8
		const int		dith_n = int8_t (rnd_state >> 24);			// s8

		const int		dither = (dith_o * ao + dith_n * an) >> AMP_BITS;	// s16 = s8 * s8 // s16 = s16 >> cst
		const int		dif = (dither + lsb + 128) >> 8;	// s16 = s16 + u8 + ?8 # s8 = s16 >> 8

		const int		pix = fstb::limit (msb + dif, 0, 255);	// u8 = u8 + s8
		dst_ptr [pos] = uint8_t (pix);
	}

	generate_rnd_eol (rnd_state);
}



void	AvsFilterDitherPost::process_segment_ord_sse2 (uint8_t *dst_ptr, const uint8_t *msb_ptr, const uint8_t *lsb_ptr, const int16_t pattern [PAT_LEVELS] [PAT_WIDTH], int w, uint32_t &rnd_state) const
{
	const __m128i	zero = _mm_setzero_si128 ();
	const __m128i	c128 = _mm_set1_epi16 (128);
	const __m128i	c1   = _mm_set1_epi16 (1);
	const __m128i	c256 = _mm_set1_epi16 (256);

	const __m128i	ampo_i = _mm_set1_epi16 (int16_t (_ampo_i)); // 8 ?16 [0 ; 255]
	const __m128i	ampn_i = _mm_set1_epi16 (int16_t (_ampn_i)); // 8 ?16 [0 ; 255]

	const __m128i	thr_a = _mm_set1_epi16 (int16_t (_thr_a));   // 8 s16 [0 ; 255]
	const __m128i	thr_b = _mm_set1_epi16 (int16_t (_thr_b));   // 8 s16 [0 ; 255]

	for (int pos = 0; pos < w; pos += 8)
	{
		// Random generation
		generate_rnd (rnd_state);
		const int		rnd_03 = rnd_state;
		generate_rnd (rnd_state);
		const int		rnd_47 = rnd_state;
		const __m128i	rnd_val = _mm_set_epi32 (0, 0, rnd_47, rnd_03);

		// Amplitudes
		__m128i			ampo = ampo_i;										// 8 ?16 [0 ; 255]
		__m128i			ampn = ampn_i;										// 8 ?16 [0 ; 255]

		__m128i			lsb = _mm_loadl_epi64 (
			reinterpret_cast <const __m128i *> (lsb_ptr + pos)
		);
		lsb = _mm_unpacklo_epi8 (lsb, zero);							// 8 ?16 [0 ; 255]

		// Thresholding
		if (_threshold_flag)
		{
			__m128i			f = _mm_sub_epi16 (lsb, c128);

			// Absolute value
			const __m128i	neg_mask = _mm_cmplt_epi16 (f, zero);
			const __m128i	neg_fix = _mm_and_si128 (c1, neg_mask);
			f = _mm_xor_si128 (f, neg_mask);
			f = _mm_add_epi16 (f, neg_fix);								// 8 ?16 [0 ; 128]

			f = _mm_mullo_epi16 (f, thr_a);
			f = _mm_add_epi16 (f, thr_b);
			f = _mm_min_epi16 (f, c256);
			f = _mm_max_epi16 (f, zero);

			ampo = _mm_mullo_epi16 (ampo, f);
			ampn = _mm_mullo_epi16 (ampn, f);
			ampo = _mm_srai_epi16 (ampo, 8);
			ampn = _mm_srai_epi16 (ampn, 8);
		}

		// Dithering
		__m128i			dith_o = _mm_load_si128 (						// 8 s16 [-128 ; 127]
			reinterpret_cast <const __m128i *> (&pattern [0] [0])
		);
		__m128i			dith_n = _mm_unpacklo_epi8 (rnd_val, zero);
		dith_n = _mm_sub_epi16 (dith_n, c128);							// 8 s16 [-128 ; 127]

		dith_o = _mm_mullo_epi16 (dith_o, ampo);						// 8 s16 (full range)
		dith_n = _mm_mullo_epi16 (dith_n, ampn);						// 8 s16 (full range)
		__m128i			dither = _mm_adds_epi16 (dith_o, dith_n);
		dither = _mm_srai_epi16 (dither, AMP_BITS);					// 8 s16

		// Pixel computation
		__m128i			dif = _mm_add_epi16 (dither, lsb);			// 8 s16
		dif = _mm_add_epi16 (dif, c128);									// 8 s16
		dif = _mm_srai_epi16 (dif, 8);									// 8 s16 (small signed range)

		__m128i			msb = _mm_loadl_epi64 (
			reinterpret_cast <const __m128i *> (msb_ptr + pos)
		);
		msb = _mm_unpacklo_epi8 (msb, zero);							// 8 ?16 [0 ; 255]

		__m128i			pix = _mm_add_epi16 (msb, dif);				// 8 s16
		pix = _mm_max_epi16 (pix, zero);									// 8 u16
		pix = _mm_packus_epi16 (pix, pix);								// 8 u8

		_mm_storel_epi64 (reinterpret_cast <__m128i *> (dst_ptr + pos), pix);
	}

	generate_rnd_eol (rnd_state);
}



// With default settings: ampo = 1, ampn = 0, thr = 0
void	AvsFilterDitherPost::process_segment_ord_sse2_simple (uint8_t *dst_ptr, const uint8_t *msb_ptr, const uint8_t *lsb_ptr, const int16_t pattern [PAT_LEVELS] [PAT_WIDTH], int w, uint32_t & /*rnd_state*/) const
{
	const __m128i	zero = _mm_setzero_si128 ();
	const __m128i	c128 = _mm_set1_epi16 (128);

	const __m128i	ampo_i = _mm_set1_epi16 (int16_t (_ampo_i)); // 8 ?16 [0 ; 255]

	for (int pos = 0; pos < w; pos += 8)
	{
		// Amplitudes
		__m128i			ampo = ampo_i;										// 8 ?16 [0 ; 255]

		__m128i			lsb = _mm_loadl_epi64 (
			reinterpret_cast <const __m128i *> (lsb_ptr + pos)
		);
		lsb = _mm_unpacklo_epi8 (lsb, zero);							// 8 ?16 [0 ; 255]

		// Dithering
		__m128i			dither = _mm_load_si128 (						// 8 s16 [-128 ; 127]
			reinterpret_cast <const __m128i *> (&pattern [0] [0])
		);

		// Pixel computation
		__m128i			dif = _mm_add_epi16 (dither, lsb);			// 8 s16
		dif = _mm_add_epi16 (dif, c128);									// 8 s16
		dif = _mm_srai_epi16 (dif, 8);									// 8 s16 (small signed range)

		__m128i			msb = _mm_loadl_epi64 (
			reinterpret_cast <const __m128i *> (msb_ptr + pos)
		);
		msb = _mm_unpacklo_epi8 (msb, zero);							// 8 ?16 [0 ; 255]

		__m128i			pix = _mm_add_epi16 (msb, dif);				// 8 s16
		pix = _mm_max_epi16 (pix, zero);									// 8 u16
		pix = _mm_packus_epi16 (pix, pix);								// 8 u8

		_mm_storel_epi64 (reinterpret_cast <__m128i *> (dst_ptr + pos), pix);
	}
}



void	AvsFilterDitherPost::process_segment_2b_cpp (uint8_t *dst_ptr, const uint8_t *msb_ptr, const uint8_t *lsb_ptr, const int16_t pattern [PAT_LEVELS] [PAT_WIDTH], int w, uint32_t & /*rnd_state*/) const
{
	for (int pos = 0; pos < w; ++pos)
	{
		const int		lsb = lsb_ptr [pos];
		int				pix = msb_ptr [pos];
		const int		lvl = ((lsb + 32) >> 6) - 1;
		if (lvl >= PAT_LEVELS)
		{
			++ pix;
		}
		else if (lvl >= 0)
		{
			pix += pattern [lvl] [pos & (PAT_WIDTH-1)];
		}
		dst_ptr [pos] = uint8_t (pix);
	}
}



void	AvsFilterDitherPost::process_segment_2b_sse2 (uint8_t *dst_ptr, const uint8_t *msb_ptr, const uint8_t *lsb_ptr, const int16_t pattern [PAT_LEVELS] [PAT_WIDTH], int w, uint32_t & /*rnd_state*/) const
{
	const __m128i	zero = _mm_setzero_si128 ();
	const __m128i	c1   = _mm_set1_epi16 (1);
	const __m128i	c32  = _mm_set1_epi16 (32);

	const __m128i	p_0 = _mm_load_si128 (
		reinterpret_cast <const __m128i *> (&pattern [0] [0])
	);
	const __m128i	p_1 = _mm_load_si128 (
		reinterpret_cast <const __m128i *> (&pattern [1] [0])
	);
	const __m128i	p_2 = _mm_load_si128 (
		reinterpret_cast <const __m128i *> (&pattern [2] [0])
	);

	for (int pos = 0; pos < w; pos += 8)
	{
		__m128i			lsb = _mm_loadl_epi64 (
			reinterpret_cast <const __m128i *> (lsb_ptr + pos)
		);
		lsb = _mm_unpacklo_epi8 (lsb, zero);

		__m128i			lvl = _mm_add_epi16 (lsb, c32);
		lvl = _mm_srai_epi16 (lvl, 6);

		__m128i			mask;
		__m128i			dif = zero;

		mask = _mm_cmpeq_epi16 (lvl, c1);
		lvl = _mm_sub_epi16 (lvl, c1);
		dif = _mm_or_si128 (dif, _mm_and_si128 (mask, p_0));

		mask = _mm_cmpeq_epi16 (lvl, c1);
		lvl = _mm_sub_epi16 (lvl, c1);
		dif = _mm_or_si128 (dif, _mm_and_si128 (mask, p_1));

		mask = _mm_cmpeq_epi16 (lvl, c1);
		lvl = _mm_sub_epi16 (lvl, c1);
		dif = _mm_or_si128 (dif, _mm_and_si128 (mask, p_2));

		mask = _mm_cmpeq_epi16 (lvl, c1);
		dif = _mm_or_si128 (dif, _mm_and_si128 (mask, c1));

		__m128i			msb = _mm_loadl_epi64 (
			reinterpret_cast <const __m128i *> (msb_ptr + pos)
		);
		msb = _mm_unpacklo_epi8 (msb, zero);

		__m128i			pix = _mm_add_epi16 (msb, dif);
		pix = _mm_max_epi16 (pix, zero);
		pix = _mm_packus_epi16 (pix, pix);

		_mm_storel_epi64 (reinterpret_cast <__m128i *> (dst_ptr + pos), pix);
	}
}



void	AvsFilterDitherPost::process_segment_floydsteinberg_cpp (uint8_t *dst_ptr, const uint8_t *msb_ptr, const uint8_t *lsb_ptr, fmtcl::ErrDifBuf &ed_buf, int w, int buf_ofs, int y, uint32_t &rnd_state) const
{
	assert (dst_ptr != 0);
	assert (msb_ptr != 0);
	assert (lsb_ptr != 0);
	assert (&ed_buf != 0);
	assert (w > 0);
	assert (w <= MAX_SEG_LEN);
	assert (buf_ofs >= 0);
	assert (y >= 0);
	assert (&rnd_state != 0);

	int16_t *		err_ptr = ed_buf.get_buf <int16_t> (0);
	err_ptr += buf_ofs;

	int				e1;
	int				e3;
	int				e5;
	int				e7;
	int				err_nxt = ed_buf.use_mem <int16_t> (0);

	// Forward
	if ((y & 1) == 0)
	{
		for (int x = 0; x < w; ++x)
		{
			int				err = err_nxt;

			quantize_pix (dst_ptr, msb_ptr, lsb_ptr, x, err, rnd_state, _ampe_i, _ampn_i);
			diffuse_floydsteinberg (err, e1, e3, e5, e7);

			err_nxt = err_ptr [x + 1];
			err_ptr [x - 1] += int16_t (e3);
			err_ptr [x    ] += int16_t (e5);
			err_ptr [x + 1]  = int16_t (e1);
			err_nxt         += e7;
		}
	}

	// Backward
	else
	{
		for (int x = w - 1; x >= 0; --x)
		{
			int				err = err_nxt;

			quantize_pix (dst_ptr, msb_ptr, lsb_ptr, x, err, rnd_state, _ampe_i, _ampn_i);
			diffuse_floydsteinberg (err, e1, e3, e5, e7);

			err_nxt = err_ptr [x - 1];
			err_ptr [x + 1] += int16_t (e3);
			err_ptr [x    ] += int16_t (e5);
			err_ptr [x - 1]  = int16_t (e1);
			err_nxt         += e7;
		}
	}

	ed_buf.use_mem <int16_t> (0) = int16_t (err_nxt);

	generate_rnd_eol (rnd_state);
}



void	AvsFilterDitherPost::process_segment_stucki_cpp (uint8_t *dst_ptr, const uint8_t *msb_ptr, const uint8_t *lsb_ptr, fmtcl::ErrDifBuf &ed_buf, int w, int buf_ofs, int y, uint32_t &rnd_state) const
{
	assert (dst_ptr != 0);
	assert (msb_ptr != 0);
	assert (lsb_ptr != 0);
	assert (&ed_buf != 0);
	assert (w > 0);
	assert (w <= MAX_SEG_LEN);
	assert (buf_ofs >= 0);
	assert (y >= 0);
	assert (&rnd_state != 0);

	int16_t *		err0_ptr = ed_buf.get_buf <int16_t> (     y & 1 );
	int16_t *		err1_ptr = ed_buf.get_buf <int16_t> (1 - (y & 1));
	err0_ptr += buf_ofs;
	err1_ptr += buf_ofs;

	int				e1;
	int				e2;
	int				e4;
	int				e8;
	int				err_nxt0 = ed_buf.use_mem <int16_t> (0);
	int				err_nxt1 = ed_buf.use_mem <int16_t> (1);

	// Forward
	if ((y & 1) == 0)
	{
		for (int x = 0; x < w; ++x)
		{
			int				err = err_nxt0;

			quantize_pix (dst_ptr, msb_ptr, lsb_ptr, x, err, rnd_state, _ampe_i, _ampn_i);
			diffuse_stucki (err, e1, e2, e4, e8);

			err_nxt0 = err_nxt1 + e8;
			err_nxt1 = err1_ptr [x + 2] + e4;
			err0_ptr [x - 2] += int16_t (e2);
			err0_ptr [x - 1] += int16_t (e4);
			err0_ptr [x    ] += int16_t (e8);
			err0_ptr [x + 1] += int16_t (e4);
			err0_ptr [x + 2] += int16_t (e2);
			err1_ptr [x - 2] += int16_t (e1);
			err1_ptr [x - 1] += int16_t (e2);
			err1_ptr [x    ] += int16_t (e4);
			err1_ptr [x + 1] += int16_t (e2);
			err1_ptr [x + 2]  = int16_t (e1);
		}
	}

	// Backward
	else
	{
		for (int x = w - 1; x >= 0; --x)
		{
			int				err = err_nxt0;

			quantize_pix (dst_ptr, msb_ptr, lsb_ptr, x, err, rnd_state, _ampe_i, _ampn_i);
			diffuse_stucki (err, e1, e2, e4, e8);

			err_nxt0 = err_nxt1 + e8;
			err_nxt1 = err1_ptr [x - 2] + e4;
			err0_ptr [x + 2] += int16_t (e2);
			err0_ptr [x + 1] += int16_t (e4);
			err0_ptr [x    ] += int16_t (e8);
			err0_ptr [x - 1] += int16_t (e4);
			err0_ptr [x - 2] += int16_t (e2);
			err1_ptr [x + 2] += int16_t (e1);
			err1_ptr [x + 1] += int16_t (e2);
			err1_ptr [x    ] += int16_t (e4);
			err1_ptr [x - 1] += int16_t (e2);
			err1_ptr [x - 2]  = int16_t (e1);
		}
	}

	ed_buf.use_mem <int16_t> (0) = int16_t (err_nxt0);
	ed_buf.use_mem <int16_t> (1) = int16_t (err_nxt1);

	generate_rnd_eol (rnd_state);
}



void	AvsFilterDitherPost::process_segment_atkinson_cpp (uint8_t *dst_ptr, const uint8_t *msb_ptr, const uint8_t *lsb_ptr, fmtcl::ErrDifBuf &ed_buf, int w, int buf_ofs, int y, uint32_t &rnd_state) const
{
	assert (dst_ptr != 0);
	assert (msb_ptr != 0);
	assert (lsb_ptr != 0);
	assert (&ed_buf != 0);
	assert (w > 0);
	assert (w <= MAX_SEG_LEN);
	assert (buf_ofs >= 0);
	assert (y >= 0);
	assert (&rnd_state != 0);

	int16_t *		err0_ptr = ed_buf.get_buf <int16_t> (     y & 1 );
	int16_t *		err1_ptr = ed_buf.get_buf <int16_t> (1 - (y & 1));
	err0_ptr += buf_ofs;
	err1_ptr += buf_ofs;

	int				e1;
	int				err_nxt0 = ed_buf.use_mem <int16_t> (0);
	int				err_nxt1 = ed_buf.use_mem <int16_t> (1);

	// Forward
	if ((y & 1) == 0)
	{
		for (int x = 0; x < w; ++x)
		{
			int				err = err_nxt0;

			quantize_pix (dst_ptr, msb_ptr, lsb_ptr, x, err, rnd_state, _ampe_i, _ampn_i);
			diffuse_atkinson (err, e1);

			err_nxt0 = err_nxt1 + e1;
			err_nxt1 = err1_ptr [x + 2] + e1;
			err0_ptr [x - 1] += int16_t (e1);
			err0_ptr [x    ] += int16_t (e1);
			err0_ptr [x + 1] += int16_t (e1);
			err1_ptr [x    ]  = int16_t (e1);
		}
	}

	// Backward
	else
	{
		for (int x = w - 1; x >= 0; --x)
		{
			int				err = err_nxt0;

			quantize_pix (dst_ptr, msb_ptr, lsb_ptr, x, err, rnd_state, _ampe_i, _ampn_i);
			diffuse_atkinson (err, e1);

			err_nxt0 = err_nxt1 + e1;
			err_nxt1 = err1_ptr [x - 2] + e1;
			err0_ptr [x + 1] += int16_t (e1);
			err0_ptr [x    ] += int16_t (e1);
			err0_ptr [x - 1] += int16_t (e1);
			err1_ptr [x    ]  = int16_t (e1);
		}
	}

	ed_buf.use_mem <int16_t> (0) = int16_t (err_nxt0);
	ed_buf.use_mem <int16_t> (1) = int16_t (err_nxt1);

	generate_rnd_eol (rnd_state);
}



void	AvsFilterDitherPost::apply_mask_cpp (uint8_t *dst_ptr, const uint8_t *msb_ptr, const uint8_t *msk_ptr, int w) const
{
	for (int pos = 0; pos < w; ++pos)
	{
		if (msk_ptr [pos] >= 128)
		{
			dst_ptr [pos] = msb_ptr [pos];
		}
	}
}



void	AvsFilterDitherPost::apply_mask_sse2 (uint8_t *dst_ptr, const uint8_t *msb_ptr, const uint8_t *msk_ptr, int w) const
{
	const __m128i	zero = _mm_setzero_si128 ();

	for (int pos = 0; pos < w; pos += 16)
	{
		const __m128i	msk = _mm_load_si128 (
			reinterpret_cast <const __m128i *> (msk_ptr + pos)
		);
		const __m128i	msb = _mm_load_si128 (
			reinterpret_cast <const __m128i *> (msb_ptr + pos)
		);
		__m128i			dst = _mm_load_si128 (
			reinterpret_cast <const __m128i *> (dst_ptr + pos)
		);

		const __m128i	copy_mask = _mm_cmplt_epi8 (msk, zero);
		const __m128i	src = _mm_and_si128 (copy_mask, msb);
		dst = _mm_andnot_si128 (copy_mask, dst);
		dst = _mm_or_si128 (dst, src);

		_mm_store_si128 (reinterpret_cast <__m128i *> (dst_ptr + pos), dst);
	}
}



void	AvsFilterDitherPost::protect_pix_generic (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int pos, int stride_msb, bool l_flag, bool r_flag, bool t_flag, bool b_flag)
{
	assert (dst_msb_ptr != 0);
	assert (dst_lsb_ptr != 0);
	assert (src_msb_ptr != 0);
	assert (src_lsb_ptr != 0);

	int				msb = src_msb_ptr [pos];
	int				mi = msb;
	int				ma = msb;

	if (! l_flag)
	{
		update_min_max (mi, ma, src_msb_ptr [pos - 1]);
	}
	if (! r_flag)
	{
		update_min_max (mi, ma, src_msb_ptr [pos + 1]);
	}

	if (! t_flag)
	{
		update_min_max (mi, ma, src_msb_ptr [pos - stride_msb]);

		if (! l_flag)
		{
			update_min_max (mi, ma, src_msb_ptr [pos - stride_msb - 1]);
		}
		if (! r_flag)
		{
			update_min_max (mi, ma, src_msb_ptr [pos - stride_msb + 1]);
		}
	}

	if (! b_flag)
	{
		update_min_max (mi, ma, src_msb_ptr [pos + stride_msb]);

		if (! l_flag)
		{
			update_min_max (mi, ma, src_msb_ptr [pos + stride_msb - 1]);
		}
		if (! r_flag)
		{
			update_min_max (mi, ma, src_msb_ptr [pos + stride_msb + 1]);
		}
	}

	protect_pix_decide (dst_msb_ptr, dst_lsb_ptr, msb, src_lsb_ptr, pos, mi, ma);
}



void	AvsFilterDitherPost::update_min_max (int &mi, int &ma, int val)
{
	if (val < mi)
	{
		mi = val;
	}
	else if (val > ma)
	{
		ma = val;
	}
}



void	AvsFilterDitherPost::update_min_max (__m128i &mi_0, __m128i &mi_1, __m128i &ma_0, __m128i &ma_1, const uint8_t *val_ptr)
{
	assert (val_ptr != 0);

	const __m128i	zero = _mm_setzero_si128 ();
	__m128i			val = _mm_loadu_si128 (
		reinterpret_cast <const __m128i *> (val_ptr)
	);
	const __m128i	val_0 = _mm_unpacklo_epi8 (val, zero);
	const __m128i	val_1 = _mm_unpackhi_epi8 (val, zero);

	mi_0 = _mm_min_epi16 (mi_0, val_0);
	mi_1 = _mm_min_epi16 (mi_1, val_1);
	ma_0 = _mm_max_epi16 (ma_0, val_0);
	ma_1 = _mm_max_epi16 (ma_1, val_1);
}



void	AvsFilterDitherPost::protect_pix_decide (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, int msb, const uint8_t *src_lsb_ptr, int pos, int mi, int ma)
{
	assert (dst_msb_ptr != 0);
	assert (dst_lsb_ptr != 0);
	assert (src_lsb_ptr != 0);
	assert (mi <= ma);

	int				lsb = src_lsb_ptr [pos];
	if (mi != msb)
	{
		lsb = 0;
	}
	else if (ma == msb + 1)
	{
#if 1	// Quicker solution
		lsb = 255;
#else
		lsb = 0;
		msb = ma;
#endif
	}

	dst_msb_ptr [pos] = uint8_t (msb);
	dst_lsb_ptr [pos] = uint8_t (lsb);
}



void	AvsFilterDitherPost::generate_rnd (uint32_t &state)
{
	state = state * uint32_t (1664525) + 1013904223;
}



void	AvsFilterDitherPost::generate_rnd_eol (uint32_t &state)
{
	state = state * uint32_t (1103515245) + 12345;
	if ((state & 0x2000000) != 0)
	{
		state = state * uint32_t (134775813) + 1;
	}
}



void	AvsFilterDitherPost::quantize_pix (uint8_t *dst_ptr, const uint8_t *msb_ptr, const uint8_t *lsb_ptr, int x, int &err, uint32_t &rnd_state, int ampe_i, int ampn_i)
{
	const int		msb = msb_ptr [x];
	const int		lsb = lsb_ptr [x];

	generate_rnd (rnd_state);
	const int		rnd_val = int8_t (rnd_state >> 24);			// s8
	const int		rnd_add = (rnd_val * ampn_i) >> AMP_BITS;	// s16 = s8 * s8 // s16 = s16 >> cst

	const int		base = lsb + err;
	const int		err_add = (err < 0) ? -ampe_i : ampe_i;
	const int		quant = (base + rnd_add + err_add + 0x80) >> 8;
	const int		raw_val = msb + quant;
	const int		val = std::max (std::min (raw_val, 255), 0);
	err = base + ((msb - raw_val) << 8);

	dst_ptr [x] = uint8_t (val);
}



void	AvsFilterDitherPost::diffuse_floydsteinberg (int err, int &e1, int &e3, int &e5, int &e7)
{
	e1 = (err     + 8) >> 4;
	e3 = (err * 3 + 8) >> 4;
	e5 = (err * 5 + 8) >> 4;
	e7 = err - e1 - e3 - e5;
}



void	AvsFilterDitherPost::diffuse_stucki (int err, int &e1, int &e2, int &e4, int &e8)
{
	const int		m = (err << 4) / 42;

	e1 = (m + 8) >> 4;
	e2 = (m + 4) >> 3;
	e4 = (m + 2) >> 2;
//	e8 = (m + 1) >> 1;
	const int		sum = (e1 << 1) + ((e2 + e4) << 2);
	e8 = (err - sum + 1) >> 1;
}



void	AvsFilterDitherPost::diffuse_atkinson (int err, int &e1)
{
	e1 = (err + 4) >> 3;
}



#undef	AvsFilterDitherPost_CHECK_REF_CODE



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
