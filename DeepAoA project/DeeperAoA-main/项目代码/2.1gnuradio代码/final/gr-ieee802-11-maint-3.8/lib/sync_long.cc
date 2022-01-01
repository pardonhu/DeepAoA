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
#include <ieee802_11/sync_long.h>
#include "utils.h"
#include <gnuradio/io_signature.h>
#include <gnuradio/filter/fir_filter.h>
#include <gnuradio/fft/fft.h>

#include <list>
#include <tuple>

using namespace gr::ieee802_11;
using namespace std;


bool compare_abs(const std::pair<gr_complex, int>& first, const std::pair<gr_complex, int>& second) {
	return abs(get<0>(first)) > abs(get<0>(second));
}

// static int i_s [] = {sizeof(gr_complex), sizeof(gr_complex), sizeof(gr_complex), sizeof(gr_complex), sizeof(gr_complex)};
// static std::vector<int> i_sig(i_s, i_s+sizeof(i_s)/sizeof(int));
// static int o_s [] = {sizeof(gr_complex), sizeof(gr_complex), sizeof(gr_complex), sizeof(gr_complex), sizeof(float)};
// static std::vector<int> o_sig(o_s, o_s+sizeof(o_s)/sizeof(int));

class sync_long_impl : public sync_long {

public:
sync_long_impl(unsigned int sync_length, bool log, bool debug, unsigned int nin, bool raw) : block("sync_long",
		gr::io_signature::make2((nin+1)*2, (nin+1)*2, sizeof(gr_complex), sizeof(gr_complex)),
		gr::io_signature::make2(nin+1, nin+1, sizeof(gr_complex), sizeof(gr_complex))),
		d_fir(gr::filter::kernel::fir_filter_ccc(1, LONG)),
		d_log(log),
		d_debug(debug),
		d_offset(0),
		d_state(SYNC),
		SYNC_LENGTH(sync_length),
		d_nin(nin+1),
		d_raw(raw){

	set_tag_propagation_policy(block::TPP_DONT);
	d_correlation = new gr_complex*[d_nin];
	d_cor = new list<pair<gr_complex, int> >[d_nin];
	d_tags = new std::vector<gr::tag_t>[d_nin];
	d_freq_offset = new float[d_nin];
	d_freq_offset_short = new double[d_nin];
	for(int i = 0; i < d_nin; ++i){
		d_correlation[i] = gr::fft::malloc_complex(8192);
	}
	
}

~sync_long_impl(){
	for(int i = 0; i < d_nin; ++i){
		gr::fft::free(d_correlation[i]);
	}
	delete[] d_correlation;
	delete[] d_cor;
	delete[] d_tags;
	delete[] d_freq_offset;
	delete[] d_freq_offset_short;
}

int general_work (int noutput, gr_vector_int& ninput_items,
		gr_vector_const_void_star& input_items,
		gr_vector_void_star& output_items) {

	const gr_complex *in[d_nin];
	const gr_complex *in_delayed[d_nin];

	gr_complex *pass_out[d_nin-1];
	for(int i = 0; i < d_nin; i++){
		in[i] = (const gr_complex*)input_items[i];
		in_delayed[i] = (const gr_complex*)input_items[i+d_nin];
		if(i < d_nin-1)
			pass_out[i] = (gr_complex*)output_items[i+1];
	}
	gr_complex *out = (gr_complex*)output_items[0];



	dout << "LONG ninput[0] " << ninput_items[0] << "   ninput[1] " <<
			ninput_items[1] << "  noutput " << noutput <<
			"   state " << d_state << std::endl;

	// int ninput = std::min(std::min(ninput_items[0], ninput_items[1]), 8192);
	int ninput = 8192;
	// std::cout << "LONG ninput: " << ninput << std::endl;
	// make sure every port is under control
	for(int ind = 0; ind < 2*d_nin; ind++){
		ninput = std::min(ninput, ninput_items[ind]);
	}
	// std::cout << "LONG ninput (mod): " << ninput << std::endl;

	uint64_t nread[d_nin];
	for(int ii = 0; ii < d_nin; ++ii){
		nread[ii] = nitems_read(ii);
		get_tags_in_range(d_tags[ii], ii, nread[ii], nread[ii] + ninput);
	}
	if (d_tags[0].size()) {
		for(int ii = 0; ii < d_nin; ++ii){
			std::sort(d_tags[ii].begin(), d_tags[ii].end(), gr::tag_t::offset_compare);
		}

		const uint64_t offset = d_tags[0].front().offset;

		if(offset > nread[0]) {
			ninput = offset - nread[0];
		} else {
			if(d_offset && (d_state == SYNC)) {
				throw std::runtime_error("wtf");
			}
			if(d_state == COPY) {
				d_state = RESET;
			}
			for(int ii = 0; ii < d_nin; ++ii){
				d_freq_offset_short[ii] = pmt::to_double(d_tags[ii].front().value);
			}
		}
	}


	int i = 0;
	int o = 0;

	switch(d_state) {

	case SYNC:
		for(int ii = 0; ii < d_nin; ++ii){
			d_fir.filterN(d_correlation[ii], in[ii], std::min(SYNC_LENGTH, std::max(ninput - 63, 0)));
		}

		while(i + 63 < ninput) {

			for(int ii = 0; ii < d_nin; ++ii){
				d_cor[ii].push_back(pair<gr_complex, int>(d_correlation[ii][i], d_offset));
			}

			i++;
			d_offset++;

			if(d_offset == SYNC_LENGTH) {
				search_frame_start();
				mylog(boost::format("LONG: frame start at %1%") % d_frame_start);
				d_offset = 0;
				d_count = 0;
				d_state = COPY;

				break;
			}
		}

		break;

	case COPY:
		while(i < ninput && o < noutput) {

			int rel = d_offset - d_frame_start;

			if(!rel)  {
				for(int ii = 0; ii < d_nin-1; ++ii){
					add_item_tag(ii+1, nitems_written(ii+1),
						pmt::string_to_symbol("wifi_start"),
						pmt::from_double(d_freq_offset_short[ii+1] - d_freq_offset[ii+1]),
						pmt::string_to_symbol(name()));
				}
				add_item_tag(0, nitems_written(0),
						pmt::string_to_symbol("wifi_start"),
						pmt::from_double(d_freq_offset_short[0] - d_freq_offset[0]),
						pmt::string_to_symbol(name()));
			}

			if(rel >= 0 && (rel < 128 || ((rel - 128) % 80) > 15)) {
				out[o] = in_delayed[0][i] * exp(gr_complex(0, d_offset * d_freq_offset[0]));
				dout<<"should be same"<<'\n';
				dout<<out[0]<<' '<<out[1]<<' '<<out[2]<<'\n';
				if(d_raw == true){
					for(int p = 0; p < d_nin-1; p++){
						pass_out[p][o] = in_delayed[p+1][i] * exp(gr_complex(0, d_offset * d_freq_offset[0]));
						dout<<pass_out[0][0]<<' '<<pass_out[0][1]<<' '<<pass_out[0][2]<<"I am true"<<'\n';
					};
				}else{
					for(int p = 0; p < d_nin-1; p++){
						pass_out[p][o] = in_delayed[p+1][i] * exp(gr_complex(0, d_offset * d_freq_offset[p+1]));
						dout<<pass_out[0][0]<<' '<<pass_out[0][1]<<' '<<pass_out[0][2]<<"I am false"<<'\n';
					};
				}
				dout<<pass_out[0][0]<<' '<<pass_out[0][1]<<' '<<pass_out[0][2]<<'\n';
				
				o++;
			}

			i++;
			d_offset++;
		}

		break;

	case RESET: {
		while(o < noutput) {
			if(((d_count + o) % 64) == 0) {
				d_offset = 0;
				d_state = SYNC;
				break;
			} else {
				out[o] = 0;
				for(int p = 0; p < d_nin-1; p++){
					pass_out[p][o] = 0;
				};
				o++;
			}
		}

		break;
	}
	}

	dout << "produced : " << o << " consumed: " << i << std::endl;

	d_count += o;
	consume_each(i);
	return o;
}

void forecast (int noutput_items, gr_vector_int &ninput_items_required) {

	// in sync state we need at least a symbol to correlate
	// with the pattern
	if(d_state == SYNC) {
		for(int i = 0; i < d_nin*2; i++){
			ninput_items_required[i] = 64;
		}
	} else {
		for(int i = 0; i < d_nin*2; i++){
			ninput_items_required[i] = noutput_items;
		}
	}
}

void search_frame_start() {

	vector<pair<gr_complex, int> > vec[d_nin];
	// sort list (highest correlation first)
	for(int ii = 0; ii < d_nin; ++ii){
		assert(d_cor[ii].size() == SYNC_LENGTH);
		d_cor[ii].sort(compare_abs);

		// copy list in vector for nicer access
		vec[ii] = {d_cor[ii].begin(), d_cor[ii].end()};
		d_cor[ii].clear();
	}

	// in case we don't find anything use SYNC_LENGTH
	d_frame_start = SYNC_LENGTH;

	for(int i = 0; i < 3; i++) {
		for(int k = i + 1; k < 4; k++) {
			gr_complex first[d_nin];
			gr_complex second[d_nin];
			if(get<1>(vec[0][i]) > get<1>(vec[0][k])) {
				for(int ii = 0; ii < d_nin; ++ii){
					first[ii] = get<0>(vec[ii][k]);
					second[ii] = get<0>(vec[ii][i]);
				}
			} else {
				for(int ii = 0; ii < d_nin; ++ii){
					first[ii] = get<0>(vec[ii][i]);
					second[ii] = get<0>(vec[ii][k]);
				}
			}
			int diff  = abs(get<1>(vec[0][i]) - get<1>(vec[0][k]));
			if(diff == 64) {
				d_frame_start = min(get<1>(vec[0][i]), get<1>(vec[0][k]));
				for(int ii = 0; ii < d_nin; ++ii){
					d_freq_offset[ii] = arg(first[ii] * conj(second[ii])) / 64;
				}
				// nice match found, return immediately
				return;

			} else if(diff == 63) {
				d_frame_start = min(get<1>(vec[0][i]), get<1>(vec[0][k]));
				for(int ii = 0; ii < d_nin; ++ii){
					d_freq_offset[ii] = arg(first[ii] * conj(second[ii])) / 63;
				}
			} else if(diff == 65) {
				d_frame_start = min(get<1>(vec[0][i]), get<1>(vec[0][k]));
				for(int ii = 0; ii < d_nin; ++ii){
					d_freq_offset[ii] = arg(first[ii] * conj(second[ii])) / 65;
				}
			}
		}
	}
}

private:
	enum {SYNC, COPY, RESET} d_state;
	int         d_count;
	int         d_offset;
	int         d_frame_start;
	float       *d_freq_offset;
	double      *d_freq_offset_short;

