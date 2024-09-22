#!/bin/bash


# Store the argument in a variable
value=194657
value1=131072
value2=294657

# Call the Python script and pass the value

python3 testli.py $value |./bin/assembler | python3 testliout.py $value
python3 testli.py $value1 |./bin/assembler | python3 testliout.py $value1
python3 testli.py $value2 |./bin/assembler | python3 testliout.py $value2