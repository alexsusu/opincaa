(page 419)
8.4.1 Basic Algorithm
The above high-level description results in the following basic algorithm:
1. Subtract exponents (d = Ex - Ey).
2. Align significands. This consists of the following:
 - Shift right d positions the significand of the operand with the smallest
exponent.
 - Select as the exponent of the result the largest exponent.

3. Add (subtract) significands and produce sign of result. This is a signed
addition. The effective operation (add or subtract) is determined by the
floating-point operation (ADD or SUBTRACT) and the signs of the
operands, as follows:
From now on we refer to the effective operation.
The sign of the result depends on the signs of the operands, the
operation, and the relative magnitude of the operands (see exercise 8.15).
Normalization of result. Three situations can occur:
(a) The result is already normalized: no action is needed.
  1.10011111
  0.00101011
  ADD
  1.11001010

(b) When the effective operation is an addition, there might be an
overflow of the significand. The normalization consists of the
following:
 - Shift right the significand one position.
 - Increment by one the exponent.

  1.1001111
  0.0110110
  ADD
 10.0000101

NORM 1.00000101

(c) When the effective operation is subtraction, the result might have
leading zeros. The normalization consists of the following:
 - Shift left the significand by a number of positions corresponding
to the number of leading zeros.
 - Decrement the exponent by the number of leading zeros.
From now on we refer to the effective operation.
The sign of the result depends on the signs of the operands, the
operation, and the relative magnitude of the operands (see exercise 8.15).
Normalization of result. Three situations can occur:

(a) The result is already normalized: no action is needed.
1.10011111
0.00101011
ADD
1.11001010

(b) When the effective operation is an addition, there might be an
overflow of the significand. The normalization consists of the
following:

 - Shift right the significand one position.

 - Increment by one the exponent.
  1.1001111
  0.0110110
  ADD
 10.0000101

 NORM 1.00000101

(c) When the effective operation is subtraction, the result might have
leading zeros. The normalization consists of the following:
 - Shift left the significand by a number of positions corresponding
to the number of leading zeros.
 - Decrement the exponent by the number of leading zeros.


8.4.4
Exceptions and Special Values
We now discuss the exceptions and special values that may occur in floating-point
addition and subtraction.
Overflow: This situation can occur when the exponent is incremented
during normalization (because of overflow of addition requiring a right
shift of significand) and because of overflow of significand during the
rounding step. It is detected by an exponent E > 255. The overflow flag is
set, and the result is set to -+-infinity.
 - Underflow: This situation can occur when the exponent is decremented
during normalization (left shift of significand). The underflow flag is set,
and the result exponent is set to E = 0. The fraction is left unnormalized
(denormal, gradual underflow).
 - Zero: This situation occurs when the significand of the result of addition is 0.
   The result is E = 0 and F = 0.
 - Inexact: This situation is detected before the rounding;
  the result is inexact if G + R + T = 1. The inexact flag is set.
 - NAN: If one operand (or both) is a NAN, then the result is set to NAN.
