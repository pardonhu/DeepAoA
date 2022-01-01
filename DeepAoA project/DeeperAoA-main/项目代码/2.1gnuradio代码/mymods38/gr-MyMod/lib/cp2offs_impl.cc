/* -*- c++ -*- */
/* 
 * Copyright 2019 Dexter(zhuyf95@sjtu.edu.cn).
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
#include "cp2offs_impl.h"
#include <cmath>
#include "volk/volk.h"
#include <gnuradio/math.h>
#include <fstream>
#include <gnuradio/block.h>

#define OFFS_PATH "/home/lion/dir4ieee802_11/cp2offs"

namespace gr {
  namespace MyMod {

    cp2offs::sptr
    cp2offs::make(int nin, int window_size, bool en)
    {
      return gnuradio::get_initial_sptr
        (new cp2offs_impl(nin, window_size, en));
    }

    /*
     * The private constructor
     */
    cp2offs_impl::cp2offs_impl(int nin, int window_size, bool en)
      : gr::block("cp2offs",
              gr::io_signature::make(nin, nin, sizeof(gr_complex)),
              gr::io_signature::make(nin-1, nin-1, sizeof(float))),
              d_nin(nin),
              d_window_size(window_size),
              d_en(en),
              d_cnt(0),
              d_start(0){
                noff = nin*(nin-1)/2;
                d_remain = 0;
                d_offset = -1;
                d_packet_len = 0;
                set_tag_propagation_policy(block::TPP_DONT);
                set_output_multiple(d_window_size);
                std::cout << "cp2offs: output_multiple: " << output_multiple() << std::endl;

                // for volk
                const int alignment_multiple = volk_get_alignment() / sizeof(float);
                set_alignment(std::max(1,alignment_multiple));
              }

    /*
     * Our virtual destructor.
     */
    cp2offs_impl::~cp2offs_impl()
    {
    }

    void
    cp2offs_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      for(int i = 0; i < d_nin; i++){
        ninput_items_required[i] = noutput_items;
      }
    }

    int
    cp2offs_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      const gr_complex *in[d_nin];
      float *d_arcs[d_nin];   // Arcs pointer arrs
      float *d_offs[noff]; // Offsets pointer arrs
      float *d_offs_window[noff]; // Offsets(averaged across d_window_size) pointer arrs
      // unsigned int dd_window_size = std::min(d_window_size, noutput_items);
      float *d_offs_window_4ci[noff];

      float *out[d_nin-1]; 

      gr_complex *d_offs_c_arr[noff];
      gr_complex *d_offs_window_c_arr[noff];
      
      
      for (int i = 0; i < d_nin; i++){
        in[i] = (const gr_complex *)input_items[i];
        // out[i] = (float *) output_items[i];
        d_arcs[i] = new float[noutput_items];
      }

      for (int i = 0; i < d_nin-1; i++){
        out[i] = (float *) output_items[i];
      }
      
      
      for (int i = 0; i < noff; i++){
        d_offs[i] = new float[noutput_items];
        d_offs_window[i] = new float[noutput_items];
        d_offs_window_4ci[i] = new float[noutput_items];
      }

      int ninput = ninput_items[0];
      for (int ind = 1; ind < d_nin; ind++){
        ninput = std::min(ninput, ninput_items[ind]);
      }
      int nread = nitems_read(0);
      int packet_offset = 0;
      get_tags_in_range(d_tags, 0, nread, nread + ninput, pmt::string_to_symbol("num_samples_packet"));

      if(d_tags.size()){
        for(int i = 0; i < d_tags.size(); i++){
          std::cout << "cp2offs New packet, offset: " << d_tags[i].offset-nread << ", length: " << d_tags[i].value << std::endl;
        }
        packet_offset = d_tags[0].offset-nread;
        if(d_remain > packet_offset){
          std::cout << "cp2offs WARNING: Packet collision!" << std::endl;
        }else if(d_remain == 0){
          const pmt::pmt_t key = pmt::string_to_symbol("num_samples_packet");
	        const pmt::pmt_t value = d_tags[0].value;
	        const pmt::pmt_t srcid = pmt::string_to_symbol(name());
          add_item_tag(0, nitems_written(0), key, value, srcid);
          d_remain = pmt::to_double(d_tags[0].value);
          d_packet_len = d_remain;
          d_offset = 0;
          d_cnt++;
          std::cout << "cp2offs: Tags inserted." << std::endl;
        }else{
          std::cout << "cp2offs: Waiting for current packet to finish processing." << std::endl;
          std::cout << "cp2offs: Current packet remain: " << d_remain << std::endl;
        }
      }else if(d_remain == 0){
        for (int i = 0; i < d_nin; i++){
          delete[] d_arcs[i];
        }
        for (int i = 0; i < noff; i++){
          delete[] d_offs[i];
          delete[] d_offs_window[i];
          delete[] d_offs_window_4ci[i];
        }
        return 0;
      }

      // Make sure no packets overlap.
      if(d_remain < noutput_items){
        if(d_remain == 0){
          return 0;
        }
        // debug
        std::cout << "cp2offs: Not enough samples left in the packet for noutput: " << noutput_items << ", reset the noutput as: " << d_remain << "." << std::endl;
        noutput_items = d_remain;
        d_remain = 0;
        // d_offset = -1;
        // d_packet_len = 0;
      }else{
        // debug
        std::cout << "cp2offs: noutput: " << noutput_items << std::endl;
        d_remain -= noutput_items;
      }

      // Calculate the arc_tan of each input complex samples
      for (int j = 0; j < noutput_items; j++){
        for (int i = 0; i < d_nin; i++){
          d_arcs[i][j] = gr::fast_atan2f(in[i][j]); // Faster than volk,according to complex_to_arcs mod in gnuradio.
        }
      }
      // Compute the offset of each sample between arcs[j] and arcs[i], i=0,1,2,...,noff, j=i+1,...,noff, stores in offs[noff]
      int cnt = 0;
      for(int i = 0; i < d_nin; i++) {
        for(int j = i+1; j < d_nin; j++)
        {
          volk_32f_x2_subtract_32f(d_offs[cnt], d_arcs[j], d_arcs[i], noutput_items);
          cnt++;
        }
      }
      assert(cnt == noff);

      for (int i = 0; i < d_nin-1; i++){
        // std::memcpy(out[i], d_offs[i], noutput_items);
        for(int j = 0; j < noutput_items; j++){
          out[i][j] = d_offs[0][j];
        }
      }
      for (int i = d_nin-1; i < d_nin-1; i++){
        // std::memcpy(out[i], d_offs[i], noutput_items);
        for(int j = 0; j < noutput_items; j++){
          out[i][j] = d_offs[0][j];
        }
      }

      if((d_en == true) && (d_remain == 0)){
        if(d_start == 0){
          d_start = d_cnt;
        }
        // Store the offs to disk.
        std::stringstream ss, ss2;
        // ss << d_channel;
        // ss2 << d_resolution;
        // std::string vec_tab_name = "/chan-"+ss.str()+"_"+"res-"+ss2.str();
        ss << d_window_size;
        ss2 << d_cnt;
        std::string d_file_path = OFFS_PATH;
        std::string d_filename = "/offs-window_size-"+ss.str()+"-packet_no-"+ss2.str()+".txt";
        std::ofstream offs2disk(d_file_path+d_filename, std::ios::out);
        offs2disk.precision(32);
        std::cout << "Saving to " << d_file_path+d_filename << std::endl;
        if(offs2disk.is_open() == true){
          for(int i = 0; i < d_window_size; i++){
            std::cout << "Saving the " << i+1 << " off..." << std::endl;
            std::cout << i << ":";
            offs2disk << i << ":";
            std::cout << d_offs[0][i];
            offs2disk << d_offs[0][i];
            std::cout << "\n";
            offs2disk << "\n";
          }
          offs2disk.close();
          std::cout << "Offs have been stored to disk." << std::endl;
          if((d_cnt-d_start) == 9){
            d_en = false; // Or there will be a loop
          }
        }else{
          std::cout << "Open file error! Quitting..." << std::endl;
        }
      }    

      for (int i = 0; i < d_nin; i++){
        delete[] d_arcs[i];
      }

      for (int i = 0; i < noff; i++){
        delete[] d_offs[i];
        delete[] d_offs_window[i];
        delete[] d_offs_window_4ci[i];
      }
      
      // Tell runtime system how many input items we consumed on
      // each input stream.
      consume_each (noutput_items);

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

    void
    cp2offs_impl::set_en(bool en){
      d_en = en;
      std::string a = "True";
      if(en == false){
        a = "False";
      }
      std::cout << "Reset enable as: " << a << "." << std::endl;
    }

  } /* namespace MyMod */
} /* namespace gr */

