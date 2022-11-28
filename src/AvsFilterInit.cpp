/*****************************************************************************

        AvsFilterInit.cpp
        Author: Laurent de Soras, 2010-2013

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

#define	WIN32_LEAN_AND_MEAN
#define	NOMINMAX
#include <windows.h>
#include	"avisynth.h"

#include	"AvsFilterAdd16.h"
#include	"AvsFilterBilateral16.h"
#include	"AvsFilterBoxFilter16.h"
#include	"AvsFilterDitherPost.h"
#include	"AvsFilterLimitDif16.h"
#include	"AvsFilterMaxDif16.h"
#include	"AvsFilterMedian16.h"
#include	"AvsFilterMerge16.h"
#include	"AvsFilterOut.h"
#include	"AvsFilterRemoveGrain16.h"
#include	"AvsFilterRepair16.h"
#include	"AvsFilterResize16.h"
#include	"AvsFilterSmoothGrad.h"
#include	"PlaneProcMode.h"

#if defined (_MSC_VER) && ! defined (NDEBUG) && defined (_DEBUG)
	#include	<crtdbg.h>
#endif



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



// This is the function that created the filter, when the filter has been called.
// This can be used for simple parameter checking, so it is possible to create different filters,
// based on the arguments recieved.
::AVSValue __cdecl	AvsFilterInit_create_dither_post (::AVSValue args, void * /*user_data_ptr*/, ::IScriptEnvironment *env_ptr)
{
	return (new AvsFilterDitherPost (
		env_ptr,
		args [ 0].AsClip (),                          // src
		args [ 1].IsClip () ? args [1].AsClip () : 0, // clsb
		args [ 2].AsInt (0),                          // mode
		args [ 3].AsFloat (1),                        // ampo
		args [ 4].AsFloat (0),                        // ampn
		args [ 5].AsInt (1),                          // pat
		args [ 6].AsBool (false),                     // dyn
		args [ 7].AsBool (false),                     // prot
		args [ 8].IsClip () ? args [8].AsClip () : 0, // mask
		args [ 9].AsFloat (-1),                       // thr
		args [10].AsBool (true),                      // stacked
		args [11].AsBool (false),                     // interlaced
		args [12].AsInt (PlaneProcMode_PROCESS),      // y
		args [13].AsInt (PlaneProcMode_PROCESS),      // u
		args [14].AsInt (PlaneProcMode_PROCESS),      // v
		args [15].AsBool (false),                     // staticnoise
		args [16].AsBool (true)                       // slice
	));  
}



::AVSValue __cdecl	AvsFilterInit_create_smooth_grad (::AVSValue args, void * /*user_data_ptr*/, ::IScriptEnvironment *env_ptr)
{
	return (new AvsFilterSmoothGrad (
		env_ptr,
		args [0].AsClip (),                          // src
		args [1].IsClip () ? args [1].AsClip () : 0, // clsb
		args [2].AsInt (16),                         // radius
		args [3].AsFloat (0.25),                     // thr
		args [4].AsBool (true),                      // stacked
		args [5].IsClip () ? args [5].AsClip () : 0, // ref
		args [6].AsFloat (3),                        // elast
		args [7].AsInt (PlaneProcMode_PROCESS),      // y
		args [8].AsInt (PlaneProcMode_PROCESS),      // u
		args [9].AsInt (PlaneProcMode_PROCESS)       // v
	));  
}



::AVSValue __cdecl	AvsFilterInit_create_add16 (::AVSValue args, void * /*user_data_ptr*/, ::IScriptEnvironment *env_ptr)
{
	return (new AvsFilterAdd16 (
		env_ptr,
		args [0].AsClip (),                     // src
		args [1].AsClip (),                     // src2
		args [2].AsBool (false),                // wrap
		args [3].AsInt (PlaneProcMode_PROCESS), // y
		args [4].AsInt (PlaneProcMode_PROCESS), // u
		args [5].AsInt (PlaneProcMode_PROCESS), // v
		args [6].AsBool (false),                // dif
		false
	));  
}



