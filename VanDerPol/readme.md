# VanDerPol

The model implements the [Van der Pol oscillator](https://en.wikipedia.org/wiki/Van_der_Pol_oscillator).

```
der(x0) = x1
der(x1) = mu * ((1 - x0 * x0) * x1) - x0
```

with

| Variable      | Description      | Start |
|:--------------|:-----------------| -----:|
| x0            | Position         |     2 |
| der(x0)       | Derivative of x0 |       |
| x1            | Velocity         |     0 |
| der(x1)       | Derivative of x1 |       |
| mu            | Parameter        |     1 |

The plot shows the [reference result](VanDerPol_ref.csv) computed with [simulate\_fmi3\_me.c](https://github.com/modelica/Reference-FMUs/blob/master/examples/simulate_fmi3_me.c).

![Plot](VanDerPol_ref.svg)
