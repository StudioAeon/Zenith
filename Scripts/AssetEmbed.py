import argparse
import sys
from pathlib import Path
from typing import Iterator


def create_c_array_name(filename: str) -> str:
	stem = Path(filename).stem
	identifier = ''.join(c if c.isalnum() else '_' for c in stem)
	if identifier and identifier[0].isdigit():
		identifier = f'g_{identifier}'
	elif identifier:
		identifier = f'g_{identifier}'
	else:
		identifier = 'g_Buffer'
	return identifier


def format_bytes_as_c_array(data: bytes, bytes_per_line: int = 16) -> Iterator[str]:
	if not data:
		return

	for i in range(0, len(data), bytes_per_line):
		chunk = data[i:i + bytes_per_line]
		hex_values = [f'0x{byte:02X}' for byte in chunk]

		suffix = ',' if i + bytes_per_line < len(data) else ''
		yield f'    {", ".join(hex_values)}{suffix}'


def convert_binary_to_c_array(input_path: Path, output_path: Path, array_name: str) -> int:
	try:
		data = input_path.read_bytes()

		with output_path.open('w', encoding='utf-8') as output_file:
			output_file.write(f'// Generated from: {input_path.name}\n')
			output_file.write(f'// Size: {len(data)} bytes\n\n')

			output_file.write('#include <cstdint>\n\n')
			output_file.write(f'const uint8_t {array_name}[] =\n{{\n')

			if data:
				for line in format_bytes_as_c_array(data):
					output_file.write(f'{line}\n')

			output_file.write('};\n\n')

			size_name = array_name.replace('g_', 'g_') + '_Size'
			output_file.write(f'const size_t {size_name} = sizeof({array_name});\n')

		return len(data)

	except FileNotFoundError:
		raise FileNotFoundError(f"Input file not found: {input_path}")
	except PermissionError as e:
		if input_path in str(e):
			raise PermissionError(f"Cannot read input file: {input_path}")
		else:
			raise PermissionError(f"Cannot write to output file: {output_path}")


def main() -> int:
	parser = argparse.ArgumentParser(
		description='Convert binary files to C-style uint8_t arrays for embedding',
		formatter_class=argparse.RawDescriptionHelpFormatter,
		epilog="""
Examples:
  %(prog)s data.bin                    # Creates Buffer.embed
  %(prog)s image.png image_data.embed  # Creates image_data.embed
  %(prog)s -n my_data file.bin         # Uses 'g_my_data' as array name
        """
	)

	parser.add_argument(
		'input_file',
		help='Input binary file to convert'
	)

	parser.add_argument(
		'output_file',
		nargs='?',
		default='Buffer.embed',
		help='Output .embed filename (default: Buffer.embed)'
	)

	parser.add_argument(
		'-n', '--name',
		dest='array_name',
		help='Custom name for the C array variable (default: derived from input filename)'
	)

	parser.add_argument(
		'-v', '--verbose',
		action='store_true',
		help='Enable verbose output'
	)

	args = parser.parse_args()

	input_path = Path(args.input_file)
	output_path = Path(args.output_file)

	if output_path.suffix != '.embed':
		output_path = output_path.with_suffix('.embed')

	array_name = args.array_name or create_c_array_name(input_path.name)

	if not input_path.exists():
		print(f"Error: Input file '{input_path}' does not exist.", file=sys.stderr)
		return 1

	if not input_path.is_file():
		print(f"Error: '{input_path}' is not a regular file.", file=sys.stderr)
		return 1

	try:
		if args.verbose:
			print(f"Converting: {input_path}")
			print(f"Output: {output_path}")
			print(f"Array name: {array_name}")

		byte_count = convert_binary_to_c_array(input_path, output_path, array_name)

		print(f"Successfully converted {byte_count:,} bytes to '{output_path}'")

		if args.verbose:
			print(f"Include in your C++ code with: #include \"{output_path.name}\"")

		return 0

	except (FileNotFoundError, PermissionError, OSError) as e:
		print(f"Error: {e}", file=sys.stderr)
		return 1

	except KeyboardInterrupt:
		print("\nOperation cancelled by user.", file=sys.stderr)
		return 1

	except Exception as e:
		print(f"Unexpected error: {e}", file=sys.stderr)
		return 1


if __name__ == '__main__':
	sys.exit(main())