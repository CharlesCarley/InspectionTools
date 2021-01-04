# Base Print

## Usage

```txt
Usage: bprint <options>

  <options>:

    -h, --help    Display this help message.
    -p, --pad     Pad the result with the symbol at index 0
        --symbols Supply a symbol string
    -b, --base    Convert the current byte to the supplied base.
    -s, --shift   Shift the symbol index by the supplied value.
    -r, --range   Specify a start address and a range.
                    - Arguments: [address, range]
                      - Address Base 16 [0 - file length]
                      - Range   Base 10 [0 - file length]

        --ws      Add white space between bytes.
        --nl      Add a newline every N bytes.
```
