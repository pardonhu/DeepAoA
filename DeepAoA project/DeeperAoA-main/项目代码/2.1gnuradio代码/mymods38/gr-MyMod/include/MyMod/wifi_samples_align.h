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


#ifndef INCLUDED_MYMOD_WIFI_SAMPLES_ALIGN_H
#define INCLUDED_MYMOD_WIFI_SAMPLES_ALIGN_H

#include <MyMod/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace MyMod {

    /*!
     * \brief <+description of block+>
     * \ingroup MyMod
     *
     */
    class MYMOD_API wifi_samples_align : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<wifi_samples_align> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of MyMod::wifi_samples_align.
       *
       * To avoid accidental use of raw pointers, MyMod::wifi_samples_align's
       * constructor is in a private implementation
       * class. MyMod::wifi_samples_align::make is the public interface for
       * creating new instances.
       */
      static sptr make();
    };

  } // namespace MyMod
} // namespace gr

#endif /* INCLUDED_MYMOD_WIFI_SAMPLES_ALIGN_H */

