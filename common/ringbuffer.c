/*
  Copyright (C) 2000 Paul Davis
  Copyright (C) 2003 Rohan Drape

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

  ISO/POSIX C version of Paul Davis's lock free ringbuffer C++ code.
  This is safe for the case of one read thread and one write thread.
*/

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#ifdef USE_MLOCK
#include <sys/mman.h>
#endif /* USE_MLOCK */
#include "jack/ringbuffer.h"
#include "JackCompilerDeps.h"

/* Create a new ringbuffer to hold at least `sz' bytes of data. The
   actual buffer size is rounded up to the next power of two.  */

EXPORT jack_ringbuffer_t *
jack_ringbuffer_create (size_t sz)
{
  int power_of_two;
  jack_ringbuffer_t *rb;

  rb = (jack_ringbuffer_t*)malloc (sizeof (jack_ringbuffer_t));

  for (power_of_two = 1; 1 << power_of_two < sz; power_of_two++);

  rb->size = 1 << power_of_two;
  rb->size_mask = rb->size;
  rb->size_mask -= 1;
  rb->write_ptr = 0;
  rb->read_ptr = 0;
  rb->buf = (char*)malloc (rb->size);
  memset(rb->buf, 0, rb->size);
  rb->mlocked = 0;

  return rb;
}

/* Free all data associated with the ringbuffer `rb'. */

EXPORT void
jack_ringbuffer_free (jack_ringbuffer_t * rb)
{
#ifdef USE_MLOCK
  if (rb->mlocked) {
    munlock (rb->buf, rb->size);
  }
#endif /* USE_MLOCK */
  free (rb->buf);
  free (rb);
}

/* Lock the data block of `rb' using the system call 'mlock'.  */

EXPORT int
jack_ringbuffer_mlock (jack_ringbuffer_t * rb)
{
#ifdef USE_MLOCK
  if (mlock (rb->buf, rb->size)) {
    return -1;
  }
#endif /* USE_MLOCK */
  rb->mlocked = 1;
  return 0;
}

/* Reset the read and write pointers to zero. This is not thread
   safe. */

EXPORT void
jack_ringbuffer_reset (jack_ringbuffer_t * rb)
{
  rb->read_ptr = 0;
  rb->write_ptr = 0;
}

/* Return the number of bytes available for reading.  This is the
   number of bytes in front of the read pointer and behind the write
   pointer.  */

EXPORT size_t
jack_ringbuffer_read_space (const jack_ringbuffer_t * rb)
{
  size_t w, r;

  w = rb->write_ptr;
  r = rb->read_ptr;

  if (w > r) {
    return w - r;
  } else {
    return (w - r + rb->size) & rb->size_mask;
  }
}

/* Return the number of bytes available for writing.  This is the
   number of bytes in front of the write pointer and behind the read
   pointer.  */

EXPORT size_t
jack_ringbuffer_write_space (const jack_ringbuffer_t * rb)
{
  size_t w, r;

  w = rb->write_ptr;
  r = rb->read_ptr;

  if (w > r) {
    return ((r - w + rb->size) & rb->size_mask) - 1;
  } else if (w < r) {
    return (r - w) - 1;
  } else {
    return rb->size - 1;
  }
}

/* The copying data reader.  Copy at most `cnt' bytes from `rb' to
   `dest'.  Returns the actual number of bytes copied. */

EXPORT size_t
jack_ringbuffer_read (jack_ringbuffer_t * rb, char *dest, size_t cnt)
{
  size_t free_cnt;
  size_t cnt2;
  size_t to_read;
  size_t n1, n2;

  if ((free_cnt = jack_ringbuffer_read_space (rb)) == 0) {
    return 0;
  }

  to_read = cnt > free_cnt ? free_cnt : cnt;

  cnt2 = rb->read_ptr + to_read;

  if (cnt2 > rb->size) {
    n1 = rb->size - rb->read_ptr;
    n2 = cnt2 & rb->size_mask;
  } else {
    n1 = to_read;
    n2 = 0;
  }

  memcpy (dest, &(rb->buf[rb->read_ptr]), n1);
  rb->read_ptr += n1;
  rb->read_ptr &= rb->size_mask;

  if (n2) {
    memcpy (dest + n1, &(rb->buf[rb->read_ptr]), n2);
    rb->read_ptr += n2;
    rb->read_ptr &= rb->size_mask;
  }

  return to_read;
}

/* The copying data reader w/o read pointer advance.  Copy at most
   `cnt' bytes from `rb' to `dest'.  Returns the actual number of bytes
copied. */

