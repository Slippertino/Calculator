#pragma once

#include <string>
#include <sstream>
#include "object.hpp"
#include "op.hpp"
#include "status.hpp"

namespace calculator {

std::tuple<number_t, status_type, op_ptr> eval(
	const object_t& obj, 
	int prec = 1 << 6
);

} // namespace calculator
