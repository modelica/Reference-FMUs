# LinearTransform

Implements the equation

```
y' = u * A
```

where `A` is an `m x n` matrix, `u` a vector of size `m`, and `y` a vector of size `n`.

The plot shows the [reference result](LinearTransform_ref.csv) computed with [simulate_fmi3_me.c](https://github.com/modelica/Reference-FMUs/blob/master/examples/simulate_fmi3_me.c).

![Plot](LinearTransform_ref.svg)
