/*
 * Copyright © 2013 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of Red Hat
 * not be used in advertising or publicity pertaining to distribution
 * of the software without specific, written prior permission.  Red
 * Hat makes no representations about the suitability of this software
 * for any purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * THE AUTHORS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <linux/input.h>
#include "touchpad-int.h"
#include "touchpad-util.h"

int
touchpad_phys_button_handle_state(struct touchpad *tp, void *userdata)
{
       uint32_t current, old, shift;

       current = tp->buttons.state;
       old = tp->buttons.old_state;
       shift = 0;
       while (current || old) {
               if ((current & 0x1) ^ (old  & 0x1))
                       tp->interface->button(tp, userdata, BTN_LEFT + shift, !!(current & 0x1));
               shift++;
               current >>= 1;
               old >>= 1;
       }
       tp->buttons.old_state = tp->buttons.state;

       return 0;
}

int
touchpad_phys_button_handle_timeout(struct touchpad *tp, unsigned int now, void *userdata)
{
	return 0;
}

bool
touchpad_phys_button_select_pointer_touch(struct touchpad *tp, struct touch *t)
{
	return t->state != TOUCH_NONE;
}
