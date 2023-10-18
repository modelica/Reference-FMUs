## Regenerate parser for structured variables names

- `sudo apt install flex bison`
- `cd src`
- `bison -d structured_variable_name.y`
- `flex --outfile=structured_variable_name.yy.c structured_variable_name.l`
- `mv structured_variable_name.tab.h ../include/`
