# String Print

## Usage

```txt
Usage: sp <options>

  <options>:

    -h, --help          Display this help message.
        --show-address  Display the start address of each string.
        --no-whitespace Exclude whitespace.
    -l, --lowercase     Print the [a-z] character set.
    -u, --uppercase     Print the [A-Z] character set.
    -d, --digit         Print the [0-9] character set.
    -m, --merge         Merge the string printout into a large block.
                          - Arguments: column max [0-N]

    -n, --number        Set a minimum string length.
                          - Minimum qualifier [0-N]

        --hex           Print the [0-9Aa-Ff] character set.
                          - Takes precedence over other filters.

        --base64        Print the [0-9Aa-Zz+/=] character set.
                          - Takes precedence over other filters.

    -r, --range         Specify a start address and a range.
                          - Arguments: [address, range]
                            - Address Base 16 [0 - file length]
                            - Range   Base 10 [0 - file length]

```
