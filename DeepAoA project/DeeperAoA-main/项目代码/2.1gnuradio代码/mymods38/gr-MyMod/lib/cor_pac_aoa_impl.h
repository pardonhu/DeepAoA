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

#ifndef INCLUDED_MYMOD_COR_PAC_AOA_IMPL_H
#define INCLUDED_MYMOD_COR_PAC_AOA_IMPL_H

#include <MyMod/cor_pac_aoa.h>
#include <map>
#include <string>

namespace gr {
  namespace MyMod {

    class cor_pac_aoa_impl : public cor_pac_aoa
    {
     private:
      int d_packet_len; // The total number of samples in the current packet
      int d_offset; // Sample index in the current packet
      bool d_pac_end; // Finishing processing all the samples in the current packet, ready for output.
      int d_remain;
      std::vector<gr::tag_t> d_tags;
      int d_res; // Resolution, used to decide the length of the array for candidate degrees
      int* d_count;
      int d_mode;

     public:
      cor_pac_aoa_impl(int mode, int res);
      ~cor_pac_aoa_impl();

      // Where all the action really happens
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
    };

  } // namespace MyMod
} // namespace gr

#endif /* INCLUDED_MYMOD_COR_PAC_AOA_IMPL_H */

