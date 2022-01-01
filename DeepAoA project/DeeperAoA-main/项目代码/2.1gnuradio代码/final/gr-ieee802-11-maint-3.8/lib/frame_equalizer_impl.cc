/*
 * Copyright (C) 2016 Bastian Bloessl <bloessl@ccs-labs.org>
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
 * super version
 */

#include "frame_equalizer_impl.h"
#include "equalizer/base.h"
#include "equalizer/comb.h"
#include "equalizer/lms.h"
#include "equalizer/ls.h"
#include "equalizer/sta.h"
#include "utils.h"
#include <gnuradio/io_signature.h>
#include <ctime>
#include <sys/timeb.h>


namespace gr {
namespace ieee802_11 {

frame_equalizer::sptr
frame_equalizer::make(Equalizer algo, double freq, double bw, bool log, bool debug, unsigned int nin) {
	return gnuradio::get_initial_sptr
		(new frame_equalizer_impl(algo, freq, bw, log, debug, nin));
}


frame_equalizer_impl::frame_equalizer_impl(Equalizer algo, double freq, double bw, bool log, bool debug, unsigned int nin) :
	gr::block("frame_equalizer",
			gr::io_signature::make(nin+1, nin+1, 64 * sizeof(gr_complex)),
			gr::io_signature::make2(nin+1, nin+1, 48, 64 * sizeof(gr_complex))),
	d_current_symbol(0),d_nin(nin), d_log(log), d_debug(debug), d_equalizer(NULL),
	d_freq(freq), d_bw(bw), d_frame_bytes(0), d_frame_symbols(0),d_cnt(0),
	d_freq_offset_from_synclong(0.0) {

	message_port_register_out(pmt::mp("symbols"));

	d_bpsk = constellation_bpsk::make();
	d_qpsk = constellation_qpsk::make();
	d_16qam = constellation_16qam::make();
	d_64qam = constellation_64qam::make();

	d_frame_mod = d_bpsk;

	set_tag_propagation_policy(block::TPP_DONT);
	set_algorithm(algo);
	tags = new std::vector<gr::tag_t>[d_nin+1];
	d_freq_offset_from_synclong_pass = new double[d_nin];
	d_er_pass = new double[d_nin];
	d_epsilon0_pass = new double[d_nin];
	d_prev_pilots_pass = new gr_complex[d_nin][4];

	
	for(int i = 0; i < d_nin; ++i){
		d_freq_offset_from_synclong_pass[i] = 0.0;
		d_er_pass[i] = 0;
		d_epsilon0_pass[i] = 0;
	}
}

frame_equalizer_impl::~frame_equalizer_impl() {
	for(int i = 0; i < d_equalizer_pass.size(); ++i){
		delete d_equalizer_pass[i];
	}
	delete[] tags;
	delete[] d_freq_offset_from_synclong_pass;
	delete[] d_er_pass;
	delete[] d_epsilon0_pass;
}


void
frame_equalizer_impl::set_algorithm(Equalizer algo) {
	gr::thread::scoped_lock lock(d_mutex);
	delete d_equalizer;
	for(int i = 0; i < d_equalizer_pass.size(); ++i){
		delete d_equalizer_pass[i];
	}
	d_equalizer_pass.clear();

	switch(algo) {

	case COMB:
		dout << "Comb" << std::endl;
		d_equalizer = new equalizer::comb();
		for(int i = 0; i < d_nin; ++i){
			d_equalizer_pass.push_back(new equalizer::comb());
		}
		break;
	case LS:
		dout << "LS" << std::endl;
		d_equalizer = new equalizer::ls();
		for(int i = 0; i < d_nin; ++i){
			d_equalizer_pass.push_back(new equalizer::ls());
		}
		break;
	case LMS:
		dout << "LMS" << std::endl;
		d_equalizer = new equalizer::lms();
		for(int i = 0; i < d_nin; ++i){
			d_equalizer_pass.push_back(new equalizer::lms());
		}
		break;
	case STA:
		dout << "STA" << std::endl;
		d_equalizer = new equalizer::sta();
		for(int i = 0; i < d_nin; ++i){
			d_equalizer_pass.push_back(new equalizer::sta());
		}
		break;
	default:
		throw std::runtime_error("Algorithm not implemented");
	}
}

void
frame_equalizer_impl::set_bandwidth(double bw) {
	gr::thread::scoped_lock lock(d_mutex);
	d_bw = bw;
}

void
frame_equalizer_impl::set_frequency(double freq) {
	gr::thread::scoped_lock lock(d_mutex);
	d_freq = freq;
}

void
frame_equalizer_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required) {
	ninput_items_required[0] = noutput_items;
	for(int i = 0; i < d_nin; i++){
		ninput_items_required[i+1] = noutput_items;
	}
}

int
frame_equalizer_impl::general_work (int noutput_items,
		gr_vector_int &ninput_items,
		gr_vector_const_void_star &input_items,
		gr_vector_void_star &output_items) {

	gr::thread::scoped_lock lock(d_mutex);

	const gr_complex *in = (const gr_complex *) input_items[0];
	uint8_t *out = (uint8_t *) output_items[0];
	const gr_complex *pass[d_nin];
	gr_complex *H_out[d_nin];
	for(int i = 0; i < d_nin; i++){
		pass[i] = (const gr_complex *) input_items[i+1];
		H_out[i] = (gr_complex *) output_items[i+1];
	}

	int i = 0;//, i_p = 0;
	int o = 0;
	int hh=0;
	gr_complex symbols[48];
	gr_complex current_symbol[64];  //use for 0 channel
	gr_complex current_symbol_pass[d_nin][64]; //use for other channel
	gr_complex tmp_current_symbol_pass[64];
	gr_complex *temp;

	//dout << "FRAME EQUALIZER: input " << ninput_items[0] << "  output " << noutput_items << std::endl;

	while((i < ninput_items[0]) && (o < noutput_items) && (hh < noutput_items)) {
		int ind = 0;
		for (; ind < d_nin; ind++){
			if(i >= ninput_items[ind+1]){
				break;
			}
		}
		if(ind < d_nin){
			break;
		}
		
		get_tags_in_window(tags[0], 0, i, i + 1, pmt::string_to_symbol("wifi_start"));
		for(int ii = 0; ii < d_nin; ++ii){
			get_tags_in_window(tags[ii+1], ii, i, i + 1, pmt::string_to_symbol("wifi_start"));
		}

		// new frame
		if(tags[0].size()) {
			// debug
			// std::cout << "Equalizer, compared to LONG, " << pass[0][d_current_symbol] << ", " << pass[0][d_current_symbol-1] << ", " <<pass[0][d_current_symbol-2] << std::endl;
			
			d_current_symbol = 0;
			d_frame_symbols = 0;
			d_frame_mod = d_bpsk;

			d_freq_offset_from_synclong = pmt::to_double(tags[0].front().value) * d_bw / (2 * M_PI);
			d_epsilon0 = pmt::to_double(tags[0].front().value) * d_bw / (2 * M_PI * d_freq);
			d_er = 0;
			for(int ii = 0; ii < d_nin; ++ii){
				d_freq_offset_from_synclong_pass[ii] = pmt::to_double(tags[ii+1].front().value) * d_bw / (2 * M_PI);
				d_epsilon0_pass[ii] = pmt::to_double(tags[ii+1].front().value) * d_bw / (2 * M_PI * d_freq);
				d_er_pass[ii] = 0;
			}

			//dout << "epsilon: " << d_epsilon0 << std::endl;

		}


		// not interesting -> skip
		if(d_current_symbol > (d_frame_symbols + 2)) {
			dout<<"if is work!!!!"<<' '<<d_current_symbol<<' '<<d_frame_symbols<<std::endl;
			i++;
			continue;
		}
		

		std::memcpy(current_symbol, in + i*64, 64*sizeof(gr_complex));
		for(int ii = 0; ii < d_nin; ++ii){
			std::memcpy(current_symbol_pass[ii], pass[ii] + i*64, 64*sizeof(gr_complex));
		}

		// compensate sampling offset
		//for(int i = 0; i < 64; i++) {
		//	current_symbol[i] *= exp(gr_complex(0, 2*M_PI*d_current_symbol*80*(d_epsilon0 + d_er)*(i-32)/64));
		//}
		//for(int ii = 0; ii < d_nin; ++ii){
		//	for(int i = 0; i < 64; i++) {
		//		current_symbol_pass[ii][i] *= exp(gr_complex(0, 2*M_PI*d_current_symbol*80*(d_epsilon0_pass[ii] + d_er_pass[ii])*(i-32)/64));
		//	}
		//}

		gr_complex p = equalizer::base::POLARITY[(d_current_symbol - 2) % 127];

		double beta;
		double beta_pass[d_nin];
		if(d_current_symbol < 2) {
			beta = arg(
					current_symbol[11] -
					current_symbol[25] +
					current_symbol[39] +
					current_symbol[53]);
		} else {
			beta = arg(
					(current_symbol[11] *  p) +
					(current_symbol[39] *  p) +
					(current_symbol[25] *  p) +
					(current_symbol[53] * -p));
		}
		if(d_current_symbol < 2) {
			for(int ii = 0; ii < d_nin; ++ii){
				beta_pass[ii] = arg(
					current_symbol_pass[ii][11] -
					current_symbol_pass[ii][25] +
					current_symbol_pass[ii][39] +
					current_symbol_pass[ii][53]);
				
			}
		}else{
			for(int ii = 0; ii < d_nin; ++ii){
				beta_pass[ii] = arg(
					(current_symbol_pass[ii][11] *  p) +
					(current_symbol_pass[ii][39] *  p) +
					(current_symbol_pass[ii][25] *  p) +
					(current_symbol_pass[ii][53] * -p));
			}
		}
		

		double er = arg(
				(conj(d_prev_pilots[0]) * current_symbol[11] *  p) +
				(conj(d_prev_pilots[1]) * current_symbol[25] *  p) +
				(conj(d_prev_pilots[2]) * current_symbol[39] *  p) +
				(conj(d_prev_pilots[3]) * current_symbol[53] * -p));
		// DEBUG:
		// if(d_current_symbol < 10){
		// 	std::cout << "frame_equalizer debug: d_er_pass[0]: " << d_er_pass[0] << ", d_prev_pilots[0]: "
		//           	  << d_prev_pilots[0] << std::endl;
		// }
		double er_pass[d_nin];
		for(int ii = 0; ii < d_nin; ++ii){
			er_pass[ii] = arg(
				(conj(d_prev_pilots_pass[ii][0]) * current_symbol_pass[ii][11] *  p) +
				(conj(d_prev_pilots_pass[ii][1]) * current_symbol_pass[ii][25] *  p) +
				(conj(d_prev_pilots_pass[ii][2]) * current_symbol_pass[ii][39] *  p) +
				(conj(d_prev_pilots_pass[ii][3]) * current_symbol_pass[ii][53] * -p));
			er_pass[ii] *= d_bw / (2 * M_PI * d_freq * 80);
		} 

		er *= d_bw / (2 * M_PI * d_freq * 80);

		if(d_current_symbol < 2) {
				d_prev_pilots[0] = current_symbol[11];
				d_prev_pilots[1] = -current_symbol[25];
				d_prev_pilots[2] = current_symbol[39];
				d_prev_pilots[3] = current_symbol[53];
			for(int ii = 0; ii < d_nin; ++ii){
				d_prev_pilots_pass[ii][0] = current_symbol_pass[ii][11];
				d_prev_pilots_pass[ii][1] = current_symbol_pass[ii][25];
				d_prev_pilots_pass[ii][2] = current_symbol_pass[ii][39];
				d_prev_pilots_pass[ii][3] = current_symbol_pass[ii][53];
			}	
		} else {
			d_prev_pilots[0] = current_symbol[11] *  p;
			d_prev_pilots[1] = current_symbol[25] *  p;
			d_prev_pilots[2] = current_symbol[39] *  p;
			d_prev_pilots[3] = current_symbol[53] * -p;

			for(int ii = 0; ii < d_nin; ++ii){
				d_prev_pilots_pass[ii][0] = current_symbol_pass[ii][11] *  p;
				d_prev_pilots_pass[ii][1] = current_symbol_pass[ii][25] *  p;
				d_prev_pilots_pass[ii][2] = current_symbol_pass[ii][39] *  p;
				d_prev_pilots_pass[ii][3] = current_symbol_pass[ii][53] * -p;
			}
		}

		// compensate residual frequency offset
		for(int i = 0; i < 64; i++) {
			current_symbol[i] *= exp(gr_complex(0, -beta));
			//*(H_out[0]+hh*64+i) = exp(gr_complex(0, beta));
		}

		
		for(int ii = 0; ii < d_nin; ++ii){
			for(int i = 0; i < 64; i++) {
				current_symbol_pass[ii][i] *= exp(gr_complex(0, -beta_pass[ii]));
				*(H_out[ii]+hh*64+i) = exp(gr_complex(0, beta_pass[ii]));
			}
		}

		// dout<<"should be same"<<'\n';
		// dout<<"current symbol: "<< current_symbol[0]<<' '<< current_symbol[1]<<' '<<current_symbol[2]<<'\n';
		// dout<<"current symbol 1: "<< current_symbol_pass[0][0]<<' '<< current_symbol_pass[0][1]<<' '<<current_symbol_pass[0][2]<<'\n';

		// update estimate of residual frequency offset
		if(d_current_symbol >= 2) {

			double alpha = 0.1;
			d_er = (1-alpha) * d_er + alpha * er;
			for(int ii = 0; ii < d_nin; ++ii){
				d_er_pass[ii] = (1-alpha) * d_er_pass[ii] + alpha * er_pass[ii];
			}
		}


		//do equalization
		d_equalizer->equalize(current_symbol, d_current_symbol, symbols, out + o * 48, d_frame_mod);
		//save channel estimation
		temp = d_equalizer->get_channel_esti();
		// for(int k=0;k<64;k++){	
		// 	*(H_out[0]+k) *= *(temp + k);
			
		// }
		dout<<"d_current_symbol: "<<d_current_symbol<<std::endl;
		dout<<"d_frame_symbols: "<<d_frame_symbols<<std::endl;
		for(int i=0;i<d_nin;++i){
			for(int kk=0;kk<64;++kk){
				tmp_current_symbol_pass[kk]=current_symbol_pass[i][kk];
			}
			d_equalizer_pass[i]->equalize(tmp_current_symbol_pass, d_current_symbol, symbols, out + o * 48, d_frame_mod);
			temp = d_equalizer_pass[i]->get_channel_esti();
			//dout<<"I will output to H_out "<<i<<std::endl;
			for(int k=0;k<64;k++){
					*(H_out[i] + hh*64 + k) *= *(temp + k);
			}
			*(H_out[i]+hh*64+59) = gr_complex(d_equalizer_pass[i]->get_snr(),0); 
			std::cout<<"Get a package!!!"<<"it number is:"<<i<<" Its snr is "<<d_equalizer_pass[i]->get_snr()<<std::endl;
			std::time_t now = time(0);
			std::tm *ltm = localtime(&now);
			struct timeb tb;
			ftime(&tb);
			*(H_out[i]+hh*64+60) = gr_complex(ltm->tm_hour,0);
			*(H_out[i]+hh*64+61) = gr_complex(ltm->tm_min,0);
			*(H_out[i]+hh*64+62) = gr_complex(ltm->tm_sec,0);
			*(H_out[i]+hh*64+63) = gr_complex(tb.millitm,0);
		}

		// signal field
		if(d_current_symbol == 2) {
			
			if(decode_signal_field(out + o * 48)) {

				pmt::pmt_t dict = pmt::make_dict();
				dict = pmt::dict_add(dict, pmt::mp("frame_bytes"), pmt::from_uint64(d_frame_bytes));
				dict = pmt::dict_add(dict, pmt::mp("encoding"), pmt::from_uint64(d_frame_encoding));
				dict = pmt::dict_add(dict, pmt::mp("snr"), pmt::from_double(d_equalizer->get_snr()));
				dict = pmt::dict_add(dict, pmt::mp("freq"), pmt::from_double(d_freq));
				dict = pmt::dict_add(dict, pmt::mp("freq_offset"), pmt::from_double(d_freq_offset_from_synclong));
				add_item_tag(0, nitems_written(0) + o,
						pmt::string_to_symbol("wifi_start"),
						dict,
						pmt::string_to_symbol(alias()));

				//const pmt::pmt_t key = pmt::string_to_symbol("wifi_lts_start");
	        	//const pmt::pmt_t value = pmt::from_double(0);
	        	//const pmt::pmt_t srcid = pmt::string_to_symbol(name());
          		//add_item_tag(1, nitems_written(1)+o, key, value, srcid);
				
			}
		}

		if(d_current_symbol > 2) {
			o++;
			// i_p++;
			pmt::pmt_t pdu = pmt::make_dict();
			message_port_pub(pmt::mp("symbols"), pmt::cons(pmt::make_dict(), pmt::init_c32vector(48, symbols)));
		}
		

		i++;
		hh++;
		d_current_symbol++;
	}

	consume_each(i);
	return o;
}

bool
frame_equalizer_impl::decode_signal_field(uint8_t *rx_bits) {

	static ofdm_param ofdm(BPSK_1_2);
	static frame_param frame(ofdm, 0);

	deinterleave(rx_bits);
	uint8_t *decoded_bits = d_decoder.decode(&ofdm, &frame, d_deinterleaved);

	return parse_signal(decoded_bits);
}

void
frame_equalizer_impl::deinterleave(uint8_t *rx_bits) {
	for(int i = 0; i < 48; i++) {
		d_deinterleaved[i] = rx_bits[interleaver_pattern[i]];
	}
}

bool
frame_equalizer_impl::parse_signal(uint8_t *decoded_bits) {

	int r = 0;
	d_frame_bytes = 0;
	bool parity = false;
	for(int i = 0; i < 17; i++) {
		parity ^= decoded_bits[i];

		if((i < 4) && decoded_bits[i]) {
			r = r | (1 << i);
		}

		if(decoded_bits[i] && (i > 4) && (i < 17)) {
			d_frame_bytes = d_frame_bytes | (1 << (i-5));
		}
	}

	if(parity != decoded_bits[17]) {
		dout << "SIGNAL: wrong parity" << std::endl;
		return false;
	}

	switch(r) {
	case 11:
		d_frame_encoding = 0;
		d_frame_symbols = (int) ceil((16 + 8 * d_frame_bytes + 6) / (double) 24);
		d_frame_mod = d_bpsk;
		dout << "Encoding: 3 Mbit/s   ";
		break;
	case 15:
		d_frame_encoding = 1;
		d_frame_symbols = (int) ceil((16 + 8 * d_frame_bytes + 6) / (double) 36);
		d_frame_mod = d_bpsk;
		dout << "Encoding: 4.5 Mbit/s   ";
		break;
	case 10:
		d_frame_encoding = 2;
		d_frame_symbols = (int) ceil((16 + 8 * d_frame_bytes + 6) / (double) 48);
		d_frame_mod = d_qpsk;
		dout << "Encoding: 6 Mbit/s   ";
		break;
	case 14:
		d_frame_encoding = 3;
		d_frame_symbols = (int) ceil((16 + 8 * d_frame_bytes + 6) / (double) 72);
		d_frame_mod = d_qpsk;
		dout << "Encoding: 9 Mbit/s   ";
		break;
	case 9:
		d_frame_encoding = 4;
		d_frame_symbols = (int) ceil((16 + 8 * d_frame_bytes + 6) / (double) 96);
		d_frame_mod = d_16qam;
		dout << "Encoding: 12 Mbit/s   ";
		break;
	case 13:
		d_frame_encoding = 5;
		d_frame_symbols = (int) ceil((16 + 8 * d_frame_bytes + 6) / (double) 144);
		d_frame_mod = d_16qam;
		dout << "Encoding: 18 Mbit/s   ";
		break;
	case 8:
		d_frame_encoding = 6;
		d_frame_symbols = (int) ceil((16 + 8 * d_frame_bytes + 6) / (double) 192);
		d_frame_mod = d_64qam;
		dout << "Encoding: 24 Mbit/s   ";
		break;
	case 12:
		d_frame_encoding = 7;
		d_frame_symbols = (int) ceil((16 + 8 * d_frame_bytes + 6) / (double) 216);
		d_frame_mod = d_64qam;
		dout << "Encoding: 27 Mbit/s   ";
		break;
	default:
		dout << "unknown encoding" << std::endl;
		return false;
	}

	// mylog(boost::format("encoding: %1% - length: %2% - symbols: %3%")
	// 		% d_frame_encoding % d_frame_bytes % d_frame_symbols);
	return true;
}

const int
frame_equalizer_impl::interleaver_pattern[48] = {
	 0, 3, 6, 9,12,15,18,21,
	24,27,30,33,36,39,42,45,
	 1, 4, 7,10,13,16,19,22,
	25,28,31,34,37,40,43,46,
	 2, 5, 8,11,14,17,20,23,
	26,29,32,35,38,41,44,47};

} /* namespace ieee802_11 */
} /* namespace gr */
