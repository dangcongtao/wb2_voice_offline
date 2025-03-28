#include "lpf.h"
/** LOW PASS FILTER EQUATIONS
 *  Wikipedia equation: new = gain*x + (1-gain)*old.
 *  <=> new = old - gain*(old-x).
 *  with: x is input adc value which need apply filter.
 */
void lpf_update_estimate(lpf_data_t *current, uint32_t *measure)
{
    // Old = Old + Gain*(Old - new)     Gain : percent
    int old = current->estimate_value;
    int new = *measure;

    current->estimate_value = current->estimate_value - 0.1 * (old - new);
}