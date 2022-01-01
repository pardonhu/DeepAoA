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
#include "smooth_pi_impl.h"

namespace gr {
  namespace MyMod {

    smooth_pi::sptr
    smooth_pi::make(int nin, bool en, float offset_threshold, int nlast)
    {
      return gnuradio::get_initial_sptr
        (new smooth_pi_impl(nin, en, offset_threshold, nlast));
    }

    /*
     * The private constructor
     */
    smooth_pi_impl::smooth_pi_impl(int nin, bool en, float offset_threshold, int nlast)
      : gr::block("smooth_pi",
              gr::io_signature::make(nin, nin, sizeof(float)),
              gr::io_signature::make(nin, nin, sizeof(float))),
              d_nin(nin),
              d_en(en),
              d_threshold(offset_threshold),
              sub_cnt(0),
              d_nlast(nlast)
    {
      last = new float*[d_nin];
      for(int i = 0; i < d_nin; ++i)
        last[i] = new float[d_nlast];
    }

    /*
     * Our virtual destructor.
     */
    smooth_pi_impl::~smooth_pi_impl()
    {
      for(int i = 0; i < d_nin; ++i)
        delete[] last[i];
      delete[] last;
    }

    void
    smooth_pi_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      /* <+forecast+> e.g. ninput_items_required[0] = noutput_items */
    }

    int
    smooth_pi_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      const float *in[d_nin];
      float *out[d_nin];

      for(int i = 0; i < d_nin; ++i){
        in[i] = (const float *) input_items[i];
        out[i] = (float *) output_items[i];
      }

      // remove pi jumps
      int ninput = ninput_items[0];
      for (int i = 0; i < d_nin; ++i){
        in[i] = (const float *) input_items[i];
        out[i] = (float *) output_items[i];
        ninput = std::min(ninput, ninput_items[i]);
      }

      int i = 0, o = 0;
      while(i < ninput && o < noutput_items){      
        if(d_en){
          if(sub_cnt < 6 || sub_cnt == 32 || sub_cnt > 58){
            for(int ii = 0; ii < d_nin; ++ii){
              out[ii][o] = in[ii][i];
              if(out[ii][o] != 0){
                std::cout << "smooth_pi error: subcarrier " << sub_cnt << " from antenna " << ii << " is "
                          << out[ii][o] << ", should be 0." << std::endl;
              }
            }
          }else if(sub_cnt == 6){
            if(std::abs(in[0][i]) > 1.8){
              out[0][o] = in[0][i] > 0 ? in[0][i]-M_PI : in[0][i]+M_PI;  
            }else{
              out[0][o] = in[0][i];
            }
            for(int l = 0; l < d_nlast; ++l){
              last[0][l] = out[0][o];
            }
            for(int ii = 1; ii < d_nin; ++ii){
              if(std::abs(in[ii][i]-in[0][i]) > std::abs(M_PI-d_threshold)){
                out[ii][o] = in[ii][i] > in[0][i] ? in[ii][i]-M_PI : in[ii][i]+M_PI;  
              }else{
                out[ii][o] = in[ii][i];
              }
              for(int l = 0; l < d_nlast; ++l){
                last[ii][l] = out[ii][o];
              }
            }
          }else{
            for(int ii = 0; ii < d_nin; ++ii){
              int n = -1;
              for(int l = d_nlast-1; l >= 0; --l){
                if(std::abs(in[ii][i]-last[ii][l]) <= std::abs(M_PI-d_threshold))
                  continue;
                else{
                  n=l;
                  break;
                }
              }
              if(n > -1){
                out[ii][o] = in[ii][i] >= last[ii][n] ? in[ii][i]-M_PI : in[ii][i]+M_PI;
              }else{
                out[ii][o] = in[ii][i];
              }
              for(int l = 0; l < d_nlast-1; ++l){
                last[ii][l] = last[ii][l+1];
              }
              last[ii][d_nlast-1] = out[ii][o];
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
        sub_cnt %= sub_num;
      }
      
      
      // Tell runtime system how many input items we consumed on
      // each input stream.
      consume_each (i);

      // Tell runtime system how many output items we produced.
      return o;
    }

    void 
    smooth_pi_impl::set_en(bool en) {
      d_en = en;
      std::cout << "Set enable to: " << std::boolalpha << d_en << std::noboolalpha << std::endl;
    }

  } /* namespace MyMod */
} /* namespace gr */

