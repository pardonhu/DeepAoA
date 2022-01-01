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


#ifndef INCLUDED_MYMOD_EXTRACT_LTS_H
#define INCLUDED_MYMOD_EXTRACT_LTS_H

#include <MyMod/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace MyMod {

    /*!
     * \brief <+description of block+>
     * \ingroup MyMod
     *
     */
    class MYMOD_API extract_lts : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<extract_lts> sptr;

      virtual void set_en(bool en) = 0;

      /*!
       * \brief Return a shared_ptr to a new instance of MyMod::extract_lts.
       *
       * To avoid accidental use of raw pointers, MyMod::extract_lts's
       * constructor is in a private implementation
       * class. MyMod::extract_lts::make is the public interface for
       * creating new instances.
       */
      static sptr make(int nin = 2, int mode = 1, bool en = false, int ext_len = 128, int off = 0);
    };

  } // namespace MyMod
} // namespace gr

#endif /* INCLUDED_MYMOD_EXTRACT_LTS_H */

