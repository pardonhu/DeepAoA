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


#ifndef INCLUDED_MYMOD_CP2OFFS_H
#define INCLUDED_MYMOD_CP2OFFS_H

#include <MyMod/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace MyMod {

    /*!
     * \brief <+description of block+>
     * \ingroup MyMod
     *
     */
    class MYMOD_API cp2offs : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<cp2offs> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of MyMod::cp2offs.
       *
       * To avoid accidental use of raw pointers, MyMod::cp2offs's
       * constructor is in a private implementation
       * class. MyMod::cp2offs::make is the public interface for
       * creating new instances.
       */
      static sptr make(int nin = 2, int window_size = 1000, bool en = false);

      virtual void set_en(bool en) = 0;
    };

  } // namespace MyMod
} // namespace gr

#endif /* INCLUDED_MYMOD_CP2OFFS_H */

