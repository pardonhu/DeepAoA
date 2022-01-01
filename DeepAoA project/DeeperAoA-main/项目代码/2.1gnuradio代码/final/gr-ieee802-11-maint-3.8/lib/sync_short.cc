/*
 * Copyright (C) 2013, 2016 Bastian Bloessl <bloessl@ccs-labs.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <ieee802_11/sync_short.h>
#include <gnuradio/io_signature.h>
#include "utils.h"

#include <iostream>

using namespace gr::ieee802_11;

static const int MIN_GAP = 480;
static const int MAX_SAMPLES = 540 * 80;


// static int i_s [] = {sizeof(gr_complex), sizeof(gr_complex), sizeof(float), sizeof(gr_complex), sizeof(gr_complex), sizeof(gr_complex)};
// static std::vector<int> i_sig(i_s, i_s+sizeof(i_s)/sizeof(int));
// static int o_s [] = {sizeof(gr_complex), sizeof(gr_complex), sizeof(gr_complex), sizeof(gr_complex)};
// static std::vector<int> o_sig(o_s, o_s+sizeof(o_s)/sizeof(int));
// static std::vector<int> i_sig{sizeof(float), sizeof(float), sizeof(gr_complex), sizeof(gr_complex), sizeof(gr_complex), sizeof(gr_complex)};
static std::vector<std::vector<int>> i_sigs{
	std::vector<int>{sizeof(float), sizeof(gr_complex), sizeof(gr_complex)},
	std::vector<int>{sizeof(float), sizeof(float), sizeof(gr_complex), sizeof(gr_complex), sizeof(gr_complex), sizeof(gr_complex)},
	std::vector<int>{sizeof(float), sizeof(float), sizeof(float), sizeof(gr_complex), sizeof(gr_complex), sizeof(gr_complex), sizeof(gr_complex), sizeof(gr_complex), sizeof(gr_complex)},
	std::vector<int>{sizeof(float), sizeof(float), sizeof(float), sizeof(float), sizeof(gr_complex), sizeof(gr_complex), sizeof(gr_complex), sizeof(gr_complex), sizeof(gr_complex), sizeof(gr_complex), sizeof(gr_complex), sizeof(gr_complex) }
};

class sync_short_impl : public sync_short {

public:
sync_short_impl(double threshold, unsigned int min_plateau, bool log, bool debug, unsigned int nin, bool raw) :
		block("sync_short",
			gr::io_signature::makev(nin*3, nin*3, i_sigs[nin-1]),
			gr::io_signature::make(nin+1, nin+1, sizeof(gr_complex))),
		d_log(log),
		d_debug(debug),
		d_state(SEARCH),
		d_plateau(0),
		d_copied(0),
		MIN_PLATEAU(min_plateau),
		d_nin(nin),
		d_raw(raw),
		d_threshold(threshold) {
			d_freq_offset = new float[d_nin];
			for(int i = 0; i < d_nin; i++){
				d_freq_offset[i] = 0;
			}
			set_tag_propagation_policy(block::TPP_DONT);
		}
~sync_short_impl(){
	delete[] d_freq_offset;
}

int general_work (int noutput_items, gr_vector_int& ninput_items,
		gr_vector_const_void_star& input_items,
		gr_vector_void_star& output_items) {
	
	const float *in_cor[d_nin];
	const gr_complex *in[d_nin];
	const gr_complex *in_abs[d_nin];
	
	gr_complex *pass_out[d_nin];
	for(int i = 0; i < d_nin; i++){
		in_cor[i] = (const float*)input_items[i];
		in[i] = (const gr_complex*)input_items[i+d_nin];
		in_abs[i] = (const gr_complex*)input_items[i+2*d_nin];
		pass_out[i] = (gr_complex*)output_items[i+1];
	}

	gr_complex *out = (gr_complex*)output_items[0];

	int noutput = noutput_items;
	// int ninput = std::min(std::min(ninput_items[0], ninput_items[1]), ninput_items[2]);
	int ninput = ninput_items[0];
	// debug
	// std::cout << "SHORT ninput: " << ninput << std::endl;
	// make sure every port is under control
	for(int ind = 0; ind < 3*d_nin; ind++){
		ninput = std::min(ninput, ninput_items[ind]);
	}
	// std::cout << "SHORT ninput (mod): " << ninput << std::endl;

	// dout << "SHORT noutput : " << noutput << " ninput: " << ninput_items[0] << std::endl;

	switch(d_state) {

	case SEARCH: {
		int i;

		for(i = 0; i < ninput; i++) {
			if(in_cor[0][i] > d_threshold) {
				if(d_plateau < MIN_PLATEAU) {
					d_plateau++;

				} else {
					d_state = COPY;
					d_copied = 0;
					for(int ii = 0; ii < d_nin; ++ii){
						d_freq_offset[ii] = arg(in_abs[ii][i]) / 16;
						insert_tag(nitems_written(ii+1), d_freq_offset[ii], nitems_read(ii+d_nin) + i, ii+1);
					}
					d_plateau = 0;
					insert_tag(nitems_written(0), d_freq_offset[0], nitems_read(d_nin) + i, 0);
					dout << "SHORT Frame!" << std::endl;
					break;
				}
			} else {
				d_plateau = 0;
			}
		}

		consume_each(i);
		return 0;
	}

	case COPY: {

		int o = 0;
		while( o < ninput && o < noutput && d_copied < MAX_SAMPLES) {
			if(in_cor[0][o] > d_threshold) {
				if(d_plateau < MIN_PLATEAU) {
					d_plateau++;

				// there's another frame
				} else if(d_copied > MIN_GAP) {
					d_copied = 0;
					d_plateau = 0;
					for(int ii = 0; ii < d_nin; ++ii){
						d_freq_offset[ii] = arg(in_abs[ii][o]) / 16;
						// std::cout << ii << ": " << d_freq_offset[ii] << std::endl;
						insert_tag(nitems_written(ii+1) + o, d_freq_offset[ii], nitems_read(ii+d_nin) + o, ii+1);
					}
					insert_tag(nitems_written(0) + o, d_freq_offset[0], nitems_read(d_nin) + o, 0);
					dout << "SHORT Frame!" << std::endl;
					break;
				}

			} else {
				d_plateau = 0;
			}

			out[o] = in[0][o] * exp(gr_complex(0, -d_freq_offset[0] * d_copied));
			if(d_raw == true){
				for(int p = 0; p < d_nin; p++){
					pass_out[p][o] = in[p][o];
				};
			}else{
				for(int p = 0; p < d_nin; p++){
					pass_out[p][o] = in[p][o] * exp(gr_complex(0, -d_freq_offset[p] * d_copied));
				};
			}
			o++;
			d_copied++;
		}

		if(d_copied == MAX_SAMPLES) {
			d_state = SEARCH;
		}

		dout << "SHORT copied " << o << std::endl;

		consume_each(o);
		return o;
	}
	}

	throw std::runtime_error("sync short: unknown state");
	return 0;
}

void insert_tag(uint64_t item, double freq_offset, uint64_t input_item, unsigned port) {
	mylog(boost::format("frame start at in: %2% out: %1%") % item % input_item);

	const pmt::pmt_t key = pmt::string_to_symbol("wifi_start");
	const pmt::pmt_t value = pmt::from_double(freq_offset);
	const pmt::pmt_t srcid = pmt::string_to_symbol(name());
	add_item_tag(port, item, key, value, srcid);
}

private:
	enum {SEARCH, COPY} d_state;
	int d_copied;
	int d_plateau;
	float *d_freq_offset;
	const double d_threshold;
	const bool d_log;
	const bool d_debug;
	const unsigned int MIN_PLATEAU;
	const unsigned int d_nin;
	bool d_raw;
};

sync_short::sptr
sync_short::make(double threshold, unsigned int min_plateau, bool log, bool debug, unsigned int nin, bool raw) {
	return gnuradio::get_initial_sptr(new sync_short_impl(threshold, min_plateau, log, debug, nin, raw));
}
