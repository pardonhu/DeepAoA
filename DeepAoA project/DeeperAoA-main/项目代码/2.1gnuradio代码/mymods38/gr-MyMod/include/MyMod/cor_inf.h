/* -*- c++ -*- */
/* 
 * Copyright 2018 <+YOU OR YOUR COMPANY+>.
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


#ifndef INCLUDED_MYMOD_COR_INF_H
#define INCLUDED_MYMOD_COR_INF_H

#include <MyMod/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace MyMod {

    /*!
     * \brief <+description of block+>
     * \ingroup MyMod
     *
     */
    class MYMOD_API cor_inf : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<cor_inf> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of MyMod::cor_inf.
       *
       * To avoid accidental use of raw pointers, MyMod::cor_inf's
       * constructor is in a private implementation
       * class. MyMod::cor_inf::make is the public interface for
       * creating new instances.
       */
      static sptr make(int nin = 3, int mode = 1, int method = 0, bool en = false, bool finish = false, int chan = 178, int window_size = 64, int resolution = 30, bool phase_p2pi = false);

      virtual void set_en(bool en) = 0;
      virtual void set_finish(bool finish) = 0; 
      virtual void set_phase_p2pi(bool phase_p2pi) = 0;
    };

  } // namespace MyMod
} // namespace gr

#endif /* INCLUDED_MYMOD_COR_INF_H */

