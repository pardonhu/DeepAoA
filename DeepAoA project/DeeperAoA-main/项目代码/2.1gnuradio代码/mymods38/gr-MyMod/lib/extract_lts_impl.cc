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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "extract_lts_impl.h"
#include <iomanip>


namespace gr {
  namespace MyMod {

    extract_lts::sptr
    extract_lts::make(int nin, int mode, bool en, int ext_len, int off)
    {
      return gnuradio::get_initial_sptr
        (new extract_lts_impl(nin, mode, en, ext_len, off));
    }

    /*
     * The private constructor
     */
    extract_lts_impl::extract_lts_impl(int nin, int mode, bool en, int ext_len, int off)
      : gr::block("extract_lts",
              gr::io_signature::make(nin, nin, sizeof(gr_complex)),
              gr::io_signature::make(nin, nin, sizeof(gr_complex))),
              d_nin(nin),
              d_mode(mode),
              d_en(en),
              d_finish(false),
              d_ext_len(ext_len),
              d_count(0),
              d_offset(off),
              d_cur(0),
              d_remain(0),
              d_packet_len(0)
    {
      set_tag_propagation_policy(block::TPP_DONT);
      std::cout << "extract_lts: Extracting from " << ((d_offset == 0) ? "start":"position "+std::to_string(d_offset)) << " of a packet of lengh " << d_ext_len << " samples." << std::endl;
    }

    /*
     * Our virtual destructor.
     */
    extract_lts_impl::~extract_lts_impl()
    {
    }

    void
    extract_lts_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      for(int i = 0; i < d_nin; ++i)
        // ninput_items_required[i] = d_ext_len; // hope can extract in just on call.
        ninput_items_required[i] = noutput_items;
    }

    int
    extract_lts_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      const gr_complex *in[d_nin];
      gr_complex *out[d_nin];
      
      int ninput = ninput_items[0];
      for(int i = 0; i < d_nin; ++i){
        in[i] = (const gr_complex *) input_items[i];
        out[i] = (gr_complex *) output_items[i];
        ninput = std::min(ninput, ninput_items[i]);
      }

      // assert(ninput == noutput_items);
      int noutput = noutput_items; // used for return
      // std::cout << "extract_lts: noutput: " << noutput_items << std::endl;

      int nread = nitems_read(0);
      int packet_offset = 0;
      get_tags_in_range(d_tags, 0, nread, nread+ninput, pmt::string_to_symbol("num_samples_packet"));

      if(d_tags.size()){
        std::cout << "extract_lts: New packet, offset: " << d_tags[0].offset - nread << ", length: "
                  << d_tags[0].value << std::endl;
        if(d_ext_len > pmt::to_double(d_tags[0].value)){
          std::cout << "extract_lts: Try to extract more than packet length!" << std::endl
                    << "             reset extract length to packet length: " << d_tags[0].value << std::endl;
          d_ext_len = pmt::to_double(d_tags[0].value);
        }else{
          if(d_ext_len + d_offset > pmt::to_double(d_tags[0].value)){
            std::cout << "extract_lts: Try to extract out of boundary!" << std::endl
                      << "             reset extract length till end of packet: " << pmt::to_double(d_tags[0].value) - d_offset << std::endl;
          d_ext_len = pmt::to_double(d_tags[0].value) - d_offset;
          }
        }
        packet_offset = d_tags[0].offset - nread;
        if(d_remain > packet_offset){
          std::cerr << "extract_lts: WARNING: Packet collision! "
                    << "d_remain: " << d_remain << std::endl;
        }else if(d_remain == 0){
          const pmt::pmt_t key = pmt::string_to_symbol("num_samples_packet");
          // const pmt::pmt_t value = d_tags[0].value;
          const pmt::pmt_t value = pmt::from_uint64(d_ext_len); // using extracted length, passing this value to lts_cor module.
          const pmt::pmt_t srcid =  pmt::string_to_symbol(name());
          add_item_tag(0, nitems_written(0), key, value, srcid);
          d_remain = pmt::to_double(d_tags[0].value);
          d_packet_len = d_remain;
          d_count = 0;
          d_finish = false;
        }else{
          std::cout << "extract_lts: Waiting for current packet to finish processing." << std::endl
                    << "             Current packet remain: " << d_remain << std::endl;
        }
      }else{
        // std::cout << "extract_lts: No new tags !" 
                  // << "d_remain: " << d_remain << std::endl;
      }

      if(d_remain < noutput_items){
        if(d_remain == 0){
          // std::cout << "extract_lts: case 0 remain: " << d_remain << std::endl;
          consume_each(0);
          return 0;
        }
        std::cout << "extract_lts: case 0-phase 1 remain: " << d_remain << std::endl;
        noutput_items = d_remain;
        noutput = noutput_items;
        d_remain = 0;
        std::cout << "extract_lts: case 0-phase 2 remain: " << d_remain << std::endl;
      }else{
        d_remain -= noutput_items;
        std::cout << "extract_lts: case 1 remain: " << d_remain << std::endl;
      }

      if(d_en){
        if(!d_finish){
          if(d_cur < d_offset){
            noutput_items = d_cur+noutput_items > d_offset ? d_cur+noutput_items-d_offset : noutput_items;
            noutput = 0;
          }else if(d_count < d_ext_len){
            noutput = d_ext_len-d_count <= noutput_items ? d_ext_len-d_count : noutput_items;
            
            for(int i = 0; i < d_nin; ++i){
              std::memcpy(out[i], in[i], noutput*sizeof(gr_complex));
            }
            d_count += noutput;
            if(d_count == d_ext_len){
              d_finish = true;
            }
          }
        }else{
          noutput = 0;
        }
        d_cur += noutput_items;
      }else{
        for(int i = 0; i < d_nin; ++i){
          std::memcpy(out[i], in[i], noutput*sizeof(gr_complex));
        }
      }

      
      consume_each (noutput_items);

      // Tell runtime system how many output items we produced.
      return noutput;
    }

    void 
    extract_lts_impl::set_en(bool en){
      d_en = en;
      std::cout << "Set enable to: " << std::boolalpha << d_en << std::noboolalpha << std::endl;
    }

  } /* namespace MyMod */
} /* namespace gr */

