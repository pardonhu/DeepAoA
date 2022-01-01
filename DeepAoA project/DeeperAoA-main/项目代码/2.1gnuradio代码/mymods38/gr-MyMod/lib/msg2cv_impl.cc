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
#include "msg2cv_impl.h"
#include <gnuradio/block_detail.h>


namespace gr {
  namespace MyMod {

    msg2cv::sptr
    msg2cv::make(unsigned int nin)
    {
      return gnuradio::get_initial_sptr
        (new msg2cv_impl(nin));
    }

    /*
     * The private constructor
     */
    msg2cv_impl::msg2cv_impl(unsigned int nin)
      : gr::block("msg2cv",
              gr::io_signature::make(0, 0, 0),
              gr::io_signature::make(nin, nin, sizeof(gr_complex))),
    d_nin(nin){
      d_flag = false;
      d_len = 0;
      d_vlen = 0;
      d_ptr = 0;
      d_len_remain = 0;
      out_buf = new gr_complex[130000];
      // for (int i = 0; i < d_nin; i++){
			// 	// std::string t_pname = "in" + i;
      //   // message_port_register_in(pmt::mp("t_pname"));
	    //   // set_msg_handler(pmt::mp("t_pname"), boost::bind(&parse, this, _1));
      //   out_buf[i] = new gr_complex[6000];
			// }
      message_port_register_in(pmt::mp("data_in"));
	    set_msg_handler(pmt::mp("data_in"), boost::bind(&msg2cv_impl::parse, this, _1));
      
    }

    /*
     * Our virtual destructor.
     */
    msg2cv_impl::~msg2cv_impl()
    {
      // for (int i = 0; i < d_nin; i++){
			// 	std::string t_pname = "in" + i;
      //   delete[] out_buf[i];
			// }
      delete[] out_buf;
    }

    void
    msg2cv_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      /* <+forecast+> e.g. ninput_items_required[0] = noutput_items */
    }

    int
    msg2cv_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      // const <+ITYPE+> *in = (const <+ITYPE+> *) input_items[0];
      gr_complex *out[d_nin];
      
      for (int i = 0; i < d_nin; i++){
        out[i] = (gr_complex *)output_items[i];
      }

      if(d_flag == true){
        // std::cout << "debug: msg2cv working!!" << std::endl;
        // int nout = std::min(int(d_len_remain), noutput_items);
        int nout = int(d_len_remain);
        // std::cout << "msg2cv: nout: " << nout << std::endl;
        // Insert a tag that contains the value of the length of
        // the numbers of the samples of the a packet into outstream 
        // Only perform the inserting at the start of the streams of a packet
        if(d_len_remain == d_len){
          const pmt::pmt_t key = pmt::string_to_symbol("num_samples_packet");
	        const pmt::pmt_t value = pmt::from_double(d_len);
	        const pmt::pmt_t srcid = pmt::string_to_symbol(name());
          add_item_tag(0, nitems_written(0), key, value, srcid);
        }
        

        // int o = 0;
        d_ptr = d_len - d_len_remain;
        // while(o < nout){
        //   // std::cout << "debug: msg2cv copying: " << o << std::endl;
        //   // std::cout << "debug: msg2cv noutput_items: " << noutput_items << std::endl;
        //   for (int i = 0; i < d_nin; i++){
        //     // std::cout << "debug: msg2cv copying: " << i << std::endl;
        //     std::memcpy(out[i],out_buf+i*d_len+d_ptr,nout*sizeof(gr_complex));
        //   }
        //   o++; 
        // }
        for (int i = 0; i < d_nin; i++){
          // std::cout << "debug: msg2cv copying: " << i << std::endl;
          std::memcpy(out[i],out_buf+i*d_len+d_ptr,nout*sizeof(gr_complex));
        }

        d_len_remain = d_len_remain - nout;
        if (d_len_remain == 0){
          d_flag = false;
        }
        


        // std::cout << "debug: msg2cv working!!" << std::endl;
        // for (int j = 0; j < d_len; j++){
        //   std::cout << "debug: msg2cv copying: " << j << std::endl;
        //   std::cout << "debug: msg2cv noutput_items: " << noutput_items << std::endl;
        //   for (int i = 0; i < d_nin; i++){
        //     std::cout << "debug: msg2cv copying: " << i << std::endl;
        //     std::memcpy(out[i]+j,out_buf+i*d_len+j,sizeof(gr_complex));
        //   }
          
        // }
        // d_flag = false;
        // std::cout << "debug: msg2cv done!!" << std::endl;
        consume_each (0);
        return nout;
      }
      // Do <+signal processing+>
      // Tell runtime system how many input items we consumed on
      // each input stream.
      consume_each (0);
      // std::cout << "debug: msg2cv not working!!" << std::endl;

      // Tell runtime system how many output items we produced.
      return 0;
    }

    void 
    msg2cv_impl::parse(pmt::pmt_t msg){
      // std::cout << "debug: parse!!" << std::endl;
      if(pmt::is_eof_object(msg)) {
	    	detail().get()->set_done(true);
	    	return;
	    } else if(pmt::is_symbol(msg)) {
	    	return;
	    }
      if(d_flag == true){
        std::cout << "msg2cv: WARNING! Collision!" << std::endl; 
      }
      pmt::pmt_t metadata = pmt::car(msg);
	    pmt::pmt_t cv = pmt::cdr(msg);
      d_len = pmt::to_uint64(pmt::dict_ref(metadata, pmt::mp("len"), pmt::get_PMT_NIL()));
      assert(d_len >= 128); // long trainning symbols are 128 samples long
      // d_len = 128; // only use the 128 samples from the long trainning symbols
      d_len_remain = d_len;
      size_t nport = pmt::to_uint64(pmt::dict_ref(metadata, pmt::mp("nport"), pmt::get_PMT_NIL()));
      std::cout << "msg2cv: debug: len: " << d_len << std::endl;
      // std::cout << "debug: nport: " << nport << std::endl;
      assert(d_nin == nport);
      d_vlen = d_len*nport;
      // std::cout << "debug: vlen: " << d_vlen << std::endl;
      const gr_complex* temp = pmt::c32vector_elements(cv,d_vlen);
      // for (int i = 0; i < nport; i++){
      //   std::memcpy(out_buf+d_len*i, temp+d_len*i, d_len*sizeof(gr_complex));
      // }
      std::memcpy(out_buf, temp, d_vlen*sizeof(gr_complex));
      // out_buf = pmt::c32vector_writable_elements(cv,d_vlen);
      // std::cout << "debug: out_buf[d_len]: " << out_buf[d_len] << std::endl;
      // delete[] temp;
      // d_init = true;
      d_flag = true;
    }

  } /* namespace MyMod */
} /* namespace gr */