EXPORT size_t
jack_ringbuffer_peek (jack_ringbuffer_t * rb, char *dest, size_t cnt)
{
  size_t free_cnt;
  size_t cnt2;
  size_t to_read;
  size_t n1, n2;
  size_t tmp_read_ptr;

  tmp_read_ptr = rb->read_ptr;

  if ((free_cnt = jack_ringbuffer_read_space (rb)) == 0) {
    return 0;
  }

  to_read = cnt > free_cnt ? free_cnt : cnt;

  cnt2 = tmp_read_ptr + to_read;

  if (cnt2 > rb->size) {
    n1 = rb->size - tmp_read_ptr;
    n2 = cnt2 & rb->size_mask;
  } else {
    n1 = to_read;
    n2 = 0;
  }

  memcpy (dest, &(rb->buf[tmp_read_ptr]), n1);
  tmp_read_ptr += n1;
  tmp_read_ptr &= rb->size_mask;

  if (n2) {
    memcpy (dest + n1, &(rb->buf[tmp_read_ptr]), n2);
    tmp_read_ptr += n2;
    tmp_read_ptr &= rb->size_mask;
  }

  return to_read;
}

/* The copying data writer.  Copy at most `cnt' bytes to `rb' from
   `src'.  Returns the actual number of bytes copied. */

EXPORT size_t
jack_ringbuffer_write (jack_ringbuffer_t * rb, const char *src, size_t cnt)
{
  size_t free_cnt;
  size_t cnt2;
  size_t to_write;
  size_t n1, n2;

  if ((free_cnt = jack_ringbuffer_write_space (rb)) == 0) {
    return 0;
  }

  to_write = cnt > free_cnt ? free_cnt : cnt;

  cnt2 = rb->write_ptr + to_write;

  if (cnt2 > rb->size) {
    n1 = rb->size - rb->write_ptr;
    n2 = cnt2 & rb->size_mask;
  } else {
    n1 = to_write;
    n2 = 0;
  }

  memcpy (&(rb->buf[rb->write_ptr]), src, n1);
  rb->write_ptr += n1;
  rb->write_ptr &= rb->size_mask;

  if (n2) {
    memcpy (&(rb->buf[rb->write_ptr]), src + n1, n2);
    rb->write_ptr += n2;
    rb->write_ptr &= rb->size_mask;
  }

  return to_write;
}

/* Advance the read pointer `cnt' places. */

EXPORT void
jack_ringbuffer_read_advance (jack_ringbuffer_t * rb, size_t cnt)
{
  rb->read_ptr += cnt;
  rb->read_ptr &= rb->size_mask;
}

/* Advance the write pointer `cnt' places. */

EXPORT void
jack_ringbuffer_write_advance (jack_ringbuffer_t * rb, size_t cnt)
{
  rb->write_ptr += cnt;
  rb->write_ptr &= rb->size_mask;
}

/* The non-copying data reader.  `vec' is an array of two places.  Set
   the values at `vec' to hold the current readable data at `rb'.  If
   the readable data is in one segment the second segment has zero
   length.  */

EXPORT void
jack_ringbuffer_get_read_vector (const jack_ringbuffer_t * rb,
				 jack_ringbuffer_data_t * vec)
{
  size_t free_cnt;
  size_t cnt2;
  size_t w, r;

  w = rb->write_ptr;
  r = rb->read_ptr;

  if (w > r) {
    free_cnt = w - r;
  } else {
    free_cnt = (w - r + rb->size) & rb->size_mask;
  }

  cnt2 = r + free_cnt;

  if (cnt2 > rb->size) {

    /* Two part vector: the rest of the buffer after the current write
       ptr, plus some from the start of the buffer. */

    vec[0].buf = &(rb->buf[r]);
    vec[0].len = rb->size - r;
    vec[1].buf = rb->buf;
    vec[1].len = cnt2 & rb->size_mask;

  } else {

    /* Single part vector: just the rest of the buffer */

    vec[0].buf = &(rb->buf[r]);
    vec[0].len = free_cnt;
    vec[1].len = 0;
  }
}

/* The non-copying data writer.  `vec' is an array of two places.  Set
   the values at `vec' to hold the current writeable data at `rb'.  If
   the writeable data is in one segment the second segment has zero
   length.  */

EXPORT void
jack_ringbuffer_get_write_vector (const jack_ringbuffer_t * rb,
				  jack_ringbuffer_data_t * vec)
{
  size_t free_cnt;
  size_t cnt2;
  size_t w, r;

  w = rb->write_ptr;
  r = rb->read_ptr;

  if (w > r) {
    free_cnt = ((r - w + rb->size) & rb->size_mask) - 1;
  } else if (w < r) {
    free_cnt = (r - w) - 1;
  } else {
    free_cnt = rb->size - 1;
  }

  cnt2 = w + free_cnt;

  if (cnt2 > rb->size) {

    /* Two part vector: the rest of the buffer after the current write
       ptr, plus some from the start of the buffer. */

    vec[0].buf = &(rb->buf[w]);
    vec[0].len = rb->size - w;
    vec[1].buf = rb->buf;
    vec[1].len = cnt2 & rb->size_mask;
  } else {
    vec[0].buf = &(rb->buf[w]);
    vec[0].len = free_cnt;
    vec[1].len = 0;
  }
}
