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

#ifndef INCLUDED_MYMOD_AVG_FF_IMPL_H
#define INCLUDED_MYMOD_AVG_FF_IMPL_H

#include <MyMod/avg_ff.h>

namespace gr {
  namespace MyMod {

    class avg_ff_impl : public avg_ff
    {
     private:
      unsigned int d_num_samples;
      unsigned int d_snap;
      unsigned int d_nonoverlap;
      unsigned int d_num_aoa;
      unsigned int d_sum;
      unsigned int d_count;

      std::vector<gr::tag_t> d_tags;

     public:
      avg_ff_impl(unsigned int num_samples, unsigned int snapshot_size, unsigned int nonoverlap, unsigned int method);
      ~avg_ff_impl();

      // Where all the action really happens
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
    };

  } // namespace MyMod
} // namespace gr

#endif /* INCLUDED_MYMOD_AVG_FF_IMPL_H */