::AVSValue __cdecl	AvsFilterInit_create_sub16 (::AVSValue args, void * /*user_data_ptr*/, ::IScriptEnvironment *env_ptr)
{
	return (new AvsFilterAdd16 (
		env_ptr,
		args [0].AsClip (),                     // src
		args [1].AsClip (),                     // src2
		args [2].AsBool (false),                // wrap
		args [3].AsInt (PlaneProcMode_PROCESS), // y
		args [4].AsInt (PlaneProcMode_PROCESS), // u
		args [5].AsInt (PlaneProcMode_PROCESS), // v
		args [6].AsBool (false),                // dif
		true
	));  
}



::AVSValue __cdecl	AvsFilterInit_create_merge16 (::AVSValue args, void * /*user_data_ptr*/, ::IScriptEnvironment *env_ptr)
{
	const bool     luma_flag = args [3].AsBool (false);
	const PlaneProcMode  def_mode_chroma =
		  (luma_flag)
		? PlaneProcMode_PROCESS
		: PlaneProcMode_COPY1;

	return (new AvsFilterMerge16 (
		env_ptr,
		args [0].AsClip (),                     // src
		args [1].AsClip (),                     // src2
		args [2].AsClip (),                     // mask
		luma_flag,                              // luma
		args [4].AsInt (PlaneProcMode_PROCESS), // y
		args [5].AsInt (def_mode_chroma),       // u
		args [6].AsInt (def_mode_chroma)        // v
	));  
}



::AVSValue __cdecl	AvsFilterInit_create_limit_dif16 (::AVSValue args, void * /*user_data_ptr*/, ::IScriptEnvironment *env_ptr)
{
	return (new AvsFilterLimitDif16 (
		env_ptr,
		args [0].AsClip (),                          // filtered
		args [1].AsClip (),                          // src
		args [2].IsClip () ? args [2].AsClip () : 0,	// ref
		args [3].AsFloat (0.25),                     // thr
		args [4].AsFloat (3),                        // elast
		args [5].AsInt (PlaneProcMode_PROCESS),      // y
		args [6].AsInt (PlaneProcMode_PROCESS),      // u
		args [7].AsInt (PlaneProcMode_PROCESS),      // v
		args [8].AsBool (false)                      // refabsdif
	));  
}



::AVSValue __cdecl	AvsFilterInit_create_max_dif16 (::AVSValue args, void * /*user_data_ptr*/, ::IScriptEnvironment *env_ptr)
{
	return (new AvsFilterMaxDif16 (
		env_ptr,
		args [0].AsClip (),                     // src1
		args [1].AsClip (),                     // src2
		args [2].AsClip (),                     // ref
		args [3].AsInt (PlaneProcMode_PROCESS), // y
		args [4].AsInt (PlaneProcMode_PROCESS), // u
		args [5].AsInt (PlaneProcMode_PROCESS), // v
		false                                   // MaxDif
	));  
}



::AVSValue __cdecl	AvsFilterInit_create_min_dif16 (::AVSValue args, void * /*user_data_ptr*/, ::IScriptEnvironment *env_ptr)
{
	return (new AvsFilterMaxDif16 (
		env_ptr,
		args [0].AsClip (),                     // src1
		args [1].AsClip (),                     // src2
		args [2].AsClip (),                     // ref
		args [3].AsInt (PlaneProcMode_PROCESS), // y
		args [4].AsInt (PlaneProcMode_PROCESS), // u
		args [5].AsInt (PlaneProcMode_PROCESS), // v
		true                                    // MinDif
	));  
}



::AVSValue __cdecl	AvsFilterInit_create_box_filter16 (::AVSValue args, void * /*user_data_ptr*/, ::IScriptEnvironment *env_ptr)
{
	return (new AvsFilterBoxFilter16 (
		env_ptr,
		args [0].AsClip (),                     // src
		args [1].AsInt (16),                    // radius
		args [2].AsInt (PlaneProcMode_PROCESS), // y
		args [3].AsInt (PlaneProcMode_PROCESS), // u
		args [4].AsInt (PlaneProcMode_PROCESS)  // v
	));  
}



