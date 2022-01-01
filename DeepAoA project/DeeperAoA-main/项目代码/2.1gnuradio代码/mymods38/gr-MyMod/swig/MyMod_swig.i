/* -*- c++ -*- */

#define MYMOD_API

%include "gnuradio.i"           // the common stuff

//load generated python docstrings
%include "MyMod_swig_doc.i"

%{
#include "MyMod/avg_ff.h"
#include "MyMod/avg_v.h"
#include "MyMod/avg_v_ff.h"
#include "MyMod/cal_csi.h"
#include "MyMod/cal_multi_usrp.h"
#include "MyMod/cor_inf.h"
#include "MyMod/cor_pac_aoa.h"
#include "MyMod/cp2offs.h"
#include "MyMod/extract_lts.h"
#include "MyMod/lts_cor.h"
#include "MyMod/msg2cv.h"
#include "MyMod/smooth_pi.h"
#include "MyMod/unwrap.h"
#include "MyMod/wifi_samples_align.h"
%}

%include "MyMod/avg_ff.h"
GR_SWIG_BLOCK_MAGIC2(MyMod, avg_ff);
%include "MyMod/avg_v.h"
GR_SWIG_BLOCK_MAGIC2(MyMod, avg_v);
%include "MyMod/avg_v_ff.h"
GR_SWIG_BLOCK_MAGIC2(MyMod, avg_v_ff);
%include "MyMod/cal_csi.h"
GR_SWIG_BLOCK_MAGIC2(MyMod, cal_csi);
%include "MyMod/cal_multi_usrp.h"
GR_SWIG_BLOCK_MAGIC2(MyMod, cal_multi_usrp);
%include "MyMod/cor_inf.h"
GR_SWIG_BLOCK_MAGIC2(MyMod, cor_inf);
%include "MyMod/cor_pac_aoa.h"
GR_SWIG_BLOCK_MAGIC2(MyMod, cor_pac_aoa);
%include "MyMod/cp2offs.h"
GR_SWIG_BLOCK_MAGIC2(MyMod, cp2offs);
%include "MyMod/extract_lts.h"
GR_SWIG_BLOCK_MAGIC2(MyMod, extract_lts);
%include "MyMod/lts_cor.h"
GR_SWIG_BLOCK_MAGIC2(MyMod, lts_cor);
%include "MyMod/msg2cv.h"
GR_SWIG_BLOCK_MAGIC2(MyMod, msg2cv);
%include "MyMod/smooth_pi.h"
GR_SWIG_BLOCK_MAGIC2(MyMod, smooth_pi);
%include "MyMod/unwrap.h"
GR_SWIG_BLOCK_MAGIC2(MyMod, unwrap);
%include "MyMod/wifi_samples_align.h"
GR_SWIG_BLOCK_MAGIC2(MyMod, wifi_samples_align);
