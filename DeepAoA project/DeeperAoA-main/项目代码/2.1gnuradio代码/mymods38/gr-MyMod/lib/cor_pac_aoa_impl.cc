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
#include "cor_pac_aoa_impl.h"

namespace gr {
  namespace MyMod {

    cor_pac_aoa::sptr
    cor_pac_aoa::make(int mode, int res)
    {
      return gnuradio::get_initial_sptr
        (new cor_pac_aoa_impl(mode, res));
    }

    /*
     * The private constructor
     */
    cor_pac_aoa_impl::cor_pac_aoa_impl(int mode, int res)
      : gr::block("cor_pac_aoa",
              gr::io_signature::make(1, 1, sizeof(float)),
              gr::io_signature::make(1, 1, sizeof(float)))
    {
      d_packet_len = 0;
      d_offset = -1;
      d_pac_end = false;
      d_remain = 0;
      d_res = res;
      d_mode = mode;
      d_count = new int[360/d_res];
      for(int i = 0; i < 360/d_res; i++){
        d_count[i] = 0;
      }
      set_tag_propagation_policy(block::TPP_DONT);
    }

    /*
     * Our virtual destructor.
     */
    cor_pac_aoa_impl::~cor_pac_aoa_impl()
    {
      delete[] d_count;
    }

    void
    cor_pac_aoa_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      // if(d_packet_len > 0){
      //   ninput_items_required[0] = d_packet_len;
      // }else{
      //   ninput_items_required[0] = 1000;
      // }
      ninput_items_required[0] = 1000;
    }

    int
    cor_pac_aoa_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      const float *in = (const float *) input_items[0];
      float *out = (float *) output_items[0];

      // Do <+signal processing+>
      int ninput = ninput_items[0];
      // int current_remain = ninput;
      int nread = nitems_read(0);
      int packet_offset = 0;
      get_tags_in_range(d_tags, 0, nread, nread + ninput, pmt::string_to_symbol("num_samples_packet"));

      if(d_mode == 0){
        consume(0, ninput);
        return 0;
      }
      
      if(d_tags.size()){
        for(int i = 0; i < d_tags.size(); i++){
          std::cout << "cor_pac_aoa New packet, offset: " << d_tags[i].offset-nread << ", length: " << d_tags[i].value << std::endl;
        }
        packet_offset = d_tags[0].offset-nread;
        if(d_remain > packet_offset){
          std::cout << "cor_pac_aoa WARNING: Packet collision!" << std::endl;
        }else if(d_remain == 0){
          const pmt::pmt_t key = pmt::string_to_symbol("num_samples_packet");
	        const pmt::pmt_t value = d_tags[0].value;
	        const pmt::pmt_t srcid = pmt::string_to_symbol(name());
          add_item_tag(0, nitems_written(0), key, value, srcid);
          d_remain = pmt::to_uint64(d_tags[0].value);
          d_packet_len = d_remain;
          d_offset = 0;
          std::cout << "cor_pac_aoa: Tags inserted." << std::endl;
          // current_remain -= packet_offset;
        }else{
          std::cout << "cor_pac_aoa: Waiting for current packet to finish processing." << std::endl;
          std::cout << "cor_pac_aoa: Current packet remain: " << d_remain << std::endl;
        }
      }else if(d_remain == 0){      
        return 0;
      }

      // Make sure no packets overlap.
      if(d_remain < ninput){
        // debug
        // std::cout << "cor_pac_aoa: Over-read. Current remain: " << d_remain << ", ninput: " << ninput << ", reset the ninput as: " << d_remain << "." << std::endl;
        std::cout << "cor_pac_aoa: Over-read. Current remain: " << d_remain << ", ninput: " << ninput << std::endl;
        // ninput = d_remain;
        d_remain = 0;
        // d_offset = -1;
        // d_packet_len = 0;
      }else{
        // debug
        std::cout << "cor_pac_aoa: ninput: " << ninput << std::endl;
        d_remain -= ninput;
      }

      int i = 0;
      while((d_packet_len > 0) && (d_offset < d_packet_len) && (i < ninput)){
        int ind = in[i]/d_res;
        assert(ind < 360/d_res);
        d_count[ind]++;
        d_offset++;
        i++;
        if(d_offset == d_packet_len-1){
          d_pac_end = true;
          break;
        }
      }
      if(d_pac_end == true){
        // Output the final degree.
        int fin_deg = 0;
        int cnt = 0;
        for(int i = 0; i < 360/d_res; i++){
          // debug
          std::cout << "d_count[" << i << "]: " << d_count[i] << std::endl;
          fin_deg = d_count[i] > cnt ? (i*d_res) : fin_deg;
          cnt = d_count[i] > cnt ? d_count[i] : cnt;
          d_count[i] = 0;
        }
        out[0] = fin_deg;
        d_offset = -1;
        d_packet_len = 0;
        d_pac_end = false;
      }else{
        consume_each (i);
        return 0;
      }

      // Tell runtime system how many input items we consumed on
      // each input stream.
      consume_each (i);

      // Tell runtime system how many output items we produced.
      /////////////////////////////////
      return 1;
      /////////////////////////////////
    }

  } /* namespace MyMod */
} /* namespace gr */

