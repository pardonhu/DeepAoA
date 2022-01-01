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

#ifndef INCLUDED_MYMOD_AVG_V_IMPL_H
#define INCLUDED_MYMOD_AVG_V_IMPL_H

#include <MyMod/avg_v.h>

namespace gr {
  namespace MyMod {

    class avg_v_impl : public avg_v
    {
     private:
      int d_len;
      int d_cnt;
      int d_vlen;
      gr_complex **d_buf;
      gr_complex *d_avg;

     public:
      avg_v_impl(int vlen, int acc_cnt);
      ~avg_v_impl();

      // Where all the action really happens
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
    };

  } // namespace MyMod
} // namespace gr

#endif /* INCLUDED_MYMOD_AVG_V_IMPL_H */

