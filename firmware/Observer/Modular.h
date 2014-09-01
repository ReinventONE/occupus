/*
 * Modular.h
 *
 *  Created on: Sep 1, 2014
 *      Author: Konstantin Gredeskoul
 *
 *  (c) 2014 All rights reserved.  Please see LICENSE.
 */

#ifndef MODULAR_H_
#define MODULAR_H_

#define SERIAL_LCD

/*
 * define either SENDER_DOWNSTAIRS or SENDER_UPSTAIRS depending on the unit.
 */
//#define SENDER_DOWNSTAIRS
#define SENDER_UPSTAIRS

#ifdef SENDER_DOWNSTAIRS
#define HAVE_ROTARY_KNOB
#endif

#endif /* MODULAR_H_ */
