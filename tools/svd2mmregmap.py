#!/usr/bin/env python3
import argparse
import pathlib
from cmsis_svd import SVDParser
from cmsis_svd.model import SVDAccessType, SVDCPUNameType

def instance_to_type(name):
    return f"{name}_t"

def sized_int(bit_size):
    return f"::bitfilled::sized_unsigned_t<{bit_size // 8}>"

def is_bitband_range(address):
    return address >= 0x40000000 and address < 0x42000000

def convert_access(svd_access):
    match svd_access:
        case SVDAccessType.READ_ONLY:
            return "r"
        case SVDAccessType.WRITE_ONLY | SVDAccessType.WRITE_ONCE:
            return "w"
        case SVDAccessType.READ_WRITE | SVDAccessType.READ_WRITE_ONCE | _:
            return "rw"

def generate_peripheral(peripheral, bitband):
    peripheral_name = peripheral.name
    if len(peripheral.group_name):
        peripheral_name = peripheral.group_name

    parts = []
    parts.append(
        f"struct {instance_to_type(peripheral_name)} {{")
    # TODO: only use bitband if all peripherals of the chip are in bitband range
    parts.append(
         "    using mmr_ops = ::bitfilled::bitband<PERIPH_BASE>;" if bitband and is_bitband_range(peripheral.base_address) else
         "    using mmr_ops = ::bitfilled::base;")

    # TODO: in the first round of iteration, generate enum types where enumeratedValues is provided
    # also de-duplicate enum types across registers

    nametrim = peripheral.name + "_"
    offset = 0
    for register in peripheral.get_registers():
        # filling gaps in the register map with reserved
        if (offset < register.address_offset):
            reserved_size = (register.address_offset - offset) * 8 // register.size
            parts.append(
        f"    BF_MMREG_RESERVED({register.size // 8}, {reserved_size})")

        # define the register
        regname = register.name.removeprefix(nametrim)
        regnametype = instance_to_type(regname)
        parts.append(
        f"    struct {regnametype} : BF_MMREG({sized_int(register.size)}, {convert_access(register.access)}, mmr_ops) {{\n"
        f"        BF_COPY_SUPERCLASS({regnametype});")

        # define register fields
        for field in register.get_fields():
            # TODO: group the fields into bitfieldset, if they are contiguous, share properties and are named accordingly

            access = field.access if field.access else register.access
            lsb = field.bit_offset
            msb = field.bit_offset + field.bit_width - 1
            parts.append(
        f"        BF_MMREGBITS({sized_int(register.size)}, {convert_access(access)}, {lsb}, {msb}) {field.name};")

        parts.append(
        f"    }} {regname};")
        offset = register.address_offset + register.size // 8

    parts.append(
         "};")
    return "\n".join(parts)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Parse an SVD file and generate bitfilled register map for a peripheral type."
    )
    parser.add_argument(
        "path",
        type=pathlib.Path,
        help="Path to the input SVD file"
    )
    parser.add_argument(
        "peripheral",
        type=str,
        help="Name of the peripheral or peripheral group for code generation"
    )

    args = parser.parse_args()

    parser = SVDParser.for_xml_file(str(args.path))
    device = parser.get_device()
    bitband_support = device.cpu.name == SVDCPUNameType.CM3 or device.cpu.name == SVDCPUNameType.CM4

    peripherals = device.get_peripherals()
    if args.peripheral == None:
        raise ValueError("Peripheral name must be provided")

    for peripheral in peripherals:
        if peripheral.name == args.peripheral or peripheral.group_name == args.peripheral:
            print(generate_peripheral(peripheral, bitband_support))
            exit(0)

    raise ValueError(f"Peripheral {args.peripheral} not found in the SVD file")
