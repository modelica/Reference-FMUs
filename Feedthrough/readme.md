# Feedthrough

| Variable                  | Start     | Causality | Variability | Description
|:--------------------------| ----------|-----------|-------------|:---------------
| Float64\_fixed\_parameter | 0         | parameter | fixed       | Fixed parameter
| String\_parameter         | "Set me!" | parameter | fixed       | String parameter

In order to reproduce the reference results the following parameters need to be set

```
Float64_fixed_parameter: 1
String_parameter:        "FMI is awesome!"
```

and the [input signals](Feedthrough_in.csv) must be applied.

The plot shows the [reference result](Feedthrough_ref.csv) computed with [simulate\_fmi3\_me.c](https://github.com/modelica/Reference-FMUs/blob/master/examples/simulate_fmi3_me.c).

![Plot](Feedthrough_ref.svg)
