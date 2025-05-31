#pragma once

#include <array>
#include <string>
#include <cmath>

namespace Zenith::Utils {

		static constexpr std::array<const char*, 5> suffixes = { "ns", "us", "ms", "s", "ks" };

		static std::string dur_to_str_manual(double nanos) {
				if (nanos <= 0.0) return "0";

				int exponent = 0;
				if (nanos < 1e3) exponent = 0;      // ns
				else if (nanos < 1e6) exponent = 1; // us
				else if (nanos < 1e9) exponent = 2; // ms
				else if (nanos < 1e12) exponent = 3;// s
				else exponent = 4;                  // ks (kilo-seconds, very large)

				double scaled = nanos / std::pow(1000.0, exponent);

				if (scaled < 1.0) scaled = 1.0;
				if (scaled >= 1000.0) scaled = 999.999;

				int intPart = static_cast<int>(scaled);

				double frac = scaled - intPart;
				int d1 = static_cast<int>(frac * 10); frac = frac * 10 - d1;
				int d2 = static_cast<int>(frac * 10); frac = frac * 10 - d2;
				int d3 = static_cast<int>(frac * 10);

				std::string result = std::to_string(intPart) + "."
						+ static_cast<char>('0' + d1)
						+ static_cast<char>('0' + d2)
						+ static_cast<char>('0' + d3)
						+ suffixes[exponent];

				return result;
		}

}
