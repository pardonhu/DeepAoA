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

#ifndef INCLUDED_MYMOD_CAL_MULTI_USRP_IMPL_H
#define INCLUDED_MYMOD_CAL_MULTI_USRP_IMPL_H

#define MY_2PI 2*M_PIl

#include <MyMod/cal_multi_usrp.h>

namespace gr {
  namespace MyMod {

    class cal_multi_usrp_impl : public cal_multi_usrp
    {
     private:
      const int d_num_in; // How many input streams
      const int d_cal_samps;  // The window size used for method 1
      bool d_en;  // Enable calibration
      const int d_cal_method;  // Method 0: using arc_off from a single sample
      // Method 1: using the average arc_off across all the samples in a window 
      gr_complex* d_offs_c; // Store the arc_off power of e used for method 0, used for disable state
      gr_complex* d_offs_window_c; // Store the arc_off power of e used for method 1.


     public:
      cal_multi_usrp_impl(unsigned int nin, unsigned int cal_time, bool en, unsigned int method);
      ~cal_multi_usrp_impl();

      // Where all the action really happens
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
           
      void set_en(bool en); // Set enable or disable during runtime
    };

  } // namespace MyMod
} // namespace gr

#endif /* INCLUDED_MYMOD_CAL_MULTI_USRP_IMPL_H */

