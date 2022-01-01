/* -*- c++ -*- */
/* 
 * Copyright 2018 Dexter.
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
#include "cal_multi_usrp_impl.h"
#include <cmath>
#include "volk/volk.h"

namespace gr {
  namespace MyMod {

    cal_multi_usrp::sptr
    cal_multi_usrp::make(unsigned int nin, unsigned int cal_time, bool en, unsigned int method)
    {
      return gnuradio::get_initial_sptr
        (new cal_multi_usrp_impl(nin, cal_time, en, method));
    }

    // static int o_s [] = {sizeof(gr_complex), sizeof(gr_complex), sizeof(gr_complex), sizeof(gr_complex), sizeof(float)};
    // static std::vector<int> o_sig(o_s, o_s+sizeof(o_s)/sizeof(int));
    /*
     * The private constructor
     */
    cal_multi_usrp_impl::cal_multi_usrp_impl(unsigned int nin, unsigned int cal_time, bool en, unsigned int method)
      : gr::block("cal_multi_usrp",
              gr::io_signature::make(nin, nin, sizeof(gr_complex)),
              gr::io_signature::make(nin, nin, sizeof(gr_complex))),
      d_num_in(nin),
      d_cal_samps(cal_time),
      d_en(en),
      d_cal_method(method)

    {
      // if(en == true){
      //   //std::cout << "Calibration enabled." << std::endl;
      //   //std::cout << "Input and output numbers: " << d_num_in << std::endl;
      //   switch(d_cal_method){
      //   case 0:
      //     //std::cout << "Calibration method 0: based on the last sample." << std::endl;
      //     break;
      //   case 1:
      //     //std::cout << "Calibration method 0: average all the samples falling in cal_time window, " << "window size: " << d_cal_samps << "." << std::endl;
      //     break;
      //   default:
      //     //std::cout << "Undefined method!" << std::endl;
      //     return;
      //   }
      // } 
      // else{
      //   //std::cout << "Calibration disabled.";
      //}
      // Allocate memory for an arr used to store the arc_off power of e,
      // For n input, there will be n-1 arc_offs in the arr.
      // value0 for arc0-arc1, value1 for arc0-arc2, etc.
      // d_offs_c for method 0, d_offs_window_c for method 1.
      // Initial value is set as 1, in case we don't want to do the cal.

      // For volk
      const int alignment_multiple = volk_get_alignment() / sizeof(gr_complex);
      set_alignment(std::max(1, alignment_multiple));
      
      d_offs_c = new gr_complex[d_num_in-1];
      d_offs_window_c = new gr_complex[d_num_in-1];
      for(int i = 0; i < d_num_in-1; i++){
        d_offs_c[i] = gr_complex(1,0);
        d_offs_window_c[i] = gr_complex(1,0);
      }
    }

    /*
     * Our virtual destructor.
     */
    cal_multi_usrp_impl::~cal_multi_usrp_impl()
    { 
      // Release the memory.
      delete[] d_offs_c;
      delete[] d_offs_window_c;
    }

    void
    cal_multi_usrp_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      /* <+forecast+> e.g. ninput_items_required[0] = noutput_items */
      for (int i = 0; i < ninput_items_required.size(); i++){
        ninput_items_required[i] = noutput_items;
      }
    }

    int
    cal_multi_usrp_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      // Cast in&out pointer 
      const gr_complex *in[d_num_in]; // Input pointer arrs
      gr_complex *out[d_num_in]; // Output pointer arrs
      float *arcs[d_num_in];   // Arcs pointer arrs
      float *offs[d_num_in-1]; // Offsets pointer arrs
      float *offs_window[d_num_in-1]; // Offsets(averaged across d_window_size) pointer arrs
      // float d_window[d_num_in]; //for each input, a d_window[d_window_size] stores the previous sum of arcs.
      // float d_arc = 0.0;  // Var used for store the temp ars for each sample in each input
      unsigned int d_window_size = std::min(d_cal_samps, noutput_items);
      // int loops = noutput_items/d_window_size;
      // int remain = noutput_items%d_window_size;
      // int total = loops;
      // float* arc_his[d_num_in];
      //float *arc_history = new float[d_window_size];
      //float **arc_his_n = new float*[d_num_in];

      // Notes in the next part.
      gr_complex *d_offs_c_arr[d_num_in-1];
      gr_complex *d_offs_window_c_arr[d_num_in-1];

      

      for (int i = 0; i < d_num_in; i++){
        in[i] = (const gr_complex *) input_items[i];
        out[i] = (gr_complex *) output_items[i];
        arcs[i] = new float[noutput_items];
        // arc_his[i] = new float[d_window_size];
        if(i < d_num_in-1){
          offs[i] = new float[noutput_items];
          offs_window[i] = new float[noutput_items];
          // Allocate memory for arrs used to store the arc_off power of e,
          // For n input, there will be n-1 arrs.
          // Arr0 for arc0-arc1, arr1 for arc1-arc2, etc.
          // d_offs_c_arr for method 0, d_offs_window_c_arr for method 1.
          d_offs_c_arr[i] = new gr_complex[noutput_items];
          d_offs_window_c_arr[i] = new gr_complex[noutput_items];
        }
        // d_window[i] = 0;
        //arc_his_n[i] = new float[d_window_size];
      }
      
      

      // Do <+signal processing+>

      
      if(d_en == true){
        switch(d_cal_method){
          case 0: // For method 0, real time cal.
          {
            // Calculate the arc_tan of each input complex samples
            for (int j = 0; j < noutput_items; j++){
              for (int i = 0; i < d_num_in; i++){
                arcs[i][j] = gr::fast_atan2f(in[i][j]); // Faster than volk,according to complex_to_arcs mod in gnuradio.
              }
            }

            // Compute the offset of each sample between arcs[0] and arcs [i], i=1,2,..., stores in offs[i-1]
            for(int i = 1; i < d_num_in; i++) {
              volk_32f_x2_subtract_32f(offs[i-1], arcs[0], arcs[i], noutput_items);
            }

            // Handling the 2pi differences
            for(int i = 0; i < d_num_in-1; i++) {
              for(int j = 0; j < noutput_items; j++){
                if(offs[i][j] < 0){
                  offs[i][j] += MY_2PI;
                }
                out[i+1][j] = exp(gr_complex(0,offs[i][j]));
                if((j+1)%d_window_size == 0){
                  d_offs_c[i] = out[i+1][j];  // Store the arc_off power of e, used for cal when this block is disabled.
                }
              }
            }
            memcpy(out[0], in[0], noutput_items*sizeof(gr_complex));
            for(size_t i = 1; i < input_items.size(); i++)
              volk_32fc_x2_multiply_32fc(out[i], in[i], out[i], noutput_items); //For input 1,2..., out=in*exp(arc_off*j)
          }
          break;
          case 1:
          {
            // for (int j = 0; j < noutput_items; j++){
            //   std::vector<float> temp;
            //   assert(temp.empty()==true);
            //   for (int i = 0; i < d_num_in; i++){
            //     d_arc = gr::fast_atan2f(in[i][j]);
            //     arcs[i][j] = d_arc;
            //     temp.push_back(d_arc);
            //     d_window[i] = d_window[i] + d_arc;
            //     if(j < d_window_size){
            //       out[i][j] = d_window[i]/(j+1);
            //     }
            //     else{
            //       d_window[i] = d_window[i] - arc_his.front()[i];
            //       out[i][j] = d_window[i]/d_window_size;
            //     }
            //     if(i == d_num_in-1){
            //       arc_his.push(temp);
            //       if(j >= d_window_size){
            //         arc_his.pop();
            //       }
            //     }
            //     temp.clear();
            //     assert(temp.empty()==true);
            //   }
            // }
            
            // Calculate the arc_tan of each input complex samples
            for (int j = 0; j < noutput_items; j++){
              for (int i = 0; i < d_num_in; i++){
                arcs[i][j] = gr::fast_atan2f(in[i][j]);
              }
            }

            // Compute the offset of each sample between arcs[0] and arcs [i], i=1,2,..., stores in offs[i-1]
            for(int i = 1; i < d_num_in; i++) {
              volk_32f_x2_subtract_32f(offs[i-1], arcs[0], arcs[i], noutput_items);
            }

            for(int i = 0; i < d_num_in-1; i++) {
              for(int j = 0; j < noutput_items; j++){
                if(offs[i][j] < 0){
                  offs[i][j] += MY_2PI;
                }
                if(j < d_window_size){
                  // For the first part of samples less than window size, we use
                  // the actual numbers to do the average.
                  volk_32f_accumulator_s32f(offs_window[i]+j,offs[i],j+1);
                  offs_window[i][j] /= (j+1);
                }else{
                  // For each sample after the index greater than window size,
                  // Using the previous window size of samples(including the current one)
                  // to do the average.
                  volk_32f_accumulator_s32f(offs_window[i]+j,offs[i]+j+1-d_window_size,d_window_size);
                  offs_window[i][j] /= d_window_size;
                }
                out[i+1][j] = exp(gr_complex(0,offs_window[i][j]));
                d_offs_window_c[i] = out[i+1][j];
              }
            }

            memcpy(out[0], in[0], noutput_items*sizeof(gr_complex));
            for(size_t i = 1; i < input_items.size(); i++)
              volk_32fc_x2_multiply_32fc(out[i], in[i], out[i], noutput_items); // Do the compensation for all the input except input0.
          }
            
        }
      }
      else{

        // After cal, using the stored value to do the phase offset compensation.
        memcpy(out[0], in[0], noutput_items*sizeof(gr_complex));
        
        // gr_complex* temp_gr = new gr_complex[noutput_items]{d_offs[0]};
        // std::cout << "debug: " << std::endl;
        // for(size_t i = 0; i < 10; i++){
        //   std::cout << temp_gr[i] << ", ";
        // }
        // volk_32fc_x2_add_32fc(out[i], in[i], temp_gr, noutput_items);
        switch(d_cal_method){
          case 0:{
            for(size_t i = 0; i < input_items.size()-1; i++){
              // memset(d_offs_c_arr[i], d_offs_c[i], noutput_items);
              // Fill all the noutput_items values of arr with the stored arc_off power of e value for this input.
              std::fill(d_offs_c_arr[i],d_offs_c_arr[i]+noutput_items,d_offs_c[i]);
              // Do the compensation.
              volk_32fc_x2_multiply_32fc(out[i+1], in[i+1], d_offs_c_arr[i], noutput_items);
            }
          }
          break;
          case 1:{
            for(size_t i = 0; i < input_items.size()-1; i++){
              // memset(d_offs_c_arr[i], d_offs_c[i], noutput_items);
              std::fill(d_offs_window_c_arr[i],d_offs_window_c_arr[i]+noutput_items,d_offs_window_c[i]);
              volk_32fc_x2_multiply_32fc(out[i+1], in[i+1], d_offs_window_c_arr[i], noutput_items);
            }
          }
        }
        
        // delete[] temp_gr;
          
      }
      // Release the memory allocated by new, otherwise memory will run out.
      for (int i = 0; i < d_num_in; i++){
        delete[] arcs[i];
        // delete[] arc_his[i];
        if(i < d_num_in-1){
          delete[] offs[i];
          delete[] offs_window[i];
          delete[] d_offs_c_arr[i];
          delete[] d_offs_window_c_arr[i];
        }
      }


      // Tell runtime system how many input items we consumed on
      // each input stream.
      consume_each (noutput_items);

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

    void
    cal_multi_usrp_impl::set_en(bool en){
      d_en = en;
      std::string a = "True";
      if(en == false){
        a = "False";
      }
      //std::cout << "Reset enable as: " << a << "." << std::endl;
    }

  } /* namespace MyMod */
} /* namespace gr */