::AVSValue __cdecl	AvsFilterInit_create_bilateral16 (::AVSValue args, void * /*user_data_ptr*/, ::IScriptEnvironment *env_ptr)
{
	return (new AvsFilterBilateral16 (
		env_ptr,
		args [0].AsClip (),                          // src
		args [1].IsClip () ? args [1].AsClip () : 0, // ref
		args [2].AsInt (16),                         // radius
		args [3].AsFloat (2.5),                      // thr
		args [4].AsFloat (0.4),                      // flat
		args [5].AsFloat (0.0),                      // wmin
		args [6].AsFloat (0.0),                      // subspl
		args [7].AsInt (PlaneProcMode_PROCESS),      // y
		args [8].AsInt (PlaneProcMode_PROCESS),      // u
		args [9].AsInt (PlaneProcMode_PROCESS)       // v
	));  
}



static AvsFilterResize16::OptBool	AvsFilterInit_opt_bool (const ::AVSValue &arg)
{
	return (  ! arg.Defined ()     ? AvsFilterResize16::OptBool_UNDEF
	        :   arg.AsBool (false) ? AvsFilterResize16::OptBool_TRUE
			  :                        AvsFilterResize16::OptBool_FALSE);
}

::AVSValue __cdecl	AvsFilterInit_create_resize16 (::AVSValue args, void * /*user_data_ptr*/, ::IScriptEnvironment *env_ptr)
{
	std::string    cplace = args [17].AsString ("MPEG2");

	return (new AvsFilterResize16 (
		env_ptr,
		args [0].AsClip (),                      // src
		args [1].AsInt (),                       // target_width
		args [2].AsInt (),                       // target_height
		args [3].AsFloat (0.0),                  // src_left
		args [4].AsFloat (0.0),                  // src_top
		args [5].AsFloat (0.0),                  // src_width
		args [6].AsFloat (0.0),                  // src_height
		args [7].AsString ("spline36"),          // kernel
		args [8].AsFloat (1.0),                  // fh
		args [9].AsFloat (1.0),                  // fv
		args [10].AsInt (4),                     // taps
		args [11].Defined (),                    // a1
		args [11].AsFloat (0),
		args [12].Defined (),                    // a2
		args [12].AsFloat (0),
		args [13].Defined (),                    // a3
		args [13].AsFloat (0),
		args [14].AsInt (0),                     // kovrspl
		args [15].AsBool (true),                 // cnorm
		args [16].AsBool (true),                 // center
		args [18].AsInt (PlaneProcMode_PROCESS), // y
		args [19].AsInt (PlaneProcMode_PROCESS), // u
		args [20].AsInt (PlaneProcMode_PROCESS), // v
		args [21].AsString (""),                 // kernelh
		args [22].AsString (""),                 // kernelv
		args [23].AsFloat (0.0),                 // totalh
		args [24].AsFloat (0.0),                 // totalv
		args [25].AsBool (false),                // invks
		AvsFilterInit_opt_bool (args [26]),      // invksh
		AvsFilterInit_opt_bool (args [27]),      // invksv
		args [28].AsInt (5),                     // invkstaps
		args [29].AsString (cplace.c_str ()),    // cplaces
		args [30].AsString (cplace.c_str ()),    // cplaced
		args [31].AsString ("")                  // csp
	));  
}



::AVSValue __cdecl	AvsFilterInit_create_removegrain16 (::AVSValue args, void * /*user_data_ptr*/, ::IScriptEnvironment *env_ptr)
{
	const ::PClip	src   = args [0].AsClip ();     // src
	const int		mode  = args [1].AsInt (2);     // mode
	const int		modeu = args [2].AsInt (mode);  // modeU
	const int		modev = args [3].AsInt (modeu); // modeV

	return (new AvsFilterRemoveGrain16 (env_ptr, src, mode, modeu, modev));  
}



