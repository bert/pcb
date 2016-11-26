/*!
 * \file src/vector.h
 *
 * \brief Prototypes for vectors routines.
 *
 * \author this file, vector.c, was written and is
 * Copyright (c) 2001 C. Scott Ananian.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996 Thomas Nau
 *
 * Copyright (C) 1998,1999,2000,2001 harry eaton
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Contact addresses for paper mail and Email:
 *
 * harry eaton, 6697 Buttonhole Ct, Columbia, MD 21044 USA
 *
 * haceaton@aplcomm.jhuapl.edu
 */

#ifndef PCB_VECTOR_H
#define PCB_VECTOR_H

/*!
 * \brief What a vector looks like.
 */
typedef struct vector_struct vector_t;

/*!
 * \brief What data in a vector looks like.
 */
typedef void *vector_element_t;

vector_t *vector_create ();
void vector_destroy (vector_t ** vector);
vector_t *vector_duplicate (vector_t * vector);

/* -- interrogation -- */
int vector_is_empty (vector_t * vector);
int vector_size (vector_t * vector);
vector_element_t vector_element (vector_t * vector, int N);
vector_element_t vector_element_first (vector_t * vector);
vector_element_t vector_element_last (vector_t * vector);

/* -- mutation -- */
void vector_append (vector_t * vector, vector_element_t data);
void vector_append_many (vector_t * vector,
			 vector_element_t data[], int count);
void vector_append_vector (vector_t * vector, vector_t * other_vector);
void vector_insert (vector_t * vector, int N, vector_element_t data);
void vector_insert_many (vector_t * vector, int N,
			 vector_element_t data[], int count);
vector_element_t vector_remove_last (vector_t * vector);
vector_element_t vector_remove (vector_t * vector, int N);
vector_element_t vector_replace (vector_t * vector,
				 vector_element_t data, int N);

#endif /* PCB_VECTOR_H */
