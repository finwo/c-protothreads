/*
 * Copyright (c) 2004-2005, Swedish Institute of Computer Science.
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 * 3. Neither the name of the Institute nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software 
 *    without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE 
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
 * SUCH DAMAGE. 
 *
 * This file is part of the protothreads library.
 * 
 * Author: Adam Dunkels <adam@sics.se>
 *
 * $Id: pt.h,v 1.2 2005/02/24 10:36:59 adam Exp $
 */

/**
 * \addtogroup pt
 * @{
 */

/**
 * \file
 * Protothreads implementation.
 * \author
 * Adam Dunkels <adam@sics.se>
 *
 */

#ifndef __PT_H__
#define __PT_H__

#include "lc.h"

struct pt {
  lc_t lc;
};

#define PT_THREAD_WAITING 0
#define PT_THREAD_EXITED  1

/**
 * Declaration of a protothread.
 *
 * This macro is used to declare a protothread. All protothreads must
 * be declared with this macro.
 *
 * Example:
 \code
 PT_THREAD(consumer(struct pt *p, int event)) {
   PT_BEGIN(p);
   while(1) {
     PT_WAIT_UNTIL(event == AVAILABLE);
     consume();
     PT_WAIT_UNTIL(event == CONSUMED);
     acknowledge_consumed();
   }
   PT_END(p);
 }
 \endcode
 *
 * \param name_args The name and arguments of the C function
 * implementing the protothread.
 *
 * \hideinitializer
 */
#define PT_THREAD(name_args) char name_args

/**
 * Initialize a protothread.
 *
 * Initializes a protothread. Initialization must be done prior to
 * starting to execute the protothread.
 *
 * \param pt A pointer to the protothread control structure.
 *
 * Example:
 *
 \code
 void main(void) {
   struct pt p;
   int event;
   
   PT_INIT(&p);
   while(PT_SCHEDULE(consumer(&p, event))) {
     event = get_event();
   }
 }
 \endcode
 *
 * \hideinitializer
 */
#define PT_INIT(pt)				\
  LC_INIT((pt)->lc)

/**
 * Declare the start of a protothread inside the C function
 * implementing the protothread.
 *
 * This macro is used to declare the starting point of a
 * protothread. It should be placed at the start of the function in
 * which the protothread runs. All C statements above the PT_BEGIN()
 * invokation will be executed each time the protothread is scheduled.
 *
 * \param pt A pointer to the protothread control structure.
 *
 * Example:
 *
 \code
 PT_THREAD(producer(struct pt *p, int event)) {
   PT_BEGIN(p);
   while(1) {
     PT_WAIT_UNTIL(event == CONSUMED || event == DROPPED);
     produce();
     PT_WAIT_UNTIL(event == PRODUCED);
   }
   
   PT_END(p);
 }
 \endcode
 *
 * \hideinitializer
 */
#define PT_BEGIN(pt) LC_RESUME((pt)->lc)
/*\
  do {						\
    if((pt)->lc != LC_NULL) {			\
      LC_RESUME((pt)->lc);			\
    } 						\
    } while(0)*/

/**
 * Block and wait until condition is true.
 *
 * This macro blocks the protothread until the specified condition is
 * true.
 *
 * \param pt A pointer to the protothread control structure.
 * \param condition The condition.
 *
 * Example:
 \code
 PT_THREAD(seconds(struct pt *p)) {
   PT_BEGIN(p);

   PT_WAIT_UNTIL(p, time >= 2 * SECOND);
   printf("Two seconds have passed\n");
   
   PT_END(p);
 }
 \endcode
 *
 * \hideinitializer
 */
#define PT_WAIT_UNTIL(pt, condition)	        \
  do {						\
    LC_SET((pt)->lc);				\
    if(!(condition)) {				\
      return PT_THREAD_WAITING;			\
    }						\
  } while(0)

/**
 * Block and wait while condition is true.
 *
 * This function blocks and waits while condition is true. See
 * PT_WAIT_UNTIL().
 *
 * \param pt A pointer to the protothread control structure.
 * \param cond The condition.
 *
 * \hideinitializer
 */
#define PT_WAIT_WHILE(pt, cond)			\
  PT_WAIT_UNTIL((pt), !(cond))


/**
 * Block and wait until a child protothread completes.
 *
 * This macro schedules a child protothread. The current protothread
 * will block until the child protothread completes.
 *
 * \note The child protothread must be manually initialized with the
 * PT_INIT() function before this function is used.
 *
 * \param pt A pointer to the protothread control structure.
 * \param thread The child protothread with arguments
 *
 * Example:
 \code
 PT_THREAD(child(struct pt *p, int event)) {
   PT_BEGIN(p);

   PT_WAIT_UNTIL(event == EVENT1);   
   
   PT_END(p);
 }

 PT_THREAD(parent(struct pt *p, struct pt *child_pt, int event)) {
   PT_BEGIN(p);

   PT_INIT(child_pt);
   
   PT_WAIT_THREAD(p, child(child_pt, event));
   
   PT_END(p);
 }
 \endcode
 *
 * \hideinitializer 
 */
#define PT_WAIT_THREAD(pt, thread)		\
  PT_WAIT_UNTIL((pt), (thread))

/**
 * Spawn a child protothread and wait until it exits.
 *
 * This macro spawns a child protothread and waits until it exits. The
 * macro can only be used within a protothread.
 *
 * \param pt A pointer to the protothread control structure.
 * \param thread The child protothread with arguments
 *
 * \hideinitializer
 */
#define PT_SPAWN(pt, thread)			\
  do {						\
    PT_INIT((pt));				\
    PT_WAIT_THREAD((pt), (thread));		\
  } while(0)

/**
 * Restart the protothread.
 *
 * This macro will block and cause the running protothread to restart
 * its execution at the place of the PT_BEGIN() call.
 *
 * \param pt A pointer to the protothread control structure.
 *
 * \hideinitializer
 */
#define PT_RESTART(pt)				\
  do {						\
    PT_INIT(pt);				\
    return PT_THREAD_WAITING;			\
  } while(0)

/**
 * Exit the protothread.
 *
 * This macro causes the protothread to exit. If the protothread was
 * spawned by another protothread, the parent protothread will become
 * unblocked and can continue to run.
 *
 * \param pt A pointer to the protothread control structure.
 *
 * \hideinitializer
 */
#define PT_EXIT(pt)				\
  do {						\
    PT_INIT(pt);				\
    return PT_THREAD_EXITED;			\
  } while(0)

/**
 * Declare the end of a protothread.
 *
 * This macro is used for declaring that a protothread ends. It should
 * always be used together with a matching PT_BEGIN() macro.
 *
 * \param pt A pointer to the protothread control structure.
 *
 * \hideinitializer
 */
#define PT_END(pt) LC_END((pt)->lc); PT_EXIT(pt)


/**
 * Schedule a protothread.
 *
 * This function shedules a protothread. The return value of the
 * function is non-zero if the protothread is running or zero if the
 * protothread has exited.
 *
 * Example
 \code
 void main(void) {
   struct pt p;
   int event;
   
   PT_INIT(&p);
   while(PT_SCHEDULE(consumer(&p, event))) {
     event = get_event();
   }   
 }
 \endcode
 *
 * \param f The call to the C function implementing the protothread to
 * be scheduled
 *
 * \hideinitializer
 */
#define PT_SCHEDULE(f) (f == PT_THREAD_WAITING)

#endif /* __PT_H__ */


/** @} */
