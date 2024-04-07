#include "serialization.h"

std::string netlib::detail::next_datum(string_it& begin, string_it end) {
	auto datumBreak = std::ranges::find(begin, end, DATUM_BREAK_CHR);
	std::string ret{ begin, end };
	begin = std::next(datumBreak);
	return ret;
}