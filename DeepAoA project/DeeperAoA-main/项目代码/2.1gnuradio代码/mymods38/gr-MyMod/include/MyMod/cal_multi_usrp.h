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


#ifndef INCLUDED_MYMOD_CAL_MULTI_USRP_H
#define INCLUDED_MYMOD_CAL_MULTI_USRP_H

#include <MyMod/api.h>
#include <gnuradio/block.h>
#include <gnuradio/math.h>

namespace gr {
  namespace MyMod {

    /*!
     * cal_multi_usrp used for calibration multiple sync usrps, compensating the
     * phase offset between each input. When this block is enabled, it calculate the
     * offset using method 0 or 1, then do the compensation. When it's diabled, it using
     * the stored offset to do the compensation.
     */ 
    class MYMOD_API cal_multi_usrp : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<cal_multi_usrp> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of MyMod::cal_multi_usrp.
       *
       * To avoid accidental use of raw pointers, MyMod::cal_multi_usrp's
       * constructor is in a private implementation
       * class. MyMod::cal_multi_usrp::make is the public interface for
       * creating new instances.
       */
      static sptr make(unsigned int nin = 2, unsigned int cal_time = 64, bool en = false, unsigned int method = 0);

      virtual void set_en(bool en) = 0;
    };

  } // namespace MyMod
} // namespace gr

#endif /* INCLUDED_MYMOD_CAL_MULTI_USRP_H */

