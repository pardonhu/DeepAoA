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
#include "unwrap_impl.h"

namespace gr {
  namespace MyMod {

    unwrap::sptr
    unwrap::make(int nin, double tol, bool en)
    {
      return gnuradio::get_initial_sptr
        (new unwrap_impl(nin, tol, en));
    }

    /*
     * The private constructor
     */
    unwrap_impl::unwrap_impl(int nin, double tol, bool en)
      : gr::block("unwrap",
              gr::io_signature::make(nin, nin, sizeof(float)),
              gr::io_signature::make(nin, nin, sizeof(float))),
              d_nin(nin),
              sub_cnt(0),
              d_tol(tol),
              d_en(en)
    {
      last = new float[d_nin];
      for(int i = 0; i < d_nin; ++i)
        last[i] = 0;
    }

    /*
     * Our virtual destructor.
     */
    unwrap_impl::~unwrap_impl()
    {
      delete[] last;
    }

    void
    unwrap_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      for(int i = 0; i < d_nin; ++i)
        ninput_items_required[i] = noutput_items;
    }

    int
    unwrap_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      const float *in[d_nin];
      float *out[d_nin];
      int ninput = ninput_items[0];
      for (int i = 0; i < d_nin; ++i){
        in[i] = (const float *) input_items[i];
        out[i] = (float *) output_items[i];
        ninput = std::min(ninput, ninput_items[i]);
      }

      int i = 0, o = 0;
      while(i < ninput && o < noutput_items){      
        if(d_en){
          if(sub_cnt == 0){
            for(int ii = 0; ii < d_nin; ++ii){
              last[ii] = in[ii][i];
              out[ii][o] = in[ii][i];
            }
          }else{
            for(int ii = 0; ii < d_nin; ++ii){
              if(std::abs(in[ii][i]-last[ii]) > d_tol){
                int d = std::abs(in[ii][i]-last[ii])/2/M_PI;
                out[ii][o] = in[ii][i] >= last[ii] ? in[ii][i]-(d+1)*2*M_PI : in[ii][i]+(d+1)*2*M_PI;
              }else{
                out[ii][o] = in[ii][i];
              }
              last[ii] = out[ii][o];
              // std::cout << last[ii] << std::endl;
            }
          }
        }else{
          for(int ii = 0; ii < d_nin; ++ii)
            out[ii][o] = in[ii][i];
        }
        // std::cout << out[0][o] << std::endl;
        // std::cout << sub_cnt << std::endl;
        // std::cout << "i: " << i << ", o: " << o << std::endl;
        ++i;
        ++o;
        ++sub_cnt;
        sub_cnt %= 64;
      }
      
      
      // Tell runtime system how many input items we consumed on
      // each input stream.
      consume_each (i);

      // Tell runtime system how many output items we produced.
      return o;
    }

    void 
    unwrap_impl::set_en(bool en){
      d_en = en;
      std::cout << "Set enable to: " << std::boolalpha << d_en << std::noboolalpha << std::endl;
    }

  } /* namespace MyMod */
} /* namespace gr */