::AVSValue __cdecl	AvsFilterInit_create_repair16 (::AVSValue args, void * /*user_data_ptr*/, ::IScriptEnvironment *env_ptr)
{
	const ::PClip	src1  = args [0].AsClip ();     // src1
	const ::PClip	src2  = args [1].AsClip ();     // src2
	const int		mode  = args [2].AsInt (2);     // mode
	const int		modeu = args [3].AsInt (mode);  // modeU
	const int		modev = args [4].AsInt (modeu); // modeV

	return (new AvsFilterRepair16 (env_ptr, src1, src2, mode, modeu, modev));
}



::AVSValue __cdecl	AvsFilterInit_create_median16 (::AVSValue args, void * /*user_data_ptr*/, ::IScriptEnvironment *env_ptr)
{
	return (new AvsFilterMedian16 (
		env_ptr,
		args [0].AsClip (),                     // src
		args [1].AsInt (1),                     // rx
		args [2].AsInt (1),                     // ry
		args [3].AsInt (0),                     // rt
		args [4].AsInt (-1),                    // ql
		args [5].AsInt (-1),                    // qh
		args [6].AsInt (PlaneProcMode_PROCESS), // y
		args [7].AsInt (PlaneProcMode_PROCESS), // u
		args [8].AsInt (PlaneProcMode_PROCESS)  // v
	));  
}



::AVSValue __cdecl	AvsFilterInit_create_out (::AVSValue args, void * /*user_data_ptr*/, ::IScriptEnvironment *env_ptr)
{
	return (new AvsFilterOut (
		env_ptr,
		args [0].AsClip (),                     // src
		args [1].AsBool (false)                 // bigendian
	));  
}



// The following function is the function that actually registers the filter in AviSynth
// It is called automatically, when the plugin is loaded to see which functions this filter contains.
extern "C" __declspec (dllexport) const char * __stdcall	AvisynthPluginInit2 (::IScriptEnvironment *env_ptr)
{
	// The AddFunction has the following paramters:
	// AddFunction(Filtername , Arguments, Function to call,0);

	// Arguments is a string that defines the types and optional names of the arguments for your filter.
	// c - Video Clip
	// i - Integer number
	// f - Float number
	// s - String
	// b - boolean

	// The word inside the [ ] lets you used named parameters in your script
	// e.g last=SimpleSample(last,windowclip,size=100).
	// but last=SimpleSample(last,windowclip, 100) will also work 
	env_ptr->AddFunction (
		"DitherPost",
		"c[clsb]c[mode]i[ampo]f[ampn]f[pat]i[dyn]b[prot]b"                //  0- 7
		"[mask]c[thr]f[stacked]b[interlaced]b[y]i[u]i[v]i[staticnoise]b"  //  8-15
		"[slice]b",                                                       // 16-16
		AvsFilterInit_create_dither_post,
		0
	);
	env_ptr->AddFunction (
		"SmoothGrad",
		"c[clsb]c[radius]i[thr]f[stacked]b[ref]c[elast]f[y]i[u]i[v]i",
		AvsFilterInit_create_smooth_grad,
		0
	);
	env_ptr->AddFunction (
		"Dither_box_filter16",
		"c[radius]i[y]i[u]i[v]i",
		AvsFilterInit_create_box_filter16,
		0
	);
	env_ptr->AddFunction (
		"Dither_bilateral16",
		"c[ref]c[radius]i[thr]f[flat]f[wmin]f[subspl]f[y]i[u]i[v]i",
		AvsFilterInit_create_bilateral16,
		0
	);
	env_ptr->AddFunction (
		"Dither_limit_dif16",
		"cc[ref]c[thr]f[elast]f[y]i[u]i[v]i[refabsdif]b",
		AvsFilterInit_create_limit_dif16,
		0
	);
	env_ptr->AddFunction (
		"Dither_max_dif16",
		"ccc[y]i[u]i[v]i",
		AvsFilterInit_create_max_dif16,
		0
	);
	env_ptr->AddFunction (
		"Dither_min_dif16",
		"ccc[y]i[u]i[v]i",
		AvsFilterInit_create_min_dif16,
		0
	);
	env_ptr->AddFunction (
		"Dither_add16",
		"cc[wrap]b[y]i[u]i[v]i[dif]b",
		AvsFilterInit_create_add16,
		0
	);
	env_ptr->AddFunction (
		"Dither_sub16",
		"cc[wrap]b[y]i[u]i[v]i[dif]b",
		AvsFilterInit_create_sub16,
		0
	);
	env_ptr->AddFunction (
		"Dither_merge16",
		"ccc[luma]b[y]i[u]i[v]i",
		AvsFilterInit_create_merge16,
		0
	);
	env_ptr->AddFunction (
		"Dither_resize16",
		"cii[src_left]f[src_top]f[src_width]f[src_height]f[kernel]s"                 //  0- 7
		"[fh]f[fv]f[taps]i[a1]f[a2]f[a3]f[kovrspl]i[cnorm]b"                         //  8-15
		"[center]b[cplace]s[y]i[u]i[v]i[kernelh]s[kernelv]s[totalh]f"                // 16-23
		"[totalv]f[invks]b[invksh]b[invksv]b[invkstaps]i[cplaces]s[cplaced]s[csp]s", // 24-31
		AvsFilterInit_create_resize16,
		0
	);
	env_ptr->AddFunction (
		"Dither_removegrain16",
		"c[mode]i[modeU]i[modeV]i",
		AvsFilterInit_create_removegrain16,
		0
	);
	env_ptr->AddFunction (
		"Dither_repair16",
		"cc[mode]i[modeU]i[modeV]i",
		AvsFilterInit_create_repair16,
		0
	);
	env_ptr->AddFunction (
		"Dither_median16",
		"c[rx]i[ry]i[rt]i[ql]i[qh]i[y]i[u]i[v]i",
		AvsFilterInit_create_median16,
		0
	);
	env_ptr->AddFunction (
		"Dither_out",
		"c[bigendian]b",
		AvsFilterInit_create_out,
		0
	);

	// A freeform name of the plugin.
	return ("Dither");
}



