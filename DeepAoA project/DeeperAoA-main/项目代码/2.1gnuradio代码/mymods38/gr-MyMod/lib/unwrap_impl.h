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

#ifndef INCLUDED_MYMOD_UNWRAP_IMPL_H
#define INCLUDED_MYMOD_UNWRAP_IMPL_H

#include <MyMod/unwrap.h>

namespace gr {
  namespace MyMod {

    class unwrap_impl : public unwrap
    {
     private:
      int d_nin;
      int d_tol;
      bool d_en;
      int sub_cnt;
      float *last;
      const int sub_num = 64;

     public:
      unwrap_impl(int nin, double tol, bool en);
      ~unwrap_impl();

      // Where all the action really happens
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
      
      void set_en(bool en);
    };

  } // namespace MyMod
} // namespace gr

#endif /* INCLUDED_MYMOD_UNWRAP_IMPL_H */

