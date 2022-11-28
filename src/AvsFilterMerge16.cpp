/*****************************************************************************

        AvsFilterMerge16.cpp
        Author: Laurent de Soras, 2011

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

#include	"fstb/ToolsSse2.h"
#include	"AvsFilterMerge16.h"

#include	<cassert>



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



AvsFilterMerge16::AvsFilterMerge16 (::IScriptEnvironment *env_ptr, ::PClip src1_clip_sptr, ::PClip src2_clip_sptr, ::PClip mask_clip_sptr, bool luma_flag, int y, int u, int v)
:	GenericVideoFilter (src1_clip_sptr)
,	_src1_clip_sptr (src1_clip_sptr)
,	_src2_clip_sptr (src2_clip_sptr)
,	_mask_clip_sptr (mask_clip_sptr)
#if defined (_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4355)
#endif // 'this': used in base member initializer list
,	_plane_proc (vi, vi, *this)
#if defined (_MSC_VER)
#pragma warning (pop)
#endif
,	_luma_flag (luma_flag)
,	_luma_type (0)
,	_pitch_mul (1)
,	_process_default_subplane_ptr (&AvsFilterMerge16::process_subplane_cpp <Mask444>)
,	_process_chroma_subplane_ptr (&AvsFilterMerge16::process_subplane_cpp <Mask444>)
{
	// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
	// Parameter check

	if (! vi.IsPlanar ())
	{
		env_ptr->ThrowError ("Dither_merge16: input must be planar.");
	}

	PlaneProcessor::check_same_format (
		env_ptr, vi, src2_clip_sptr, "Dither_merge16", "src2"
	);

	int            chk_flags = PlaneProcessor::FmtChkFlag_ALL;
	if (_luma_flag)
	{
		chk_flags &= ~int (PlaneProcessor::FmtChkFlag_CS_ALL);
		chk_flags |=  int (PlaneProcessor::FmtChkFlag_CS_LAYOUT);
	}
	PlaneProcessor::check_same_format (
		env_ptr, vi, mask_clip_sptr, "Dither_merge16", "mask", chk_flags
	);

	if (! PlaneProcessor::check_stack16_height (vi))
	{
		env_ptr->ThrowError (
			"Dither_merge16: stacked MSB/LSB data must have a height\n"
			"multiple of twice the maximum chroma subsampling."
		);
	}

	const long     cpu_flags = env_ptr->GetCPUFlags ();
	const bool     sse2_flag = ((cpu_flags & CPUF_SSE2) != 0);

	if (sse2_flag)
	{
		_process_default_subplane_ptr =
			&AvsFilterMerge16::process_subplane_sse2 <Mask444>;
	}

	if (_luma_flag)
	{
		if (! vi.IsYUV ())
		{
			env_ptr->ThrowError (
				"Dither_merge16: Colorspace must be YUV in luma mode."
			);
		}

		if (vi.IsY8 ())
		{
			_luma_type = VideoInfo::CS_YV24;
			_pitch_mul = 1;
			if (sse2_flag)
			{
				_process_chroma_subplane_ptr =
					&AvsFilterMerge16::process_subplane_sse2 <Mask444>;
			}
			else
			{
				_process_chroma_subplane_ptr =
					&AvsFilterMerge16::process_subplane_cpp <Mask444>;
			}
		}

		else
		{
			const int		ssh = vi.GetPlaneWidthSubsampling (PLANAR_U);
			const int		ssv = vi.GetPlaneHeightSubsampling (PLANAR_U);
			switch ((ssv << 4) + ssh)
			{
			case	0x00:		// 4:4:4
				_luma_type = VideoInfo::CS_YV24;
				_pitch_mul = 1;
				if (sse2_flag)
				{
					_process_chroma_subplane_ptr =
						&AvsFilterMerge16::process_subplane_sse2 <Mask444>;
				}
				else
				{
					_process_chroma_subplane_ptr =
						&AvsFilterMerge16::process_subplane_cpp <Mask444>;
				}
				break;
			case	0x01:		// 4:2:2
				_luma_type = VideoInfo::CS_YV16;
				_pitch_mul = 1;
				if (sse2_flag)
				{
					_process_chroma_subplane_ptr =
						&AvsFilterMerge16::process_subplane_sse2 <Mask422>;
				}
				else
				{
					_process_chroma_subplane_ptr =
						&AvsFilterMerge16::process_subplane_cpp <Mask422>;
				}
				break;
			case	0x02:		// 4:1:1
				_luma_type = VideoInfo::CS_YV411;
				_pitch_mul = 1;
				if (sse2_flag)
				{
					_process_chroma_subplane_ptr =
						&AvsFilterMerge16::process_subplane_sse2 <Mask411>;
				}
				else
				{
					_process_chroma_subplane_ptr =
						&AvsFilterMerge16::process_subplane_cpp <Mask411>;
				}
				break;
			case	0x11:		// 4:2:0
				_luma_type = VideoInfo::CS_YV12;
				_pitch_mul = 2;
				if (sse2_flag)
				{
					_process_chroma_subplane_ptr =
						&AvsFilterMerge16::process_subplane_sse2 <Mask420>;
				}
				else
				{
					_process_chroma_subplane_ptr =
						&AvsFilterMerge16::process_subplane_cpp <Mask420>;
				}
				break;
			default:
				env_ptr->ThrowError (
					"Dither_merge16: unsupported chroma subsampling in luma mode."
				);
			}
		}
	}

	// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -

	_plane_proc.set_proc_mode (y, u, v);
	_plane_proc.set_clip_info (0, PlaneProcessor::ClipType_STACKED_16);
	_plane_proc.set_clip_info (1, PlaneProcessor::ClipType_STACKED_16);
	_plane_proc.set_clip_info (2, PlaneProcessor::ClipType_STACKED_16);
	_plane_proc.set_clip_info (3, PlaneProcessor::ClipType_STACKED_16);
}



::PVideoFrame __stdcall	AvsFilterMerge16::GetFrame (int n, ::IScriptEnvironment *env_ptr)
{
	::PVideoFrame	dst_sptr = env_ptr->NewVideoFrame (vi);

	_plane_proc.process_frame (
		dst_sptr, n, *env_ptr,
		&_src1_clip_sptr, &_src2_clip_sptr, &_mask_clip_sptr
	);

	return (dst_sptr);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	AvsFilterMerge16::do_process_plane (::PVideoFrame &dst_sptr, int n, ::IScriptEnvironment &env, int /*plane_index*/, int plane_id, void * /*ctx_ptr*/)
{
	::PVideoFrame	src1_sptr = _src1_clip_sptr->GetFrame (n, &env);
	::PVideoFrame	src2_sptr = _src2_clip_sptr->GetFrame (n, &env);
	::PVideoFrame	mask_sptr = _mask_clip_sptr->GetFrame (n, &env);

	const int		w             = _plane_proc.get_width (dst_sptr, plane_id);
	const int		h             = _plane_proc.get_height16 (dst_sptr, plane_id);
	uint8_t *		data_msbd_ptr = dst_sptr->GetWritePtr (plane_id);
	const int		stride_dst    = dst_sptr->GetPitch (plane_id);

	const uint8_t*	data_msb1_ptr = src1_sptr->GetReadPtr (plane_id);
	const int		stride_sr1    = src1_sptr->GetPitch (plane_id);

	const uint8_t*	data_msb2_ptr = src2_sptr->GetReadPtr (plane_id);
	const int		stride_sr2    = src2_sptr->GetPitch (plane_id);

	const int		mask_id       = (_luma_flag) ? PLANAR_Y : plane_id;

	const int      pitch_mul_msk = (plane_id == PLANAR_Y) ? 1 : _pitch_mul;
	const uint8_t*	data_msbm_ptr = mask_sptr->GetReadPtr (mask_id);
	const int		stride_msk    = mask_sptr->GetPitch (mask_id);

	TaskDataGlobal		tdg;
	tdg._this_ptr    = this;
	tdg._w           = w;
	tdg._dst_msb_ptr = data_msbd_ptr;
	tdg._dst_lsb_ptr = data_msbd_ptr + h * stride_dst;
	tdg._sr1_msb_ptr = data_msb1_ptr;
	tdg._sr1_lsb_ptr = data_msb1_ptr + h * stride_sr1;
	tdg._sr2_msb_ptr = data_msb2_ptr;
	tdg._sr2_lsb_ptr = data_msb2_ptr + h * stride_sr2;
	tdg._msk_msb_ptr = data_msbm_ptr;
	tdg._msk_lsb_ptr = data_msbm_ptr + h * stride_msk * pitch_mul_msk;
	tdg._stride_dst  = stride_dst;
	tdg._stride_sr1  = stride_sr1;
	tdg._stride_sr2  = stride_sr2;
	tdg._stride_msk  = stride_msk;
	tdg._pitch_mul   = pitch_mul_msk;

	PlaneProcPtr	plane_proc_ptr = _process_default_subplane_ptr;
	if (_luma_flag && plane_id != PLANAR_Y)
	{
		plane_proc_ptr = _process_chroma_subplane_ptr;
	}

	Slicer			slicer;
	slicer.start (h, tdg, plane_proc_ptr);
	slicer.wait ();
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <class T>
void	AvsFilterMerge16::process_subplane_cpp (Slicer::TaskData &td)
{
	assert (&td != 0);

	const TaskDataGlobal &	tdg = *(td._glob_data_ptr);
	const int		stride_msk  = tdg._stride_msk * tdg._pitch_mul;
	uint8_t *		dst_msb_ptr = tdg._dst_msb_ptr + td._y_beg * tdg._stride_dst;
	uint8_t *		dst_lsb_ptr = tdg._dst_lsb_ptr + td._y_beg * tdg._stride_dst;
	const uint8_t*	sr1_msb_ptr = tdg._sr1_msb_ptr + td._y_beg * tdg._stride_sr1;
	const uint8_t*	sr1_lsb_ptr = tdg._sr1_lsb_ptr + td._y_beg * tdg._stride_sr1;
	const uint8_t*	sr2_msb_ptr = tdg._sr2_msb_ptr + td._y_beg * tdg._stride_sr2;
	const uint8_t*	sr2_lsb_ptr = tdg._sr2_lsb_ptr + td._y_beg * tdg._stride_sr2;
	const uint8_t*	msk_msb_ptr = tdg._msk_msb_ptr + td._y_beg * stride_msk;
	const uint8_t*	msk_lsb_ptr = tdg._msk_lsb_ptr + td._y_beg * stride_msk;

	for (int y = td._y_beg; y < td._y_end; ++y)
	{
		for (int x = 0; x < tdg._w; ++x)
		{
			const int      src_1 = (sr1_msb_ptr [x] << 8) + sr1_lsb_ptr [x];
			const int      src_2 = (sr2_msb_ptr [x] << 8) + sr2_lsb_ptr [x];
			const int      mask  =
				T::get_mask_cpp (msk_msb_ptr, msk_lsb_ptr, x, tdg._stride_msk);

			int            res;
			if (mask == 0)
			{
				res = src_1;
			}
			else if (mask == 65535)
			{
				res = src_2;
			}
			else
			{
				const int      diff = src_2 - src_1;
				const int      lerp = (diff * (mask >> 1)) >> 15;
				res = src_1 + lerp;
			}

			dst_msb_ptr [x] = uint8_t (res >> 8 );
			dst_lsb_ptr [x] = uint8_t (res & 255);
		}

		dst_msb_ptr += tdg._stride_dst;
		dst_lsb_ptr += tdg._stride_dst;
		sr1_msb_ptr += tdg._stride_sr1;
		sr1_lsb_ptr += tdg._stride_sr1;
		sr2_msb_ptr += tdg._stride_sr2;
		sr2_lsb_ptr += tdg._stride_sr2;
		msk_msb_ptr += stride_msk;
		msk_lsb_ptr += stride_msk;
	}
}



template <class T>
void	AvsFilterMerge16::process_subplane_sse2 (Slicer::TaskData &td)
{
	assert (&td != 0);

	const TaskDataGlobal &	tdg = *(td._glob_data_ptr);
	const int		stride_msk  = tdg._stride_msk * tdg._pitch_mul;
	uint8_t *		dst_msb_ptr = tdg._dst_msb_ptr + td._y_beg * tdg._stride_dst;
	uint8_t *		dst_lsb_ptr = tdg._dst_lsb_ptr + td._y_beg * tdg._stride_dst;
	const uint8_t*	sr1_msb_ptr = tdg._sr1_msb_ptr + td._y_beg * tdg._stride_sr1;
	const uint8_t*	sr1_lsb_ptr = tdg._sr1_lsb_ptr + td._y_beg * tdg._stride_sr1;
	const uint8_t*	sr2_msb_ptr = tdg._sr2_msb_ptr + td._y_beg * tdg._stride_sr2;
	const uint8_t*	sr2_lsb_ptr = tdg._sr2_lsb_ptr + td._y_beg * tdg._stride_sr2;
	const uint8_t*	msk_msb_ptr = tdg._msk_msb_ptr + td._y_beg * stride_msk;
	const uint8_t*	msk_lsb_ptr = tdg._msk_lsb_ptr + td._y_beg * stride_msk;

	const __m128i  zero      = _mm_setzero_si128 ();
	const __m128i  ffff      = _mm_set1_epi16 (-1);
	const __m128i  mask_sign = _mm_set1_epi16 (-0x8000);
	const __m128i  offset_s  = _mm_set1_epi32 (0x8000);
	const __m128i  mask_lsb  = _mm_set1_epi16 (0x00FF);

	const int      w8 = tdg._w & -8;
	const int      w7 = tdg._w - w8;

	for (int y = td._y_beg; y < td._y_end; ++y)
	{
		for (int x = 0; x < w8; x += 8)
		{
			const __m128i  src_1 =
				fstb::ToolsSse2::load_8_16ml (&sr1_msb_ptr [x], &sr1_lsb_ptr [x]);
			const __m128i  src_2 =
				fstb::ToolsSse2::load_8_16ml (&sr2_msb_ptr [x], &sr2_lsb_ptr [x]);
			const __m128i  mask  =
				T::get_mask_sse2 (msk_msb_ptr, msk_lsb_ptr, x, tdg._stride_msk);

			const __m128i  res =
				process_vect_sse2 (src_1, src_2, mask, zero, ffff, mask_sign, offset_s);

			fstb::ToolsSse2::store_8_16ml (&dst_msb_ptr [x], &dst_lsb_ptr [x],res, mask_lsb);
		}

		if (w7 > 0)
		{
			const __m128i  src_1 =
				fstb::ToolsSse2::load_8_16ml (&sr1_msb_ptr [w8], &sr1_lsb_ptr [w8]);
			const __m128i  src_2 =
				fstb::ToolsSse2::load_8_16ml (&sr2_msb_ptr [w8], &sr2_lsb_ptr [w8]);
			const __m128i  mask  =
				T::get_mask_sse2 (msk_msb_ptr, msk_lsb_ptr, w8, tdg._stride_msk);

			const __m128i  res =
				process_vect_sse2 (src_1, src_2, mask, zero, ffff, mask_sign, offset_s);

			fstb::ToolsSse2::store_8_16ml_partial (
				&dst_msb_ptr [w8], &dst_lsb_ptr [w8], res, mask_lsb, w7
			);
		}

		dst_msb_ptr += tdg._stride_dst;
		dst_lsb_ptr += tdg._stride_dst;
		sr1_msb_ptr += tdg._stride_sr1;
		sr1_lsb_ptr += tdg._stride_sr1;
		sr2_msb_ptr += tdg._stride_sr2;
		sr2_lsb_ptr += tdg._stride_sr2;
		msk_msb_ptr += stride_msk;
		msk_lsb_ptr += stride_msk;
	}
}



__forceinline __m128i	AvsFilterMerge16::process_vect_sse2 (const __m128i &src_1, const __m128i &src_2, const __m128i &mask, const __m128i &zero, const __m128i &ffff, const __m128i &mask_sign, const __m128i &offset_s)
{
	const __m128i  src_1_03 = _mm_unpacklo_epi16 (src_1, zero);
	const __m128i  src_1_47 = _mm_unpackhi_epi16 (src_1, zero);
	const __m128i  src_2_03 = _mm_unpacklo_epi16 (src_2, zero);
	const __m128i  src_2_47 = _mm_unpackhi_epi16 (src_2, zero);
	const __m128i  diff_03  = _mm_sub_epi32 (src_2_03, src_1_03);
	const __m128i  diff_47  = _mm_sub_epi32 (src_2_47, src_1_47);

	const __m128i  mask1    = _mm_srli_epi16 (mask, 1);
	const __m128i  mask1_03 = _mm_unpacklo_epi16 (mask1, zero);
	const __m128i  mask1_47 = _mm_unpackhi_epi16 (mask1, zero);

	__m128i        lerp_03  = fstb::ToolsSse2::mullo_epi32 (diff_03, mask1_03);
	__m128i        lerp_47  = fstb::ToolsSse2::mullo_epi32 (diff_47, mask1_47);
	lerp_03                 = _mm_srai_epi32 (lerp_03, 15);
	lerp_47                 = _mm_srai_epi32 (lerp_47, 15);
	lerp_03                 = _mm_sub_epi32 (lerp_03, offset_s);
	lerp_47                 = _mm_sub_epi32 (lerp_47, offset_s);
	const __m128i  res_03   = _mm_add_epi32 (lerp_03, src_1_03);
	const __m128i  res_47   = _mm_add_epi32 (lerp_47, src_1_47);
	__m128i        res      = _mm_packs_epi32 (res_03, res_47);
	res                     = _mm_xor_si128 (res, mask_sign);

	const __m128i  cond     = _mm_cmpeq_epi16 (mask, ffff);
	res                     = fstb::ToolsSse2::select (cond, src_2, res);

	return (res);
}



int	AvsFilterMerge16::Mask444::get_mask_cpp (const uint8_t *msk_msb_ptr, const uint8_t *msk_lsb_ptr, int x, int /*stride_msk*/)
{
	return ((msk_msb_ptr [x] << 8) + msk_lsb_ptr [x]);
}



__m128i	AvsFilterMerge16::Mask444::get_mask_sse2 (const uint8_t *msk_msb_ptr, const uint8_t *msk_lsb_ptr, int x, int /*stride_msk*/)
{
	return (fstb::ToolsSse2::load_8_16ml (&msk_msb_ptr [x], &msk_lsb_ptr [x]));
}



int	AvsFilterMerge16::Mask420::get_mask_cpp (const uint8_t *msk_msb_ptr, const uint8_t *msk_lsb_ptr, int x, int stride_msk)
{
	const int      x2 = x << 1;
	const int      msb =
		  msk_msb_ptr [x2             ] + msk_msb_ptr [x2 + 1             ]
		+ msk_msb_ptr [x2 + stride_msk] + msk_msb_ptr [x2 + 1 + stride_msk];
	const int      lsb =
		  msk_lsb_ptr [x2]              + msk_lsb_ptr [x2 + 1             ]
		+ msk_lsb_ptr [x2 + stride_msk] + msk_lsb_ptr [x2 + 1 + stride_msk];

	return ((msb << 6) + ((lsb + 2) >> 2));
}



__m128i	AvsFilterMerge16::Mask420::get_mask_sse2 (const uint8_t *msk_msb_ptr, const uint8_t *msk_lsb_ptr, int x, int stride_msk)
{
	const int      x2  = x << 1;
	const __m128i  m00 = fstb::ToolsSse2::load_8_16ml (&msk_msb_ptr [x2                 ], &msk_lsb_ptr [x2                 ]);
	const __m128i  m08 = fstb::ToolsSse2::load_8_16ml (&msk_msb_ptr [x2              + 8], &msk_lsb_ptr [x2              + 8]);
	const __m128i  m10 = fstb::ToolsSse2::load_8_16ml (&msk_msb_ptr [x2 + stride_msk    ], &msk_lsb_ptr [x2 + stride_msk    ]);
	const __m128i  m18 = fstb::ToolsSse2::load_8_16ml (&msk_msb_ptr [x2 + stride_msk + 8], &msk_lsb_ptr [x2 + stride_msk + 8]);

	__m128i        m0  = _mm_avg_epu16 (m00, m10);
	__m128i	      m8  = _mm_avg_epu16 (m08, m18);
	const __m128i  m0s = _mm_srli_si128 (m0, 2);
	const __m128i  m8s = _mm_srli_si128 (m8, 2);
	m0                 = _mm_avg_epu16 (m0, m0s);
	m8                 = _mm_avg_epu16 (m8, m8s);
	const __m128i  m   = fstb::ToolsSse2::pack_epi16 (m0, m8);

	return (m);
}



int	AvsFilterMerge16::Mask422::get_mask_cpp (const uint8_t *msk_msb_ptr, const uint8_t *msk_lsb_ptr, int x, int /*stride_msk*/)
{
	const int      x2  = x << 1;
	const int      msb = msk_msb_ptr [x2] + msk_msb_ptr [x2 + 1];
	const int      lsb = msk_lsb_ptr [x2] + msk_lsb_ptr [x2 + 1];

	return ((msb << 7) + ((lsb + 1) >> 1));
}



__m128i	AvsFilterMerge16::Mask422::get_mask_sse2 (const uint8_t *msk_msb_ptr, const uint8_t *msk_lsb_ptr, int x, int /*stride_msk*/)
{
	const int      x2  = x << 1;
	__m128i        m0  = fstb::ToolsSse2::load_8_16ml (&msk_msb_ptr [x2    ], &msk_lsb_ptr [x2    ]);
	__m128i        m8  = fstb::ToolsSse2::load_8_16ml (&msk_msb_ptr [x2 + 8], &msk_lsb_ptr [x2 + 8]);

	const __m128i  m0s = _mm_srli_si128 (m0, 2);
	const __m128i  m8s = _mm_srli_si128 (m8, 2);
	m0                 = _mm_avg_epu16 (m0, m0s);
	m8                 = _mm_avg_epu16 (m8, m8s);
	const __m128i  m   = fstb::ToolsSse2::pack_epi16 (m0, m8);

	return (m);
}



int	AvsFilterMerge16::Mask411::get_mask_cpp (const uint8_t *msk_msb_ptr, const uint8_t *msk_lsb_ptr, int x, int /*stride_msk*/)
{
	const int      x4  = x << 2;
	const int      msb =
		  msk_msb_ptr [x4    ]
		+ msk_msb_ptr [x4 + 1]
		+ msk_msb_ptr [x4 + 2]
		+ msk_msb_ptr [x4 + 3];
	const int      lsb =
		  msk_lsb_ptr [x4    ]
		+ msk_lsb_ptr [x4 + 1]
		+ msk_lsb_ptr [x4 + 2]
		+ msk_lsb_ptr [x4 + 3];

	return ((msb << 6) + ((lsb + 2) >> 2));
}



__m128i	AvsFilterMerge16::Mask411::get_mask_sse2 (const uint8_t *msk_msb_ptr, const uint8_t *msk_lsb_ptr, int x, int /*stride_msk*/)
{
	const int      x4   = x << 2;
	__m128i        m00  = fstb::ToolsSse2::load_8_16ml (&msk_msb_ptr [x4     ], &msk_lsb_ptr [x4     ]);
	__m128i        m08  = fstb::ToolsSse2::load_8_16ml (&msk_msb_ptr [x4 +  8], &msk_lsb_ptr [x4 +  8]);
	__m128i        m16  = fstb::ToolsSse2::load_8_16ml (&msk_msb_ptr [x4 + 16], &msk_lsb_ptr [x4 + 16]);
	__m128i        m24  = fstb::ToolsSse2::load_8_16ml (&msk_msb_ptr [x4 + 24], &msk_lsb_ptr [x4 + 24]);

	__m128i        m00s = _mm_srli_si128 (m00, 2);
	const __m128i  m08s = _mm_srli_si128 (m08, 2);
	__m128i        m16s = _mm_srli_si128 (m16, 2);
	const __m128i  m24s = _mm_srli_si128 (m24, 2);
	m00                 = _mm_avg_epu16 (m00, m00s);
	m08                 = _mm_avg_epu16 (m08, m08s);
	m16                 = _mm_avg_epu16 (m16, m16s);
	m24                 = _mm_avg_epu16 (m24, m24s);

	m00                 = fstb::ToolsSse2::pack_epi16 (m00, m08);
	m16                 = fstb::ToolsSse2::pack_epi16 (m16, m24);

	m00s                = _mm_srli_si128 (m00, 2);
	m16s                = _mm_srli_si128 (m16, 2);
	m00                 = _mm_avg_epu16 (m00, m00s);
	m16                 = _mm_avg_epu16 (m16, m16s);
	const __m128i  m    = fstb::ToolsSse2::pack_epi16 (m00, m16);

	return (m);
}



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
