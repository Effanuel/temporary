const LOWER_BOUNDARY = 0;
const UPPER_BOUNDARY = 100_000;

const validatePaymentAmount = (value) => {
  if (value < LOWER_BOUNDARY) return {errorMessage: `Payment amount can't be less than ${LOWER_BOUNDARY}`, valid: false};
  if (value >= UPPER_BOUNDARY) return {errorMessage: `Payment amount has to be less than ${UPPER_BOUNDARY}`, valid: false};
  return {valid: true};
};

module.exports = {validatePaymentAmount};
