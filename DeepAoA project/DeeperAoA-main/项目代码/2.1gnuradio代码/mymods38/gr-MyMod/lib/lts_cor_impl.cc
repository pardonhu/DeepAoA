/* -*- c++ -*- */
/* 
 * Copyright 2019 <+YOU OR YOUR COMPANY+>.
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
#include "lts_cor_impl.h"
#include <cmath>
#include "volk/volk.h"
#include <gnuradio/math.h>
#include <fstream>

#define VEC_TAB_PATH "/home/dexter/dir4ieee802-11/2019--2-19/lts_cor_vec"

namespace gr {
  namespace MyMod {

    lts_cor::sptr
    lts_cor::make(int nin, int mode, int method, bool en, bool finish, int chan, int window_size, int resolution, bool phase_p2pi)
    {
      return gnuradio::get_initial_sptr
        (new lts_cor_impl(nin, mode, method, en, finish, chan, window_size, resolution, phase_p2pi));
    }

    /*
     * The private constructor
     */
    lts_cor_impl::lts_cor_impl(int nin, int mode, int method, bool en, bool finish, int chan, int window_size, int resolution, bool phase_p2pi)
      : gr::block("lts_cor",
              gr::io_signature::make(nin, nin, sizeof(gr_complex)),
              gr::io_signature::make2(2*nin-1, 2*nin-1, sizeof(float), sizeof(float))),
              d_nin(nin),
              d_mode(mode),
              d_method(method),
              d_en(en),
              d_finish(finish),
              d_window_size(window_size),
              d_phase_p2pi(phase_p2pi),
              d_count(0),
              d_channel(chan),
              d_resolution(resolution),
              d_lts_finish(false){
                channel.insert(std::make_pair(172,"5.86 GHz"));
                channel.insert(std::make_pair(174,"5.87 GHz"));
                channel.insert(std::make_pair(176,"5.88 GHz"));
                channel.insert(std::make_pair(178,"5.89 GHz"));
                channel.insert(std::make_pair(180,"5.90 GHz"));
                channel.insert(std::make_pair(182,"5.91 GHz"));
                channel.insert(std::make_pair(184,"5.92 GHz"));
                
                d_table_len = 360/resolution;
                noff = nin*(nin-1)/2;

                d_vec_table = arma::fmat(noff, d_table_len);
                d_vec_table_ci = arma::fmat(noff, d_table_len); // For mode 1
                d_vec_sq = arma::dmat(1, d_table_len); // For mode 0, store the square of vector in the vec_table
                d_vec_sq_ci = arma::dmat(1, d_table_len); // For mode 1, store the square of vector in the vec_table
                d_read_tab = false;
                
                d_remain = 0;
                d_offset = -1;
                d_packet_len = 0;
                set_tag_propagation_policy(block::TPP_DONT);
                set_output_multiple(d_window_size);
                d_his_offs_sum = new float[noff];
                d_offs_p2pi = new bool[noff];

                d_lts_offs = new float**[lts_num];
                for(int i = 0; i < lts_num; ++i){
                  d_lts_offs[i] = new float*[noff];
                  for(int j = 0; j < noff; ++j){
                    d_lts_offs[i][j] = new float[128];
                  }
                }

                d_reset = false;
                
                d_his_offs_window = new float*[noff]; // Offsets(averaged across d_window_size) pointer arrs
                for (int i = 0; i < noff; i++){
                  d_his_offs_window[i] = new float[d_window_size];
                  d_his_offs_sum[i] = 0;
                  d_offs_p2pi[i] = false;
                }

                // for volk
                const int alignment_multiple = volk_get_alignment() / sizeof(float);
                set_alignment(std::max(1,alignment_multiple));

                std::cout << "Mode 0: Construct phase difference vector table." << std::endl;
                std::cout << "Input num: " << d_nin << std::endl;
                std::map<int, std::string>::iterator it = channel.find(d_channel);
                if(it != channel.end()){
                  std::cout << "Channel: " << d_channel << ", " << it->second << std::endl;
                }else{
                  std::cout << "Error! Illegal channel!" << std::endl;
                  // return 0;
                }
                // std::cout << "Current angle: " << d_angle << " degrees." << std::endl;
                std::cout << "Resolution: " << d_resolution << " degrees." << std::endl;
                std::cout << "Table length: " << d_table_len << " vectors." << std::endl;
              }

    /*
     * Our virtual destructor.
     */
    lts_cor_impl::~lts_cor_impl()
    {
      for (int i = 0; i < noff; i++){
        delete[] d_his_offs_window[i];
      }
      delete[] d_his_offs_window;
      delete[] d_his_offs_sum;
      delete[] d_offs_p2pi;
    }

    void
    lts_cor_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      /* <+forecast+> e.g. ninput_items_required[0] = noutput_items */
      for(int i = 0; i < d_nin; i++){
        ninput_items_required[i] = noutput_items;
        // two long trainning symbols has 64*2=128 samples
        // concatenate 128 samples to form a long vector for every two input (phase differences).
      }
    }

    int
    lts_cor_impl::general_work (int noutput_items,
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

      float *out[2*d_nin-1]; 

      gr_complex *d_offs_c_arr[noff];
      gr_complex *d_offs_window_c_arr[noff];
      
      
      for (int i = 0; i < d_nin; i++){
        in[i] = (const gr_complex *)input_items[i];
        // out[i] = (float *) output_items[i];
        d_arcs[i] = new float[noutput_items];
      }

      for (int i = 0; i < 2*d_nin-1; i++){
        out[i] = (float *) output_items[i];
      }
      
      
      for (int i = 0; i < noff; i++){
        d_offs[i] = new float[noutput_items];
        d_offs_window[i] = new float[noutput_items];
        d_offs_window_4ci[i] = new float[noutput_items];
      }
      
      
      // float *out = (float *) output_items[0];

      int ninput = ninput_items[0];
      for (int ind = 1; ind < d_nin; ind++){
        ninput = std::min(ninput, ninput_items[ind]);
      }
      assert(ninput >= d_window_size);
      // int current_remain = ninput;
      int nread = nitems_read(0);
      int packet_offset = 0;
      get_tags_in_range(d_tags, 0, nread, nread + ninput, pmt::string_to_symbol("num_samples_packet"));

      if(d_tags.size()){
        for(int i = 0; i < d_tags.size(); i++){
          std::cout << "lts_cor New packet, offset: " << d_tags[i].offset-nread << ", length: " << d_tags[i].value << std::endl;
          if((i > 0) && (d_tags[i].offset-d_tags[i-1].offset < d_window_size)){
            std::cout << "lts_cor WARNING: Packet length less than Windoe size!" << std::endl;
          }
        }
        packet_offset = d_tags[0].offset-nread;
        if(d_reset == true){
          std::cout << "lts_cor: Waiting for current packet to finish processing." << std::endl;
          std::cout << "lts_cor: Reset mode." << std::endl;
          std::cout << "lts_cor: Current packet remain: " << d_remain << std::endl;
        }else if((d_reset == false)&&(d_remain > packet_offset)){
          std::cout << "lts_cor WARNING: Packet collision!" << std::endl;
        }else if((d_reset == false)&&(d_remain == 0)){
          const pmt::pmt_t key = pmt::string_to_symbol("num_samples_packet");
	        const pmt::pmt_t value = d_tags[0].value;
	        const pmt::pmt_t srcid = pmt::string_to_symbol(name());
          add_item_tag(1, nitems_written(1), key, value, srcid);
          d_remain = pmt::to_double(d_tags[0].value);
          d_packet_len = d_remain;
          d_offset = 0;
          d_lts_finish = false;
          std::cout << "lts_cor: Tags inserted." << std::endl;
          // current_remain -= packet_offset;
        }else{
          std::cout << "lts_cor: Waiting for current packet to finish processing." << std::endl;
          std::cout << "lts_cor: Current packet remain: " << d_remain << std::endl;
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

      if(d_reset == true){
        reset_state(noutput_items);
        return 0;
      }

      // Make sure no packets overlap.
      if(d_remain < noutput_items){
        if(d_remain == 0){
          return 0;
        }
        // debug
        std::cout << "lts_cor: Not enough samples left in the packet for noutput: " << noutput_items << ", reset the noutput as: " << d_remain << "." << std::endl;
        noutput_items = d_remain;
        d_remain = 0;
        // d_offset = -1;
        // d_packet_len = 0;
      }else{
        // debug
        std::cout << "lts_cor: window_size: "<< d_window_size << ", noutput: " << noutput_items << std::endl;
        d_remain -= noutput_items;
      }

      if(d_mode == 0){
        //if(d_en == true){ // |+++ if(d_en == true) begin +++|
          if(d_finish == true){
            // debug
            std::cout << "ninput: " << ninput_items[0] << ", noutput: " << noutput_items << std::endl;
            // std::cout << "Finish sampling for angle " << d_count*d_resolution << " degrees." << std::endl;
            // Perform some checking and saving and updating works.
            std::cout << "Finish lts_offs vector constructing for " << d_count << " packets." << std::endl;
            if(d_count == lts_num){
              // Store the vector table to disk.
              // Now pretend we have done this shit.
              std::stringstream ss, ss2;
              ss << d_channel;
              ss2 << d_resolution;
              std::string vec_tab_name = "/chan-"+ss.str()+"_"+"lts";

              std::string vec_tab_path = VEC_TAB_PATH;
              std::ofstream vec2disk(vec_tab_path+vec_tab_name, std::ios::out);

              vec2disk.precision(32);

              std::cout << "Saving to " << vec_tab_path+vec_tab_name << std::endl;

              if(vec2disk.is_open() == true){
                float* vec_table;
                vec_table = new float[noff];
                for(int i = 0; i < lts_num; i++){
                  std::cout << "Saving the " << i+1 << " vector..." << std::endl;
                  std::cout << i << ":" << std::endl;
                  // vec2disk << i << ":" << std::endl;
                  for(int j = 0; j < noff; ++j){
                    for(int k = 0; k < 128; ++k){
                      std::cout << d_lts_offs[i][j][k];
                      vec2disk << d_lts_offs[i][j][k];
                      if(k == 128-1){
                        std::cout << "\n";
                        vec2disk << "\n";
                      }else{
                        std::cout << " ";
                        vec2disk << " ";
                      }
                    }
                  }
                
                }
                vec2disk.close();
                std::cout << "Vector table for channel " << d_channel << " has been stored to disk." << std::endl;
              }else{
                std::cout << "Open file error! Quitting..." << std::endl;
                d_en = false; // Or there will be a loop
              }
            }else{
              std::cout << "Error! Not enough vectors saved." << std::endl;
              // std::cout << "Current required signal from " << d_count*d_resolution << " degrees." << std::endl;
            }
            d_finish = false;
            d_en = false;
            d_reset = true;
            // reset all the values, e.g. d_remain.
          }
          else{
            // Sampling
            if(d_count >= lts_num){
              std::cout << "Reset module." << std::endl;
              // For now, only reset for current configuration.
              d_count = 0;
            }
            // std::cout << "lts_cor enabled." << std::endl;
            // Construct phase difference vector table, angle parameter is necessary.
            std::map<int, std::string>::iterator it = channel.find(d_channel);
            if(it == channel.end()){
              std::cout << "Error! Illegal channel!" << std::endl;
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
            
            switch (d_method){
              case 0:{
                // Real time
                if(d_lts_finish == false){
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

                  // Handling the 2pi differences
                  // float* vec_table;
                  // vec_table = new float[noff];
                  // debug
                  // std::cout << "Log: handling the " << d_count+1 << " vector out of " << d_table_len << " vectors." << std::endl;
                  // double vec_sq = 0;
                  for(int i = 0; i < noff; i++) {
                    for(int j = 0; j < noutput_items; j++){
                      if(d_phase_p2pi == true){
                        if(d_offs[i][j] < 0){
                          d_offs[i][j] += MY_2PI;
                        }
                        
                      }
                      if(d_offset+j < 128)
                        d_lts_offs[d_count][i][d_offset+j] = d_offs[i][j];
                      if(i < d_nin-1){
                        out[i+1][j] = d_offs[i][j];
                        out[i+d_nin][j] = d_offs[i][j];
                      }

                      if(d_offset+j+1 == 128){
                        // d_vec_table(i, d_count) = d_offs[i][j];  // Store the arc_off vector, used for aoa calculation.
                        // if(i < d_nin-1){
                        //   // out[i+1][j] = d_offs[i][j];
                        //   // out[i+d_nin][j] = d_offs[i][j];
                        //   memcpy(out[i+1], d_lts_offs[i], 128*sizeof(float));
                        //   memcpy(out[i+d_nin], d_lts_offs[i], 128*sizeof(float));
                        // }
                        if(i == noff-1){
                          d_lts_finish = true;
                          ++d_count;
                          std::cout << "lts_cor: Finished " << d_count << " packet." << std::endl;
                        }
                      }
                      // if(j == noutput_items-1){
                      //   vec_sq += d_vec_table(i, d_count)*d_vec_table(i, d_count);
                      //   if(i == noff-1){
                      //     d_vec_sq(0,d_count) = vec_sq;
                      //   }
                      // }
                    }
                  }
                }else{
                  for(int i = 0; i < noff; i++) {
                    for(int j = 0; j < noutput_items; j++){
                      if(i < d_nin-1){
                        out[i+1][j] = 0;
                        out[i+d_nin][j] = 0;
                      }
                    }
                  }
                }
                d_offset += noutput_items;
                assert(d_offset <= d_packet_len);
                // delete[] vec_table;
                memcpy(out[0], d_arcs[0], noutput_items*sizeof(float));
                if(d_count == lts_num)
                  d_finish = true;
                // Better set this flag by the user self.
                // p.s. For wifi packets, this should be done by the code, or it's too hard.
              } // case 0 end
              break;

              case 1:{
                // Average window
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
                
                int offs_cnt[noff];
                float offs_acc[noff];
                bool offs_p2pi[noff];
                float offs_avg[noff];
                bool offs_p2pi2[noff];
                for(int i = 0; i < noff; i++){
                  offs_cnt[i] = 1;
                  offs_acc[i] = 0;
                  offs_p2pi[i] = false;
                  offs_avg[i] = 0;
                  offs_p2pi2[i] = false;
                  for(int j = 0; j < noutput_items; j++){
                    if((d_offs[i][j] < CHECK_UPBOUND) && (d_offs[i][j] > CHECK_LOWBOUND)){
                      offs_cnt[i]++;
                      offs_acc[i] += d_offs[i][j];
                    }
                  }

                  // 0.333 is a hyperparameter, it's just a try. If it doesn't work well, we'll change it.
                  if(float(offs_cnt[i])/float(noutput_items) > 0.333){
                    offs_avg[i] = offs_acc[i]/offs_cnt[i];
                    if((offs_avg[i] > P2PI_LOWBOUND) && (offs_avg[i] < P2PI_UPBOUND)){
                      // do not use 0 as boundary, use P2PI_LOWBOUND as new boundary.
                      // Then decide to p2pi or not based on the avg.
                      // EDIT: maybe dynamically adjust the boundary based on the offs_avg[i] is better
                      if(offs_avg[i] < 0){
                        offs_p2pi2[i] = true;
                      }
                      // std::cout << "lts_cor: offs_avg[" << i << "], " << offs_avg[i] << std::endl;

                    }else{
                      offs_p2pi[i] = true; 
                    }
                  }else{
                    offs_p2pi[i] = true; // do not use 0 as boundary, use P2PI_LOWBOUND as new boundary.
                  }
                }

                // Handling the 2pi differences (NECESSARY OR NOT?)
                // float* vec_table;
                // vec_table = new float[noff];
                // debug
                // std::cout << "Log: handling the " << d_count << " vector out of " << d_table_len << " vectors." << std::endl;
                double vec_sq = 0;
                for(int i = 0; i < noff; i++) {
                  for(int j = 0; j < noutput_items; j++){
                    if(d_phase_p2pi == true){
                      // simple method
                      if(offs_p2pi[i] == true){
                        if(d_offs[i][j] < 0){
                          d_offs[i][j] += MY_2PI;
                        }
                      }else{
                        // new method, try to deal with the around 0 hop issue.
                        // maybe use a new condition
                        if(d_offs[i][j] < offs_avg[i]-1.2){
                          d_offs[i][j] += MY_2PI;
                        }else if(d_offs[i][j] > offs_avg[i]+1.2){
                          d_offs[i][j] -= MY_2PI;
                        }
                        // after window, we need to decide whether to add 2pi again.
                      }
                    }
                    if(i < d_nin-1){
                      out[i+1][j] = d_offs[i][j];
                    }
                    
                    
                    // new average window implementation
                    if((j+d_offset) < d_window_size){
                      // volk_32f_accumulator_s32f(d_offs_window[i],d_offs[i],d_window_size);
                      if(j == 0){
                        d_offs_window[i][j] = d_offs[i][j];
                      }else{
                        d_offs_window[i][j] = d_offs[i][j] + d_offs_window[i][j-1];
                      }

                      if(offs_p2pi[i] == false){
                        if(offs_p2pi2[i] == true){
                          d_offs_window_4ci[i][j] = d_offs_window[i][j]/(j+1);
                          d_offs_window_4ci[i][j] = d_offs_window_4ci[i][j] > 0 ? d_offs_window_4ci[i][j] : d_offs_window_4ci[i][j]+MY_2PI;
                        }else{
                          d_offs_window_4ci[i][j] = d_offs_window[i][j]/(j+1);
                        }
                      }else{
                        d_offs_window_4ci[i][j] = d_offs_window[i][j]/(j+1);
                      }
                      if(i < d_nin-1){
                        out[i+d_nin][j] = d_offs_window_4ci[i][j];
                      }

                    }else{
                      if(j < d_window_size){
                        if(j == 0){
                          // volk_32f_accumulator_s32f(d_offs_window[i]+j,d_his_offs_window[i],d_window_size);
                          if(offs_p2pi[i] != d_offs_p2pi[i]){ // in the same packet, different methods are used, so we don't use the his_window from last work call.
                            d_offs_window[i][j] = d_offs[i][j];
                          }else{
                            d_offs_window[i][j] = d_his_offs_sum[i] + d_offs[i][j] - d_his_offs_window[i][j];
                          }
                        }else{
                          if(offs_p2pi[i] != d_offs_p2pi[i]){
                            d_offs_window[i][j] = d_offs[i][j] + d_offs_window[i][j-1];
                          }else{
                            d_offs_window[i][j] = d_offs_window[i][j-1] + d_offs[i][j] - d_his_offs_window[i][j];
                          }
                        }
                      }else{
                        d_offs_window[i][j] = d_offs[i][j] + d_offs_window[i][j-1] - d_offs[i][j-d_window_size];
                      }
                      if(offs_p2pi[i] == false){
                        if(offs_p2pi2[i] == true){
                          if(offs_p2pi[i] != d_offs_p2pi[i]){ 
                            d_offs_window_4ci[i][j] = d_offs_window[i][j]/(j+1);
                          }else{
                            d_offs_window_4ci[i][j] = d_offs_window[i][j]/d_window_size;
                          }
                          d_offs_window_4ci[i][j] = d_offs_window_4ci[i][j] > 0 ? d_offs_window_4ci[i][j] : d_offs_window_4ci[i][j]+MY_2PI;
                        }else{
                          if(offs_p2pi[i] != d_offs_p2pi[i]){ 
                            d_offs_window_4ci[i][j] = d_offs_window[i][j]/(j+1);
                          }else{
                            d_offs_window_4ci[i][j] = d_offs_window[i][j]/d_window_size;
                          }
                        }
                      }else{
                        if(offs_p2pi[i] != d_offs_p2pi[i]){ 
                          d_offs_window_4ci[i][j] = d_offs_window[i][j]/(j+1);
                        }else{
                          d_offs_window_4ci[i][j] = d_offs_window[i][j]/d_window_size;
                        }
                      }

                      if(i < d_nin-1){   
                        out[i+d_nin][j] = d_offs_window_4ci[i][j];
                      }
                    }

                    if(j == noutput_items-1){
                      d_vec_table(i, d_count) = d_offs_window_4ci[i][j];
                      vec_sq += d_vec_table(i, d_count)*d_vec_table(i, d_count);
                        if(i == noff-1){
                        d_vec_sq(0,d_count) = vec_sq;

                      }
                    }
                    // update history offset buffer
                    if((j+1) % d_window_size == 0){
                      memcpy(d_his_offs_window[i], d_offs[i]+j-d_window_size+1, d_window_size*sizeof(float));
                      d_his_offs_sum[i] = d_offs_window[i][j];
                      d_offs_p2pi[i] = offs_p2pi[i];
                    }
                  }
                }
                d_offset += noutput_items;
                assert(d_offset <= d_packet_len);
                // delete[] vec_table;
                memcpy(out[0], d_arcs[0], noutput_items*sizeof(float));
                if(d_en == true){
                  d_finish = true;
                }else{
                  // do nothing.
                }
                
                // Better set this flag by the user self.
                // p.s. For wifi packets, this should be done by the code, or it's too hard.
              } // case 1 end
            } // switch end
            
          }
        

      }else if(d_mode == 1){
        // mode == 1, correlative interferometer.
        if(d_read_tab == false){
          std::cout << "Mode 1: Correlative interferometer." << std::endl;
          int read_state = read_vec();
          if(read_state < 0){
            std::cout << "Error occurred while reading vec table! Quitting now.";
            d_mode = 0;
            d_en = false; // Or there will be a loop
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
        }else{
          // Already read table, do not need do that again.
          // 
        }
        // CI

        switch (d_method){
          case 0:{
            // Real time
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
            // Handling the 2pi differences
            if(d_phase_p2pi == true){
              for(int j = 0; j < noutput_items; j++) {
                for(int i = 0; i < noff; i++){
                  if(d_offs[i][j] < 0){
                    d_offs[i][j] += MY_2PI;
                  }
                  if(i < d_nin-1){
                    out[i+1][j] = d_offs[i][j];
                    out[i+d_nin][j] = d_offs[i][j];
                  }
                }
              }
            }
            
            // Fast sq calculation for all input
            float* sq_rst[noff+1];
            for (int i = 0; i < noff+1; i++){
              sq_rst[i] = new float[noutput_items];
            }
            
            for(int i = 0; i < noff; i++){
              volk_32f_x2_multiply_32f(sq_rst[i], d_offs[i], d_offs[i], noutput_items);
            }
            assert(noff > 1);
            volk_32f_x2_add_32f(sq_rst[noff], sq_rst[0], sq_rst[1], noutput_items);
            for(int i = 2; i < noff; i++){
              volk_32f_x2_add_32f(sq_rst[noff], sq_rst[noff], sq_rst[i], noutput_items);
            }

            // correlation calculation
            for(int j = 0; j < noutput_items; j++) {
              int arg_max_cor = 0;
              float max_cor = -1.0;
              for(int p = 0; p < d_table_len; p++){
                float cross_sq = 0;
                if(j == 0){
                  int t = 0;
                  while(t < noff){
                    std::cout << d_vec_table_ci(t,p) << ", ";
                    t++;
                  }
                    std::cout << std::endl;
                  }
                for(int i = 0; i < noff; i++){
                  cross_sq += d_offs[i][j]*d_vec_table_ci(i,p);
                  // debug
                  if(j == 0){
                    std::cout << d_offs[i][j];
                    if(i == noff-1){
                      std::cout << std::endl;
                    }else{
                      std::cout << ", ";
                    }
                  }
                  
                }
                float cor = cross_sq/(std::sqrt(d_vec_sq_ci(0,p))*std::sqrt(sq_rst[noff][j]));
                // debug
                if(j == 0){
                  std::cout << "vec in table number: " << p+1 << " , cor: " << cor << std::endl;
                }
                arg_max_cor = max_cor < cor ? d_resolution*p : arg_max_cor;
                max_cor = max_cor < cor ? cor : max_cor;
              }
              // std::cout << "Cor for angle " << d_resolution*p << ": " << cor << std::endl;
              out[0][j] = arg_max_cor;
            }
            d_offset += noutput_items;
            assert(d_offset <= d_packet_len);
            for (int i = 0; i < noff; i++){
              delete[] sq_rst[i];
            }
          } // case 0 end
          break;
          case 1:{
            // Average window
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
            // Handling the 2pi differences
            // First scan the input buffer, calculate the mean of all this points
            // also log the number of the points in the range between the upper and lower bound.
            // It's used to decide whether all these points are the major or not.
            int offs_cnt[noff];
            float offs_acc[noff];
            bool offs_p2pi[noff];
            float offs_avg[noff];
            bool offs_p2pi2[noff];
            for(int i = 0; i < noff; i++){
              offs_cnt[i] = 1;
              offs_acc[i] = 0;
              offs_p2pi[i] = false;
              offs_avg[i] = 0;
              offs_p2pi2[i] = false;
              for(int j = 0; j < noutput_items; j++){
                if((d_offs[i][j] < CHECK_UPBOUND) && (d_offs[i][j] > CHECK_LOWBOUND)){
                  offs_cnt[i]++;
                  offs_acc[i] += d_offs[i][j];
                }
              }

              // 0.333 is a hyperparameter, it's just a try. If it doesn't work well, we'll change it.
              if(float(offs_cnt[i])/float(noutput_items) > 0.333){
                offs_avg[i] = offs_acc[i]/offs_cnt[i];
                if((offs_avg[i] > P2PI_LOWBOUND) && (offs_avg[i] < P2PI_UPBOUND)){
                  // do not use 0 as boundary, use P2PI_LOWBOUND as new boundary.
                  // Then decide to p2pi or not based on the avg.
                  // EDIT: maybe dynamically adjust the boundary based on the offs_avg[i] is better
                  if(offs_avg[i] < 0){
                    offs_p2pi2[i] = true;
                  }
                  // std::cout << "lts_cor: offs_avg[" << i << "], " << offs_avg[i] << std::endl;

                }else{
                  offs_p2pi[i] = true; 
                }
              }else{
                offs_p2pi[i] = true; // do not use 0 as boundary, use P2PI_LOWBOUND as new boundary.
              }
            }

            for(int j = 0; j < noutput_items; j++) {
              for(int i = 0; i < noff; i++){
                if(d_phase_p2pi == true){
                  // simple method
                  if(offs_p2pi[i] == true){
                    if(d_offs[i][j] < 0){
                      d_offs[i][j] += MY_2PI;
                    }
                  }else{
                    // new method, try to deal with the around 0 hop issue.
                    // maybe use a new condition
                    if(d_offs[i][j] < offs_avg[i]-1.2){
                      d_offs[i][j] += MY_2PI;
                    }else if(d_offs[i][j] > offs_avg[i]+1.2){
                      d_offs[i][j] -= MY_2PI;
                    }
                    // after window, we need to decide whether to add 2pi again.
                  }
                  
                }

                if(i < d_nin-1){
                  out[i+1][j] = d_offs[i][j];
                }
                
                // new average window implementation
                if((j+d_offset) < d_window_size){
                  // volk_32f_accumulator_s32f(d_offs_window[i],d_offs[i],d_window_size);
                  if(j == 0){
                    d_offs_window[i][j] = d_offs[i][j];
                  }else{
                    d_offs_window[i][j] = d_offs[i][j] + d_offs_window[i][j-1];
                  }

                  if(offs_p2pi[i] == false){
                    if(offs_p2pi2[i] == true){
                      d_offs_window_4ci[i][j] = d_offs_window[i][j]/(j+1);
                      d_offs_window_4ci[i][j] = d_offs_window_4ci[i][j] > 0 ? d_offs_window_4ci[i][j] : d_offs_window_4ci[i][j]+MY_2PI;
                    }else{
                      d_offs_window_4ci[i][j] = d_offs_window[i][j]/(j+1);
                    }
                  }else{
                    d_offs_window_4ci[i][j] = d_offs_window[i][j]/(j+1);
                  }
                  if(i < d_nin-1){
                    out[i+d_nin][j] = d_offs_window_4ci[i][j];
                  }
                
                }else{
                  if(j < d_window_size){
                    if(j == 0){
                      // volk_32f_accumulator_s32f(d_offs_window[i]+j,d_his_offs_window[i],d_window_size);
                      if(offs_p2pi[i] != d_offs_p2pi[i]){ // in the same packet, different methods are used, so we don't use the his_window from last work call.
                        d_offs_window[i][j] = d_offs[i][j];
                      }else{
                        d_offs_window[i][j] = d_his_offs_sum[i] + d_offs[i][j] - d_his_offs_window[i][j];
                      }
                    }else{
                      if(offs_p2pi[i] != d_offs_p2pi[i]){
                        d_offs_window[i][j] = d_offs[i][j] + d_offs_window[i][j-1];
                      }else{
                        d_offs_window[i][j] = d_offs_window[i][j-1] + d_offs[i][j] - d_his_offs_window[i][j];
                      }
                    }
                  }else{
                    d_offs_window[i][j] = d_offs[i][j] + d_offs_window[i][j-1] - d_offs[i][j-d_window_size];
                  }
                  if(offs_p2pi[i] == false){
                    if(offs_p2pi2[i] == true){
                      if(offs_p2pi[i] != d_offs_p2pi[i]){ 
                        d_offs_window_4ci[i][j] = d_offs_window[i][j]/(j+1);
                      }else{
                        d_offs_window_4ci[i][j] = d_offs_window[i][j]/d_window_size;
                      }
                      d_offs_window_4ci[i][j] = d_offs_window_4ci[i][j] > 0 ? d_offs_window_4ci[i][j] : d_offs_window_4ci[i][j]+MY_2PI;
                    }else{
                      if(offs_p2pi[i] != d_offs_p2pi[i]){ 
                        d_offs_window_4ci[i][j] = d_offs_window[i][j]/(j+1);
                      }else{
                        d_offs_window_4ci[i][j] = d_offs_window[i][j]/d_window_size;
                      }
                    }
                  }else{
                    if(offs_p2pi[i] != d_offs_p2pi[i]){ 
                      d_offs_window_4ci[i][j] = d_offs_window[i][j]/(j+1);
                    }else{
                      d_offs_window_4ci[i][j] = d_offs_window[i][j]/d_window_size;
                    }
                  }

                  if(i < d_nin-1){   
                    out[i+d_nin][j] = d_offs_window_4ci[i][j];
                  }
                }

                if((j+1) % d_window_size == 0){
                  // debug
                  // std::cout << "Updated window, j: " << j << std::endl;
                  memcpy(d_his_offs_window[i], d_offs[i]+j-d_window_size+1, d_window_size*sizeof(float));
                  d_his_offs_sum[i] = d_offs_window[i][j];
                  d_offs_p2pi[i] = offs_p2pi[i];
                }
              }
            }

            // Fast sq calculation for all input
            float* sq_rst[noff+1];
            for (int i = 0; i < noff+1; i++){
              sq_rst[i] = new float[noutput_items];
            }

            for(int i = 0; i < noff; i++){
              volk_32f_x2_multiply_32f(sq_rst[i], d_offs_window_4ci[i], d_offs_window_4ci[i], noutput_items);
            }
            assert(noff > 1);
            volk_32f_x2_add_32f(sq_rst[noff], sq_rst[0], sq_rst[1], noutput_items);
            for(int i = 2; i < noff; i++){
              volk_32f_x2_add_32f(sq_rst[noff], sq_rst[noff], sq_rst[i], noutput_items);
            }

            // correlation calculation
            // pass the num_samples_packet info, used in the next module to decide the final degree for one packet
            for(int j = 0; j < noutput_items; j++) {
              int arg_max_cor = 0;
              float max_cor = -1.0;
              for(int p = 0; p < d_table_len; p++){
                float cross_sq = 0;
                for(int i = 0; i < noff; i++){
                  cross_sq += d_offs[i][j]*d_vec_table_ci(i,p);
                }
                float cor = cross_sq/(std::sqrt(d_vec_sq_ci(0,p))*std::sqrt(sq_rst[noff][j]));
                arg_max_cor = max_cor < cor ? d_resolution*p : arg_max_cor;
                max_cor = max_cor < cor ? cor : max_cor;
              }
              // std::cout << "Cor for angle " << d_resolution*p << ": " << cor << std::endl;
              out[0][j] = arg_max_cor;
              if((j == 0) && (d_offset == 0) ){
                const pmt::pmt_t key_d = pmt::string_to_symbol("num_samples_packet");
	              const pmt::pmt_t value_d = pmt::from_uint64(d_packet_len);
	              const pmt::pmt_t srcid_d = pmt::string_to_symbol(name());
                add_item_tag(0, nitems_written(0), key_d, value_d, srcid_d);
              }
            }
            d_offset += noutput_items;
            assert(d_offset <= d_packet_len);
            for (int i = 0; i < noff; i++){
              delete[] sq_rst[i];
            }
            // d_finish = true;
          } // case 1 end
        } // switch end
      }else{
        std::cout << "Undefined mode for lts_cor!" << std::endl;
        for(int i = 0; i < noutput_items; i++){
          for(int j = 0; j < 2*d_nin-1; j++){
            out[j][i] = 0;
          }
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
      consume_each (noutput_items);
      return noutput_items;
      // consume_each (noutput_items);

      // Tell runtime system how many output items we produced.
      // return noutput_items;
    }

    int
    lts_cor_impl::read_vec(void){
      std::cout << "Input num: " << d_nin << std::endl;
      // Choose different vecter table based on the channel and resolution parameter.
      std::map<int, std::string>::iterator it = channel.find(d_channel);
      if(it != channel.end()){
        std::cout << "Channel: " << d_channel << ", " << it->second << std::endl;
      }else{
        std::cout << "Error! Illegal channel!" << std::endl;
        return -1;
      }
      std::cout << "Resolution: " << d_resolution << " degrees." << std::endl;
      std::stringstream ss, ss2;
      ss << d_channel;
      ss2 << d_resolution;
      std::string vec_tab_name = "/chan-"+ss.str()+"_"+"lts";

      std::string vec_tab_path = VEC_TAB_PATH;
      std::ifstream disk2vec(vec_tab_path+vec_tab_name, std::ios::in);

      disk2vec.precision(32);

      std::cout << "Reading table from " << vec_tab_path+vec_tab_name << std::endl;

      if(disk2vec.is_open() == true){
        // float* vec_table[d_table_len];
        for(int i = 0; i < d_table_len; i++){
          std::cout << "Reading the " << i+1 << " vector..." << std::endl;
          // vec_table[i] = new float[noff];
          std::string tmp;
          std::getline(disk2vec, tmp);
          // debug
          // std::cout << "tmp: " << tmp << std::endl;
          tmp = tmp.substr(tmp.find_first_of(":")+1);
          // std::cout << "tmp first: " << tmp << std::endl;
          std::cout << i << ":";
          for(int j = 0; j < noff; j++){
            std::stringstream ssint;
            float tmpf;
            int ind = tmp.find_first_of(",");
            if(ind == std::string::npos){
              assert(j == noff-1);
              ssint << tmp;
              ssint >> tmpf;
            }else{
              ssint << tmp.substr(0, ind);
              ssint >> tmpf;
              tmp = tmp.substr(ind+1);
              // std::cout << "tmp next: " << tmp << std::endl;
            }
            // vec_table[i][j] = tmpf;
            d_vec_table_ci(j,i) = tmpf;
            std::cout << d_vec_table_ci(j,i);

            if(j == noff-1){
              std::cout << "\n";
            }else{
              std::cout << ",";
            }
          }
        }
        disk2vec.close();

        std::cout << "Vector table for channel " << d_channel << " and resolution " << d_resolution << " degrees has been read from disk." << std::endl;
        // d_read_tab = true;
      }else{
        std::cout << "Open file error! Quitting..." << std::endl;
        return -2;
      }
      
      return 0;
    }

    void
    lts_cor_impl::set_en(bool en){
      d_en = en;
      std::string a = "True";
      if(en == false){
        a = "False";
      }
      std::cout << "Reset enable as: " << a << "." << std::endl;
    }

    void
    lts_cor_impl::set_finish(bool finish){
      d_finish = finish;
      std::string a = "True";
      if(finish == false){
        a = "False";
      }
      std::cout << "Reset finish as: " << a << "." << std::endl;
    }

    void
    lts_cor_impl::set_phase_p2pi(bool phase_p2pi){
      d_phase_p2pi = phase_p2pi;
      std::string a = "True";
      if(phase_p2pi == false){
        a = "False";
      }
      std::cout << "Reset phase_p2pi as: " << a << "." << std::endl;
    }

    void
    lts_cor_impl::reset_state(int noutput_items){
      if(d_remain > 0){
          if(d_remain <= noutput_items){
            consume_each(d_remain);
            std::cout << "lts_cor: Empty the buffer..." << std::endl;
            std::cout << "lts_cor: Current packet remain: " << d_remain << std::endl;
            d_remain = 0;
            d_offset = -1;
            d_packet_len = 0;
            d_reset = false;
          }else{
            consume_each(noutput_items);
            std::cout << "lts_cor: Empty the buffer..." << std::endl;
            std::cout << "lts_cor: Current packet remain: " << d_remain << std::endl;
            d_remain -= noutput_items;
            d_offset += noutput_items;
          }
        }else if(d_remain == 0){
          std::cout << "lts_cor: Reset finished." << std::endl;
          d_reset = false;
          d_offset = -1;
          d_packet_len = 0;
        }else{
          throw std::runtime_error("lts_cor: d_remain < 0.");
        }
    }

  } /* namespace MyMod */
} /* namespace gr */

