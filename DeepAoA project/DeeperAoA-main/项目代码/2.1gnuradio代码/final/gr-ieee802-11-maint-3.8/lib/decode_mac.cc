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
#include <ieee802_11/decode_mac.h>

#include "utils.h"
#include "viterbi_decoder/viterbi_decoder.h"

#include <boost/crc.hpp>
#include <gnuradio/io_signature.h>

using namespace gr::ieee802_11;

#define LINKTYPE_IEEE802_11 105 /* http://www.tcpdump.org/linktypes.html */

// static int i_s [] = {48, 64 * sizeof(gr_complex), 64 * sizeof(gr_complex), 64 * sizeof(gr_complex)};
// static std::vector<int> i_sig(i_s, i_s+sizeof(i_s)/sizeof(int));
// std::vector<int> i_sig{48, 64*sizeof(gr_complex)};

class decode_mac_impl : public decode_mac {

public:
decode_mac_impl(bool log, bool debug, int nin) :
	block("decode_mac",
			gr::io_signature::make2(3*nin+1, 3*nin+1, 48, 64*sizeof(gr_complex)),
			gr::io_signature::make(2*nin, 2*nin, 64*sizeof(gr_complex))),
	d_log(log),
	d_debug(debug),
	d_nin(nin),
	d_snr(0),
	d_nom_freq(0.0),
	d_freq_offset(0.0),
	d_ofdm(BPSK_1_2),
	d_frame(d_ofdm, 0),
	d_frame_complete(true),
	out_cnt(0) {

	message_port_register_out(pmt::mp("out"));
	d_pass = new gr_complex*[nin];
	H = new gr_complex*[nin];
	H_avg = new gr_complex*[nin];
	
	for(int i = 0; i < d_nin; i++){
		d_pass[i] = new gr_complex[64 * MAX_SYM];
		H[i] = new gr_complex[64 * MAX_SYM];
		H_avg[i] = new gr_complex[64 * MAX_SYM];
	}
	// for (int i = 0; i < nin; i++){
	// 	std::string t_pname = "pass_out" + i;
	// 	std::cout << "debug: port name: " << t_pname << std::endl;
	// 	message_port_register_out(pmt::mp("t_pname"));
	// }
	// message_port_register_out(pmt::mp("pass_out"));
	// message_port_register_out(pmt::mp("pass_out2"));
	// message_port_register_out(pmt::mp("pass_out3"));
	message_port_register_out(pmt::mp("pass_out"));
}

~decode_mac_impl(){
	for(int i = 0; i < d_nin; i++){
		delete[] d_pass[i];
		delete[] H[i];
		delete[] H_avg[i];
	}
	delete[] d_pass;
	delete[] H;
	delete[] H_avg;
}

int general_work (int noutput_items, gr_vector_int& ninput_items,
		gr_vector_const_void_star& input_items,
		gr_vector_void_star& output_items) {

	const uint8_t *in = (const uint8_t*)input_items[0];

	const gr_complex *pass[d_nin];
	const gr_complex *H_pass[d_nin];
	const gr_complex *H_avg_pass[d_nin];
	gr_complex *H_out[d_nin];
	gr_complex *H_avg_out[d_nin];

	for(int i = 0; i < d_nin; i++){
		pass[i] = (const gr_complex*)input_items[i+1];
		H_pass[i] = (const gr_complex*)input_items[i+1+d_nin];
		H_avg_pass[i] = (const gr_complex*)input_items[i+1+2*d_nin];
		H_out[i] = (gr_complex*)output_items[i];
		H_avg_out[i] = (gr_complex*)output_items[i+d_nin];
	}
	// unsigned int *out_vlen = (unsigned int *)output_items[0];
	

	int i = 0;
	int o = 0;

	std::vector<gr::tag_t> tags;
	const uint64_t nread = this->nitems_read(0);

	dout << "Decode MAC: input " << ninput_items[0] << std::endl;

	// debug
	// bool ttt=true;

	// while(i < ninput_items[0] && o < noutput_items) {
	while(i < ninput_items[0]) {
		int ind = 0;
		for (; ind < d_nin; ind++){
			if(i >= ninput_items[ind+1]){
				break;
			}
		}
		if(ind < d_nin){
			break;
		}

		get_tags_in_range(tags, 0, nread + i, nread + i + 1,
			pmt::string_to_symbol("wifi_start"));

		if(tags.size()) {
			if (d_frame_complete == false) {
				dout << "Warning: starting to receive new frame before old frame was complete" << std::endl;
				dout << "Already copied " << copied << " out of " << d_frame.n_sym << " symbols of last frame" << std::endl;
			}
			d_frame_complete = false;

			pmt::pmt_t dict = tags[0].value;
			int len_data = pmt::to_uint64(pmt::dict_ref(dict, pmt::mp("frame_bytes"), pmt::from_uint64(MAX_PSDU_SIZE+1)));
			int encoding = pmt::to_uint64(pmt::dict_ref(dict, pmt::mp("encoding"), pmt::from_uint64(0)));
			d_snr = pmt::to_double(pmt::dict_ref(dict, pmt::mp("snr"), pmt::from_double(0)));
			d_nom_freq = pmt::to_double(pmt::dict_ref(dict, pmt::mp("freq"), pmt::from_double(0)));
			d_freq_offset = pmt::to_double(pmt::dict_ref(dict, pmt::mp("freq_offset"), pmt::from_double(0)));

			ofdm_param ofdm = ofdm_param((Encoding)encoding);
			frame_param frame = frame_param(ofdm, len_data);
			
			
			// check for maximum frame size
			if(frame.n_sym <= MAX_SYM && frame.psdu_size <= MAX_PSDU_SIZE) {
				d_ofdm = ofdm;
				d_frame = frame;
				copied = 0;
				dout << "Decode MAC: frame start -- len " << len_data
					<< "  symbols " << frame.n_sym << "  encoding "
					<< encoding << std::endl;
			} else {
				dout << "Dropping frame which is too large (symbols or bits)" << std::endl;
			}
		}
		
		if(copied < d_frame.n_sym) {
			dout << "copy one symbol, copied " << copied << " out of " << d_frame.n_sym << std::endl;
			std::memcpy(d_rx_symbols + (copied * 48), in, 48);
			for (int p = 0; p < d_nin; p++){
				std::memcpy(d_pass[p] + (copied * 64), pass[p], 64*sizeof(gr_complex));
				std::memcpy(H[p] + (copied * 64), H_pass[p], 64*sizeof(gr_complex));
				std::memcpy(H_avg[p] + (copied * 64), H_avg_pass[p], 64*sizeof(gr_complex));
			}

			copied++;

			if(copied == d_frame.n_sym) {
				dout << "received complete frame - decoding" << std::endl;
				// std::cout << "received complete frame - decoding" << std::endl;
				bool t = decode();
				// for(int ii = 0; ii < d_nin; ++ii){
				// 	std::memcpy(H[ii], H_pass[ii], 64*sizeof(gr_complex));
				// }
				in += 48;
				for (int p = 0; p < d_nin; p++){
					pass[p] += 64;
					H_pass[p] += 64;
					H_avg_pass[p] += 64;
				}

				i++;
				d_frame_complete = true;

				if(t == true){
					m_out(copied);
					out_cnt = copied;
					const pmt::pmt_t key = pmt::string_to_symbol("num_samples_packet");
	        		const pmt::pmt_t value = pmt::from_double(64*copied);
	        		const pmt::pmt_t srcid = pmt::string_to_symbol(name());
          			add_item_tag(0, nitems_written(0), key, value, srcid);
					add_item_tag(d_nin, nitems_written(d_nin), key, value, srcid);
				}
				
				break;
			}
		}

		in += 48;

		for (int p = 0; p < d_nin; p++){
			(pass[p]) += 64;
			(H_pass[p]) += 64;
			(H_avg_pass[p]) += 64;
		}
		i++;
		if(out_cnt > 0){
			for(int ii = 0; ii < d_nin; ++ii){
				std::memcpy(H_out[ii], H[ii], out_cnt*64*sizeof(gr_complex));
				std::memcpy(H_avg_out[ii], H_avg[ii], out_cnt*64*sizeof(gr_complex));
			}
			// o++;
			// out_cnt--;
			std::cout << "decode_mac: " << out_cnt << std::endl;
			o += out_cnt;
			out_cnt = 0;
		}
	}

	consume_each(i);

	return o;
}

void m_out(unsigned int vlen){
	pmt::pmt_t metadata = pmt::make_dict();
	metadata = pmt::dict_add(metadata, pmt::mp("len"), pmt::from_uint64(64*vlen));
	metadata = pmt::dict_add(metadata, pmt::mp("nport"), pmt::from_uint64(d_nin));
	gr_complex* temp = new gr_complex[64*vlen*d_nin];
	for (int p = 0; p < d_nin; p++){
		std::memcpy(temp+64*vlen*p,d_pass[p],64*vlen*sizeof(gr_complex));
		// metadata = pmt::dict_add(metadata, pmt::mp("port"), pmt::from_uint64(p));
	}
	message_port_pub(pmt::mp("pass_out"), pmt::cons(metadata, pmt::init_c32vector(64*vlen*d_nin, temp)));
	delete[] temp;
}

// void forecast (int noutput_items, gr_vector_int &ninput_items_required) {

// 	// in sync state we need at least a symbol to correlate
// 	// with the pattern
// 	if(d_state == SYNC) {
// 		ninput_items_required[0] = 64;
// 		ninput_items_required[1] = 64;

// 	} else {
// 		ninput_items_required[0] = noutput_items;
// 		ninput_items_required[1] = noutput_items;
// 	}
// }


bool decode() {

	for(int i = 0; i < d_frame.n_sym * 48; i++) {
		for(int k = 0; k < d_ofdm.n_bpsc; k++) {
			d_rx_bits[i*d_ofdm.n_bpsc + k] = !!(d_rx_symbols[i] & (1 << k));
		}
	}

	deinterleave();
	uint8_t *decoded = d_decoder.decode(&d_ofdm, &d_frame, d_deinterleaved_bits);
	descramble(decoded);
	print_output();

	// skip service field
	boost::crc_32_type result;
	result.process_bytes(out_bytes + 2, d_frame.psdu_size);
	if(result.checksum() != 558161692) {
		dout << "checksum wrong -- dropping" << std::endl;
		return false;
	}

	mylog(boost::format("encoding: %1% - length: %2% - symbols: %3%")
			% d_ofdm.encoding % d_frame.psdu_size % d_frame.n_sym);

	// create PDU
	pmt::pmt_t blob = pmt::make_blob(out_bytes + 2, d_frame.psdu_size - 4);
	pmt::pmt_t enc = pmt::from_uint64(d_ofdm.encoding);
	pmt::pmt_t dict = pmt::make_dict();
	dict = pmt::dict_add(dict, pmt::mp("encoding"), enc);
	dict = pmt::dict_add(dict, pmt::mp("snr"), pmt::from_double(d_snr));
	dict = pmt::dict_add(dict, pmt::mp("nomfreq"), pmt::from_double(d_nom_freq));
	dict = pmt::dict_add(dict, pmt::mp("freqofs"), pmt::from_double(d_freq_offset));
	dict = pmt::dict_add(dict, pmt::mp("dlt"), pmt::from_long(LINKTYPE_IEEE802_11));
	message_port_pub(pmt::mp("out"), pmt::cons(dict, blob));
	return true;
}

void deinterleave() {

	int n_cbps = d_ofdm.n_cbps;
	int first[n_cbps];
	int second[n_cbps];
	int s = std::max(d_ofdm.n_bpsc / 2, 1);

	for(int j = 0; j < n_cbps; j++) {
		first[j] = s * (j / s) + ((j + int(floor(16.0 * j / n_cbps))) % s);
	}

	for(int i = 0; i < n_cbps; i++) {
		second[i] = 16 * i - (n_cbps - 1) * int(floor(16.0 * i / n_cbps));
	}

	int count = 0;
	for(int i = 0; i < d_frame.n_sym; i++) {
		for(int k = 0; k < n_cbps; k++) {
			d_deinterleaved_bits[i * n_cbps + second[first[k]]] = d_rx_bits[i * n_cbps + k];
		}
	}
}


void descramble (uint8_t *decoded_bits) {

	int state = 0;
	std::memset(out_bytes, 0, d_frame.psdu_size+2);

	for(int i = 0; i < 7; i++) {
		if(decoded_bits[i]) {
			state |= 1 << (6 - i);
		}
	}
	out_bytes[0] = state;

	int feedback;
	int bit;

	for(int i = 7; i < d_frame.psdu_size*8+16; i++) {
		feedback = ((!!(state & 64))) ^ (!!(state & 8));
		bit = feedback ^ (decoded_bits[i] & 0x1);
		out_bytes[i/8] |= bit << (i%8);
		state = ((state << 1) & 0x7e) | feedback;
	}
}

void print_output() {

	dout << std::endl;
	dout << "psdu size" << d_frame.psdu_size << std::endl;
	for(int i = 2; i < d_frame.psdu_size+2; i++) {
		dout << std::setfill('0') << std::setw(2) << std::hex << ((unsigned int)out_bytes[i] & 0xFF) << std::dec << " ";
		if(i % 16 == 15) {
			dout << std::endl;
		}
	}
	dout << std::endl;
	for(int i = 2; i < d_frame.psdu_size+2; i++) {
		if((out_bytes[i] > 31) && (out_bytes[i] < 127)) {
			dout << ((char) out_bytes[i]);
		} else {
			dout << ".";
		}
	}
	dout << std::endl;
}

private:
	bool d_debug;
	bool d_log;
	const int d_nin;
	int out_cnt;
	gr_complex **H;
	gr_complex **H_avg;

	frame_param d_frame;
	ofdm_param d_ofdm;
	double d_snr;  // dB
	double d_nom_freq;  // nominal frequency, Hz
	double d_freq_offset;  // frequency offset, Hz
	viterbi_decoder d_decoder;

	uint8_t d_rx_symbols[48 * MAX_SYM];
	uint8_t d_rx_bits[MAX_ENCODED_BITS];
	uint8_t d_deinterleaved_bits[MAX_ENCODED_BITS];
	uint8_t out_bytes[MAX_PSDU_SIZE + 2]; // 2 for signal field

	// gr_complex d_pass[64 * MAX_SYM];
	// gr_complex d_pass2[64 * MAX_SYM];
	// gr_complex d_pass3[64 * MAX_SYM];
	gr_complex** d_pass;;

	int copied;
	bool d_frame_complete;
};

decode_mac::sptr
decode_mac::make(bool log, bool debug, int nin) {
	return gnuradio::get_initial_sptr(new decode_mac_impl(log, debug, nin));
}
