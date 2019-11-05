
#include <hv_compiler.h> /* Because of __unused */
#include "Std_Types.h" /* E_OK */
#include "bsp_kldd.h"
#include "board_stuff.h" /* because of MEMORY_WORD */
#include "leds.h"

unsigned int bsp_kldd_led_on
(
  void         *arg0 __unused,
  unsigned long arg1,
  unsigned long arg2 __unused,
  unsigned long arg3 __unused
)
{
    led_on(arg1);

    return 0;
}

unsigned int bsp_kldd_led_off
(
  void         *arg0 __unused,
  unsigned long arg1,
  unsigned long arg2 __unused,
  unsigned long arg3 __unused
)
{
    led_off(arg1);

    return 0;
}

unsigned int bsp_kldd_led_toggle
(
  void         *arg0 __unused,
  unsigned long arg1,
  unsigned long arg2 __unused,
  unsigned long arg3 __unused
)
{
    led_toggle(arg1);

    return 0;
}

unsigned int bsp_kldd_led_display
(
  void         *arg0 __unused,
  unsigned long arg1,
  unsigned long arg2 __unused,
  unsigned long arg3 __unused
)
{
    led_display(arg1);

    return 0;
}

unsigned int bsp_kldd_reg_write8
(
    void         *arg0     __unused,
    unsigned long address,
    unsigned long value,
    unsigned long arg3     __unused
)
{
    MEMORY_BYTE(address) = value;
    return 0;
}

unsigned int bsp_kldd_reg_write16
(
    void         *arg0     __unused,
    unsigned long address,
    unsigned long value,
    unsigned long arg3     __unused
)
{
    MEMORY_HALF(address) = value;
    return 0;
}

unsigned int bsp_kldd_reg_write32
(
    void         *arg0     __unused,
    unsigned long address,
    unsigned long value,
    unsigned long arg3     __unused
)
{
    MEMORY_WORD(address) = value;
    return 0;
}

unsigned int bsp_kldd_reg_read8
(
    void         *arg0     __unused,
    unsigned long address,
    unsigned long output,
    unsigned long arg3     __unused
)
{
    unsigned int ret = E_NOT_OK;
    
    if (output != 0)
    {
        MEMORY_BYTE(output) = MEMORY_BYTE(address);
        ret = E_OK;
    }
    return ret;
}

unsigned int bsp_kldd_reg_read32
(
    void         *arg0     __unused,
    unsigned long address,
    unsigned long output,
    unsigned long arg3     __unused
)
{
    unsigned int ret = E_NOT_OK;
    
    if (output != 0)
    {
        MEMORY_WORD(output) = MEMORY_WORD(address);
        ret = E_OK;
    }
    return ret;
}

unsigned int bsp_kldd_reg_bit_clear32
(
    void         *arg0     __unused,
    unsigned long address,
    unsigned long mask,
    unsigned long arg3     __unused
)
{
    MEMORY_WORD(address) &= (~(mask));
    return 0;
}

unsigned int bsp_kldd_reg_bit_set32
(
    void         *arg0     __unused,
    unsigned long address,
    unsigned long mask,
    unsigned long arg3     __unused
)
{
    MEMORY_WORD(address) |= (mask);
    return 0;
}
