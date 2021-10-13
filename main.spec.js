const uut = require("./main");

describe("validatePaymentAmount()", () => {
  const {validatePaymentAmount} = uut;

  it("should return error message if amount is less than 0", () => {
    expect(validatePaymentAmount(-100_000)).toEqual({errorMessage: `Payment amount can't be less than 0`, valid: false});
    expect(validatePaymentAmount(-1)).toEqual({errorMessage: `Payment amount can't be less than 0`, valid: false});
  });

  it("should return error message if amount is more or equal to 100k", () => {
    expect(validatePaymentAmount(100_000)).toEqual({errorMessage: `Payment amount has to be less than 100000`, valid: false});
    expect(validatePaymentAmount(100_001)).toEqual({errorMessage: `Payment amount has to be less than 100000`, valid: false});
    expect(validatePaymentAmount(950_000)).toEqual({errorMessage: `Payment amount has to be less than 100000`, valid: false});
  });

  it("should not return error for valid amounts", () => {
    expect(validatePaymentAmount(99_999.999)).toEqual({valid: true});
    expect(validatePaymentAmount(5555)).toEqual({valid: true});
    expect(validatePaymentAmount(1)).toEqual({valid: true});
    expect(validatePaymentAmount(0)).toEqual({valid: true});
  });
});
