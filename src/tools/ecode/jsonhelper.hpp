#pragma once

#include <nlohmann/json.hpp>

template <typename T> static constexpr nlohmann::detail::value_t json_get_value_type() {
	if constexpr ( std::is_same_v<T, std::string> ) {
		return nlohmann::detail::value_t::string;
	} else if constexpr ( std::is_same_v<T, bool> ) {
		return nlohmann::detail::value_t::boolean;
	} else if constexpr ( std::is_same_v<T, int64_t> || std::is_same_v<T, int32_t> ||
						  std::is_same_v<T, int16_t> || std::is_same_v<T, int8_t> ) {
		return nlohmann::detail::value_t::number_integer;
	} else if constexpr ( std::is_same_v<T, uint64_t> || std::is_same_v<T, uint32_t> ||
						  std::is_same_v<T, uint16_t> || std::is_same_v<T, uint8_t> ) {
		return nlohmann::detail::value_t::number_unsigned;
	} else if constexpr ( std::is_same_v<T, double> || std::is_same_v<T, float> ) {
		return nlohmann::detail::value_t::number_float;
	} else if constexpr ( std::is_same_v<T, nlohmann::json> ) {
		return nlohmann::detail::value_t::object;
	} else if constexpr ( std::is_same_v<T, std::vector<std::uint8_t>> ) {
		return nlohmann::detail::value_t::binary;
	} else {
		return nlohmann::detail::value_t::null;
	}
}

template <typename T>
static std::optional<T> json_get_if( nlohmann::json& json, std::string_view key, T defVal ) {
	if ( json.contains( key ) && json[key].type() == json_get_value_type<T>() ) {
		return json.value( key, defVal );
	}
	return {};
}

template <typename T>
static std::optional<T> json_get_if( nlohmann::json& json, std::string_view key ) {
	if ( json.contains( key ) && json[key].type() == json_get_value_type<T>() )
		return json[key].get<T>();
	return {};
}
