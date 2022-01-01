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

#ifndef INCLUDED_MYMOD_CP2OFFS_IMPL_H
#define INCLUDED_MYMOD_CP2OFFS_IMPL_H

#include <MyMod/cp2offs.h>
#include <map>
#include <string>

namespace gr {
  namespace MyMod {

    class cp2offs_impl : public cp2offs
    {
     private:
      int d_nin;
      int d_window_size;
      bool d_en;
      int noff;
      std::vector<gr::tag_t> d_tags;
      int d_remain; // used to record how many samples remained in one packet
      int d_offset; // index of sample in current packet under processing
      int d_packet_len;
      int d_cnt; // count packet number
      int d_start;

     public:
      cp2offs_impl(int nin, int window_size, bool en);
      ~cp2offs_impl();
      void set_en(bool en); // Set enable or disable during runtime

      // Where all the action really happens
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
    };

  } // namespace MyMod
} // namespace gr

#endif /* INCLUDED_MYMOD_CP2OFFS_IMPL_H */