	gr_complex **d_correlation;
	list<pair<gr_complex, int> > *d_cor;
	std::vector<gr::tag_t> *d_tags;
	gr::filter::kernel::fir_filter_ccc d_fir;

	const bool d_log;
	const bool d_debug;
	const int  SYNC_LENGTH;
	const int d_nin;
	bool d_raw;

	static const std::vector<gr_complex> LONG;
};

sync_long::sptr
sync_long::make(unsigned int sync_length, bool log, bool debug, unsigned int nin, bool raw) {
	return gnuradio::get_initial_sptr(new sync_long_impl(sync_length, log, debug, nin, raw));
}

const std::vector<gr_complex> sync_long_impl::LONG = {

gr_complex(-0.0455, -1.0679), gr_complex( 0.3528, -0.9865), gr_complex( 0.8594,  0.7348), gr_complex( 0.1874,  0.2475),
gr_complex( 0.5309, -0.7784), gr_complex(-1.0218, -0.4897), gr_complex(-0.3401, -0.9423), gr_complex( 0.8657, -0.2298),
gr_complex( 0.4734,  0.0362), gr_complex( 0.0088, -1.0207), gr_complex(-1.2142, -0.4205), gr_complex( 0.2172, -0.5195),
gr_complex( 0.5207, -0.1326), gr_complex(-0.1995,  1.4259), gr_complex( 1.0583, -0.0363), gr_complex( 0.5547, -0.5547),
gr_complex( 0.3277,  0.8728), gr_complex(-0.5077,  0.3488), gr_complex(-1.1650,  0.5789), gr_complex( 0.7297,  0.8197),
gr_complex( 0.6173,  0.1253), gr_complex(-0.5353,  0.7214), gr_complex(-0.5011, -0.1935), gr_complex(-0.3110, -1.3392),
gr_complex(-1.0818, -0.1470), gr_complex(-1.1300, -0.1820), gr_complex( 0.6663, -0.6571), gr_complex(-0.0249,  0.4773),
gr_complex(-0.8155,  1.0218), gr_complex( 0.8140,  0.9396), gr_complex( 0.1090,  0.8662), gr_complex(-1.3868, -0.0000),
gr_complex( 0.1090, -0.8662), gr_complex( 0.8140, -0.9396), gr_complex(-0.8155, -1.0218), gr_complex(-0.0249, -0.4773),
gr_complex( 0.6663,  0.6571), gr_complex(-1.1300,  0.1820), gr_complex(-1.0818,  0.1470), gr_complex(-0.3110,  1.3392),
gr_complex(-0.5011,  0.1935), gr_complex(-0.5353, -0.7214), gr_complex( 0.6173, -0.1253), gr_complex( 0.7297, -0.8197),
gr_complex(-1.1650, -0.5789), gr_complex(-0.5077, -0.3488), gr_complex( 0.3277, -0.8728), gr_complex( 0.5547,  0.5547),
gr_complex( 1.0583,  0.0363), gr_complex(-0.1995, -1.4259), gr_complex( 0.5207,  0.1326), gr_complex( 0.2172,  0.5195),
gr_complex(-1.2142,  0.4205), gr_complex( 0.0088,  1.0207), gr_complex( 0.4734, -0.0362), gr_complex( 0.8657,  0.2298),
gr_complex(-0.3401,  0.9423), gr_complex(-1.0218,  0.4897), gr_complex( 0.5309,  0.7784), gr_complex( 0.1874, -0.2475),
gr_complex( 0.8594, -0.7348), gr_complex( 0.3528,  0.9865), gr_complex(-0.0455,  1.0679), gr_complex( 1.3868, -0.0000),

};
