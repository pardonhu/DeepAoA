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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "cal_csi_impl.h"

using std::cout;

namespace gr {
  namespace MyMod {

    cal_csi::sptr
    cal_csi::make(int nin, Equalizer algo)
    {
      return gnuradio::get_initial_sptr
        (new cal_csi_impl(nin, algo));
    }

    /*
     * The private constructor
     */
    cal_csi_impl::cal_csi_impl(int nin, Equalizer algo)
      : gr::block("cal_csi",
              gr::io_signature::make(nin, nin, 64*sizeof(gr_complex)),
              gr::io_signature::make(nin, nin, 64*sizeof(gr_complex))),
              d_nin(nin),
              // d_equalizer(nullptr),
              d_current_symbol(0)
    {
      set_tag_propagation_policy(block::TPP_DONT);
	    // set_algorithm(algo);
    }

    /*
     * Our virtual destructor.
     */
    cal_csi_impl::~cal_csi_impl()
    {
    }

    void
    cal_csi_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      for(int i = 0; i < d_nin; i++){
		    ninput_items_required[i] = noutput_items;
	    }
    }

    int
    cal_csi_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      const gr_complex *in[d_nin];
      gr_complex *out[d_nin];
      for(int i = 0; i < d_nin; i++){
		    in[i] = (const gr_complex *) input_items[i];
		    out[i] = (gr_complex *) output_items[i];
	    }

      int i = 0;
	    int o = 0;
	    gr_complex csi[64];
	    gr_complex current_symbol[64];

	    cout << "cal_csi: input " << ninput_items[0] << "  output " << noutput_items << std::endl;
      
      while((i < ninput_items[0]) && (o < noutput_items)) {
		    int ind = 0;
		    for (; ind < d_nin; ind++){
		    	if(i >= ninput_items[ind+1]){
		    		break;
		    	}
		    }
		    if(ind < d_nin){
		    	break;
		    }
        get_tags_in_window(tags, 0, i, i + 1, pmt::string_to_symbol("num_samples_packet"));
        // new frame
		    if(tags.size()) {
		    	d_current_symbol = 0;
		    	d_frame_symbols = pmt::to_uint64(tags.front().value)/64;
          cout << "cal_csi: Number of symbols: " << d_frame_symbols << std::endl;
        }
      }
      
      
      // Tell runtime system how many input items we consumed on
      // each input stream.
      consume_each (noutput_items);

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

    // void
    // cal_csi_impl::set_algorithm(Equalizer algo) {
    // 	gr::thread::scoped_lock lock(d_mutex);
    // 	delete d_equalizer;

    // 	switch(algo) {
      
    // 	case COMB:
    // 		cout << "Comb" << std::endl;
    // 		d_equalizer = new equalizer::comb();
    // 		break;
    // 	case LS:
    // 		cout << "LS" << std::endl;
    // 		d_equalizer = new equalizer::ls();
    // 		break;
    // 	case LMS:
    // 		cout << "LMS" << std::endl;
    // 		d_equalizer = new equalizer::lms();
    // 		break;
    // 	case STA:
    // 		cout << "STA" << std::endl;
    // 		d_equalizer = new equalizer::sta();
    // 		break;
    // 	default:
    // 		throw std::runtime_error("Algorithm not implemented");
    // 	}
    // }

  } /* namespace MyMod */
} /* namespace gr */

