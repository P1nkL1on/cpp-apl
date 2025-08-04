Horrible APL mock, which can do from this:
```cpp
    std::cout << "\nSumm fold:\n";
    c.output_stack_push(value::value_int(10));
    c.output_stack_push(value::value_op(&generate_index_array));
    c.output_stack_push(value::value_op(&print));
    c.output_stack_push(value::value_op(&break_array));
    c.output_stack_push(value::value_op(&print));
    c.output_stack_push(value::value_op(&summ_two_ints));
    c.output_stack_push(value::value_int(9));
    c.output_stack_push(value::value_op(&repeat_op_x_times));
    c.output_stack_push(value::value_op(&print));
    c.input_stack_pop();
```
to this:
```
  Summ fold:
  [0 1 2 3 4 5 6 7 8 9]
  0
  45
```
