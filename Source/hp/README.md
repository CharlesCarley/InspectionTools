# Hex Print

## Usage

```txt
Usage: hp <options>

  <options>:

    -h, --help     Display this help message.
    -m, --mark     Mark a specific hexadecimal sequence.
                     - Base 16 [00-FFFFFFFF]

        --no-color Remove color output.
    -f, --flags    Specify the print flags. 01|02|04|08|10
                     - Where the value is a bit flag of one or more of the following hex values.
                       - COLORIZE:           01
                       - SHOW_HEX:           02
                       - SHOW_ASCII:         04
                       - SHOW_ADDRESS:       08
                       - SHOW_FULL_ADDRRESS: 10

    -r, --range    Specify a start address and a range.
                     - Arguments: [address, range]
                       - Address Base 16 [0 - file length]
                       - Range   Base 10 [0 - file length]

        --csv      Converts the output to a comma separated buffer

```