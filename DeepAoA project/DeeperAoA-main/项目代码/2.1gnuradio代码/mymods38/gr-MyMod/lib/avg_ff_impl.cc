/* -*- c++ -*- */
/* 
 * Copyright 2018 Dexter(zhuyf95@sjtu.edu.cn).
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
#include "avg_ff_impl.h"

namespace gr {
  namespace MyMod {

    avg_ff::sptr
    avg_ff::make(unsigned int num_samples, unsigned int snapshot_size, unsigned int nonoverlap, unsigned int method)
    {
      return gnuradio::get_initial_sptr
        (new avg_ff_impl(num_samples, snapshot_size, nonoverlap, method));
    }

    /*
     * The private constructor
     */
    avg_ff_impl::avg_ff_impl(unsigned int num_samples, unsigned int snapshot_size, unsigned int nonoverlap, unsigned int method)
      : gr::block("avg_ff",
              gr::io_signature::make(1, 1, sizeof(float)),
              gr::io_signature::make(1, 1, sizeof(float))),
      d_num_samples(num_samples),
      d_snap(snapshot_size),
      d_nonoverlap(nonoverlap){
        d_sum = 0;
        d_count = 0;
        d_num_aoa = (std::floor((num_samples-snapshot_size)/nonoverlap)+1);
        set_tag_propagation_policy(block::TPP_DONT);
      }

    /*
     * Our virtual destructor.
     */
    avg_ff_impl::~avg_ff_impl()
    {
    }

    void
    avg_ff_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      ninput_items_required[0] = noutput_items*d_num_aoa;
    }

    int
    avg_ff_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    { 
      //debug 
      // std::cout << "Num of AoA: " << d_num_aoa << std::endl;
      const float *in = (const float *) input_items[0];
      float *out = (float *) output_items[0];

      std::cout << "avg_ff ninput: " << ninput_items[0] << std::endl;
      std::cout << "avg_ff noutput: " << noutput_items << std::endl;
      int ninput = ninput_items[0];
      int nread = nitems_read(0);
      
      get_tags_in_window(d_tags, 0, 0, ninput, pmt::string_to_symbol("num_samples_packet"));
      int offset = d_tags[0].offset - nread;
      int count = 0;
      if(d_tags.size() > 0){
        if((d_count == 0) && (d_sum == 0)){
          d_num_samples = pmt::to_double(d_tags[0].value);
          d_num_aoa = (std::floor((d_num_samples-d_snap)/d_nonoverlap)+1);
          std::cout << "avg_ff set d_num_aoa: " << d_num_aoa << std::endl;
          std::cout << "avg_ff tag offset: " << d_tags[0].offset << std::endl;
          std::cout << "avg_ff tag rel_offset: " << d_tags[0].offset - nread << std::endl;
          in += offset;
          ninput -= offset;
          count += offset;
        }else{
          if(offset >= d_num_aoa-d_count){
            std::cout << "Waiting for current packet to finish processing." << std::endl;
            std::cout << "Current packet remain: " << d_num_aoa-d_count << std::endl;
          }else{
            std::cout << "avg_ff WARNING: Packet Collision. " << std::endl;
          } 
        }
      }

      // in += offset;
      // int sum = 0;
      int sum_len = d_num_aoa - d_count;
      
      for (int i = 0; (i < sum_len) && (i < ninput); i++){
        d_sum += in[i];
        d_count++;
        count++;
      }
      if(d_count == d_num_aoa){
        out[0] = d_sum/d_num_aoa;
        d_count = 0;
        d_sum = 0;
      }else{
        consume_each (count);
        return 0;
      }
      
      // std::cout << "avg_ff AoA: " << d_sum/d_num_aoa << std::endl;
      std::cout << "avg_ff output AoA: " << out[0] << std::endl;
      consume_each (count);
      return 1;
    }

  } /* namespace MyMod */
} /* namespace gr */

