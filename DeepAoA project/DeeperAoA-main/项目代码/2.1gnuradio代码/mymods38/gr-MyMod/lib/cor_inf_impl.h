/* -*- c++ -*- */
/* 
 * Copyright 2018 <+YOU OR YOUR COMPANY+>.
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

#ifndef INCLUDED_MYMOD_COR_INF_IMPL_H
#define INCLUDED_MYMOD_COR_INF_IMPL_H

#define MY_2PI 2*M_PIl
#define CHECK_UPBOUND M_PIl-1.2
#define CHECK_LOWBOUND (-1)*M_PIl+1.2
#define P2PI_UPBOUND 1.2
#define P2PI_LOWBOUND -1.2

#include <MyMod/cor_inf.h>
#include <map>
#include <armadillo>
#include <string>

namespace gr {
  namespace MyMod {

    class cor_inf_impl : public cor_inf
    {
     private:
      int d_nin;
      int d_mode;
      int d_method;
      int d_window_size;
      // int d_angle;
      int d_resolution;
      int d_table_len;
      int d_count;
      bool d_en;
      bool d_finish;
      int d_channel;
      std::map<int, std::string> channel;
      arma::fmat d_vec_table;
      arma::fmat d_vec_table_ci;
      arma::dmat d_vec_sq;
      arma::dmat d_vec_sq_ci;
      int noff;
      bool d_read_tab;
      bool d_phase_p2pi;
      
      std::vector<gr::tag_t> d_tags;
      int d_remain; // used to record how many samples remained in one packet
      int d_offset; // index of sample in current packet under processing
      int d_packet_len;

      float **d_his_offs_window; // Offsets(averaged across d_window_size) pointer arrs
      float *d_his_offs_sum;
      bool *d_offs_p2pi;
      bool d_reset;

      int read_vec();


     public:
      cor_inf_impl(int nin, int mode, int method, bool en, bool finish, int chan, int window_size, int resolution, bool phase_p2pi);
      ~cor_inf_impl();

      // Where all the action really happens
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);

      void set_en(bool en); // Set enable or disable during runtime
      void set_finish(bool finish); // Set enable or disable during runtime
      void set_phase_p2pi(bool phase_p2pi); // Set enable or disable during runtime
    };

  } // namespace MyMod
} // namespace gr

#endif /* INCLUDED_MYMOD_COR_INF_IMPL_H */