static void	AvsFilterInit_dll_load (::HINSTANCE /*hinst*/)
{
#if defined (_MSC_VER) && ! defined (NDEBUG) && defined (_DEBUG)
	{
		const int	mode =   (1 * _CRTDBG_MODE_DEBUG)
						       | (1 * _CRTDBG_MODE_WNDW);
		::_CrtSetReportMode (_CRT_WARN, mode);
		::_CrtSetReportMode (_CRT_ERROR, mode);
		::_CrtSetReportMode (_CRT_ASSERT, mode);

		const int	old_flags = ::_CrtSetDbgFlag (_CRTDBG_REPORT_FLAG);
		::_CrtSetDbgFlag (  old_flags
		                  | (1 * _CRTDBG_LEAK_CHECK_DF)
		                  | (0 * _CRTDBG_CHECK_ALWAYS_DF));
		::_CrtSetBreakAlloc (-1);	// Specify here a memory bloc number
	}
#endif	// _MSC_VER, NDEBUG
}



static void	AvsFilterInit_dll_unload (::HINSTANCE /*hinst*/)
{
#if defined (_MSC_VER) && ! defined (NDEBUG) && defined (_DEBUG)
	{
		const int	mode =   (1 * _CRTDBG_MODE_DEBUG)
						       | (0 * _CRTDBG_MODE_WNDW);
		::_CrtSetReportMode (_CRT_WARN, mode);
		::_CrtSetReportMode (_CRT_ERROR, mode);
		::_CrtSetReportMode (_CRT_ASSERT, mode);

		::_CrtMemState	mem_state;
		::_CrtMemCheckpoint (&mem_state);
		::_CrtMemDumpStatistics (&mem_state);
	}
#endif	// _MSC_VER, NDEBUG
}



BOOL WINAPI DllMain (::HINSTANCE hinst, ::DWORD reason, ::LPVOID /*reserved_ptr*/)
{
	switch (reason)
	{
	case	DLL_PROCESS_ATTACH:
		AvsFilterInit_dll_load (hinst);
		break;

	case	DLL_PROCESS_DETACH:
		AvsFilterInit_dll_unload (hinst);
		break;
	}

	return (TRUE);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
