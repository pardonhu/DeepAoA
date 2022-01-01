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

#ifndef INCLUDED_MYMOD_EXTRACT_LTS_IMPL_H
#define INCLUDED_MYMOD_EXTRACT_LTS_IMPL_H

#include <MyMod/extract_lts.h>
#include <string>
#include <vector>

const long double MY_2PI = 2*M_PIl;

namespace gr {
  namespace MyMod {

    class extract_lts_impl : public extract_lts
    {
     private:
      int d_nin;
      int d_mode;
      bool d_en;
      bool d_finish;

      int d_count;
      int d_ext_len;
      int d_offset; // extracting start point 
      int d_cur; // current offset in packet
      
      std::vector<gr::tag_t> d_tags;
      int d_remain;
      int d_packet_len;

     public:
      extract_lts_impl(int nin, int mode, bool en, int ext_len, int off);
      ~extract_lts_impl();

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

#endif /* INCLUDED_MYMOD_EXTRACT_LTS_IMPL_H */

