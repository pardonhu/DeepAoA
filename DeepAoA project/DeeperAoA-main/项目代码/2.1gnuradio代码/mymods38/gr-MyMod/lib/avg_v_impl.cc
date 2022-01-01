/* -*- c++ -*- */
/* 
 * Copyright 2019 Dexter (zhuyf95@sjtu.edu.cn).
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "avg_v_impl.h"
#include <cmath>
#include "volk/volk.h"
#include <gnuradio/math.h>

namespace gr {
  namespace MyMod {

    avg_v::sptr
    avg_v::make(int vlen, int acc_cnt)
    {
      return gnuradio::get_initial_sptr
        (new avg_v_impl(vlen, acc_cnt));
    }

    /*
     * The private constructor
     */
    avg_v_impl::avg_v_impl(int vlen, int acc_cnt)
      : gr::block("avg_v",
              gr::io_signature::make(1, 1, vlen*sizeof(gr_complex)),
              gr::io_signature::make(1, 1, vlen*sizeof(gr_complex))),
              d_vlen(vlen),
              d_len(acc_cnt),
              d_cnt(0)
    {
      d_buf = new gr_complex*[d_vlen];
      d_avg = new gr_complex[d_vlen];
      for(int i = 0; i < d_vlen; ++i){
        d_buf[i] = new gr_complex[d_len];
        d_avg[i] = gr_complex(0,0);
      }
      
    }

    /*
     * Our virtual destructor.
     */
    avg_v_impl::~avg_v_impl()
    {
      for(int i = 0; i < d_vlen; ++i){
        delete[] d_buf[i];
      }
      delete[] d_buf;
      delete[] d_avg;
    }

    void
    avg_v_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      ninput_items_required[0] = noutput_items;
    }

    int
    avg_v_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      const gr_complex *in = (const gr_complex *) input_items[0];
      gr_complex *out = (gr_complex *) output_items[0];

      int i = 0;
      int o = 0;
      while(i < ninput_items[0] && o < noutput_items){   
        if(d_cnt < d_len){
          for(int p = 0; p < d_vlen; ++p){
            d_avg[p] += in[p];
          }
        }else{
          for(int p = 0; p < d_vlen; ++p){
            d_avg[p] += in[p];
            d_avg[p] -= d_buf[p][d_cnt%d_len];
          }
        }
        for(int p = 0; p < d_vlen; ++p){
          d_buf[p][d_cnt%d_len] = in[p];
        }
        int n = 0;
        if(d_cnt < d_len)
          n = d_cnt+1;
        else
          n = d_len;
        for(int p = 0; p < d_vlen; ++p){
          out[p] = d_avg[p]/gr_complex(n,0);
        }
        ++d_cnt;
        in += d_vlen;
        out += d_vlen;
        i++;
        o++;
      }
      
      consume_each (i);

      // Tell runtime system how many output items we produced.
      return o;
    }

  } /* namespace MyMod */
} /* namespace gr */

